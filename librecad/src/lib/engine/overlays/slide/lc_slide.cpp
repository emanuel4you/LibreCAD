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

LC_Slide::LC_Slide(const RS_Vector& d, const QString& file)
    : pos(d), file(file), darkBackground(true) {
    LC_GROUP_GUARD("Colors");
    auto bgc = QColor(LC_GET_STR("background", RS_Settings::background));
    fillBackground = bgc;
    if ((bgc.red() * 299 + bgc.green() * 587 + bgc.blue() * 114) / 1000 >= 125) {
        darkBackground = false;
    }
    slideData = slide_from_uri(qUtf8Printable(file));
    if (!slideData) {
        std::ostringstream ss;
        ss << "Slide " << qUtf8Printable(file) << " not found";
        throw std::runtime_error{ss.str()};
    }
}

LC_OverlayDrawable* LC_Slide::clone() const {
    LC_Slide* s = new LC_Slide(*this);
    return s;
}

void LC_Slide::draw(RS_Painter* painter) {
    RS_DEBUG->print("LC_Slide::draw");
    painter->fillRect(0, 0, pos.x, pos.y, fillBackground);
    auto slide = slideData.value();
    unsigned sld_width = slide->header().high_x_dot();
    unsigned sld_height = slide->header().high_y_dot();
    double sld_ratio = slide->header().aspect_ratio();
    // Draw slide.
    SlideRecordsVisitorQPainterDrawer visitor{
        painter,
        sld_width, sld_height,
        sld_ratio,
        0,
        0,
        static_cast<unsigned>(pos.x),
        static_cast<unsigned>(pos.y),
        darkBackground
    };
    slide->visit_records(visitor);
}

#endif // DEVELOPER
