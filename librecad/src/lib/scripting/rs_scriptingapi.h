/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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

#ifndef RS_SCRIPTINGAPI_H
#define RS_SCRIPTINGAPI_H

#ifdef DEVELOPER

#include "rs_vector.h"
#include "rs_solid.h"
#include "rs_graphic.h"
#include "qc_applicationwindow.h"

#include <QString>
#include <QLineEdit>

#include "commandedit.h"
#include "document_interface.h"

#define RS_SCRIPTINGAPI RS_ScriptingApi::instance()

class RS_ScriptingApiData
{
public:
    RS_ScriptingApiData()
    {
        etype = block = layer = "";
    }
    ~RS_ScriptingApiData() {}

    QString etype;
    QString block;
    QString layer;

    std::vector<std::vector<double>> gc_10;
    std::vector<std::vector<double>> gc_11;
    std::vector<std::vector<double>> gc_12;
    std::vector<std::vector<double>> gc_13;
    std::vector<std::vector<double>> gc_14;
    std::vector<std::vector<double>> gc_15;
    std::vector<std::vector<double>> gc_16;

    std::vector<unsigned long> id;
    std::vector<int> gc_70;
    std::vector<int> gc_71;
    std::vector<int> gc_72;
    std::vector<int> gc_73;

    std::vector<double> gc_40;
    std::vector<double> gc_41;
    std::vector<double> gc_42;
    std::vector<double> gc_44;
    std::vector<double> gc_45;
    std::vector<double> gc_50;
    std::vector<double> gc_51;
    std::vector<QString> gc_100;
    std::vector<QString> text;
    std::vector<QString> style;

    RS_Pen pen;
};

class RS_ScriptingApi
{
public:
    static RS_ScriptingApi* instance();
    ~RS_ScriptingApi() {}

    void command(const QString &com);
    void endList();
    void endImage();
    void msgInfo(const char *msg);
    void termDialog();
    void unloadDialog(int id);
    void help(const QString &tag="");
    void initGet(int bit, const char *str);
    void prompt(CommandEdit *cmdline, const char *prompt);

    void addArc(double x, double y, double z, double rad, double ang1, double ang2, const RS_Pen &pen);
    void addCircle(double x, double y, double z, double rad, const RS_Pen &pen);
    void addDimRadial(const RS_DimensionData &data, const RS_DimRadialData &edata, const RS_Pen &pen);
    void addEllipse(double x1, double y1, double z1, double x2, double y2, double z2, double rad, const RS_Pen &pen);
    void addLine(double x1, double y1, double z1, double x2, double y2, double z2, const RS_Pen &pen);
    void addPoint(double x, double y, double z, const RS_Pen &pen);
    void addLwPolyline(const std::vector<Plug_VertexData> &points, bool closed, const RS_Pen &pen);
    void addSolid(const RS_SolidData &data, const RS_Pen &pen);
    void addSpline(const RS_SplineData data, const std::vector<RS_Vector> &points, const RS_Pen &pen);
    void addMText(const RS_Vector &pnt, double height, double width, double angle, int spacing, int direction, int attach, const QString &txt, const QString &style, const RS_Pen &pen);
    void addText(const RS_Vector &pnt, double height, double width, double angle, int valign, int halign, int generation, const QString &txt, const QString &style, const RS_Pen &pen);

    std::string copyright() const;
    std::string credits() const;
    std::string getEntityName(unsigned int id) const;
    std::string getEntityHndl(unsigned int id) const;
    std::string getStrDlg(const char *prompt) const;
    std::string getFileNameDlg(const char *title, const char *filename, const char *ext) const;

    unsigned int entlast();
    unsigned int entnext(unsigned int current=0);

    unsigned int getEntityId(const std::string &name);
    int getIntDlg(const char *prompt);

    int loadDialog(const char *filename);
    int startDialog();
    bool trueColorDialog(int &tres, int &res, int tcolor, int color, bool by, int tbycolor, int bycolor);

    double getDoubleDlg(const char *prompt);


    char readChar();

    bool actionTile(const char *id, const char *action);
    bool addLayer(const char *name, const RS_Pen &pen, int state);
    bool addList(const char *val, std::string &result);
    bool colorDialog(int color, bool by, int &res);
    bool dimxTile(const char *key, int &x);
    bool dimyTile(const char *key, int &y);
    bool doneDialog(int res, int &x, int &y);
    bool entdel(unsigned int id);
    bool entsel(CommandEdit *cmdline, const QString &prompt, unsigned long &id, RS_Vector &point);
    bool entmake(const RS_ScriptingApiData &apiData);
    bool entmod(RS_Entity *entity, const RS_ScriptingApiData &apiData);
    bool fillImage(int x, int y, int width, int height, int color);
    bool getAttr(const char *key, const char *attr, std::string &result);
    bool modeTile(const char *key, int mode);
    bool newDialog(const char *name, int id);
    bool pixImage(int x1, int y1, int x2, int y2, const char *path);
    bool setTile(const char *key, const char *val);
    bool slideImage(int x1, int y1, int x2, int y2, const char *path);
    bool textImage(int x1, int y1, int x2, int y2, const char *text, int color);
    bool vectorImage(int x1, int y1, int x2, int y2, int color);
    bool getFiled(const char *title, const char *def, const char *ext, int flags, std::string &filename);
    bool getDist(CommandEdit *cmdline, const char *msg, const RS_Vector &basePoint, double &distance);
    bool getOrient(CommandEdit *cmdline, const char *msg, const RS_Vector &basePoint, double &rad);
    bool getReal(CommandEdit *cmdline, const char *msg, double &res);
    bool getInteger(CommandEdit *cmdline, const char *msg, int &res);
    bool getString(CommandEdit *cmdline, bool cr, const char *msg, std::string &res);
    bool getKeyword(CommandEdit *cmdline, const char *msg, std::string &res);
    bool startImage(const char *key);
    bool startList(const char *key, int operation, int index);
    bool getTile(const char *key, std::string &result);

    RS_Vector getCorner(CommandEdit *cmdline, const char *msg, const RS_Vector &basePoint) const;
    RS_Vector getPoint(CommandEdit *cmdline, const char *msg, const RS_Vector basePoint) const;

    RS_EntityContainer* getContainer() const;
    RS_Document* getDocument() const;
    RS_Graphic *getGraphic() const;
    RS_GraphicView* getGraphicView() const;

private:
    RS_ScriptingApi() {}
    static RS_ScriptingApi* unique;
};

#endif // DEVELOPER

#endif // RS_PYTHONCORE_H


