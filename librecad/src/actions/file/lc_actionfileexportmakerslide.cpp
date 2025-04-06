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

#include "lc_actionfileexportmakerslide.h"

#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_graphic.h"


LC_ActionFileExportMakerSlide::LC_ActionFileExportMakerSlide(RS_EntityContainer& container,
                                                         RS_GraphicView& graphicView)
    : RS_ActionInterface("Export as Slide ...", container, graphicView){
    setActionType(RS2::ActionFileExportMakerSlide);
}

void LC_ActionFileExportMakerSlide::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

bool LC_ActionFileExportMakerSlide::writeSlide(const QString& fileName, RS_Graphic& graphic){
    if (fileName.isEmpty()) {
        LC_ERR<<__func__<<"(): empty file name, no Slide is generated";
        return false;
    }

    Q_UNUSED(graphic)
    qDebug() << "[LC_ActionFileExportMakerSlide::writeSlide] filename:" << fileName;
    // implement foo here

    return true;
}

void LC_ActionFileExportMakerSlide::trigger() {

	RS_DEBUG->print("LC_ActionFileExportMakerSlide::trigger()");

    if (graphic != nullptr) {
        QString filename = RS_DIALOGFACTORY->requestFileSaveAsDialog(tr("Export as"),
                                                                         "",
                                                                        "AutoCAD Slide (*.sld)");
        writeSlide(filename, *graphic);
    }

    finish(false);
}

#endif // DEVELOPER
