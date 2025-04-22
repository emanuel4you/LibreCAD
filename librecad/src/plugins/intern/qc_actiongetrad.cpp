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

#include "qc_actiongetrad.h"

#include <QPointF>
#include <QMouseEvent>
#include "rs_snapper.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct QC_ActionGetRad::Points {
    RS_MoveData data;
    RS_Vector referencePoint;
    RS_Vector targetPoint;
    QString message;
};

QC_ActionGetRad::QC_ActionGetRad(LC_ActionContext* actionContext)
        :RS_PreviewActionInterface("Get Point", actionContext)
        , m_canceled(false)
        , m_completed{false}
        , m_setTargetPoint{false}
        , m_rad(0.0)
        , m_pPoints(std::make_unique<Points>())
{
    m_pPoints->targetPoint = RS_Vector(0,0);
}

QC_ActionGetRad::~QC_ActionGetRad() = default;

void QC_ActionGetRad::trigger() {
    RS_DEBUG->print("QC_ActionGetRad::trigger()");
    m_completed = true;
    updateMouseButtonHints();
}

void QC_ActionGetRad::mouseMoveEvent(QMouseEvent* e) {
    deletePreview();
    RS_DEBUG->print("QC_ActionGetRad::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    if(m_setTargetPoint){
        if (m_pPoints->referencePoint.valid) {
            m_pPoints->targetPoint = mouse;
            RS_Line *line =new RS_Line{m_preview.get(),
                                       m_pPoints->referencePoint, mouse};
            line->setPen(RS_Pen(RS_Color(0,0,0), RS2::Width00, RS2::DotLine ));
            m_preview->addEntity(line);
            RS_DEBUG->print("QC_ActionGetRad::mouseMoveEvent: draw preview");
            m_preview->addSelectionFrom(*m_container,m_graphicView->getViewPort());
        }
    } else {
        m_pPoints->targetPoint = mouse;
    }

    RS_DEBUG->print("QC_ActionGetRad::mouseMoveEvent end");
    drawPreview();
}

void QC_ActionGetRad::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
        if (m_pPoints) {
            m_rad = m_pPoints->targetPoint.angleTo(m_pPoints->referencePoint) - M_PI;
        }
    } else if (e->button()==Qt::RightButton) {
        m_canceled = true;
        m_completed = true;
        finish();
    }
}

void QC_ActionGetRad::onCoordinateEvent( [[maybe_unused]]int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    m_pPoints->targetPoint = pos;
    moveRelativeZero(m_pPoints->targetPoint);
    trigger();
}

void QC_ActionGetRad::updateMouseButtonHints() {
    if (!m_completed)
        updateMouseWidget(m_pPoints->message, tr("Cancel"));
    else
        updateMouseWidget();
}

RS2::CursorType QC_ActionGetRad::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
void QC_ActionGetRad::setBasepoint(QPointF* basepoint){
    m_pPoints->referencePoint.x = basepoint->x();
    m_pPoints->referencePoint.y = basepoint->y();
    m_pPoints->referencePoint.valid = true;
    m_setTargetPoint = true;
}

void QC_ActionGetRad::setMessage(QString msg){
    m_pPoints->message = msg;
}

#endif
