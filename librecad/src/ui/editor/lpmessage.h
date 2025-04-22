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

#ifndef LPMESSAGE_H
#define LPMESSAGE_H

#ifdef DEVELOPER

#include <QObject>
#include <QWidget>
#include <QDialog>
#include <QLabel>
#include <QTimer>

#define LP_MSG_STYLE "border: 3px solid #329a77; border-radius: 4px; border-width: 2px; background: #c6ecd6; color: black; font: 10pt; padding-top: 8px; padding-right: 4px; padding-bottom: 8px;padding-left: 4px;"

class LpMessage : public QWidget
{
    Q_OBJECT
public:
    explicit LpMessage(QWidget *parent = nullptr, const QString &message ="", int timeout=3000);
    ~LpMessage();

    static void info(const QString &info, QWidget *parent = nullptr, int timeout=3000);

protected:
    void mousePressEvent(QMouseEvent *event);

private slots:
    void closeMe();
    void init();

private:
    QTimer *m_initTimer;
    QTimer *m_acceptTimer = nullptr;
    QLabel *m_info;
    int m_res;
};

#endif // DEVELOPER

#endif // LPMESSAGE_H
