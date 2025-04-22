/*******************************************************************************
*
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 emanuel

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#ifndef LIBRELISP_H
#define LIBRELISP_H

#ifdef DEVELOPER

#include "texteditor.h"
#include "librepad.h"
#include <QWidget>

class QG_Lsp_CommandWidget;

class LibreLisp : public Librepad
{
    Q_OBJECT
public:
    LibreLisp(QWidget *parent = nullptr, const QString& fileName="");

    void run() override;
    void loadScript() override;
    void cmdDock() override;

public slots:
    void debug() override;
    void trace() override;
    void untrace() override;

private slots:
    void docVisibilityChanged(bool visible);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QG_Lsp_CommandWidget* commandWidget {nullptr};
    QDockWidget *m_dock;

    void setCommandWidgetHeight(int height);
    void writeSettings();
    void readSettings();
};

#endif // DEVELOPER

#endif // LIBRELISP_H
