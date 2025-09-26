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

#ifndef INCLUDE_RS_LISP_ENV_H
#define INCLUDE_RS_LISP_ENV_H

#ifdef DEVELOPER

#include "rs_lisp_lcl.h"

#include <map>

class lclEnv : public RefCounted {
public:
    lclEnv(lclEnvPtr outer = NULL);
    lclEnv(lclEnvPtr outer,
           const StringVec& bindings,
           lclValueIter argsBegin,
           lclValueIter argsEnd);

    ~lclEnv();

    void setLamdaMode(bool mode) { m_isLamda = mode; }
    bool isLamda() const { return m_isLamda; }

    lclValuePtr get(const String& symbol);
    lclEnvPtr   find(const String& symbol);
    lclValuePtr set(const String& symbol, lclValuePtr value);
    lclEnvPtr   getRoot();

private:
    typedef std::map<String, lclValuePtr> Map;
    Map m_map;
    lclEnvPtr m_outer;
    StringVec m_bindings;
    bool m_isLamda = false;
};

extern lclEnvPtr replEnv;

extern lclEnvPtr shadowEnv;

extern lclEnvPtr dclEnv;

#endif // DEVELOPER

#endif // INCLUDE_RS_LISP_ENV_H
