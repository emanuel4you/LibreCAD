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

#include "qc_actiongrdraw.h"

#include "lc_overlayentitiescontainer.h"
#include "lc_grdraw.h"

#include "rs_debug.h"
#include "rs_graphicview.h"
#include "lc_graphicviewport.h"

#include <QFileDialog>

QC_ActionGrDraw::QC_ActionGrDraw(LC_ActionContext *actionContext)
    : RS_ActionInterface("grdraw", actionContext, RS2::ActionGrDraw) {
}
void QC_ActionGrDraw::init(int status)
{
    RS_ActionInterface::init(status);
}

void QC_ActionGrDraw::trigger()
{
    finish(false);
}

void QC_ActionGrDraw::resume()
{
    if (getStatus() == SetGrDraw)
    {
        setStatus(ShowGrDraw);
        RS_ActionInterface::resume();
    }
}

void QC_ActionGrDraw::finish(bool updateTB)
{
    RS_DEBUG->print("QC_ActionGrDraw::finish");

    RS_ActionInterface::finish(updateTB);
    clear();
}

void QC_ActionGrDraw::mouseMoveEvent(QMouseEvent* e)
{
    Q_UNUSED(e)
}

void QC_ActionGrDraw::mouseReleaseEvent(QMouseEvent* e)
{
    switch(e->button()){
            case Qt::RightButton:
                trigger();
                break;
            default:
                break;
        }
}

void QC_ActionGrDraw::drawOverlayDrawgr(const RS_Vector &start, const RS_Vector &end, int color, bool highlight)
{
    RS_DEBUG->print("QC_ActionGrDraw::drawOverlayDrawgr");

    auto line = new LC_Grdraw(start, end, color, highlight);
    LC_OverlayDrawablesContainer *drawablesContainer = m_viewport->getOverlaysDrawablesContainer(RS2::OverlayGraphics::ActionPreviewEntity);
    drawablesContainer->add(line);
}

void QC_ActionGrDraw::drawLine(const RS_Vector &start, const RS_Vector &end, int color, bool highlight)
{
    if (m_graphic != nullptr)
    {
        drawOverlayDrawgr(start, end, color, highlight);
    }
}

void QC_ActionGrDraw::clear()
{
    RS_DEBUG->print("QC_ActionGrDraw::clear");

    LC_OverlayDrawablesContainer *drawablesContainer = m_viewport->getOverlaysDrawablesContainer(RS2::OverlayGraphics::ActionPreviewEntity);
    drawablesContainer->clear();
}

#endif // DEVELOPER
