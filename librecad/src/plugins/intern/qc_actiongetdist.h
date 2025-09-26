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

#ifndef QC_ACTIONGETDIST_H
#define QC_ACTIONGETDIST_H

#ifdef DEVELOPER

#include "rs_previewactioninterface.h"
#include "rs_modification.h"

/**
 * This action class can handle user events to get a point from plugin.
 *
 * @author  Emanuel
 */
class QC_ActionGetDist : public RS_PreviewActionInterface {
	Q_OBJECT
public:
    QC_ActionGetDist(LC_ActionContext *actionContext);
    ~QC_ActionGetDist();

    void trigger() override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void setBasepoint(QPointF* basepoint);
    void setMessage(QString msg);
    bool isCompleted(){return m_completed;}
    bool wasCanceled(){return m_canceled;}
    double getDist() { return m_distance; }
protected:
    bool m_canceled;
    bool m_completed;
    bool m_setTargetPoint;
    double m_distance;
    struct Points;
    std::unique_ptr<Points> m_pPoints;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;
};

#endif // DEVELOPER

#endif // QC_ACTIONGETDIST_H
