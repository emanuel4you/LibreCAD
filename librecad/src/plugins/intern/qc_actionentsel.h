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

#ifndef QC_ACTIONEMTSEL_H
#define QC_ACTIONEMTSEL_H

#ifdef DEVELOPER

#include "rs_previewactioninterface.h"
#include "rs_modification.h"

 class Plugin_Entity;
 class Doc_plugin_interface;

/**
 * This action class can handle user events to select entities from lisp.
 *
 * @author  Emanuel
 */

class QC_ActionEntSel : public RS_ActionInterface {
	Q_OBJECT
public:
    QC_ActionEntSel(LC_ActionContext* actionContext );
    ~QC_ActionEntSel() override;
    void trigger() override;
    void keyPressEvent(QKeyEvent* e) override;

    void setMessage(QString msg);
    bool isCompleted() const {return m_completed;}
    Plugin_Entity *getSelected(Doc_plugin_interface* d);

    int getEntityId();
    RS_Vector getPoint() {return toUCS(m_targetPoint);}
    bool wasCanceled(){return m_canceled;}

protected:
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent * e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent * e) override;
    void updateMouseButtonHints() override;
    void mouseMoveEvent(QMouseEvent* e) override;

protected:
    bool m_canceled;
    bool m_completed;
    std::unique_ptr<QString> m_message;
    RS_Entity* m_en;
    RS_Vector m_targetPoint;
};

#endif // DEVELOPER

#endif // QC_ACTIONEMTSEL_H
