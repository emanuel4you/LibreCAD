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

#ifndef INCLUDE_DEBUG_H
#define INCLUDE_DEBUG_H

#ifdef DEVELOPER

#include <stdio.h>
#include <stdlib.h>

#define DEBUG_TRACE                    1
//#define DEBUG_OBJECT_LIFETIMES         1
//#define DEBUG_ENV_LIFETIMES            1

#define DEBUG_TRACE_FILE    stderr

#define NOOP    do { } while (false)
#define NOTRACE(...)    NOOP

#if DEBUG_TRACE
    #define TRACE(...) fprintf(DEBUG_TRACE_FILE, __VA_ARGS__)
#else
    #define TRACE NOTRACE
#endif

#if DEBUG_OBJECT_LIFETIMES
    #define TRACE_OBJECT TRACE
#else
    #define TRACE_OBJECT NOTRACE
#endif

#if DEBUG_ENV_LIFETIMES
    #define TRACE_ENV TRACE
#else
    #define TRACE_ENV NOTRACE
#endif

#define __ASSERT(file, line, condition, ...) \
    if (!(condition)) { \
        printf("Assertion failed at %s(%d): ", file, line); \
        printf(__VA_ARGS__); \
        try { throw std::runtime_error(STRF(__VA_ARGS__)); } catch (const std::exception& e) { std::cerr << e.what() << std::endl; } \
    } else { }


#define ASSERT(condition, ...) \
    __ASSERT(__FILE__, __LINE__, condition, __VA_ARGS__)

#endif // DEVELOPER

#endif // INCLUDE_DEBUG_H
