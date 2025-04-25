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

#ifndef RS_PYTHON_LISP_H
#define RS_PYTHON_LISP_H

#ifdef DEVELOPER

class RS_PythonLisp
{
public:
    RS_PythonLisp() {}
    ~RS_PythonLisp() {}

    int RunSimpleString(const char *cmd);
    int RunSimpleFile(const char *filename);

    const char *EvalSimpleString(const char *cmd);
    const char *EvalSimpleFile(const char *filename);
};

#endif // DEVELOPER

#endif // RS_PYTHON_LISP_H
