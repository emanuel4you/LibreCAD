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

#ifndef RS_LISP_H
#define RS_LISP_H

#ifdef DEVELOPER

#include <QString>
#include <QThread>
#include "rs_lisp_main.h"

#define RS_LISP RS_Lisp::instance()

/*
 * LibreCAD Lisp scripting support.
 *
 * written by
 *
 * @author Emanuel Strobel
 *
 * 2024
 *
 */

class RS_Lisp
{
public:
    static RS_Lisp* instance();
    ~RS_Lisp();

    QString Lisp_GetVersionString() const { return QString(Lisp_GetVersion()) + QString(LISP_COPYRIGHT); }

    std::string runCommand(const QString& command);
    std::string runFileCmd(const QString& name);
    int runFile(const QString& name);
    int runString(const QString& str);
private:
    RS_Lisp();
    static RS_Lisp* unique;
};

class SleeperThread : public QThread
{
public:
    static void msleep(unsigned long msecs)
    {
        QThread::msleep(msecs);
    }
};

#endif // DEVELOPER

#endif // RS_PYTHON_H

