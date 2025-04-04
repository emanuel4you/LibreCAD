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

class QString;


/**
 * This action class can handle user events to select entities from script.
 *
 * @author  Emanuel
 */
class QC_ActionSelectSet : public RS_ActionInterface {
    Q_OBJECT
public:
    QC_ActionSelectSet(RS_EntityContainer& container,
                       RS_GraphicView& graphicView);

    QC_ActionSelectSet(RS2::EntityType typeToSelect, RS_EntityContainer& container,
                       RS_GraphicView& graphicView);

    ~QC_ActionSelectSet() override;
    void init(int status) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void setMessage(QString msg);
    bool isCompleted() const{return completed;}
    void getSelected(std::vector<unsigned int> &se) const;
    void unselectEntities();
    bool wasCanceled(){return canceled;}
protected:
    /**
     * Action States.
     */
    enum Status {
        Select
    };

    RS2::CursorType doGetMouseCursor(int status) override;
    void updateMouseButtonHints() override;
private:
    bool canceled;
    bool completed;
    std::unique_ptr<QString> message;
    RS2::EntityType typeToSelect = RS2::EntityType::EntityUnknown;
};
#endif
