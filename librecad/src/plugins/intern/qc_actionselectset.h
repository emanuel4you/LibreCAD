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

#ifndef QC_ACTIONSELECTSET_H
#define QC_ACTIONSELECTSET_H

#include <memory>
#include "rs_actioninterface.h"
#include "rs_entitycontainer.h"
//#include "rs_actionselectwindow.h"
#include "lc_actionpreselectionawarebase.h"

class QString;


/**
 * This action class can handle user events to select entities from script.
 *
 * @author  Emanuel
 */
#if 0
class QC_ActionSelectSet : public RS_ActionInterface {
#else
class QC_ActionSelectSet : public LC_ActionPreSelectionAwareBase
{
#endif
    Q_OBJECT
public:
    QC_ActionSelectSet(RS_EntityContainer &container, RS_GraphicView &graphicView);
    ~QC_ActionSelectSet() override;
    void init(int status) override;


    void getSelected(std::vector<unsigned int> &se) const;
    bool wasCanceled(){ return canceled; }
    bool isCompleted() const{ return completed; }

protected:
    void updateMouseButtonHintsForSelection() override;
    void doTrigger(bool keepSelected) override;
    bool isAllowTriggerOnEmptySelection() override;
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
#if 0
    QC_ActionSelectSet(RS2::EntityType typeToSelect, RS_EntityContainer& container,
                       RS_GraphicView& graphicView);

    ~QC_ActionSelectSet() override;
    void init(int status) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void setMessage(QString msg);
    void unselectEntities();
protected:
    /**
     * Action States.
     */
    enum Status {
        Select
    };

    RS2::CursorType doGetMouseCursor(int status) override;
    void updateMouseButtonHints() override;
#endif

private:
    bool canceled;
    bool completed;
#if 0
    std::unique_ptr<QString> message;
    RS2::EntityType typeToSelect = RS2::EntityType::EntityUnknown;
#endif
};
#endif
