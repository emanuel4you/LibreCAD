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

#include "lc_actionfileviewslide.h"

#include "lc_overlayentitiescontainer.h"

#include "rs_debug.h"
#include "rs_graphicview.h"
#include "lc_graphicviewport.h"

#include <QFileDialog>

LC_ActionFileViewSlide::LC_ActionFileViewSlide(LC_ActionContext *actionContext)
    : RS_ActionInterface("View slide", actionContext, RS2::ActionFileViewSlide) {
}
void LC_ActionFileViewSlide::init(int status)
{
    RS_ActionInterface::init(status);
    drawSlide();
}

void LC_ActionFileViewSlide::trigger()
{
    finish(false);
}

void LC_ActionFileViewSlide::resume()
{
    if (getStatus() == SetSlide)
    {
        setStatus(ShowSlide);
        RS_ActionInterface::resume();
    }
}

void LC_ActionFileViewSlide::finish(bool updateTB)
{
    RS_ActionInterface::finish(updateTB);
    clear();
}

void LC_ActionFileViewSlide::mouseMoveEvent(QMouseEvent* e)
{
    Q_UNUSED(e)
}

void LC_ActionFileViewSlide::mouseReleaseEvent(QMouseEvent* e)
{
    switch(e->button()){
            case Qt::RightButton:
                trigger();
                break;
            default:
                break;
        }
}

void LC_ActionFileViewSlide::drawOverlaySlide(const QString &file)
{
    RS_DEBUG->print("LC_ActionFileViewSlide::drawOverlaySlide file: %s", qUtf8Printable(file));

    auto slide = new LC_Slide(RS_Vector(m_graphicView->getWidth(), m_graphicView->getHeight()), file);
    LC_OverlayDrawablesContainer *drawablesContainer = m_viewport->getOverlaysDrawablesContainer(RS2::OverlayGraphics::ActionPreviewEntity);
    drawablesContainer->add(slide);
}

void LC_ActionFileViewSlide::drawSlide()
{
    if (m_graphic != nullptr)
    {
        QString filename = QFileDialog::getOpenFileName(NULL,
                                                        tr("open slide"),
                                                        "",
                                                        "AutoCAD Slide (*.sld)");

        if (filename.isEmpty()) {
            LC_ERR<<__func__<<"(): empty file name, no Slide is selected";
            setStatus(-1);
            return;
        }
        drawOverlaySlide(filename);
    }
}

void LC_ActionFileViewSlide::clear()
{
    LC_OverlayDrawablesContainer *drawablesContainer = m_viewport->getOverlaysDrawablesContainer(RS2::OverlayGraphics::ActionPreviewEntity);
    drawablesContainer->clear();
}

#endif // DEVELOPER
