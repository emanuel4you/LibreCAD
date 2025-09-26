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

#include <QMouseEvent>
#include <QKeyEvent>

#include "doc_plugin_interface.h"
#include "qc_actionentsel.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_selection.h"
#include "rs_snapper.h"

QC_ActionEntSel::QC_ActionEntSel(LC_ActionContext* actionContext)
        :RS_ActionInterface("Get Entity", actionContext, RS2::ActionEntSel)
        , m_canceled(false)
        , m_completed{false}
        , m_message(std::make_unique<QString>(tr("Select objects:")))
        , m_en{nullptr}
        , m_targetPoint(RS_Vector(0.0,0.0))
{
}

QC_ActionEntSel::~QC_ActionEntSel() = default;

void QC_ActionEntSel::updateMouseButtonHints() {
    if (!m_completed)
        updateMouseWidget(*m_message, tr("Cancel"));
    else
        updateMouseWidget();
}

RS2::CursorType QC_ActionEntSel::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

void QC_ActionEntSel::setMessage(QString msg){
    *m_message = msg;
}

void QC_ActionEntSel::trigger() {
    if (m_en) {
        m_completed = true;
        updateMouseButtonHints();
    } else {
        RS_DEBUG->print("QC_ActionEntSel::trigger: Entity is NULL\n");
    }
}

void QC_ActionEntSel::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("QC_ActionEntSel::mouseMoveEvent begin");

    m_targetPoint = snapFree(e);

    RS_DEBUG->print("QC_ActionEntSel::mouseMoveEvent end");
}

void QC_ActionEntSel::onMouseLeftButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent * e) {
    m_en = catchEntity(e);
    m_targetPoint = snapFree(e);
    trigger();
}
void QC_ActionEntSel::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent * e){
    m_completed = true;
    m_canceled = true;
    updateMouseButtonHints();
    finish();
}

void QC_ActionEntSel::keyPressEvent(QKeyEvent *e){
    if (e->key() == Qt::Key_Escape) {
        updateMouseWidget();
        m_completed = true;
        m_canceled = true;
    }
}

int QC_ActionEntSel::getEntityId()
{
    return (m_en != nullptr ? m_en->getId() : 0);
}

/**
 * Add selected entity from 'container' to the selection.
 */
Plugin_Entity *QC_ActionEntSel::getSelected(Doc_plugin_interface* d) {
    Plugin_Entity *pe = m_en ? new Plugin_Entity(m_en, d) : nullptr;
    return pe;
}

#endif // DEVELOPER
