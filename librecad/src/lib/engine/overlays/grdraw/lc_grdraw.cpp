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

#include "lc_grdraw.h"

#include "rs.h"
#include "rs_debug.h"
#include "rs_settings.h"
#include "rs_filterdxfrw.h"

LC_Grdraw::LC_Grdraw(const RS_Vector& start, const RS_Vector& end, int color, bool highlight)
    : m_start(start)
    , m_end(end)
    , m_pen(RS_FilterDXFRW::numberToColor(color), RS2::Width01, highlight ? RS2::DashLineTiny : RS2::SolidLine)
{
}

LC_OverlayDrawable* LC_Grdraw::clone() const {
    LC_Grdraw* s = new LC_Grdraw(*this);
    return s;
}

void LC_Grdraw::draw(RS_Painter* painter) {
    RS_DEBUG->print("LC_Grdraw::draw");
    painter->setPen(m_pen);
    painter->drawLineWCS(m_start, m_end);
}

#endif // DEVELOPER
