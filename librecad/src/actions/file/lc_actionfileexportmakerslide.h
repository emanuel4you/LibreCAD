/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2014 Christian Luginbühl (dinkel@pimprecords.com)
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

#ifndef LC_ACTIONFILEEXPORTMAKERSLIDE_H
#define LC_ACTIONFILEEXPORTMAKERSLIDE_H

#ifdef DEVELOPER

#include "rs_actioninterface.h"

class QString;
class RS_Graphic;

class LC_ActionFileExportMakerSlide : public RS_ActionInterface {
    Q_OBJECT
public:
    LC_ActionFileExportMakerSlide(RS_EntityContainer& container, RS_GraphicView& graphicView);

    void init(int status) override;
    void trigger() override;

    // helper function to generate Slide
    static bool writeSlide(const QString& fileName, RS_Graphic& graphic);
};

#endif // DEVELOPER

#endif

