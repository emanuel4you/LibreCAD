/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include<iostream>
#include "rs_overlayslide.h"

#include "rs_debug.h"
#include "rs_painter.h"
#include "rs_settings.h"
#include "lc_overlayentity.h"

#include "slide_draw_qpainter.h"


RS_OverlaySlide::RS_OverlaySlide(const QString &fileName, int width, int height)
   : m_fileName(fileName), m_width(width), m_height(height) {}

void RS_OverlaySlide::draw(RS_Painter* painter)
{
    LC_GROUP_GUARD("Colors");
    bool darkBackground = true;
    auto bgc = QColor(LC_GET_STR("background", RS_Settings::background));
    const RS_Color &fillBackground{bgc};
    if((bgc.red() * 299 + bgc.green() * 587 + bgc.blue() * 114) / 1000 >= 125)
    {
        darkBackground = false;
    }

    painter->fillRect(0, 0, m_width, m_height, fillBackground);

    slide_draw_qpainter(painter,
                        0,
                        0,
                        m_width,
                        m_height,
                        darkBackground,
                        qUtf8Printable(m_fileName));
}
