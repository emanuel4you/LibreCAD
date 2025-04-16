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

#include "lc_slide.h"

#include "rs.h"
#include "rs_debug.h"
#include "rs_settings.h"

#include "slide_draw_qpainter.h"

LC_Slide::LC_Slide(const RS_Vector &d, const QString &file)
: pos(d)
, file(file)
, darkBackground(true)
{
    LC_GROUP_GUARD("Colors");
    auto bgc = QColor(LC_GET_STR("background", RS_Settings::background));
    fillBackground = bgc;
    if((bgc.red() * 299 + bgc.green() * 587 + bgc.blue() * 114) / 1000 >= 125)
    {
        darkBackground = false;
    }
}

void LC_Slide::draw(RS_Painter* painter)
{
    RS_DEBUG->print("LC_Slide::draw");

    painter->fillRect(0, 0, pos.x, pos.y, fillBackground);
    slide_draw_qpainter(painter,
                        0,
                        0,
                        pos.x,
                        pos.y,
                        darkBackground,
                        qUtf8Printable(file));
}

#endif // DEVELOPER
