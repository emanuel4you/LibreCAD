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

#include "Environment.h"
#include "Types.h"

#include <iostream>
#include <algorithm>

lclEnvPtr replEnv(new lclEnv);

lclEnvPtr shadowEnv(new lclEnv);

lclEnvPtr dclEnv(new lclEnv);


lclEnv::lclEnv(lclEnvPtr outer)
: m_outer(outer)
{
    TRACE_ENV("Creating lclEnv %p, outer=%p\n", this, m_outer.ptr());
}

lclEnv::lclEnv(lclEnvPtr outer, const StringVec& bindings,
               lclValueIter argsBegin, lclValueIter argsEnd)
: m_outer(outer)
{
    TRACE_ENV("Creating lclEnv %p, outer=%p\n", this, m_outer.ptr());
    setLamdaMode(true);
    int n = bindings.size();
    for (auto &it : bindings) {
        // std::cout << "[lclEnv::lclEnv] bindings: " << it << std::endl;
        if (it != "&" ||
            it != "/")
            {
                m_bindings.push_back(it);
            }
    }

    auto it = argsBegin;
    // std::cout << "argsBegin->ptr: " << argsBegin->ptr() << std::endl;
    // std::cout << "argsEnd->ptr  : " << argsEnd->ptr() << std::endl;
    // std::cout << "std::distance start : " << std::distance(argsBegin, argsEnd) << std::endl;
    // std::cout << "argsBegin->ptr()->type(): " << (int) argsBegin->ptr()->type() << std::endl;

    for (int i = 0; i < n; i++) {
        // std::cout << "current it begin: " << it->ptr()->print(true) << std::endl;
        if (bindings[i] == "&" ||
            bindings[i] == "/"
        ) {
            LCL_CHECK(i == n - 2, "There must be one parameter after the &");

            // std::cout << "it &: " << it->ptr()->print(true) << std::endl;
            set(bindings[n-1], lcl::list(it, argsEnd));
            return;
        }
        LCL_CHECK(it != argsEnd, "Not enough parameters");
        // std::cout << "it m_bindings.push_back + set: " << it->ptr()->print(true) << std::endl;
        // std::cout << "it->ptr()->type(): " << (int) it->ptr()->type() << std::endl;
        set(bindings[i], *it);
        ++it;
    }
    // std::cout << "it->ptr bottom: " << it->ptr() << std::endl;
    // std::cout << "argsEnd->ptr bottom: " << argsEnd->ptr() << std::endl;

    // for ( auto &str : m_bindings) {  std::cout << "m_bindings: " << str << std::endl; }
    // std::cout << "LCL_CHECK: " << (int)(it == argsEnd) << std::endl;
    // std::cout << "std::distance bottom : " << std::distance(argsBegin, argsEnd) << std::endl;
    LCL_CHECK(it == argsEnd, "Too many parameters");
}

lclEnv::~lclEnv()
{
    TRACE_ENV("Destroying lclEnv %p, outer=%p\n", this, m_outer.ptr());
}

lclEnvPtr lclEnv::find(const String& symbol)
{
    // std::cout << "[lclEnv::find] symbol: " << symbol << std::endl;
    for (lclEnvPtr env = this; env; env = env->m_outer) {
        if (env->m_map.find(symbol) != env->m_map.end()) {
            return env;
        }
    }
    return NULL;
}

lclValuePtr lclEnv::get(const String& symbol)
{
    for (lclEnvPtr env = this; env; env = env->m_outer) {
        auto it = env->m_map.find(symbol);
        if (it != env->m_map.end()) {
            return it->second;
        }
    }
#if 1
    LCL_FAIL("'%s' not found", symbol.c_str());
    return lcl::nilValue();
#else
    // std::cout << "'" << symbol << "' not found!" << std::endl;
    return lcl::nilValue();
#endif
}

lclValuePtr lclEnv::set(const String& symbol, lclValuePtr value)
{
    if (isLamda()) {
        for (auto &it : m_bindings) {
            if (symbol == it) {
                m_map[symbol] = value;
                return value;
            }
        }
        m_outer.ptr()->set(symbol, value);
    }
    else {
        m_map[symbol] = value;
    }
    return value;
}

lclEnvPtr lclEnv::getRoot()
{
    // Work our way down the the global environment.
    for (lclEnvPtr env = this; ; env = env->m_outer) {
        if (!env->m_outer) {
            return env;
        }
    }
}

#endif // DEVELOPER
