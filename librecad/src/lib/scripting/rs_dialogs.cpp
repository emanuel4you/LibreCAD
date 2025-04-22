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

#ifdef DEVELOPER

#include "rs_dialogs.h"
#include <QVBoxLayout>
#include <QLabel>

RS_InputDialog::RS_InputDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags ( Qt::FramelessWindowHint );
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel(tr("Press a character key"));
    label->setStyleSheet("font: 18pt;");
    layout->addWidget(label);
}

void RS_InputDialog::keyPressEvent(QKeyEvent *event)
{
    if(event->text() != "") {
        qDebug() << "RS_InputDialog::keyPressEvent pressed: (char)" << (uint_fast8_t)event->text().at(0).toLatin1();
        m_char = (char) event->text().at(0).toLatin1();
        accept();
    }
}

#endif // DEVELOPER
