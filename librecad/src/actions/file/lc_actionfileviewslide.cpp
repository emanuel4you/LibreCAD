/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2014 Christian Luginbühl (dinkel@pimprecords.com)
** Copyright (C) 2018 Andrey Yaromenok (ayaromenok@gmail.com)
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along
** with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
**
**********************************************************************/

#ifdef DEVELOPER

#include <QFileDialog>

#include "lc_actionfileviewslide.h"

#include "rs_debug.h"
#include "rs_graphic.h"


LC_ActionFileViewSlide::LC_ActionFileViewSlide(RS_EntityContainer& container,
                                                    RS_GraphicView& graphicView)
    : LC_OverlaySlideAction("View a Slide...", container, graphicView){
    setActionType(RS2::ActionFileViewSlide);
}

void LC_ActionFileViewSlide::init(int status)
{
    Q_UNUSED(status)
    LC_OverlaySlideAction::init(5);

    if (getStatus() == 5)
    {
        drawSlide();
    }
}

void LC_ActionFileViewSlide::drawSlide()
{
    if (graphic != nullptr)
    {
        QString filename = QFileDialog::getOpenFileName(NULL,
                                                        tr("open slide"),
                                                        "",
                                                        "AutoCAD Slide (*.sld)");

        if (filename.isEmpty()) {
            LC_ERR<<__func__<<"(): empty file name, no Slide is selected";
            setStatus(-1);
            return;
        }

        setStatus(4);
        drawOverlaySlide(filename);
    }
}

#endif // DEVELOPER
