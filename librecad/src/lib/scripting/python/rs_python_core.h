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

#ifndef RS_PYTHON_CORE_H
#define RS_PYTHON_CORE_H

#ifdef DEVELOPER

#include "Python.h"

#include "rs_entitycontainer.h"

class RS_PythonCore
{
public:
    RS_PythonCore() {}
    ~RS_PythonCore() {}

    void command(const char *cmd);
    int sslength(const char *ss);
    double angle(PyObject *pnt1, PyObject *pnt2) const;

    PyObject *assoc(int needle, PyObject *args) const;
    PyObject *entlast() const;
    PyObject *entdel(const char *ename) const;
    PyObject *entget(const char *ename) const;
    PyObject *entmake(PyObject *args) const;
    PyObject *entmod(PyObject *args) const;
    PyObject *entnext(const char *ename) const;
    PyObject *entsel(const char *prompt = "") const;
    PyObject *polar(PyObject *pnt, double ang, double dist) const;
    PyObject *ssadd(const char *ename = "", const char *ss = "") const;
    PyObject *ssdel(const char *ename, const char *ss) const;
    PyObject *ssname(const char *ss, unsigned int idx) const;

    PyObject *getvar(const char *id) const;
    PyObject *setvar(const char *id, PyObject *args) const;

    RS_Document *getDocument() const;
    RS_Graphic *getGraphic() const;
    RS_EntityContainer* getContainer() const;
};

#endif // DEVELOPER

#endif // RS_PYTHON_CORE_H
