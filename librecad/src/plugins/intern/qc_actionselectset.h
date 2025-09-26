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

#ifndef QC_ACTIONSELECTSET_H
#define QC_ACTIONSELECTSET_H

#ifdef DEVELOPER

#include "rs_entitycontainer.h"
#include "lc_actionpreselectionawarebase.h"

class QString;

/**
 * This action class can handle user events to select entities from script.
 *
 * @author  Emanuel
 */
class QC_ActionSelectSet : public LC_ActionPreSelectionAwareBase
{
    Q_OBJECT
public:
    QC_ActionSelectSet(LC_ActionContext *actionContext);
    ~QC_ActionSelectSet() override;
    void init(int status) override;


    void getSelected(std::vector<unsigned int> &se) const;
    bool wasCanceled(){ return m_canceled; }
    bool isCompleted() const{ return m_completed; }

protected:
    void updateMouseButtonHintsForSelection() override;
    void doTrigger(bool keepSelected) override;
    bool isAllowTriggerOnEmptySelection() override;
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;

private:
    bool m_canceled;
    bool m_completed;
};

#endif // DEVELOPER

#endif // QC_ACTIONSELECTSET_H
