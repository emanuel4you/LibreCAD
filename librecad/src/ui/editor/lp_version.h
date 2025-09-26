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

#ifndef LP_VERSION_H
#define LP_VERSION_H

#ifdef DEVELOPER

#define LP_STR_HELPER(x) #x
#define LP_STR(x) LP_STR_HELPER(x)

// __LP__
#ifndef __LP__
#define LP_MAJOR_VER  1
#define LP_MINOR_VER  1
#define LP_PATCHLEVEL 3
#define LP_BUILD "devel"

#define LP_VERSION LP_MAJOR_VER * 10000 \
    + LP_MINOR_VER * 100 \
    + LP_PATCHLEVEL

#define __LP__ \
    LP_STR(LP_MAJOR_VER) \
    "." \
    LP_STR(LP_MINOR_VER) \
    "." \
    LP_STR(LP_PATCHLEVEL)

#define LP_VERSION_STR_HELPER(build) \
   build

#define LP_VERSION_STR \
    LP_VERSION_STR_HELPER(__LP__)

#endif // __LP__

#endif // DEVELOPER

#endif // LP_VERSION_H

