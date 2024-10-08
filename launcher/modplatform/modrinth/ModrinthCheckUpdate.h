#pragma once

#include "modplatform/CheckUpdateTask.h"

class ModrinthCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    ModrinthCheckUpdate(QList<Resource*>& resources,
                                             std::list<Version>& mcVersions,
                                             QList<ModPlatform::ModLoaderType> loadersList,
                                             std::shared_ptr<ResourceFolderModel> resourceModel)
        : CheckUpdateTask(resources, mcVersions, loadersList, resourceModel)
        , m_hash_type(ModPlatform::ProviderCapabilities::hashType(ModPlatform::ResourceProvider::MODRINTH).first())
    {}

   public slots:
    bool abort() override;

   protected slots:
    void executeTask() override;
    void getUpdateModsForLoader(ModPlatform::ModLoaderTypes loader, bool forceModLoaderCheck = false);
    void checkVersionsResponse(std::shared_ptr<QByteArray> response, ModPlatform::ModLoaderTypes loader, bool forceModLoaderCheck = false);
    void checkNextLoader();

   private:
    Task::Ptr m_job = nullptr;
    QHash<QString, Resource*> m_mappings;
    QString m_hash_type;
    int m_next_loader_idx = 0;
};
