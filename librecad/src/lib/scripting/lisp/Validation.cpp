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

#include "Validation.h"

int checkArgsIs(const char* name, int expected, int got)
{
    LCL_CHECK(got == expected,
           "\"%s\" expects %d arg%s, %d supplied",
           name, expected, PLURAL(expected), got);
    return got;
}

int checkArgsBetween(const char* name, int min, int max, int got)
{
    LCL_CHECK((got >= min) && (got <= max),
           "\"%s\" expects between %d and %d arg%s, %d supplied",
           name, min, max, PLURAL(max), got);
    return got;
}

int checkArgsAtLeast(const char* name, int min, int got)
{
    LCL_CHECK(got >= min,
           "\"%s\" expects at least %d arg%s, %d supplied",
           name, min, PLURAL(min), got);
    return got;
}

int checkArgsEven(const char* name, int got)
{
    LCL_CHECK(got % 2 == 0,
           "\"%s\" expects an even number of args, %d supplied",
           name, got);
    return got;
}

#endif // DEVELOPER
