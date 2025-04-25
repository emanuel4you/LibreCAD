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

#ifndef RS_PYTHONDCL_H
#define RS_PYTHONDCL_H

#ifdef DEVELOPER

#include "Python.h"

class RS_PythonDcl
{
public:
    RS_PythonDcl() {}
    ~RS_PythonDcl() {}

    void unloadDialog(int id);
    void endList();
    void endImage();
    void termDialog();

    bool newDialog(const char *name, int id);
    bool setTile(const char *key, const char *val);
    bool modeTile(const char *key, int mode);
    bool actionTile(const char *id, const char *action);

    int loadDialog(const char *filename);
    int startDialog();

    PyObject *getTile(const char *key);
    PyObject *startList(const char *key, int operation=-1, int index=-1);
    PyObject* startImage(const char *key) const;
    PyObject* addList(const char *val) const;
    PyObject* dimxTile(const char *key) const;
    PyObject* dimyTile(const char *key) const;
    PyObject* doneDialog(int res=-1) const;
    PyObject* fillImage(int x, int y, int width, int height, int color) const;
    PyObject *getAttr(const char *key, const char *attr) const;
    PyObject* pixImage(int x1, int y1, int x2, int y2, const char *path) const;
    PyObject* textImage(int x1, int y1, int x2, int y2, const char *text, int color) const;
    PyObject* slideImage(int x1, int y1, int x2, int y2, const char *path) const;
    PyObject* vectorImage(int x1, int y1, int x2, int y2, int color) const;

};

#endif // DEVELOPER

#endif // RS_PYTHONDCL_H
