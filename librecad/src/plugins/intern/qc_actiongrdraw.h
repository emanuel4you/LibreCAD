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

#ifndef QC_ACTIONGRDRAW_H
#define QC_ACTIONGRDRAW_H

#ifdef DEVELOPER

#include "rs_actioninterface.h"
#include "lc_slide.h"

#include <QMouseEvent>

class QC_ActionGrDraw : public RS_ActionInterface {
    Q_OBJECT
public:
    QC_ActionGrDraw(LC_ActionContext *actionContext);
    void init(int status) override;
    void trigger() override;
    void finish(bool updateTB = true) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void drawLine(const RS_Vector &start, const RS_Vector &end, int color, bool highlight);
    void resume() override;
protected:
    enum Status {
        SetGrDraw,       /**< Setting GrDraw */
        ShowGrDraw       /**< Showing GrDraw */
    };
    void clear();
private:
    void drawOverlayDrawgr(const RS_Vector &start, const RS_Vector &end, int color, bool highlight);
};

#endif // DEVELOPER

#endif // QC_ACTIONGRDRAW_H

