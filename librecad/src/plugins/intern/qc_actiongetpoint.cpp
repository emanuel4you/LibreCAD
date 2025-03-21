/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
**
**
** This file is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include "qc_actiongetpoint.h"

#include <QPointF>
#include <QMouseEvent>
#include "rs_snapper.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct QC_ActionGetPoint::Points {
    RS_MoveData data;
    RS_Vector referencePoint;
    RS_Vector targetPoint;
    QString message;
};

QC_ActionGetPoint::QC_ActionGetPoint(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Get Point",
						   container, graphicView)
        , canceled(false)
		, completed{false}
		, setTargetPoint{false}
		, pPoints(std::make_unique<Points>()){
    pPoints->targetPoint = RS_Vector(0,0);
}

QC_ActionGetPoint::~QC_ActionGetPoint() = default;


void QC_ActionGetPoint::trigger() {
    RS_DEBUG->print("QC_ActionGetPoint::trigger()");
    completed = true;
    updateMouseButtonHints();
}

void QC_ActionGetPoint::mouseMoveEvent(QMouseEvent* e) {
    deletePreview();
    RS_DEBUG->print("QC_ActionGetPoint::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    if(setTargetPoint){
        if (pPoints->referencePoint.valid) {
            pPoints->targetPoint = mouse;
            RS_Line *line =new RS_Line{preview.get(),
                                       pPoints->referencePoint, mouse};
            line->setPen(RS_Pen(RS_Color(0,0,0), RS2::Width00, RS2::DotLine ));
            preview->addEntity(line);
            RS_DEBUG->print("QC_ActionGetPoint::mouseMoveEvent: draw preview");
            preview->addSelectionFrom(*container,viewport);
        }
    } else {
        pPoints->targetPoint = mouse;
    }

    RS_DEBUG->print("QC_ActionGetPoint::mouseMoveEvent end");
    drawPreview();
}



void QC_ActionGetPoint::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        canceled = true;
        completed = true;
        finish();
    }
}

void QC_ActionGetPoint::onCoordinateEvent( [[maybe_unused]]int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    pPoints->targetPoint = pos;
    moveRelativeZero(pPoints->targetPoint);
    trigger();
}


void QC_ActionGetPoint::updateMouseButtonHints() {
    if (!completed)
        updateMouseWidget(pPoints->message, tr("Cancel"));
    else
        updateMouseWidget();
}

RS2::CursorType QC_ActionGetPoint::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
void QC_ActionGetPoint::setBasepoint(QPointF* basepoint){
    pPoints->referencePoint.x = basepoint->x();
    pPoints->referencePoint.y = basepoint->y();
    setTargetPoint = true;
}

void QC_ActionGetPoint::setMessage(QString msg){
    pPoints->message = msg;
}

void QC_ActionGetPoint::getPoint(QPointF *point){
    if (pPoints)    {
        point->setX(pPoints->targetPoint.x);
        point->setY(pPoints->targetPoint.y);
    }
}
