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

#include "rs_lisp_debug.h"
#include "rs_lisp_string.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Adapted from: http://stackoverflow.com/questions/2342162
String stringPrintf(const char* fmt, ...) {
    int size = strlen(fmt); // make a guess
    String str;
    va_list ap;
    while (1) {
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.data(), size, fmt, ap);
        va_end(ap);
        if (n > -1 && n < size) {  // Everything worked
            str.resize(n);
            return str;
        }
        if (n > -1)  // Needed size returned
            size = n + 1;   // For null char
        else
            size *= 2;      // Guess at a larger size (OS specific)
    }
    return str;
}

String copyAndFree(char* mallocedString)
{
    String ret(mallocedString);
    free(mallocedString);
    return ret;
}

String escape(const String& in)
{
    String out;
    out.reserve(in.size() * 2 + 2); // each char may get escaped + two "'s
    out += '"';
    for (auto it = in.begin(), end = in.end(); it != end; ++it) {
        char c = *it;
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '"':  out += "\\\""; break;
            default:   out += c;      break;
        };
    }
    out += '"';
    out.shrink_to_fit();
    return out;
}

static char unescape(char c)
{
    switch (c) {
        case '\\':  return '\\';
        case 'n':   return '\n';
        case '"':   return '"';
        default:    return c;
    }
}

String unescape(const String& in)
{
    String out;
    out.reserve(in.size()); // unescaped string will always be shorter

    // in will have double-quotes at either end, so move the iterators in
    for (auto it = in.begin()+1, end = in.end()-1; it != end; ++it) {
        char c = *it;
        if (c == '\\') {
            ++it;
            if (it != end) {
                out += unescape(*it);
            }
        }
        else {
            out += c;
        }
    }
    out.shrink_to_fit();
    return out;
}

#endif // DEVELOPER
