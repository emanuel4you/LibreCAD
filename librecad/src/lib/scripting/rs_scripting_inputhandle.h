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

#ifndef RS_SCRIPTING_INPUTHANDLE_H
#define RS_SCRIPTING_INPUTHANDLE_H

#ifdef DEVELOPER

#include <QLineEdit>

class RS_Scripting_InputHandle : public QLineEdit
{
    Q_OBJECT
public:
    explicit RS_Scripting_InputHandle(const QString &prompt, QLineEdit *parent = nullptr);
    ~RS_Scripting_InputHandle() {}

    static QString readLine(const QString &prompt, QLineEdit *parent = nullptr)
    {
        RS_Scripting_InputHandle hdl(prompt, parent);
        return hdl.getString();
    }

    QString getString() const { return m_edit->text().remove(0, m_size); }

private:
    int m_size;
    QLineEdit *m_edit;
};

#endif // DEVELOPER

#endif // RS_INPUTHANDLE_H
