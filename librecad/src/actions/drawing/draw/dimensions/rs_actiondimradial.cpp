/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#include "rs_actiondimradial.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dimradial.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_preview.h"

// fixme - sand - options for selection definition point angle, ability to specify whether label is inside or outside
// todo - think about multiple adding dimensions to already selected circles

RS_ActionDimRadial::RS_ActionDimRadial(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
    :LC_ActionCircleDimBase("Draw Radial Dimensions",
                        container, graphicView, RS2::ActionDimRadial)
    , edata{ std::make_unique<RS_DimRadialData>()}{
    reset();
}

RS_ActionDimRadial::~RS_ActionDimRadial() = default;

void RS_ActionDimRadial::reset(){
    RS_ActionDimension::reset();

    *edata = {};
    entity = nullptr;
    *pos = {};
    lastStatus = SetEntity;
}

RS_Dimension *RS_ActionDimRadial::createDim(RS_EntityContainer *parent) const{
    auto *newEntity = new RS_DimRadial(parent, *data, *edata);
    return newEntity;
}

RS_Vector RS_ActionDimRadial::preparePreview(RS_Entity *en, RS_Vector &position, bool forcePosition) {
    if (en != nullptr){
        double radius = en->getRadius();
        RS_Vector center = en->getCenter();
        data->definitionPoint = center;
        double angleToUse = m_currentAngle;
        if (angleIsFree || forcePosition){
            angleToUse = data->definitionPoint.angleTo(position);
        }
        edata->definitionPoint.setPolar(radius, angleToUse);
        edata->definitionPoint += data->definitionPoint;
        RS_Vector result = center + RS_Vector::polar(radius, angleToUse);
        return result;
    }
    else{
        return RS_Vector(false);
    }
}
