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
#include "qc_actionselectset.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include "rs_actionselectsingle.h"
#include "rs_graphicview.h"
#include "rs_snapper.h"

QC_ActionSelectSet::QC_ActionSelectSet(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView)
    :RS_ActionInterface("Get Select", container, graphicView)
    , canceled(false)
    , completed(false)
    , message(std::make_unique<QString>(tr("Select objects:")))
{
    actionType = RS2::ActionGetSelect;
    //actionType = RS2::ActionSelectWindow;
}

QC_ActionSelectSet::QC_ActionSelectSet(RS2::EntityType typeToSelect, RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
    :RS_ActionInterface("Get Select", container, graphicView)
    , completed(false)
    , message(std::make_unique<QString>(tr("Select objects:")))
    , typeToSelect(typeToSelect)
{
    actionType = RS2::ActionGetSelect;
    //actionType = RS2::ActionSelectWindow;
}

QC_ActionSelectSet::~QC_ActionSelectSet() = default;

void QC_ActionSelectSet::updateMouseButtonHints()
{
    switch (getStatus()) {
        case Select:
            updateMouseWidget(*message, tr("Cancel"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType QC_ActionSelectSet::doGetMouseCursor([[maybe_unused]] int status)
{
    return RS2::SelectCursor;
}

void QC_ActionSelectSet::setMessage(QString msg)
{
    *message = std::move(msg);
}

void QC_ActionSelectSet::init(int status)
{
        RS_ActionInterface::init(status);
        graphicView->setCurrentAction(
                new RS_ActionSelectSingle(typeToSelect, *container, *graphicView, this));
}

void QC_ActionSelectSet::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button()==Qt::RightButton) {
        completed = true;
        updateMouseWidget();
        finish();
    }
}

void QC_ActionSelectSet::keyPressEvent(QKeyEvent* e)
{
    if (e->key()==Qt::Key_Escape || e->key()==Qt::Key_Enter){
        updateMouseWidget();
        finish();
        completed = true;
        canceled = true;
    }
}

/**
 * Adds all selected entities from 'container' to the selection.
 */
void QC_ActionSelectSet::getSelected(std::vector<unsigned int> &se) const
{
    qDebug() << "[QC_ActionSelectSet::getSelected]";

    for (auto e: *container) {
        //qDebug() << "getId:" << e->getId();
        if (e->isSelected()) {
            qDebug() << "is Selected Id:" << e->getId();
            se.push_back(e->getId());
        }
    }
}

void QC_ActionSelectSet::unselectEntities()
{
    qDebug() << "[QC_ActionSelectSet::unselectEntities]";
    for(auto e: *container){ // fixme - iterating all entities for selection
        if (e->isSelected()) {
            qDebug() << "is un Selected Id:" << e->getId();
            e->setSelected(false);
        }
    }
    updateSelectionWidget();
}
