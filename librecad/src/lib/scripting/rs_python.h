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

#define PYBIND11_NO_KEYWORDS
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#ifndef RS_PYTHON_H
#define RS_PYTHON_H

#include "qg_py_commandedit.h"

#ifdef RS_OPT_PYTHON

#include <QString>

#define COPYRIGHT \
"\nType \"help\", \"copyright\", \"credits\" or \"license\" " \
    "for more information."

#define RS_PYTHON RS_Python::instance()

extern class QG_Py_CommandEdit *Py_CommandEdit;

/*
 * LibreCAD Python scripting support.
 *
 * rewritten by
 *
 * @author Emanuel Strobel
 *
 * 2024
 *
 */

class RS_Python
{
public:
    static RS_Python* instance();
    ~RS_Python();

    QString Py_GetVersionString() const { return QString("Python ") + QString(Py_GetVersion()) + QString(" on ") + QString(Py_GetPlatform()) + QString(COPYRIGHT); }

    int addSysPath(const QString& path);
    int runCommand(const QString& command, QString& buf_out, QString& buf_err);
    int runFileCmd(const QString& name, QString& buf_out, QString& buf_err);
    int runFile(const QString& name);
    int evalString(const QString& command, QString& buf_out, QString& buf_err);
    int evalInteger(const QString& command, int& result, QString& buf_err);
    int evalFloat(const QString& command, double& result, QString& buf_err);
    int runString(const QString& str);
    int fflush(const QString& stream);
    int runModulFunc(const QString& module, const QString& func);
    int execFileFunc(const QString& file, const QString& func);
    int execModuleFunc(const QString& module, const QString& func);

private:
    RS_Python();
    static RS_Python* unique;
    PyObject* m_pGlobalMain;
    PyObject* m_pGlobalDict;
    PyObject* globalMain() { return m_pGlobalMain; }
    PyObject* Py_GlobalDict() { return m_pGlobalDict; }
};

#endif // RS_OPT_PYTHON

#endif // RS_PYTHON_H

