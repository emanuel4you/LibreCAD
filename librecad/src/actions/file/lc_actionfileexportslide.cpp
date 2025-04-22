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

#include <iostream>
#include <fstream>

#include <QApplication>
#include <QFileInfo>
#include <QPixmap>
#include <QImage>
#include <QImageWriter>

#include "lc_actionfileexportslide.h"

#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_graphic.h"
#include "rs_painter.h"
#include "rs_dxfcolor.h"
#include "lc_graphicviewport.h"
#include "lc_printviewportrenderer.h"

#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"

#include "slide.hpp"
#include "slide_records.hpp"
#include "slide_records_visitor.hpp"
#include "slide_binary_writer.hpp"

using namespace libslide;

static int create_slide(const QString& file,
             uint16_t width,
             uint16_t height,
             double ratio,
             const std::vector<std::shared_ptr<SlideRecord>>& records)
{
    SlideHeader header{
        2,
        width,
        height,
        ratio,
        native_endian()
    };

    Slide slide{
        QFileInfo(file).baseName().toStdString(),
        header,
        records,
        0
    };

    std::ofstream ofs{file.toStdString(), std::ios::binary};
    write_slide_binary(ofs, slide);
    return 0;
}

LC_ActionFileExportSlide::LC_ActionFileExportSlide(LC_ActionContext *actionContext)
    : RS_ActionInterface("Export as Slide ...", actionContext, RS2::ActionFileExportSlide){
}

void LC_ActionFileExportSlide::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

void LC_ActionFileExportSlide::trigger() {

	RS_DEBUG->print("LC_ActionFileExportSlide::trigger()");

    if (m_graphic == nullptr) {
        return;
    }

    qDebug() << "viewport size  :" << m_viewport->getWidth() << m_viewport->getHeight();
    qDebug() << "viewport offset:" << m_viewport->getOffsetX() << m_viewport->getOffsetY();
    qDebug() << "viewport factor:" << m_viewport->getFactor().x << m_viewport->getFactor().y;

    auto& appWin=QC_ApplicationWindow::getAppWindow();
    QC_MDIWindow *w = appWin->getCurrentMDIWindow();

    if (w) {
        w->getGraphic()->calculateBorders();
        QSize size;
#if 0
        double ratio = w->getGraphic()->getSize().x / w->getGraphic()->getSize().y * 1.0;

        size.setWidth(static_cast<int>(w->getGraphic()->getSize().x * ratio));
        size.setHeight(static_cast<int>(w->getGraphic()->getSize().y * ratio));
#else
        double width = m_viewport->getWidth();
        double height = m_viewport->getHeight();
        //double ratio = width / heigth;
        //double ratio = viewport->getFactor().x;

        size.setWidth(width);
        size.setHeight(height);
        //viewport->setBorders(2, 2, 2, 2);
#if 0
        LC_GraphicViewport vport = LC_GraphicViewport();
        vport.setSize(width, height);
        vport.setBorders(2, 2, 2, 2);
        vport.setContainer(viewport->getGraphic());
#endif

#endif
        if (size.width() == 0)
            return;

        QString file = QFileInfo(w->getFileName()).baseName();

        if (file == nullptr)
            file = tr("unnamed");

        file = RS_DIALOGFACTORY->requestFileSaveAsDialog(tr("Export as"),
                                                            file + ".sld",
                                                            "AutoCAD Slide (*.sld)");

        if (file.isEmpty())
            return;

        auto picture = std::make_shared<QPixmap>(size);
        std::shared_ptr<QPaintDevice> buffer;
        buffer = picture;

        appWin->showStatusMessage(tr("Exporting drawing..."), 2000);
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        RS_Painter painter(buffer.get());
        painter.setBackground(Qt::black);
        painter.setDrawingMode(RS2::ModeWB);
        painter.eraseRect(0, 0, size.width(), size.height());

        LC_PrintViewportRenderer renderer = LC_PrintViewportRenderer(m_viewport, &painter);
        renderer.setBackground(Qt::black);
        renderer.setLineWidthScaling(false);
        renderer.loadSettings();
        renderer.render();

        QImage image = picture->toImage();
        int rows = image.width();
        int cols = image.height();
#if 1
        QImageWriter iio;
        iio.setFileName(file + ".ppm");
        iio.setFormat("PPM");
        iio.write(image);
#endif
        qDebug() << "Width  : " << rows;
        qDebug() << "Height : " << cols;
        qDebug() << "Color depth: " << image.depth();

        QList<QColor> clist;

        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                const QColor &color = image.pixelColor(row, col);

                if (color != Qt::black)
                {
                    if (clist.size())
                    {
                        bool found = false;
                        for (const auto& c : clist)
                        {
                            if (c == color)
                            {
                                found = true;
                                break;
                            }
                        }

                        if (!found)
                        {
                            clist.append(color);
                        }
                    }
                    else
                    {
                        clist.append(color);
                    }
                }
            }
        }

        qDebug() << "Color size: " << clist.size();
        qDebug() << "Colors: " << clist;

        for (int n = 0; n < clist.size(); n++)
        {
            qDebug() << "[RS_DXFColor::fromQColor]" << clist.at(n) << RS_DXFColor::fromQColor(clist.at(n));
        }

        QColor currentColor = Qt::black;
        std::vector<std::shared_ptr<SlideRecord>> records;

        // scan rows for vectors
        for (int i = 0; i < cols; i++)
        {
            int x, rx, nrun = 0;
            QColor rpixColor = Qt::black;

            for (x = 0; x < rows; x++)
            {
                const QColor &pixColor = image.pixelColor(x, i);

                if (pixColor != rpixColor)
                {
                    if (nrun > 0) {
                        if (rpixColor != Qt::black)
                        {
                            //out(rpix, cols - 1, i, rx, x - 1);
                            if (currentColor != rpixColor)
                            {
                                records.push_back(std::make_shared<SlideRecordColor>(RS_DXFColor::fromQColor(rpixColor)));
                                currentColor = rpixColor;
                            }

                            int _ysize = rx, _y = cols - 1 - i, _xstart = x - 1, _xend = cols - 1 - i;
                            //qDebug() << "vector" << _ysize << _y << _xstart << _xend;

                            // push_back all horizontal lines > 1 pixel
                            if (_ysize != _xstart)
                            {
                                for (int b = _ysize; b <= _xstart; b++)
                                {
                                    //qDebug() << "setPixel" << b << cols;
                                    image.setPixel(b, i, qRgb(255, 255, 0));
                                    image.setPixel(b, i, qRgb(0, 0, 0));
                                }

                                records.push_back(std::make_shared<SlideRecordVector>(_ysize, _y, _xstart, _xend));
                            }
                        }
                    }
                    rpixColor = pixColor;
                    rx = x;
                    nrun = 1;
                }
            }
#if 0
            if ((nrun > 0) && (rpixColor != Qt::black))
            {
                qDebug() << "vector" << cols - 1 << i << rx << rows - 1;
                records.push_back(std::make_shared<SlideRecordVector>(cols - 1, i, rx, rows - 1));
            }
#endif
        }

        // scan cols for vectors and pixel
        for (int i = 0; i < rows; i++)
        {
            int y, ry, nrun = 0;
            QColor rpixColor = Qt::black, currentColor = Qt::black;

            for (y = 0; y < cols; y++) {
                const QColor &pixColor = image.pixelColor(i, y);

                if (pixColor != rpixColor)
                {
                    if (nrun > 0) {
                        if (rpixColor != Qt::black)
                        {
                            if (currentColor != rpixColor)
                            {
                                records.push_back(std::make_shared<SlideRecordColor>(RS_DXFColor::fromQColor(rpixColor)));
                                currentColor = rpixColor;
                            }

                            int _xsize = ry - 1, _x = i, _ystart = y - 2, _yend = i;
                            //qDebug() << "vector" << _xsize << _x << _ystart << _yend;
#if 0
                            for (int b = _xsize; b <= _ystart; b++)
                            {
                                qDebug() << "setPixel" << b << cols;
                                image.setPixel(i, b+1, qRgb(0, 0, 0));
                            }
#endif
                            // push_back all bertical lines and 1 pixels
                            records.push_back(std::make_shared<SlideRecordVector>(_x, cols-_xsize-2, _yend, cols-_ystart-2));
                        }
                    }
                    rpixColor = pixColor;
                    ry = y;
                    nrun = 1;
                }
            }
#if 0
            if ((nrun > 0) && (rpixColor != Qt::black))
            {
                qDebug() << "vector" << rows - 1 << i << ry << cols - 1;
                records.push_back(std::make_shared<SlideRecordVector>(rows - 1, i, ry, cols - 1));
            }
#endif
        }

        records.push_back(std::make_shared<SlideRecordEndOfFile>());
        create_slide(qUtf8Printable(file), rows, cols, 1.0 * rows / cols, records);

        QApplication::restoreOverrideCursor();
    }


    finish(false);
}

#endif // DEVELOPER
