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
#include <memory>
#include <set>

#include <QApplication>
#include <QCoreApplication>
#include <QImageWriter>
#include <QtCore>

#include "main.h"

#include "qc_applicationwindow.h"
#include "qg_dialogfactory.h"

#include "lc_documentsstorage.h"
#include "lc_graphicviewport.h"

#include "rs.h"
#include "rs_debug.h"
#include "rs_document.h"
#include "rs_dxfcolor.h"
#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "lc_printviewportrenderer.h"

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

///////////////////////////////////////////////////////////////////////
/// \brief openDocAndSetGraphic opens a DXF file and prepares all its graphics content
/// for further manipulations
/// \return
//////////////////////////////////////////////////////////////////////
static std::unique_ptr<RS_Document> openDocAndSetGraphic(QString);

static void touchGraphic(RS_Graphic*);

bool slotFileExportSlide(RS_Graphic* graphic,
                    const QString& name);

/////////
/// \brief console_dxf2sld is called if librecad
/// as console dxf2sld tool for converting DXF to Slide.
/// \param argc
/// \param argv
/// \return
///
int console_dxf2sld(int argc, char* argv[])
{
    RS_DEBUG->setLevel(RS_Debug::D_NOTHING);

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD");
    QCoreApplication::setApplicationVersion(XSTR(LC_VERSION));

    QFileInfo prgInfo(QFile::decodeName(argv[0]));
    QString prgDir(prgInfo.absolutePath());
    RS_Settings::init(app.organizationName(), app.applicationName());
    RS_SYSTEM->init(app.applicationName(), app.applicationVersion(),
        XSTR(QC_APPDIR), prgDir.toLatin1().data());

    QCommandLineParser parser;

    QString appDesc;
    QString librecad(prgInfo.filePath());

    if (prgInfo.baseName() != "dxf2sld") {
        librecad += " dxf2sld"; // executable is not dxf2sld, thus argv[1] must be 'dxf2sld'
        appDesc += "";
        appDesc += "dxf2sld " + QObject::tr( "usage: ") + librecad + QObject::tr( " <dxf_files>");
    }

    appDesc += "\nPrint a DXF file to a Slide file.";
    appDesc += "\n\n";
    appDesc += "Examples:\n\n";
    appDesc += "  " + librecad + " *.dxf";
    appDesc += "    -- print a dxf file to a slide file with the same name.\n";

    parser.setApplicationDescription(appDesc);

    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption outFileOpt(QStringList() << "o" << "outfile",
        "Output Slide file.", "file");
    parser.addOption(outFileOpt);

    parser.addPositionalArgument("<dxf_files>", "Input DXF file");

    parser.process(app);

    const QStringList args = parser.positionalArguments();

    if (args.isEmpty() || (args.size() == 1 && args[0] == "dxf2sld"))
        parser.showHelp(EXIT_FAILURE);

    QStringList dxfFiles;

    for (auto arg : args) {
        QFileInfo dxfFileInfo(arg);
        if (dxfFileInfo.suffix().toLower() != "dxf")
            continue; // Skip files without .dxf extension
        dxfFiles.append(arg);
    }

    if (dxfFiles.isEmpty())
        parser.showHelp(EXIT_FAILURE);

    // Output setup

    QString& dxfFile = dxfFiles[0];

    QFileInfo dxfFileInfo(dxfFile);
    QString fn = dxfFileInfo.completeBaseName(); // original DXF file name
    if(fn.isEmpty())
        fn = "unnamed";

    // Set output filename from user input if present
    QString outFile = parser.value(outFileOpt);
    if (outFile.isEmpty()) {
        outFile = dxfFileInfo.path() + "/" + fn + "." + args[0].mid(args[0].size()-3);
    } else {
        outFile = dxfFileInfo.path() + "/" + outFile;
    }

    // Open the file and process the graphics

    std::unique_ptr<RS_Document> doc = openDocAndSetGraphic(dxfFile);

    if (doc == nullptr || doc->getGraphic() == nullptr)
        return 1;
    RS_Graphic *graphic = doc->getGraphic();

    LC_LOG << "Printing" << dxfFile << "to" << outFile << ">>>>";

    touchGraphic(graphic);

    // Start of the actual conversion

    LC_LOG<< "QC_ApplicationWindow::slotFileExportSlide()";

    // read default settings:
    LC_GROUP_GUARD("Export"); // fixme settings
    QString defDir = dxfFileInfo.path();


    bool ret = false;
    ret = slotFileExportSlide(graphic, outFile);

    qDebug() << "Printing" << dxfFile << "to" << outFile << (ret ? "Done" : "Failed");
    return 0;
}

static std::unique_ptr<RS_Document> openDocAndSetGraphic(QString dxfFile){
    auto doc = std::make_unique<RS_Graphic>();
    LC_DocumentsStorage storage;
    if (!storage.loadDocument(doc.get(), dxfFile, RS2::FormatUnknown)) {
    // if (!doc->open(dxfFile, RS2::FormatUnknown)) {
        qDebug() << "ERROR: Failed to open document" << dxfFile;
        qDebug() << "Check if file exists";
        return {};
    }

    RS_Graphic* graphic = doc->getGraphic();
    if (graphic == nullptr) {
        qDebug() << "ERROR: No graphic in" << dxfFile;
        return {};
    }

    return doc;
}

static void touchGraphic(RS_Graphic* graphic)
{
    // If margin < 0.0, values from dxf file are used.
    double marginLeft = -1.0;
    double marginTop = -1.0;
    double marginRight = -1.0;
    double marginBottom = -1.0;

    int pagesH = 0;      // If number of pages < 1,
    int pagesV = 0;      // use value from dxf file.

    graphic->calculateBorders();
    graphic->setMargins(marginLeft, marginTop,
                        marginRight, marginBottom);
    graphic->setPagesNum(pagesH, pagesV);

    graphic->fitToPage(); // fit and center

}

bool slotFileExportSlide(RS_Graphic* graphic, const QString& name) {

    if (graphic==nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                "QC_ApplicationWindow::slotFileExportSlide: "
                "no graphic");
        return false;
    }

    QSize size;
    double width = graphic->getSize().x;
    double heigth = graphic->getSize().y;
    double ratio = width / heigth * 1.0;

    size.setWidth(static_cast<int>(width * ratio));
    size.setHeight(static_cast<int>(heigth * ratio));

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    bool ret = false;
    // set vars for normal picture
    QPixmap* picture = new QPixmap(size);

    // set buffer var
    QPaintDevice* buffer;
    buffer = picture;

    // set painter with buffer
    RS_Painter painter(buffer);

    painter.setBackground( Qt::black);
    painter.setDrawingMode( RS2::ModeWB);

    painter.eraseRect(0,0, size.width(), size.height());

    LC_GraphicViewport viewport;
    viewport.setSize(size.width(), size.height());
    viewport.setBorders(2, 2, 2, 2);

    viewport.setContainer(graphic);
    viewport.loadSettings();
    viewport.zoomAuto(false);

    LC_PrintViewportRenderer renderer(&viewport, &painter);
    renderer.loadSettings();
    renderer.setBackground(Qt::black);
    renderer.render();

    QImage image = picture->toImage();
    int rows = image.width();
    int cols = image.height();

    QImageWriter iio;
    iio.setFileName("test.ppm");
    iio.setFormat("PPM");
    iio.write(image);

    // GraphicView deletes painter
    painter.end();
    // delete vars
    delete picture;

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
            records.push_back(std::make_shared<SlideRecordVector>(rows - 1, i, ry, cols - 1));
        }
#endif
    }

    records.push_back(std::make_shared<SlideRecordEndOfFile>());
    if (create_slide(qUtf8Printable(name), rows, cols, 1.0 * rows / cols, records) == 0)
    {
        ret = true;
    }

    QApplication::restoreOverrideCursor();

    return ret;
}

#endif // DEVELOPER
