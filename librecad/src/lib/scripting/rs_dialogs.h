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

#ifndef RS_DIALOGS_H
#define RS_DIALOGS_H

#ifdef DEVELOPER

#include <QDialog>
#include <QKeyEvent>

class RS_InputDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RS_InputDialog(QWidget *parent = nullptr);
    ~RS_InputDialog() {}
    char getChar() { return m_char; }
    static char readChar()
    {
        RS_InputDialog dlg;
        if (dlg.exec() == QDialog::Accepted)
        {
            return dlg.getChar();
        }
        else
        {
            return '0';
        }
    }
protected:
    void keyPressEvent(QKeyEvent *event);
private:
    char m_char;
};

#endif // DEVELOPER

#endif // RS_DIALOGS_H
