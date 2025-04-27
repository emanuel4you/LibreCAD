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

#ifndef QC_ACTIONSINGLESET_H
#define QC_ACTIONSINGLESET_H

#include "rs_actioninterface.h"

class QString;


/**
 * This action class can handle user events to select entities from script api.
 *
 * @author  Emanuel
 */
class QC_ActionSingleSet : public RS_ActionInterface {
    Q_OBJECT
public:
    QC_ActionSingleSet(LC_ActionContext* actionContext );
    ~QC_ActionSingleSet() override;
    void init(int status) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void setMessage(QString msg);
    bool isCompleted() const{return m_completed;}
    void getSelected(std::vector<unsigned int> &se) const;
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
private:
    bool m_completed = false;
    std::unique_ptr<QString> m_message;
    RS2::EntityType m_entityTypeToSelect = RS2::EntityType::EntityUnknown;
};
#endif // QC_ACTIONSINGLESET_H
