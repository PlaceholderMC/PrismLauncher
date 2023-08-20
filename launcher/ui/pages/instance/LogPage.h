// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once

#include <QQuickWidget>

#include "Application.h"
#include "BaseInstance.h"

#include "launch/LaunchTask.h"

#include "ui/pages/BasePage.h"

class LogFormatProxyModel;

class LogPage : public QQuickWidget, public BasePage {
    Q_OBJECT

   public:
    explicit LogPage(InstancePtr instance, QWidget* parent = nullptr);
    virtual ~LogPage() override = default;

    [[nodiscard]] QString displayName() const override { return tr("Minecraft Log"); }
    [[nodiscard]] QIcon icon() const override { return APPLICATION->getThemedIcon("log"); }
    [[nodiscard]] QString id() const override { return "console"; }
    [[nodiscard]] QString helpPage() const override { return "Minecraft-Logs"; }

    [[nodiscard]] bool shouldDisplay() const override;

    bool apply() override;
    void retranslate() override{};

   private slots:
    void onInstanceLaunchTaskChanged(LaunchTask::Ptr);

    void onStatusChanged(QQuickWidget::Status);
    void onQuickWidgetReady();

    void wrapModeChanged(int new_state);
    void suspendedChanged(bool new_state);
    void useRegexChanged(bool new_state);

    void copyPressed(QString);
    void uploadPressed();
    void clearPressed();

    void searchRequested(QString search_string, bool reversed);

   private:
    void setInstanceLaunchTaskChanged(LaunchTask::Ptr proc, bool initial);

    void loadSettings();
    void saveSettings();

   private:
    InstancePtr m_instance;
    shared_qobject_ptr<LaunchTask> m_process;

    LogFormatProxyModel* m_proxy;
    shared_qobject_ptr<LogModel> m_model;

    // pair <string searched, used regex or not in that search>
    std::pair<QString, bool> m_search_request;
    QVector<std::tuple<int, int, int>> m_search_results;

    int m_current_search_offset = 0;
    bool m_use_regex_in_search = false;
};
