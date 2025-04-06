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

#ifdef DEVELOPER

#include "qc_actionselectset.h"
#include "rs_graphicview.h"

QC_ActionSelectSet::QC_ActionSelectSet(RS_EntityContainer &container, RS_GraphicView &graphicView)
    : LC_ActionPreSelectionAwareBase("Selection Set", container, graphicView)
    , canceled(false)
    , completed(false)
{
    actionType = RS2::ActionDefault;
}

QC_ActionSelectSet::~QC_ActionSelectSet() = default;

void QC_ActionSelectSet::init(int status)
{
    LC_ActionPreSelectionAwareBase::init(status);
}

bool QC_ActionSelectSet::isAllowTriggerOnEmptySelection()
{
    return false;
}

void QC_ActionSelectSet::doTrigger([[maybe_unused]]bool keepSelected)
{
    qDebug() << "[QC_ActionSelectSet::doTrigger]";

    completed = true;
    finish(false);
}

void QC_ActionSelectSet::updateMouseButtonHintsForSelection()
{
    updateMouseWidgetTRCancel(tr("Select Entities"), MOD_CTRL(tr("???")));
}

RS2::CursorType QC_ActionSelectSet::doGetMouseCursorSelected([[maybe_unused]]int status)
{
    return RS2::SelectCursor;
}

void QC_ActionSelectSet::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]LC_MouseEvent *e)
{
    completed = true;
    finish();
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

#endif

