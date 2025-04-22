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

#include "lpmessage.h"

#include <QVBoxLayout>

LpMessage::LpMessage(QWidget *parent, const QString &message, int timeout)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    if (timeout > 0)
    {
        m_acceptTimer = new QTimer(this);
        connect(m_acceptTimer, &QTimer::timeout, this, &LpMessage::closeMe);
        m_acceptTimer->start(timeout);
    }

    m_info = new QLabel;
    m_info->setText(message);
    m_info->setStyleSheet(LP_MSG_STYLE);
    layout->addWidget(m_info);

    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    m_initTimer = new QTimer(this);
    connect(m_initTimer, &QTimer::timeout, this, &LpMessage::init);
    m_initTimer->start(50);
}

void LpMessage::info(const QString &info, QWidget *parent, int timeout)
{
    LpMessage *dlg = new LpMessage(parent, info, timeout);
    dlg->show();
}

LpMessage::~LpMessage()
{
    if (m_acceptTimer != nullptr)
    {
        delete m_acceptTimer;
    }
    delete m_initTimer;
    delete m_info;
}

void LpMessage::init()
{
    m_initTimer->stop();

    if (parent() != nullptr)
    {
        QWidget *pa = dynamic_cast<QWidget *>(parent());
        int x = pa->x() + pa->width() - width() - 20;
        int y = pa->y() + pa->height() - height() - 100;
        move(x,y);
    }
}

void LpMessage::closeMe()
{
    m_acceptTimer->stop();
    this->close();
}

void LpMessage::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    this->close();
}

#endif // DEVELOPER
