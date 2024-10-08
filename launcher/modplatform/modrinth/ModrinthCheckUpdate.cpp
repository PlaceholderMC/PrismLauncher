#include "ModrinthCheckUpdate.h"
#include "Application.h"
#include "ModrinthAPI.h"
#include "ModrinthPackIndex.h"

#include "Json.h"

#include "QObjectPtr.h"
#include "ResourceDownloadTask.h"

#include "modplatform/helpers/HashUtils.h"

#include "tasks/ConcurrentTask.h"

static ModrinthAPI api;

bool ModrinthCheckUpdate::abort()
{
    if (m_job)
        return m_job->abort();
    return true;
}

/* Check for update:
 * - Get latest version available
 * - Compare hash of the latest version with the current hash
 * - If equal, no updates, else, there's updates, so add to the list
 * */
void ModrinthCheckUpdate::executeTask()
{
    setStatus(tr("Preparing resources for Modrinth..."));
    setProgress(0, 9);

    auto hashing_task =
        makeShared<ConcurrentTask>(this, "MakeModrinthHashesTask", APPLICATION->settings()->get("NumberOfConcurrentTasks").toInt());
    for (auto* resource : m_resources) {
        auto hash = resource->metadata()->hash;

        // Sadly the API can only handle one hash type per call, se we
        // need to generate a new hash if the current one is innadequate
        // (though it will rarely happen, if at all)
        if (resource->metadata()->hash_format != m_hash_type) {
            auto hash_task = Hashing::createHasher(resource->fileinfo().absoluteFilePath(), ModPlatform::ResourceProvider::MODRINTH);
            connect(hash_task.get(), &Hashing::Hasher::resultsReady, [this, resource](QString hash) { m_mappings.insert(hash, resource); });
            connect(hash_task.get(), &Task::failed, [this] { failed("Failed to generate hash"); });
            hashing_task->addTask(hash_task);
        } else {
            m_mappings.insert(hash, resource);
        }
    }

    connect(hashing_task.get(), &Task::finished, this, &ModrinthCheckUpdate::checkNextLoader);
    m_job = hashing_task;
    hashing_task->start();
}

void ModrinthCheckUpdate::checkVersionsResponse(std::shared_ptr<QByteArray> response,
                                                ModPlatform::ModLoaderTypes loader,
                                                bool forceModLoaderCheck)
{
    QJsonParseError parse_error{};
    QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from ModrinthCheckUpdate at " << parse_error.offset
                   << " reason: " << parse_error.errorString();
        qWarning() << *response;

        emitFailed(parse_error.errorString());
        return;
    }

    setStatus(tr("Parsing the API response from Modrinth..."));
    setProgress(m_next_loader_idx * 2, 9);

    try {
        for (auto iter = m_mappings.begin(); iter != m_mappings.end(); iter++) {
            const QString& hash = iter.key();
            Resource* resource = iter.value();

            if (forceModLoaderCheck && !(resource->metadata()->loaders & loader))
                continue;

            auto project_obj = doc[hash].toObject();

            // If the returned project is empty, but we have Modrinth metadata,
            // it means this specific version is not available
            if (project_obj.isEmpty()) {
                qDebug() << "Mod " << m_mappings.find(hash).value()->name() << " got an empty response."
                         << "Hash: " << hash;

                continue;
            }

            // Sometimes a version may have multiple files, one with "forge" and one with "fabric",
            // so we may want to filter it
            QString loader_filter;
            static auto flags = { ModPlatform::ModLoaderType::NeoForge, ModPlatform::ModLoaderType::Forge,
                                  ModPlatform::ModLoaderType::Quilt, ModPlatform::ModLoaderType::Fabric };
            for (auto flag : flags) {
                if (loader.testFlag(flag)) {
                    loader_filter = ModPlatform::getModLoaderAsString(flag);
                    break;
                }
            }

            // Currently, we rely on a couple heuristics to determine whether an update is actually available or not:
            // - The file needs to be preferred: It is either the primary file, or the one found via (explicit) usage of the
            // loader_filter
            // - The version reported by the JAR is different from the version reported by the indexed version (it's usually the case)
            // Such is the pain of having arbitrary files for a given version .-.

            auto project_ver = Modrinth::loadIndexedPackVersion(project_obj, m_hash_type, loader_filter);
            if (project_ver.downloadUrl.isEmpty()) {
                qCritical() << "Modrinth mod without download url!" << project_ver.fileName;

                continue;
            }

            auto mod_iter = m_mappings.find(hash);
            if (mod_iter == m_mappings.end()) {
                qCritical() << "Failed to remap mod from Modrinth!";
                continue;
            }
            auto mod = *mod_iter;
            m_mappings.remove(hash);

            // Fake pack with the necessary info to pass to the download task :)
            auto pack = std::make_shared<ModPlatform::IndexedPack>();
            pack->name = mod->name();
            pack->slug = mod->metadata()->slug;
            pack->addonId = mod->metadata()->project_id;
            pack->provider = ModPlatform::ResourceProvider::MODRINTH;
            if ((project_ver.hash != hash && project_ver.is_preferred) || (resource->status() == ResourceStatus::NOT_INSTALLED)) {
                auto download_task = makeShared<ResourceDownloadTask>(pack, project_ver, m_resource_model);

                QString old_version = resource->metadata()->version_number;
                if (old_version.isEmpty()) {
                    if (resource->status() == ResourceStatus::NOT_INSTALLED)
                        old_version = tr("Not installed");
                    else
                        old_version = tr("Unknown");
                }

                m_updates.emplace_back(pack->name, hash, old_version, project_ver.version_number, project_ver.version_type,
                                       project_ver.changelog, ModPlatform::ResourceProvider::MODRINTH, download_task);
            }
            m_deps.append(std::make_shared<GetModDependenciesTask::PackDependency>(pack, project_ver));
        }
    } catch (Json::JsonException& e) {
        emitFailed(e.cause() + ": " + e.what());
        return;
    }
    checkNextLoader();
}

void ModrinthCheckUpdate::getUpdateModsForLoader(ModPlatform::ModLoaderTypes loader, bool forceModLoaderCheck)
{
    auto response = std::make_shared<QByteArray>();
    QStringList hashes;
    if (forceModLoaderCheck) {
        for (auto hash : m_mappings.keys()) {
            if (m_mappings[hash]->metadata()->loaders & loader) {
                hashes.append(hash);
            }
        }
    } else {
        hashes = m_mappings.keys();
    }
    auto job = api.latestVersions(hashes, m_hash_type, m_game_versions, loader, response);

    connect(job.get(), &Task::succeeded, this,
            [this, response, loader, forceModLoaderCheck] { checkVersionsResponse(response, loader, forceModLoaderCheck); });

    connect(job.get(), &Task::failed, this, &ModrinthCheckUpdate::checkNextLoader);

    setStatus(tr("Waiting for the API response from Modrinth..."));
    setProgress(m_next_loader_idx * 2 - 1, 9);

    m_job = job;
    job->start();
}

void ModrinthCheckUpdate::checkNextLoader()
{
    if (m_mappings.isEmpty()) {
        emitSucceeded();
        return;
    }
    if (m_next_loader_idx < m_loaders_list.size()) {
        getUpdateModsForLoader(m_loaders_list.at(m_next_loader_idx));
        m_next_loader_idx++;
        return;
    }
    static auto flags = { ModPlatform::ModLoaderType::NeoForge, ModPlatform::ModLoaderType::Forge, ModPlatform::ModLoaderType::Quilt,
                          ModPlatform::ModLoaderType::Fabric };
    for (auto flag : flags) {
        if (!m_loaders_list.contains(flag)) {
            m_loaders_list.append(flag);
            m_next_loader_idx++;
            setProgress(m_next_loader_idx * 2 - 1, 9);
            for (auto resource : m_mappings) {
                if (resource->metadata()->loaders & flag) {
                    getUpdateModsForLoader(flag, true);
                    return;
                }
            }
            setProgress(m_next_loader_idx * 2, 9);
        }
    }
    for (auto m : m_mappings) {
        emit checkFailed(m,
                         tr("No valid version found for this mod. It's probably unavailable for the current game version / mod loader."));
    }
    emitSucceeded();
}
