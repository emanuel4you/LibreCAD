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

#ifndef INCLUDE_LCL_H
#define INCLUDE_LCL_H

#ifdef DEVELOPER

#include "Debug.h"
#include "RefCountedPtr.h"
#include "lstring.h"
#include "Validation.h"
#include <sstream>

#include <vector>

class lclValue;
typedef RefCountedPtr<lclValue>  lclValuePtr;
typedef std::vector<lclValuePtr> lclValueVec;
typedef lclValueVec::iterator    lclValueIter;

class lclEnv;
typedef RefCountedPtr<lclEnv>    lclEnvPtr;

// lisp.cpp
extern lclValuePtr APPLY(lclValuePtr op,
                         lclValueIter argsBegin, lclValueIter argsEnd);
extern lclValuePtr EVAL(lclValuePtr ast, lclEnvPtr env);

extern String rep(const String& input, lclEnvPtr env);

// Core.cpp
extern void installCore(lclEnvPtr env);
extern String noQuotes(const String& s);

// Reader.cpp

typedef struct LclAlias {
    String alias = "", command = "";
} LclAlias_t;

extern std::vector<LclAlias_t> LclCom;
extern bool isAlias(const String& alias);
extern lclValuePtr readStr(const String& input);
extern lclValuePtr loadDcl(const String& path);

#endif // DEVELOPER

#endif // INCLUDE_LCL_H
