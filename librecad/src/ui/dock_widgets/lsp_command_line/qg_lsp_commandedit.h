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

#ifndef QG_LSP_COMMANDEDIT_H
#define QG_LSP_COMMANDEDIT_H

#ifdef DEVELOPER

#include <commandedit.h>
#include <QString>


/**
 * A lisp command line edit with some typical console features
 * (uparrow for the history, tab, ..).
 */
class QG_Lsp_CommandEdit: public CommandEdit {
    Q_OBJECT

public:
    QG_Lsp_CommandEdit(QWidget* parent=nullptr);
    virtual ~QG_Lsp_CommandEdit() {}

    void runFile(const QString& path);
    virtual void processInput(QString input);
    virtual void reset();
    virtual void setCurrent();
};

#endif // DEVELOPER

#endif // QG_LSP_COMMANDEDIT_H

