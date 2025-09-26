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
#include "qc_actionsingleset.h"

#include <QKeyEvent>

#include "rs_actionselectsingle.h"
#include "rs_graphicview.h"
#include "rs_entitycontainer.h"

QC_ActionSingleSet::QC_ActionSingleSet(LC_ActionContext* actionContext)
    :RS_ActionInterface("Selection Singel Set", actionContext, RS2::ActionSingleSet)
    , m_completed(false)
    , m_message(std::make_unique<QString>(tr("Select objects:"))){
}

QC_ActionSingleSet::~QC_ActionSingleSet() = default;

void QC_ActionSingleSet::updateMouseButtonHints() {
    switch (getStatus()) {
        case Select:
            updateMouseWidget(*m_message, tr("Cancel"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType QC_ActionSingleSet::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

void QC_ActionSingleSet::setMessage(QString msg){
    *m_message = std::move(msg);
}

void QC_ActionSingleSet::init(int status) {
        RS_ActionInterface::init(status);
        m_graphicView->setCurrentAction(
                std::make_shared<RS_ActionSelectSingle>(m_entityTypeToSelect,  m_actionContext, this));
}

void QC_ActionSingleSet::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        m_completed = true;
        updateMouseWidget();
        finish();
    }
}

void QC_ActionSingleSet::keyPressEvent(QKeyEvent* e){
    if (e->key()==Qt::Key_Escape || e->key()==Qt::Key_Enter){
        updateMouseWidget();
        finish();
        m_completed = true;
    }
}

/**
 * Adds all selected entities from 'container' to the selection.
 */
void QC_ActionSingleSet::getSelected(std::vector<unsigned int> &se) const
{
    qDebug() << "[QC_ActionSingleSet::getSelected]";

    for (auto e: *m_container) {
        //qDebug() << "getId:" << e->getId();
        if (e->isSelected()) {
            qDebug() << "is Selected Id:" << e->getId();
            se.push_back(e->getId());
        }
    }
}

void QC_ActionSingleSet::unselectEntities(){
    for(auto e: *m_container){ // fixme - iterating all entities for selection
        if (e->isSelected()) {
            e->setSelected(false);
        }
    }
    updateSelectionWidget();
}
