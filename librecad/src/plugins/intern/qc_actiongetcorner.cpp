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

#include "qc_actiongetcorner.h"

#include <QPointF>
#include <QMouseEvent>
#include "rs_snapper.h"
#include "rs_graphicview.h"
#include "rs_overlaybox.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"
#include "rs_line.h"

struct QC_ActionGetCorner::Points {
    RS_MoveData data;
    RS_Vector referencePoint;
    RS_Vector targetPoint;
    QString message;
};

QC_ActionGetCorner::QC_ActionGetCorner(LC_ActionContext* actionContext)
        :RS_PreviewActionInterface("Get Point", actionContext, RS2::ActionGetCorner)
        , m_canceled(false)
        , m_completed{false}
        , m_setTargetPoint{false}
        , m_pPoints(std::make_unique<Points>()){
    m_pPoints->targetPoint = RS_Vector(0,0);
}

QC_ActionGetCorner::~QC_ActionGetCorner() = default;

void QC_ActionGetCorner::trigger() {
    RS_DEBUG->print("QC_ActionGetCorner::trigger()");
    m_completed = true;
    updateMouseButtonHints();
}

void QC_ActionGetCorner::mouseMoveEvent(QMouseEvent* e) {
    deletePreview();
    RS_DEBUG->print("QC_ActionGetCorner::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    if(m_setTargetPoint){
        if (m_pPoints->referencePoint.valid) {
            m_pPoints->targetPoint = mouse;

            RS_Line *h1 =new RS_Line{m_preview.get(),
                                       m_pPoints->referencePoint, RS_Vector(m_pPoints->referencePoint.x, mouse.y)};

            RS_Line *h2 =new RS_Line{m_preview.get(),
                                       RS_Vector(mouse.x, m_pPoints->referencePoint.y, 0), mouse};

            RS_Line *v1 =new RS_Line{m_preview.get(),
                                       m_pPoints->referencePoint, RS_Vector(mouse.x, m_pPoints->referencePoint.y, 0)};

            RS_Line *v2 =new RS_Line{m_preview.get(),
                                       RS_Vector(m_pPoints->referencePoint.x, mouse.y), mouse};

            h1->setPen(RS_Pen(RS_Color(0,0,0), RS2::Width00, RS2::DotLine ));
            h2->setPen(RS_Pen(RS_Color(0,0,0), RS2::Width00, RS2::DotLine ));
            v1->setPen(RS_Pen(RS_Color(0,0,0), RS2::Width00, RS2::DotLine ));
            v2->setPen(RS_Pen(RS_Color(0,0,0), RS2::Width00, RS2::DotLine ));

            m_preview->addEntity(h1);
            m_preview->addEntity(h2);
            m_preview->addEntity(v1);
            m_preview->addEntity(v2);

            RS_DEBUG->print("QC_ActionGetCorner::mouseMoveEvent: draw preview");
            m_preview->addSelectionFrom(*m_container, m_graphicView->getViewPort());
        }
    } else {
        m_pPoints->targetPoint = toUCS(mouse);
    }

    RS_DEBUG->print("QC_ActionGetCorner::mouseMoveEvent end");
    drawPreview();
}

void QC_ActionGetCorner::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        m_canceled = true;
        m_completed = true;
        finish();
    }
}

void QC_ActionGetCorner::onCoordinateEvent( [[maybe_unused]]int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    m_pPoints->targetPoint = pos;
    moveRelativeZero(m_pPoints->targetPoint);
    trigger();
}

void QC_ActionGetCorner::updateMouseButtonHints() {
    if (!m_completed)
        updateMouseWidget(m_pPoints->message, tr("Cancel"));
    else
        updateMouseWidget();
}

RS2::CursorType QC_ActionGetCorner::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

void QC_ActionGetCorner::setBasepoint(QPointF* basepoint){
    m_pPoints->referencePoint.x = basepoint->x();
    m_pPoints->referencePoint.y = basepoint->y();
    m_pPoints->referencePoint.valid = true;
    m_setTargetPoint = true;
}

void QC_ActionGetCorner::setMessage(QString msg){
    m_pPoints->message = msg;
}

void QC_ActionGetCorner::getPoint(QPointF *point){
    if (m_pPoints)    {
        point->setX(m_pPoints->targetPoint.x);
        point->setY(m_pPoints->targetPoint.y);
    }
}

#endif
