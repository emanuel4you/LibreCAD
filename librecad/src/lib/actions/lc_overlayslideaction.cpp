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

#include "lc_overlayslideaction.h"
#include "rs_overlayslide.h"
#include "lc_overlayentitiescontainer.h"
#include "rs_entitycontainer.h"

#include "rs_debug.h"
#include "rs_line.h"
#include "rs_graphic.h"

#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"

#include <QDebug>
#include <QMouseEvent>

LC_OverlaySlideAction::LC_OverlaySlideAction(const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView, RS2::ActionType actionType)
    : RS_ActionInterface(name, container, graphicView, actionType) {}

void LC_OverlaySlideAction::init(int status)
{
    RS_ActionInterface::init(status);
    setStatus(status);
}

void LC_OverlaySlideAction::trigger()
{
    qDebug() << "[LC_OverlaySlideAction::trigger]";
    LC_OverlayDrawablesContainer *drawablesContainer = viewport->getOverlaysDrawablesContainer(RS2::OverlayGraphics::ActionPreviewEntity);
    drawablesContainer->clear();

    setStatus(-1);
}

void LC_OverlaySlideAction::finish(bool updateTB)
{
    qDebug() << "[LC_OverlaySlideAction::finish]" << updateTB;

    if (getStatus() == 4)
    {
        LC_OverlayDrawablesContainer *drawablesContainer = viewport->getOverlaysDrawablesContainer(RS2::OverlayGraphics::ActionPreviewEntity);
        drawablesContainer->clear();
    }
}

void LC_OverlaySlideAction::drawOverlaySlide(const QString &fileName)
{
    qDebug() << "[LC_OverlaySlideAction::drawOverlaySlide] filename:" << fileName;

    auto* ob = new RS_OverlaySlide(fileName, graphicView->getWidth(), graphicView->getHeight());

    LC_OverlayDrawablesContainer *drawablesContainer = viewport->getOverlaysDrawablesContainer(RS2::OverlayGraphics::ActionPreviewEntity);
    drawablesContainer->add(ob);
}

void LC_OverlaySlideAction::mouseMoveEvent(QMouseEvent* e)
{
    Q_UNUSED(e)
    qDebug() << "[LC_OverlaySlideAction::mouseMoveEvent]";
}

void LC_OverlaySlideAction::mouseReleaseEvent(QMouseEvent* e)
{
    qDebug() << "[LC_OverlaySlideAction::mouseReleaseEvent]";

    switch(e->button()){
        case Qt::RightButton:
            trigger();
            break;
        default:
            break;
    }
}
