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

#include "console_slidelib.h"

#ifdef DEVELOPER

#include <QtCore>
#include <QCoreApplication>
#include <QApplication>
#include <QDebug>

#include "main.h"

#include "rs_settings.h"
#include "rs_system.h"

#include <fstream>
#include <memory>

#include "slide.hpp"
#include "slide_binary_writer.hpp"
#include "slide_library.hpp"
#include "slide_library_binary_writer.hpp"

using namespace libslide;

static int create_slide_lib(const QStringList& sldfiles)
{
    QFileInfo libfileInfo(sldfiles.at(0));

    SlideLibrary lib
    {
        libfileInfo.baseName().toStdString(),
        SlideLibraryHeader{},
        {}, {}
    };

    for (const auto& sldfile : sldfiles)
    {
        QFileInfo slideFileInfo(sldfile);

        if (slideFileInfo.suffix().toLower() == "sld")
        {
            auto slide = Slide::from_file(sldfile.toStdString());
            lib.append(slide);
        }
    }

    std::ofstream ofs{sldfiles.at(0).toStdString(), std::ios::binary};
    write_slide_library_binary(ofs, lib);
    return 0;
}

int console_slidelib(int argc, char* argv[])
{
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
    QString librecad;
    QStringList slideFiles;

    appDesc += "\n slidelib usage: " + prgInfo.filePath()
    + " slidelib <slidelib_name> <slide_files>\n";

    appDesc += "\nCompiles slide files into a slidelib.";
    appDesc += "\n\n";
    appDesc += "Examples:\n\n";
    appDesc += "  " + librecad + " slidelib *.slb *.sld ...";
    appDesc += "    -- Compiles slide files into a slide library file.\n";
    parser.setApplicationDescription(appDesc);

    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("<files>", "Input slide lib and file name");
    parser.process(app);

    const QStringList args = parser.positionalArguments();

    if (args.isEmpty() || (args.size() == 1 && (args[0] == "slidelib")))
    {
        parser.showHelp(EXIT_FAILURE);
    }

    for (auto i = 1; i < args.size(); i++)
    {
        QFileInfo slideFileInfo(args.at(i));

        if (i == 1)
        {
            QString libname = args.at(1);
            if (slideFileInfo.suffix().toLower() != "slb" ||
                slideFileInfo.suffix().toUpper() != "SLB")
            {
                libname += ".slb";
            }
            slideFiles.append(libname);
        }
        else
        {
            QString slidename = args.at(i);
            if (slideFileInfo.suffix().toLower() != "sld" ||
                    slideFileInfo.suffix().toUpper() != "SLD")
            {
                slidename += ".sld";
            }

            slideFiles.append(slidename);
        }
    }

    if (slideFiles.isEmpty())
    {
        parser.showHelp(EXIT_FAILURE);
    }

    if (slideFiles.size() >= 1)
    {
        QFileInfo slideFileInfo(slideFiles.at(0));

        if (slideFileInfo.suffix().toLower() == "slb")
        {
            try
            {
                return create_slide_lib(slideFiles);
            }
            catch (const std::exception& e)
            {
                qDebug() << "Error: " << e.what();
                return 1;
            }
        }
        else
        {
            qDebug() << "Error: Invalid library extension: " << slideFiles.at(0);
            return 1;
        }
    }

    return app.exec();
}

#endif //DEVELOPER
