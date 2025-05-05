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

#ifndef RS_SCRIPTINGAPI_H
#define RS_SCRIPTINGAPI_H

#ifdef DEVELOPER

#include <QString>
#include <QLineEdit>
#include <QList>

#include "rs_dimension.h"
#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimlinear.h"
#include "rs_dimradial.h"
#include "rs_insert.h"
#include "rs_vector.h"
#include "rs_solid.h"
#include "rs_spline.h"
#include "rs_selection.h"
#include "rs_graphic.h"
#include "rs.h"
#include "qc_applicationwindow.h"

#include "commandedit.h"
#include "document_interface.h"

#define MAX_ENTITY_ID 13

#define RS_SCRIPTINGAPI RS_ScriptingApi::instance()

typedef struct entity_id_name {
    const char* name;
    RS2::EntityType id;
} entity_id_name_t;

static const entity_id_name_t entityIds[MAX_ENTITY_ID] = {
{ "INSERT", RS2::EntityInsert, }, // block
{ "POINT", RS2::EntityPoint, },
{ "LINE", RS2::EntityLine, },
{ "LWPOLYLINE", RS2::EntityPolyline, },
{ "ARC", RS2::EntityArc, },
{ "CIRCLE", RS2::EntityCircle, },
{ "ELLIPSE", RS2::EntityEllipse, },
{ "SOLID", RS2::EntitySolid, },
{ "MTEXT", RS2::EntityMText, },
{ "TEXT", RS2::EntityText, },
{ "HATCH", RS2::EntityHatch, },
{ "IMAGE", RS2::EntityImage, },
{ "SPLINE", RS2::EntitySpline, }
};

RS2::EntityType getEntityIdbyName(const QString &name);

typedef struct grdraw_line {
    RS_Vector start;
    RS_Vector end;
    int color = 0;
    bool highlight = false;
} grdraw_line_t;

class RS_ScriptingApiData
{
public:
    RS_ScriptingApiData()
    {
        etype = block = layer = text = style = linetype = "";
    }
    ~RS_ScriptingApiData() {}

    QString etype;
    QString block;
    QString layer;
    QString text;
    QString style;
    QString linetype;

    std::vector<RS_Vector> gc_10;
    std::vector<RS_Vector> gc_11;
    std::vector<RS_Vector> gc_12;
    std::vector<RS_Vector> gc_13;
    std::vector<RS_Vector> gc_14;
    std::vector<RS_Vector> gc_15;
    std::vector<RS_Vector> gc_16;

    std::vector<unsigned long> id;
    std::vector<int> gc_lineWidth;
    std::vector<int> gc_62;
    std::vector<int> gc_70;
    std::vector<int> gc_71;
    std::vector<int> gc_72;
    std::vector<int> gc_73;

    std::vector<int> gc_281;
    std::vector<int> gc_282;
    std::vector<int> gc_283;

    std::vector<int> gc_402;

    std::vector<double> gc_40;
    std::vector<double> gc_41;
    std::vector<double> gc_42;
    std::vector<double> gc_43;
    std::vector<double> gc_44;
    std::vector<double> gc_45;
    std::vector<double> gc_50;
    std::vector<double> gc_51;
    std::vector<double> gc_52;
    std::vector<double> gc_53;

    std::vector<QString> gc_100;

    RS_Pen pen;
};

class RS_ScriptingApi
{
public:
    enum SysVarResult {
        // NOTE: Concrete values are important here
        // to work with the combobox index!
        None = 0,
        Int = 1,
        Double = 2,
        String = 4,
        Vector2D = 8,
        Vector3D = 16
    };

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

    void addArc(const RS_Vector &pnt, double rad, double ang1, double ang2, const RS_Pen &pen);
    void addBlock(const RS_InsertData &data, const RS_Pen &pen);
    void addCircle(const RS_Vector &pnt, double rad, const RS_Pen &pen);
    void addDimAligned(const RS_DimensionData &data, const RS_DimAlignedData &edata, const RS_Pen &pen);
    void addDimAngular(const RS_DimensionData &data, const RS_DimAngularData &edata, const RS_Pen &pen);
    void addDimDiametric(const RS_DimensionData &data, const RS_DimDiametricData &edata, const RS_Pen &pen);
    void addDimLinear(const RS_DimensionData &data, const RS_DimLinearData &edata, const RS_Pen &pen);
    void addDimRadial(const RS_DimensionData &data, const RS_DimRadialData &edata, const RS_Pen &pen);
    void addEllipse(const RS_Vector &center, const RS_Vector &majorP, double rad, const RS_Pen &pen);
    void addLine(const RS_Vector &start, const RS_Vector &end, const RS_Pen &pen);
    void addPoint(const RS_Vector &pnt, const RS_Pen &pen);
    void addLwPolyline(const std::vector<Plug_VertexData> &points, bool closed, const RS_Pen &pen);
    void addSolid(const RS_SolidData &data, const RS_Pen &pen);
    void addSpline(const RS_SplineData data, const std::vector<RS_Vector> &points, const RS_Pen &pen);
    void addMText(const RS_Vector &pnt, double height, double width, double angle, int spacing, int direction, int attach, const QString &txt, const QString &style, const RS_Pen &pen);
    void addText(const RS_Vector &pnt, double height, double width, double angle, int valign, int halign, int generation, const QString &txt, const QString &style, const RS_Pen &pen);
    void grdraw(const RS_Vector &start, const RS_Vector &end, int color, bool highlight);
    void grvecs(const std::vector<grdraw_line_t> &lines);

    std::string copyright() const;
    std::string credits() const;
    std::string getEntityName(unsigned int id) const;
    std::string getEntityHndl(unsigned int id) const;
    std::string getSelectionName(unsigned int id) const;
    std::string getStrDlg(const char *prompt) const;
    std::string getFileNameDlg(const char *title, const char *filename, const char *ext) const;

    unsigned int entlast();
    unsigned int entnext(unsigned int current=0);

    unsigned int getEntityId(const std::string &name);
    unsigned int getNewSelectionId() { m_selectionId++; return m_selectionId; }
    unsigned int getSelectionId(const std::string &name);
    unsigned int sslength(const std::string &name);
    int getIntDlg(const char *prompt);

    int loadDialog(const char *filename);
    int startDialog();

    bool setvar(const char *var, double v1, double v2, const char *str_value);
    SysVarResult getvar(const char *var, int &int_result, double &v1_result, double &v2_result, double &v3_result, std::string &str_result);

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
    bool pixImage(int x, int y, int width, int height, const char *path);
    bool setTile(const char *key, const char *val);
    bool slideImage(int x, int y, int width, int height, const char *path);
    bool textImage(int x, int y, int width, int height, const char *text, int color);
    bool vectorImage(int x1, int y1, int x2, int y2, int color);
    bool getFiled(const char *title, const char *def, const char *ext, int flags, std::string &filename);
    bool getDist(CommandEdit *cmdline, const char *msg, const RS_Vector &basePoint, double &distance);
    bool getAngle(CommandEdit *cmdline, const char *msg, const RS_Vector &basePoint, double &rad);
    bool getOrient(CommandEdit *cmdline, const char *msg, const RS_Vector &basePoint, double &rad);
    bool getReal(CommandEdit *cmdline, const char *msg, double &res);
    bool getInteger(CommandEdit *cmdline, const char *msg, int &res);
    bool getString(CommandEdit *cmdline, bool cr, const char *msg, std::string &res);
    bool getSelection(unsigned int &id);
    bool getSingleSelection(std::vector<unsigned int> &selection_set);
    bool selectAll(std::vector<unsigned int> &selection_set);
    bool getSelectionByData(const RS_ScriptingApiData &apiData, std::vector<unsigned int> &selection_set);
    bool filterByData(RS_Entity *entity, const RS_ScriptingApiData &apiData);
    bool getKeyword(CommandEdit *cmdline, const char *msg, std::string &res);
    bool ssadd(unsigned int id, unsigned int ss, unsigned int &newss);
    bool ssname(unsigned int ss, unsigned int idx, unsigned int &id);
    bool ssdel(unsigned int id, unsigned int ss);
    bool startImage(const char *key);
    bool startList(const char *key, int operation, int index);
    bool getTile(const char *key, std::string &result);

    RS_Vector getCorner(CommandEdit *cmdline, const char *msg, const RS_Vector &basePoint) const;
    RS_Vector getPoint(CommandEdit *cmdline, const char *msg, const RS_Vector basePoint) const;

    RS_EntityContainer* getContainer() const;
    RS_Document* getDocument() const;
    RS_Graphic *getGraphic() const;
    RS_GraphicView* getGraphicView() const;
    LC_GraphicViewport* getViewPort() const;

private:
    RS_ScriptingApi() : m_selectionId(0) { }
    static RS_ScriptingApi* unique;
    unsigned int m_selectionId;
};

#endif // DEVELOPER

#endif // RS_PYTHONCORE_H


