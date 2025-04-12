/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#ifndef LC_OVERLAYSLIDEACTION_H
#define LC_OVERLAYSLIDEACTION_H

#include "rs_actioninterface.h"
#include "rs_overlayslide.h"


class LC_OverlaySlideAction:public RS_ActionInterface {
public:
    LC_OverlaySlideAction(
        const char *name,
        RS_EntityContainer &container,
        RS_GraphicView &graphicView,
        RS2::ActionType actionType = RS2::ActionNone);

    ~LC_OverlaySlideAction() override = default;

    void init(int status) override;
    void finish(bool updateTB = true) override;
    void trigger() override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    //void wheelEvent(QWheelEvent* e) override;

protected:

    void drawOverlaySlide(const QString &fileName);
private:
    RS_Line *slide;
    bool middleButtonPressed{false};
};


#endif // LC_OVERLAYBOXACTION_H
