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

#ifndef LC_SLIDE_H
#define LC_SLIDE_H

#include "rs_color.h"
#include "rs_painter.h"
#include "lc_overlayentity.h"

#include <iostream>
#include <sstream>
#include "slide.hpp"
#include "slide_loader.hpp"
#include "slide_records_visitor_qpainter_drawer.hpp"

using namespace libslide;

class LC_Slide: public LC_OverlayDrawable
{
public:
    LC_Slide(const RS_Vector &d, const QString &file);
    void draw(RS_Painter* painter) override;
    LC_OverlayDrawable* clone() const;
private:
    RS_Vector pos;
    QString file;
    RS_Color fillBackground;
    bool darkBackground = true;
    std::optional<std::shared_ptr<const Slide>> slideData;
};

#endif // LC_SLIDE_H
