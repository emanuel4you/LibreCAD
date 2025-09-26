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

#ifndef LC_GRDRAW_H
#define LC_GRDRAW_H

#include "rs_pen.h"
#include "rs_painter.h"
#include "lc_overlayentity.h"

class LC_Grdraw: public LC_OverlayDrawable
{
public:
    LC_Grdraw(const RS_Vector& start, const RS_Vector& end, int color, bool highlight);
    void draw(RS_Painter* painter) override;
    LC_OverlayDrawable* clone() const;
private:
    RS_Vector m_start;
    RS_Vector m_end;
    RS_Pen m_pen;
};

#endif // LC_GRDRAW_H
