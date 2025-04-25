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

#ifndef RS_LISP_MAIN_H
#define RS_LISP_MAIN_H

#ifdef DEVELOPER

#include "rs_lisp_version.h"
#include "qg_lsp_commandedit.h"
#include <rs_lisp_lcl.h>

#include <stdio.h>
#include <streambuf>
#include <string>
#include <ostream>
#include <iostream>
#include <sstream>

extern int lisp_error;
extern class QG_Lsp_CommandEdit *Lisp_CommandEdit;

const char *Lisp_GetVersion();

int Lisp_Initialize(int argc=0, char* argv[]=NULL);

int Lisp_GetError();

void Lisp_FreeError();

int LispRun_SimpleFile(const char *filename);

int LispRun_SimpleString(const char *command);

const std::string Lisp_EvalString(const String& input);

const std::string Lisp_EvalFile(const char *filename);

#endif // DEVELOPER

#endif // RS_LISP_MAIN_H
