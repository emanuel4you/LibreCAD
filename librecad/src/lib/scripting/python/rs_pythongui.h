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

#ifndef RS_PYTHONGUI_H
#define RS_PYTHONGUI_H

#ifdef DEVELOPER

#include "Python.h"

#include "rs_vector.h"

class RS_PythonGui
{
public:
    RS_PythonGui() {}
    ~RS_PythonGui() {}

    void prompt(const char *prompt);
    void initGet(const char *str="", int bit=0);
    void MessageBox(const char *message);

    int GetIntDialog(const char *prompt);
    double GetDoubleDialog(const char *prompt);

    char readChar();
    const char *OpenFileDialog(const char *title, const char *filename, const char *ext);
    const char *GetStringDialog(const char *prompt);

    RS_Vector getPoint(const char *prompt = "", const RS_Vector basePoint=RS_Vector()) const;
    RS_Vector getCorner(const char *prompt = "", const RS_Vector &basePoint=RS_Vector()) const;

    PyObject *acadColorDlg(int color=0, bool by=true) const;
    PyObject *acadTrueColorDlg(PyObject *color=Py_None, bool allowbylayer=true, PyObject *byColor=Py_None) const;
    PyObject *getDist(const char *prompt = "", const RS_Vector &basePoint=RS_Vector()) const;
    PyObject *getFiled(const char *title = "", const char *def = "", const char *ext = "", int flags=0) const;
    PyObject *getAngle(const char *prompt = "", const RS_Vector &basePoint=RS_Vector()) const;
    PyObject *getOrient(const char *prompt = "", const RS_Vector &basePoint=RS_Vector()) const;
    PyObject *getInt(const char *prompt = "") const;
    PyObject *getReal(const char *prompt = "") const;
    PyObject *getString(bool cr=false, const char *prompt = "") const;
    PyObject *getKword(const char *prompt = "") const;

};

#endif // DEVELOPER

#endif // RS_PYTHONGUI_H
