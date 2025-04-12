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

#ifndef RS_OVERLAYSLIDE_H
#define RS_OVERLAYSLIDE_H

#include "lc_overlayentity.h"
#include "rs_atomicentity.h"
#include "rs_color.h"

/**
 * Class for a line entity.
 *
 * @author R. van Twisk
 */
class RS_OverlaySlide : public LC_OverlayDrawable
{
public:
    RS_OverlaySlide(const QString &fileName, int width, int height);
    void draw(RS_Painter* painter) override;
protected:
    QString m_fileName;
    int m_width;
    int m_height;
};
#endif
