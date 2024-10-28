// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
 */

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QGroupBox;
class QProgressBar;
class QTabWidget;
class QTextBrowser;
class QToolBox;
QT_END_NAMESPACE

class WidgetGalleryWidgetsPane : public QWidget {
    Q_OBJECT
   public:
    WidgetGalleryWidgetsPane(QWidget* parent = nullptr);
    ~WidgetGalleryWidgetsPane() = default;
   private slots:
    void advanceProgressBar();

   private:
    static QGroupBox* createButtonsGroupBox();
    static QTabWidget* createItemViewTabWidget();
    static QGroupBox* createSimpleInputWidgetsGroupBox();
    QToolBox* createTextToolBox();
    QProgressBar* createProgressBar();

    QProgressBar* progressBar;
    QTextBrowser* systemInfoTextBrowser;
};
