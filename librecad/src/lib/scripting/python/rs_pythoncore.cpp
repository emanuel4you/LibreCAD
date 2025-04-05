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

#include "rs_python.h"
#include "rs_pythoncore.h"
#include "rs_scriptingapi.h"
#include "rs_insert.h"
#include "rs_filterdxfrw.h"

#include "lc_defaults.h"

bool fitTolerance(PyObject *pList, RS_ScriptingApiData &apiData);

RS_Document* RS_PythonCore::getDocument() const
{
    return RS_SCRIPTINGAPI->getDocument();
}

RS_EntityContainer* RS_PythonCore::getContainer() const
{
    return RS_SCRIPTINGAPI->getContainer();
}

RS_Graphic* RS_PythonCore::getGraphic() const
{
    return RS_SCRIPTINGAPI->getGraphic();
}

void RS_PythonCore::command(const char *cmd)
{
    QString scmd = cmd;
    scmd = scmd.simplified();
    QStringList coms = scmd.split(" ");

    for(auto & s : coms)
    {
        RS_SCRIPTINGAPI->command(s);
    }
}

int RS_PythonCore::sslength(const char *ss)
{
    return RS_SCRIPTINGAPI->sslength(ss);
}

double RS_PythonCore::angle(PyObject *pnt1, PyObject *pnt2) const
{
    qDebug() << "[RS_PythonCore::angle] - start";

    double x1, x2, y1, y2;

    PyObject *pPnt1;
    PyObject *pPnt2;

    if (!PyArg_Parse(pnt1, "O!", &PyTuple_Type, &pPnt1)) {
        PyErr_SetString(PyExc_TypeError, "point must be a tuple.");
        return 0.0;
    }

    if (!PyArg_Parse(pnt2, "O!", &PyTuple_Type, &pPnt2)) {
        PyErr_SetString(PyExc_TypeError, "point must be a tuple.");
        return 0.0;
    }

    if (PyTuple_Size(pPnt1) < 2 || PyTuple_Size(pPnt2) < 2)
    {
        PyErr_SetString(PyExc_TypeError, "point must have x and y.");
        return 0.0;
    }

    x1 = PyFloat_AsDouble(PyTuple_GetItem(pPnt1, 0));
    y1 = PyFloat_AsDouble(PyTuple_GetItem(pPnt1, 1));
    x2 = PyFloat_AsDouble(PyTuple_GetItem(pPnt2, 0));
    y2 = PyFloat_AsDouble(PyTuple_GetItem(pPnt2, 1));

    return std::atan2(y2 - y1, x2 - x1);
}

PyObject *RS_PythonCore::assoc(int needle, PyObject *args) const
{
    qDebug() << "[RS_PythonCore::assoc] - start";

    PyObject *pList;

    if (!PyArg_Parse(args, "O!", &PyList_Type, &pList)) {
        PyErr_SetString(PyExc_TypeError, "parameter must be a entity list.");
        Py_RETURN_NONE;
    }

    int gc;
    PyObject *pTuple;
    PyObject *pGc;
    Py_ssize_t n = PyList_Size(pList);

    for (int i=0; i<n; i++) {
        pTuple = PyList_GetItem(pList, i);
        if(!PyTuple_Check(pTuple)) {
            PyErr_SetString(PyExc_TypeError, "list items must be a tuple.");
            Py_RETURN_NONE;
        }
        pGc = PyTuple_GetItem(pTuple, 0);
        if(!PyLong_Check(pGc)) {
            PyErr_SetString(PyExc_TypeError, "first tuple item must be an integer.");
            Py_RETURN_NONE;
        }
        gc = PyLong_AsLong(pGc);
        qDebug() << "[RS_PythonCore::assoc] i:" << i << "GC:" << gc;

        if(gc == needle)
        {
            return pTuple;
        }
    }

    Py_RETURN_NONE;
}

PyObject *RS_PythonCore::entlast() const
{
    unsigned int id = RS_SCRIPTINGAPI->entlast();
    return id > 0 ? Py_BuildValue("s", RS_SCRIPTINGAPI->getEntityName(id).c_str()) : Py_None;
}

PyObject *RS_PythonCore::entdel(const char *ename) const
{
    return RS_SCRIPTINGAPI->entdel(RS_SCRIPTINGAPI->getEntityId(ename)) ? Py_BuildValue("s", ename) : Py_None;
}

PyObject *RS_PythonCore::entmake(PyObject *args) const
{
    qDebug() << "[RS_PythonCore::entmake] - start";

    PyObject *pList;

    if (!PyArg_Parse(args, "O!", &PyList_Type, &pList)) {
        PyErr_SetString(PyExc_TypeError, "parameter must be a entity list.");
        Py_RETURN_NONE;
    }

    RS_ScriptingApiData apiData;

    if (!fitTolerance(pList, apiData) || apiData.etype == "")
    {
        Py_RETURN_NONE;
    }

    if (RS_SCRIPTINGAPI->entmake(apiData))
    {
        return entget(RS_SCRIPTINGAPI->getEntityName(RS_SCRIPTINGAPI->entlast()).c_str());
    }

    Py_RETURN_NONE;
}

PyObject *RS_PythonCore::entmod(PyObject *args) const
{
    qDebug() << "[RS_PythonCore::entmod] - start";

    PyObject *pList;

    if (!PyArg_Parse(args, "O!", &PyList_Type, &pList)) {
        PyErr_SetString(PyExc_TypeError, "parameter must be a entity list.");
        Py_RETURN_NONE;
    }

    unsigned int entityId = 0;

    int gc;
    PyObject *pTuple;
    PyObject *pGc;
    PyObject *pValue;
    Py_ssize_t n = PyList_Size(pList);

    for (int i=0; i<n; i++) {
        pTuple = PyList_GetItem(pList, i);
        if(!PyTuple_Check(pTuple)) {
            PyErr_SetString(PyExc_TypeError, "list items must be a tuple.");
            Py_RETURN_NONE;
        }
        pGc = PyTuple_GetItem(pTuple, 0);
        if(!PyLong_Check(pGc)) {
            PyErr_SetString(PyExc_TypeError, "first tuple item must be an integer.");
            Py_RETURN_NONE;
        }
        gc = PyLong_AsLong(pGc);
        qDebug() << "[RS_PythonCore::entmake] i:" << i << "GC:" << gc;

        if (gc == -1)
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyLong_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "first tuple item must be an integer.");
                Py_RETURN_NONE;
            }
            entityId = RS_SCRIPTINGAPI->getEntityId(PyUnicode_AsUTF8(pValue));
            break;
        }
    }

    if (entityId == 0)
    {
        Py_RETURN_NONE;
    }

    RS_EntityContainer* entityContainer = RS_SCRIPTINGAPI->getContainer();

    if(entityContainer->count())
    {
        for (auto entity: *entityContainer)
        {
            if (entity->getId() == entityId)
            {
                RS_ScriptingApiData apiData;
                apiData.pen = entity->getPen(false);
                if (!fitTolerance(pList, apiData) || apiData.id.empty())
                {
                    Py_RETURN_NONE;
                }
                entity->setPen(apiData.pen);

                if (RS_SCRIPTINGAPI->entmod(entity, apiData))
                {
                    return entget(RS_SCRIPTINGAPI->getEntityName(apiData.id.front()).c_str());
                }
            }
        }
    }

    qDebug() << "[RS_PythonCore::entmod] - end";

    Py_RETURN_NONE;
}

PyObject *RS_PythonCore::entnext(const char *ename) const
{
    unsigned int id = 0;

    if (std::strcmp(ename, "") == 0)
    {
        id = RS_SCRIPTINGAPI->entnext();
    }
    else
    {
        id = RS_SCRIPTINGAPI->entnext(RS_SCRIPTINGAPI->getEntityId(ename));
    }

    return id > 0 ? Py_BuildValue("s", RS_SCRIPTINGAPI->getEntityName(id).c_str()) : Py_None;
}

PyObject *RS_PythonCore::entsel(const char* prompt) const
{
    QString prom = "Select object:";
    unsigned long id;
    RS_Vector result;

    if (std::strcmp(prompt, "") == 0)
    {
        prom = prompt;
    }

    return RS_SCRIPTINGAPI->entsel(Py_CommandEdit,
                                   QObject::tr(qUtf8Printable(prom)),
                                   id,
                                   result) ? Py_BuildValue("(s(ddd))", RS_SCRIPTINGAPI->getEntityName(id).c_str(),
                                                                       result.x,
                                                                       result.y,
                                                                       result.z) : Py_None;
}

PyObject *RS_PythonCore::entget(const char *ename) const
{
    unsigned int id = RS_SCRIPTINGAPI->getEntityId(ename);
    RS_EntityContainer* entityContainer = RS_SCRIPTINGAPI->getContainer();

    if(entityContainer->count())
    {
        for (auto e: *entityContainer) {
            if (e->getId() == id)
            {
                int exact_rgb;
                RS_Pen pen = e->getPen(false);
                RS_Color color = pen.getColor();
                enum RS2::LineWidth lineWidth = pen.getWidth();
                int width = static_cast<int>(lineWidth);

                switch (e->rtti())
                {
                    case RS2::EntityPoint:
                    {
                        RS_Point* p = (RS_Point*)e;

                        return Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(iddd)]",
                            0, "POINT",
                            -1, ename,
                            330, p->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(p->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(p->getLayer()->getName()),
                            100, "AcDbPoint",
                            10, p->getPos().x, p->getPos().y, p->getPos().z,
                            210, 0.0, 0.0, 1.0
                        );
                    }
                    break;
                    case RS2::EntityLine:
                    {
                        RS_Line* l = (RS_Line*)e;
                        return Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(iddd)(iddd)]",
                            0, "LINE",
                            -1, ename,
                            330, l->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(l->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(l->getLayer()->getName()),
                            100, "AcDbLine",
                            10, l->getStartpoint().x, l->getStartpoint().y, l->getStartpoint().z,
                            11, l->getEndpoint().x, l->getEndpoint().y, l->getEndpoint().z,
                            210, 0.0, 0.0, 1.0
                        );
                    }
                    break;
                    case RS2::EntityPolyline:
                    {
                        RS_Polyline* pl = (RS_Polyline*)e;
                        bool is3d = false;
                        RS_VectorSolutions pnts = pl->getRefPoints();

                        for (auto &v : pl->getRefPoints())
                        {
                            if(v.z != 0.0)
                            {
                                is3d = true;
                                break;
                            }
                        }

                        int fl = 0;
                        fl |= 8;
                        if (pl->isClosed())
                        {
                            fl |= 1;
                        }

                        if (is3d)
                        {
                            return Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(ii)(iddd)]",
                                0, "POLYLINE",
                                -1, ename,
                                330, pl->getParent()->getId(),
                                5, RS_SCRIPTINGAPI->getEntityHndl(pl->getId()).c_str(),
                                6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                                62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                                48, width < 0 ? 1.0 : double(width) / 100.0,
                                100, "AcDbEntity",
                                67, 0,
                                410, "Model",
                                8, qUtf8Printable(pl->getLayer()->getName()),
                                100, "AcDb3dPolyline",
                                70, fl,
                                210, 0.0, 0.0, 1.0
                            );
                        }
                        else
                        {
                            PyObject* list = PyList_New(pnts.size()*4 + 14);
                            PyList_SET_ITEM(list, 0, Py_BuildValue("(is)", 0, "LWPOLYLINE"));
                            PyList_SET_ITEM(list, 1, Py_BuildValue("(is)", -1, ename));
                            PyList_SET_ITEM(list, 2, Py_BuildValue("(ii)", 330, pl->getParent()->getId()));
                            PyList_SET_ITEM(list, 3, Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(pl->getId()).c_str()));
                            PyList_SET_ITEM(list, 4, Py_BuildValue("(is)", 6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType()))));
                            PyList_SET_ITEM(list, 5, Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)));
                            PyList_SET_ITEM(list, 6, width < 0 ? Py_BuildValue("(id)", 48, 1.0) : Py_BuildValue("(id)", 48, double(width) / 100.0));
                            PyList_SET_ITEM(list, 7, Py_BuildValue("(is)", 100, "AcDbEntity"));
                            PyList_SET_ITEM(list, 8, Py_BuildValue("(ii)", 67, 0));
                            PyList_SET_ITEM(list, 9, Py_BuildValue("(is)", 410, "Model"));
                            PyList_SET_ITEM(list, 10, Py_BuildValue("(is)", 8, qUtf8Printable(pl->getLayer()->getName())));
                            PyList_SET_ITEM(list, 11, Py_BuildValue("(is)", 100, "AcDbPolyline"));
                            int n = 12;

                            for (auto &v : pl->getRefPoints())
                            {
                                PyList_SET_ITEM(list, n++, Py_BuildValue("(idd)", 10, v.x, v.y));
                                PyList_SET_ITEM(list, n++, Py_BuildValue("(id)", 40, 0.0));
                                PyList_SET_ITEM(list, n++, Py_BuildValue("(id)", 41, 0.0));
                                PyList_SET_ITEM(list, n++, Py_BuildValue("(id)", 42, 0.0));
                            }

                            PyList_SET_ITEM(list, n++, Py_BuildValue("(ii)", 70, fl));
                            PyList_SET_ITEM(list, n++, Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0));

                            return list;
                        }
                    }
                    break;
                    case RS2::EntityArc:
                    {
                        RS_Arc* a = (RS_Arc*)e;
                        return Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(id)(id)(id)(iddd)]",
                            0, "ARC",
                            -1, ename,
                            330, a->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(a->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(a->getLayer()->getName()),
                            100, "AcDbArc",
                            10, a->getCenter().x, a->getCenter().y, a->getCenter().z,
                            40, a->getRadius(),
                            50, a->isReversed() ? a->getAngle2() : a->getAngle1(),
                            51, a->isReversed() ? a->getAngle1() : a->getAngle2(),
                            210, 0.0, 0.0, 1.0
                        );
                    }
                    break;
                    case RS2::EntityCircle:
                    {
                        RS_Circle* c = (RS_Circle*)e;
                        return Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(id)(iddd)]",
                            0, "CIRCLE",
                            -1, ename,
                            330, c->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(c->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(c->getLayer()->getName()),
                            100, "AcDbCircle",
                            10, c->getCenter().x, c->getCenter().y, c->getCenter().z,
                            40, c->getRadius(),
                            210, 0.0, 0.0, 1.0
                        );
                    }
                    break;
                    case RS2::EntityEllipse:
                    {
                        RS_Ellipse* ellipse=static_cast<RS_Ellipse*>(e);
                        return Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(iddd)(id)(id)(id)(iddd)]",
                            0, "ELLIPSE",
                            -1, ename,
                            330, ellipse->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(ellipse->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(ellipse->getLayer()->getName()),
                            100, "AcDbEllipse",
                            10, ellipse->getCenter().x, ellipse->getCenter().y, ellipse->getCenter().z,
                            11, ellipse->getMajorP().x, ellipse->getMajorP().y, ellipse->getMajorP().z,
                            40, ellipse->getRatio(),
                            41, ellipse->isReversed() ? ellipse->getData().angle2 : ellipse->getData().angle1,
                            42, ellipse->isReversed() ? ellipse->getData().angle1 : ellipse->getData().angle2,
                            210, 0.0, 0.0, 1.0
                        );
                    }
                    break;
                    case RS2::EntityDimAligned:
                    {
                        RS_DimAligned* dal = (RS_DimAligned*)e;
                        return Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(iddd)(iddd)(iddd)(id)(is)(is)]",
                            0, "DIMENSION",
                            -1, ename,
                            330, dal->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(dal->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(dal->getLayer()->getName()),
                            100, "AcDbDimension",
                            10, dal->getDefinitionPoint().x, dal->getDefinitionPoint().y,dal->getDefinitionPoint().z,
                            11, dal->getMiddleOfText().x, dal->getMiddleOfText().y,dal->getMiddleOfText().z,
                            13, dal->getExtensionPoint1().x, dal->getExtensionPoint1().y, dal->getExtensionPoint1().z,
                            14, dal->getExtensionPoint2().x, dal->getExtensionPoint2().y, dal->getExtensionPoint2().z,
                            41, dal->getLineSpacingFactor(),
                            3, qUtf8Printable(dal->getData().style),
                            100, "AcDbAlignedDimension"
                        );
                    }
                    break;
                    case RS2::EntityDimAngular:
                    {
                        RS_DimAngular* da = (RS_DimAngular*)e;
                        return Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(iddd)(iddd)(iddd)(iddd)(iddd)(id)(is)(is)]",
                            0, "DIMENSION",
                            -1, ename,
                            330, da->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(da->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(da->getLayer()->getName()),
                            100, "AcDbDimension",
                            10, da->getDefinitionPoint().x, da->getDefinitionPoint().y,da->getDefinitionPoint().z,
                            11, da->getMiddleOfText().x, da->getMiddleOfText().y,da->getMiddleOfText().z,
                            13, da->getDefinitionPoint1().x, da->getDefinitionPoint1().y, da->getDefinitionPoint1().z,
                            14, da->getDefinitionPoint2().x, da->getDefinitionPoint2().y, da->getDefinitionPoint2().z,
                            13, da->getDefinitionPoint3().x, da->getDefinitionPoint3().y, da->getDefinitionPoint3().z,
                            14, da->getDefinitionPoint4().x, da->getDefinitionPoint4().y, da->getDefinitionPoint4().z,
                            41, da->getLineSpacingFactor(),
                            3, qUtf8Printable(da->getData().style),
                            100, "AcDb3PointAngularDimension"
                        );
                    }
                    break;
                    case RS2::EntityDimLinear:
                    {
                        RS_DimLinear* d = (RS_DimLinear*)e;
                        return Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(iddd)(iddd)(iddd)(id)(id)(is)(is)(is)]",
                            0, "DIMENSION",
                            -1, ename,
                            330, d->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(d->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(d->getLayer()->getName()),
                            100, "AcDbDimension",
                            10, d->getDefinitionPoint().x, d->getDefinitionPoint().y,d->getDefinitionPoint().z,
                            11, d->getMiddleOfText().x, d->getMiddleOfText().y, d->getMiddleOfText().z,
                            13, d->getExtensionPoint1().x, d->getExtensionPoint1().y, d->getExtensionPoint1().z,
                            14, d->getExtensionPoint2().x, d->getExtensionPoint2().y, d->getExtensionPoint2().z,
                            50, d->getRadius(),
                            41, d->getLineSpacingFactor(),
                            3, qUtf8Printable(d->getData().style),
                            100, "AcDbAlignedDimension",
                            100, "AcDbRotatedDimension"
                        );
                    }
                    break;
                    case RS2::EntityDimRadial:
                    {
                        RS_DimRadial* dr = (RS_DimRadial*)e;
                        return Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(iddd)(id)(id)(is)(is)]",
                            0, "DIMENSION",
                            -1, ename,
                            330, dr->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(dr->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(dr->getLayer()->getName()),
                            100, "AcDbDimension",
                            10, dr->getDefinitionPoint().x, dr->getDefinitionPoint().y, dr->getDefinitionPoint().z,
                            11, dr->getMiddleOfText().x, dr->getMiddleOfText().y, dr->getMiddleOfText().z,
                            40, dr->getRadius(),
                            41, dr->getLineSpacingFactor(),
                            3, qUtf8Printable(dr->getData().style),
                            100, "AcDbRadialDimension"
                        );
                    }
                    break;
                    case RS2::EntityDimDiametric:
                    {
                        RS_DimDiametric* dd = (RS_DimDiametric*)e;
                        return Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(iddd)(id)(id)(is)(is)]",
                            0, "DIMENSION",
                            -1, ename,
                            330, dd->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(dd->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(dd->getLayer()->getName()),
                            100, "AcDbDimension",
                            10, dd->getDefinitionPoint().x, dd->getDefinitionPoint().y, dd->getDefinitionPoint().z,
                            11, dd->getMiddleOfText().x, dd->getMiddleOfText().y, dd->getMiddleOfText().z,
                            40, dd->getRadius(),
                            41, dd->getLineSpacingFactor(),
                            3, qUtf8Printable(dd->getData().style),
                            100, "AcDbRadialDimension"
                        );
                    }
                    break;
                    case RS2::EntityInsert:
                    {
                        RS_Insert* i = (RS_Insert*)e;
                        return Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(id)(id)(id)(ii)(ii)(id)(id)(iddd)]",
                            0, "INSERT",
                            -1, ename,
                            330, i->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(i->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(i->getLayer()->getName()),
                            100, "AcDbBlockReference",
                            10, i->getInsertionPoint().x, i->getInsertionPoint().y, i->getInsertionPoint().z,
                            41, i->getScale().x,
                            42, i->getScale().y,
                            50, i->getRadius(),
                            70, i->getCols(),
                            71, i->getRows(),
                            44, i->getSpacing().y,
                            45, i->getSpacing().x,
                            210, 0.0, 0.0, 1.0
                        );
                    }
                    break;
                    case RS2::EntityMText:
                    {
                        RS_MText* t = (RS_MText*)e;
                        RS_MTextData::MTextDrawingDirection dir = t->getDrawingDirection();
                        unsigned int dxfDir = 5;

                        switch(dir)
                        {
                            case RS_MTextData::MTextDrawingDirection::LeftToRight:
                                dxfDir = 1;
                                break;
                            case RS_MTextData::MTextDrawingDirection::RightToLeft:
                                dxfDir = 2;
                                break;
                            case RS_MTextData::MTextDrawingDirection::TopToBottom:
                                dxfDir = 3;
                                break;
                            default:
                                dxfDir = 5;
                        }

                        RS_MTextData::MTextLineSpacingStyle style = t->getLineSpacingStyle();
                        unsigned int styleDxf = 2;

                        if (style == RS_MTextData::MTextLineSpacingStyle::AtLeast)
                        {
                            styleDxf = 1;
                        }

                        return Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(id)(id)(ii)(id)(is)(is)(id)(id)(ii)(id)(iddd)]",
                            0, "MTEXT",
                            -1, ename,
                            330, t->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(t->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(t->getLayer()->getName()),
                            100, "AcDbMText",
                            10, t->getInsertionPoint().x, t->getInsertionPoint().y, t->getInsertionPoint().z,
                            40, t->getUsedTextHeight(),
                            41, t->getWidth(),
                            71, t->getAlignment(),
                            72, dxfDir,
                            1, qUtf8Printable(t->getText()),
                            7, qUtf8Printable(t->getStyle()),
                            43, t->getHeight(),
                            50, t->getAngle()*180/M_PI,
                            73, styleDxf,
                            44, t->getLineSpacingFactor(),
                            210, 0.0, 0.0, 1.0
                        );
                    }
                    break;
                    case RS2::EntityText:
                    {
                        RS_Text* t = (RS_Text*)e;
                        double x=0.0,y=0.0,z=0.0;

                        if (t->getVAlign() != RS_TextData::VABaseline || t->getHAlign() != RS_TextData::HALeft)
                        {
                            if (t->getHAlign() == RS_TextData::HAAligned || t->getHAlign() == RS_TextData::HAFit)
                            {
                                x = t->getInsertionPoint().x;
                                y = t->getInsertionPoint().y;
                                z = t->getInsertionPoint().z;
                            }
                            else
                            {
                                x = t->getInsertionPoint().x;
                                y = t->getInsertionPoint().y;
                                z = t->getInsertionPoint().z;
                            }
                        }

                        return Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(id)(is)(id)(id)(is)(ii)(ii)(iddd)(ii)(iddd)]",
                            0, "TEXT",
                            -1, ename,
                            330, t->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(t->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(t->getLayer()->getName()),
                            100, "AcDbText",
                            10, t->getInsertionPoint().x, t->getInsertionPoint().y, t->getInsertionPoint().z,
                            40, t->getHeight(),
                            1, qUtf8Printable(t->getText()),
                            50, t->getAngle()*180/M_PI,
                            41, t->getWidthRel(),
                            7, qUtf8Printable(t->getStyle()),
                            71, static_cast<int>(t->getTextGeneration()),
                            72, static_cast<int>(t->getHAlign()),
                            11, x, y, z,
                            73, t->getVAlign(),
                            210, 0.0, 0.0, 1.0
                        );
                    }
                    break;
                    case RS2::EntityHatch:
                    {
                        RS_Hatch* h = (RS_Hatch*)e;
                        return h->isSolid() ?
                        Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(iddd)(is)(ii)]",
                            0, "HATCH",
                            -1, ename,
                            330, h->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(h->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(h->getLayer()->getName()),
                            100, "AcDbHatch",
                            10, 0.0, 0.0, 0.0,
                            210, 0.0, 0.0, 1.0,
                            2, qUtf8Printable(h->getPattern()),
                            70, 1
                        )

                        :Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(iddd)(is)(id)(id)(ii)]",
                            0, "HATCH",
                            -1, ename,
                            330, h->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(h->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(h->getLayer()->getName()),
                            100, "AcDbHatch",
                            10, 0.0, 0.0, 0.0,
                            210, 0.0, 0.0, 1.0,
                            2, qUtf8Printable(h->getPattern()),
                            52, h->getAngle(),
                            41, h->getScale(),
                            70, 0
                        );

                    }
                    break;
                    case RS2::EntitySolid:
                    {
                        RS_Solid* sol = (RS_Solid*)e;
                        return sol->isTriangle() ?
                        Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(iddd)(iddd)(iddd)]",
                            0, "SOLID",
                            -1, ename,
                            330, sol->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(sol->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(sol->getLayer()->getName()),
                            100, "AcDbTrace",
                            10, sol->getCorner(0).x, sol->getCorner(0).y, sol->getCorner(0).z,
                            11, sol->getCorner(1).x, sol->getCorner(1).y, sol->getCorner(1).z,
                            12, sol->getCorner(2).x, sol->getCorner(2).y, sol->getCorner(2).z,
                            210, 0.0, 0.0, 1.0
                        )

                        : Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(iddd)(iddd)(iddd)(iddd)(iddd)]",
                            0, "SOLID",
                            -1, ename,
                            330, sol->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(sol->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(sol->getLayer()->getName()),
                            100, "AcDbTrace",
                            10, sol->getCorner(0).x, sol->getCorner(0).y, sol->getCorner(0).z,
                            11, sol->getCorner(1).x, sol->getCorner(1).y, sol->getCorner(1).z,
                            12, sol->getCorner(2).x, sol->getCorner(2).y, sol->getCorner(2).z,
                            13, sol->getCorner(3).x, sol->getCorner(3).y, sol->getCorner(3).z,
                            210, 0.0, 0.0, 1.0
                        );
                    }
                    break;
                    case RS2::EntitySpline:
                    {
                        RS_Spline* spl = (RS_Spline*)e;
                        return Py_BuildValue("[(is)(is)(ii)(is)(is)(ii)(id)(is)(ii)(is)(is)(is)(ii)(ii)(ii)(ii)(ii)(iddd)]",
                            0, "SPLINE",
                            -1, ename,
                            330, spl->getParent()->getId(),
                            5, RS_SCRIPTINGAPI->getEntityHndl(spl->getId()).c_str(),
                            6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType())),
                            62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb),
                            48, width < 0 ? 1.0 : double(width) / 100.0,
                            100, "AcDbEntity",
                            67, 0,
                            410, "Model",
                            8, qUtf8Printable(spl->getLayer()->getName()),
                            100, "AcDbSpline",
                            spl->isClosed() ? Py_BuildValue("(ii)", 70, 1) : Py_BuildValue("(ii)", 70, 8),
                            71, spl->getDegree(),
                            72, spl->getNumberOfKnots(),
                            73, static_cast<int>(spl->getNumberOfControlPoints()),
                            74, 0,
                            210, 0.0, 0.0, 1.0
                        );
                    }
                    break;
                    case RS2::EntityImage:
                    {
                        RS_Image* img = (RS_Image*)e;
                        RS_VectorSolutions pnts = img->getCorners();

                        PyObject* list = PyList_New(pnts.size() + 23);
                        PyList_SET_ITEM(list, 0, Py_BuildValue("(is)", 0, "IMAGE"));
                        PyList_SET_ITEM(list, 1, Py_BuildValue("(is)", -1, ename));
                        PyList_SET_ITEM(list, 2, Py_BuildValue("(ii)", 330, img->getParent()->getId()));
                        PyList_SET_ITEM(list, 3, Py_BuildValue("(is)", 5, RS_SCRIPTINGAPI->getEntityHndl(img->getId()).c_str()));
                        PyList_SET_ITEM(list, 4, Py_BuildValue("(is)", 6, qUtf8Printable(RS_FilterDXFRW::lineTypeToName(pen.getLineType()))));
                        PyList_SET_ITEM(list, 5, Py_BuildValue("(ii)", 62, RS_FilterDXFRW::colorToNumber(color, &exact_rgb)));
                        PyList_SET_ITEM(list, 6, width < 0 ? Py_BuildValue("(id)", 48, 1.0) : Py_BuildValue("(id)", 48, double(width) / 100.0));
                        PyList_SET_ITEM(list, 7, Py_BuildValue("(is)", 100, "AcDbEntity"));
                        PyList_SET_ITEM(list, 8, Py_BuildValue("(ii)", 67, 0));
                        PyList_SET_ITEM(list, 9, Py_BuildValue("(is)", 410, "Model"));
                        PyList_SET_ITEM(list, 10, Py_BuildValue("(is)", 8, qUtf8Printable(img->getLayer()->getName())));
                        PyList_SET_ITEM(list, 11, Py_BuildValue("(iddd)", 10, img->getInsertionPoint().x, img->getInsertionPoint().y, img->getInsertionPoint().z));
                        PyList_SET_ITEM(list, 12, Py_BuildValue("(iddd)", 11, img->getUVector().x, img->getUVector().y, img->getUVector().z));
                        PyList_SET_ITEM(list, 13, Py_BuildValue("(iddd)", 12, img->getVVector().x, img->getVVector().y, img->getVVector().z));
                        PyList_SET_ITEM(list, 14, Py_BuildValue("(iii)", 13, img->getWidth(), img->getHeight()));
                        PyList_SET_ITEM(list, 15, Py_BuildValue("(is)", 340, qUtf8Printable(img->getFile())));
                        PyList_SET_ITEM(list, 16, Py_BuildValue("(ii)", 70, 1));
                        PyList_SET_ITEM(list, 17, Py_BuildValue("(ii)", 280, 0));
                        PyList_SET_ITEM(list, 18, Py_BuildValue("(ii)", 281, img->getBrightness()));
                        PyList_SET_ITEM(list, 19, Py_BuildValue("(ii)", 282, img->getContrast()));
                        PyList_SET_ITEM(list, 20, Py_BuildValue("(ii)", 283, img->getFade()));
                        PyList_SET_ITEM(list, 21, Py_BuildValue("(ii)", 71, 1));
                        int n = 22;

                        for (auto &v : img->getCorners())
                        {
                            PyList_SET_ITEM(list, n++, Py_BuildValue("(iddd)", 14, v.x, v.y, v.z));
                        }

                        PyList_SET_ITEM(list, n++, Py_BuildValue("(iddd)", 210, 0.0, 0.0, 1.0));

                        return list;
                    }
                    break;
                default:
                    break;
                }

            }
        }
    }

    Py_RETURN_NONE;
}

PyObject *RS_PythonCore::polar(PyObject *pnt, double ang, double dist) const
{
    PyObject *pPnt;
    double x, y, z;

    if (!PyArg_Parse(pnt, "O!", &PyTuple_Type, &pPnt)) {
        PyErr_SetString(PyExc_TypeError, "point must be a tuple.");
        Py_RETURN_NONE;
    }

    Py_ssize_t n = PyTuple_Size(pPnt);

    if (n < 2)
    {
        PyErr_SetString(PyExc_TypeError, "point must have x and y.");
        Py_RETURN_NONE;
    }

    x = PyFloat_AsDouble(PyTuple_GetItem(pPnt, 0));
    y = PyFloat_AsDouble(PyTuple_GetItem(pPnt, 1));

    if (n > 2)
    {
        z = PyFloat_AsDouble(PyTuple_GetItem(pPnt, 2));
        return Py_BuildValue("(ddd)", x + std::round(dist * std::sin(ang)),
                                      y + std::round(dist * std::cos(ang)),
                                      z);
    }

    return Py_BuildValue("(dd)", x + std::round(dist * std::sin(ang)),
                                 y + std::round(dist * std::cos(ang)));
}

PyObject *RS_PythonCore::ssadd(const char *ename, const char *ss) const
{

    unsigned int id = 0;
    unsigned int ssId = 0;
    unsigned int newss;

    if (std::strcmp(ename, "") != 0 || std::strcmp(ss, "") != 0)
    {
        id = RS_SCRIPTINGAPI->getEntityId(ename);
        ssId = RS_SCRIPTINGAPI->getSelectionId(ss);
    }

    return RS_SCRIPTINGAPI->ssadd(id, ssId, newss)
            ? Py_BuildValue("s", RS_SCRIPTINGAPI->getSelectionName(newss).c_str()) : Py_None;
}

PyObject *RS_PythonCore::ssdel(const char *ename, const char *ss) const
{
    return RS_SCRIPTINGAPI->ssdel(RS_SCRIPTINGAPI->getEntityId(ename),
                                  RS_SCRIPTINGAPI->getSelectionId(ss))
            ? Py_BuildValue("s", ss) : Py_None;
}

PyObject *RS_PythonCore::ssname(const char *ss, unsigned int idx) const
{
    unsigned int id;
    return RS_SCRIPTINGAPI->ssname(RS_SCRIPTINGAPI->getSelectionId(ss), idx, id)
            ? Py_BuildValue("s", RS_SCRIPTINGAPI->getEntityName(id).c_str()) : Py_None;
}

PyObject *RS_PythonCore::setvar(const char *id, PyObject *args) const
{
    QString setvar = id;

    RS_Graphic *graphic = RS_SCRIPTINGAPI->getGraphic();

    if(!graphic)
    {
        Py_RETURN_NONE;
    }

    if (setvar.toUpper() == "$ANGBASE")
    {
        double value;
        if (!PyArg_Parse(args, "d!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a float.");
            Py_RETURN_NONE;
        }

        graphic->addVariable("$ANGBASE", value, 50);

        return Py_BuildValue("d", graphic->getVariableDouble("$ANGBASE", 0.0));
    }

    else if (setvar.toUpper() == "ANGDIR")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value == 1 || value == 0)
        {
            graphic->addVariable("$ANGDIR", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$ANGDIR", 0));
    }

    else if (setvar.toUpper() == "AUNITS")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value > -1 && value < 5)
        {
            graphic->addVariable("$AUNITS", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$AUNITS", 0));
    }

    else if (setvar.toUpper() == "AUPREC")
    {
        int value;
        if (!PyArg_Parse(args, "i!", &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value > -1 && value < 9)
        {
            graphic->addVariable("$AUPREC", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$AUPREC", 4));
    }

    else if (setvar.toUpper() == "CLAYER")
    {
        std::string value;
        if (!PyArg_Parse(args, "s!", &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a string.");
            Py_RETURN_NONE;
        }
        graphic->addVariable("$CLAYER", value.c_str(), 8);

        return Py_BuildValue("s", qUtf8Printable(graphic->getVariableString("$CLAYER", "0")));
    }

    else if (setvar.toUpper() == "DIMSTYLE")
    {
        std::string value;
        if (!PyArg_Parse(args, "s!", &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a string.");
            Py_RETURN_NONE;
        }
        graphic->addVariable("$DIMSTYLE", value.c_str(), 2);

        return Py_BuildValue("s", qUtf8Printable(graphic->getVariableString("$DIMSTYLE", "Standard")));
    }

    else if (setvar.toUpper() == "DIMSCALE")
    {
        double value;
        if (!PyArg_Parse(args, "d!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a float.");
            Py_RETURN_NONE;
        }

        graphic->addVariable("$DIMSCALE", value, 40);

        return Py_BuildValue("d", graphic->getVariableDouble("$DIMSCALE", 1.0));
    }

    else if (setvar.toUpper() == "DIMASZ")
    {
        double value;
        if (!PyArg_Parse(args, "d!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a float.");
            Py_RETURN_NONE;
        }

        graphic->addVariable("$DIMASZ", value, 40);

        return Py_BuildValue("d", graphic->getVariableDouble("$DIMASZ", 2.5));
    }

    else if (setvar.toUpper() == "DIMEXO")
    {
        double value;
        if (!PyArg_Parse(args, "d!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a float.");
            Py_RETURN_NONE;
        }

        graphic->addVariable("$DIMEXO", value, 40);

        return Py_BuildValue("d", graphic->getVariableDouble("$DIMEXO", 0.625));
    }

    else if (setvar.toUpper() == "DIMEXE")
    {
        double value;
        if (!PyArg_Parse(args, "d!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a float.");
            Py_RETURN_NONE;
        }

        graphic->addVariable("$DIMEXE", value, 40);

        return Py_BuildValue("d", graphic->getVariableDouble("$DIMEXE", 1.25));
    }

    else if (setvar.toUpper() == "DIMFXL")
    {
        double value;
        if (!PyArg_Parse(args, "d!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a float.");
            Py_RETURN_NONE;
        }

        graphic->addVariable("$DIMFXL", value, 40);

        return Py_BuildValue("d", graphic->getVariableDouble("$DIMFXL", 1.0));
    }

    else if (setvar.toUpper() == "DIMTXT")
    {
        double value;
        if (!PyArg_Parse(args, "d!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a float.");
            Py_RETURN_NONE;
        }

        graphic->addVariable("$DIMTXT", value, 40);

        return Py_BuildValue("d", graphic->getVariableDouble("$DIMTXT", 2.5));
    }

    else if (setvar.toUpper() == "DIMTSZ")
    {
        double value;
        if (!PyArg_Parse(args, "d!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a float.");
            Py_RETURN_NONE;
        }

        graphic->addVariable("$DIMTSZ", value, 40);

        return Py_BuildValue("d", graphic->getVariableDouble("$DIMTSZ", 2.5));
    }

    else if (setvar.toUpper() == "DIMLFAC")
    {
        double value;
        if (!PyArg_Parse(args, "d!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a float.");
            Py_RETURN_NONE;
        }

        graphic->addVariable("$DIMLFAC", value, 40);
        return Py_BuildValue("d", graphic->getVariableDouble("$DIMLFAC", 1.0));
    }

    else if (setvar.toUpper() == "DIMGAP")
    {
        double value;
        if (!PyArg_Parse(args, "d!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a float.");
            Py_RETURN_NONE;
        }

        graphic->addVariable("$DIMGAP", value, 40);

        return Py_BuildValue("d", graphic->getVariableDouble("$DIMGAP", 0.625));
    }

    else if (setvar.toUpper() == "DIMTIH")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value == 1 || value == 0)
        {
            graphic->addVariable("$DIMTIH", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$DIMTIH", 2));
    }

    else if (setvar.toUpper() == "DIMZIN")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value > -1 && value < 13)
        {
            graphic->addVariable("$DIMZIN", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$DIMZIN", 1));
    }

    else if (setvar.toUpper() == "DIMAZIN")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value > -1 && value < 4)
        {
            graphic->addVariable("$DIMAZIN", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$DIMAZIN", 0));
    }

    else if (setvar.toUpper() == "DIMCLRD")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value > -1 && value < 257)
        {
            graphic->addVariable("$DIMCLRD", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$DIMCLRD", 0));
    }

    else if (setvar.toUpper() == "DIMCLRE")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value > -1 && value < 257)
        {
            graphic->addVariable("$DIMCLRE", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$DIMCLRE", 0));
    }

    else if (setvar.toUpper() == "DIMCLRT")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        graphic->addVariable("$DIMCLRT", value, 70);

        return Py_BuildValue("i", graphic->getVariableInt("$DIMCLRT", 0));
    }

    else if (setvar.toUpper() == "DIMADEC")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value > -2 && value < 9)
        {
            graphic->addVariable("$DIMADEC", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$DIMADEC", 0));
    }

    else if (setvar.toUpper() == "DIMDEC")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value > -2 && value < 5)
        {
            graphic->addVariable("$DIMDEC", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$DIMDEC", 2));
    }

    else if (setvar.toUpper() == "DIMAUNIT")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value > -2 && value < 4)
        {
            graphic->addVariable("$DIMAUNIT", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$DIMAUNIT", 0));
    }

    else if (setvar.toUpper() == "DIMLUNIT")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value > -1 && value < 7)
        {
            graphic->addVariable("$DIMLUNIT", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$DIMLUNIT", 2));
    }

    else if (setvar.toUpper() == "DIMDSEP")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value == 1 || value == 0)
        {
            graphic->addVariable("$DIMLUNIT", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$DIMDSEP", 0));
    }

    else if (setvar.toUpper() == "DIMFXLON")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value == 1 || value == 0)
        {
            graphic->addVariable("$DIMFXLON", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$DIMFXLON", 0));
    }

    else if (setvar.toUpper() == "DIMTXSTY")
    {
        std::string value;
        if (!PyArg_Parse(args, "s!", &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a string.");
            Py_RETURN_NONE;
        }

        graphic->addVariable("$DIMTXSTY", value.c_str(), 2);

        return Py_BuildValue("s", qUtf8Printable(graphic->getVariableString("$DIMTXSTY", "standard")));
    }

    else if (setvar.toUpper() == "DIMLWD")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        switch (value)
        {
        case -1:
        case -2:
        case -3:
        case 0:
        case 5:
        case 9:
        case 13:
        case 15:
        case 18:
        case 20:
        case 25:
        case 30:
        case 35:
        case 40:
        case 50:
        case 53:
        case 60:
        case 70:
        case 80:
        case 90:
        case 100:
        case 106:
        case 120:
        case 140:
        case 158:
        case 200:
        case 211:
        {
            graphic->addVariable("$DIMLWD", value, 70);
        }
            break;
        default:
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }
        }
        return Py_BuildValue("i", graphic->getVariableInt("$DIMLWD", -2));
    }

    else if (setvar.toUpper() == "DIMLWE")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        switch (value)
        {
        case -1:
        case -2:
        case -3:
        case 0:
        case 5:
        case 9:
        case 13:
        case 15:
        case 18:
        case 20:
        case 25:
        case 30:
        case 35:
        case 40:
        case 50:
        case 53:
        case 60:
        case 70:
        case 80:
        case 90:
        case 100:
        case 106:
        case 120:
        case 140:
        case 158:
        case 200:
        case 211:
        {
            int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

            graphic->addVariable("$DIMLWE", value, 70);
            break;
        }

        default:
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }
        }
        return Py_BuildValue("i", graphic->getVariableInt("$DIMLWE", -2));
    }

    else if (setvar.toUpper() == "DWGCODEPAGE")
    {
        std::string value;
        if (!PyArg_Parse(args, "s!", &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a string.");
            Py_RETURN_NONE;
        }

        graphic->addVariable("$DWGCODEPAGE", value.c_str(), 7);
        return Py_BuildValue("s", qUtf8Printable(graphic->getVariableString("$DWGCODEPAGE", "ANSI_1252")));
    }

    else if (setvar.toUpper() == "GRIDMODE")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value == 1 || value == 0)
        {
            graphic->addVariable("$GRIDMODE", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$GRIDMODE" , 1));
    }

    else if (setvar.toUpper() == "GRIDUNIT")
    {
        double x, y;
        PyObject *pVec;

        if (!PyArg_Parse(args, "O!", &PyTuple_Type, &pVec)) {
            PyErr_SetString(PyExc_TypeError, "vector must be a tuple.");
            Py_RETURN_NONE;
        }

        if (PyTuple_Size(pVec) != 2)
        {
            PyErr_SetString(PyExc_TypeError, "vector must have x and y.");
            Py_RETURN_NONE;
        }

        x = PyFloat_AsDouble(PyTuple_GetItem(pVec, 0));
        y = PyFloat_AsDouble(PyTuple_GetItem(pVec, 1));

        graphic->addVariable("GRIDUNIT", RS_Vector(x,y), 10);

        const RS_Vector spacing = graphic->getVariableVector("$GRIDUNIT" , RS_Vector(0.0,0.0));

        return Py_BuildValue("(dd)", spacing.x, spacing.y);
    }

    else if (setvar.toUpper() == "INSUNITS")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value > -1 && value < 22)
        {
            graphic->addVariable("$INSUNITS", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$INSUNITS", 0));
    }

#if 0
    else if (setvar.toUpper() =="JOINSTYLE")
    {
        return Py_BuildValue("i", graphic->getVariableDouble("$JOINSTYLE", -999.9));
    }
#endif

    else if (setvar.toUpper() == "LUNITS")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value > -1 && value < 6)
        {
            graphic->addVariable("$LUNITS", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$LUNITS", 2));
    }

    else if (setvar.toUpper() == "LUPREC")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value > -1 && value < 8)
        {
            graphic->addVariable("$LUPREC", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$LUPREC", 4));
    }

    else if (setvar.toUpper() == "PDMODE")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        switch (value) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 64:
        case 65:
        case 66:
        case 67:
        case 68:
        case 96:
        case 97:
        case 98:
        case 99:
        case 100:
        {
            int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

            graphic->addVariable("$PDMODE", value, DXF_FORMAT_GC_PDMode);
            break;
        }
        default:
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }
        }

        return Py_BuildValue("i", graphic->getVariableInt("$PDMODE" , LC_DEFAULTS_PDMode));
    }

    else if (setvar.toUpper() == "PDSIZE")
    {
        double value;
        if (!PyArg_Parse(args, "d!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a float.");
            Py_RETURN_NONE;
        }

        graphic->addVariable("$PDSIZE", value, DXF_FORMAT_GC_PDSize);
        return Py_BuildValue("d", graphic->getVariableDouble("$PDSIZE", LC_DEFAULTS_PDSize));
    }

    else if (setvar.toUpper() == "PSVPSCALE")
    {
        double value;
        if (!PyArg_Parse(args, "d!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a float.");
            Py_RETURN_NONE;
        }

        graphic->addVariable("PSVPSCALE", value, 40);
        return Py_BuildValue("d", graphic->getVariableDouble("$PSVPSCALE", 1.0));
    }

    else if (setvar.toUpper() == "SNAPSTYL")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value == 1 || value == 0)
        {
            graphic->addVariable("$SNAPSTYLE", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$SNAPSTYLE", 0));
    }

    else if (setvar.toUpper() == "SNAPISOPAIR")
    {
        int value;
        if (!PyArg_Parse(args, "i!",  &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be an integer.");
            Py_RETURN_NONE;
        }

        if (value > -1 && value < 3)
        {
            graphic->addVariable("$SNAPISOPAIR", value, 70);
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
            Py_RETURN_NONE;
        }

        return Py_BuildValue("i", graphic->getVariableInt("$SNAPISOPAIR", 0));
    }

    else if (setvar.toUpper() == "TEXTSTYLE")
    {
        std::string value;
        if (!PyArg_Parse(args, "s!", &value)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be a string.");
            Py_RETURN_NONE;
        }

        graphic->addVariable("$TEXTSTYLE", value.c_str(), 7);
        return Py_BuildValue("s", qUtf8Printable(graphic->getVariableString("$TEXTSTYLE", "Standard")));
    }

    else
    {
        PyErr_SetString(PyExc_ValueError, "can not set SYSVAR");
    }

    Py_RETURN_NONE;
}

PyObject *RS_PythonCore::getvar(const char *id) const
{
    QString getvar = id;

    RS_Graphic *graphic = RS_SCRIPTINGAPI->getGraphic();

    if(!graphic)
    {
        Py_RETURN_NONE;
    }

    if (getvar.toUpper() == "ACADVER")
    {
        QString acadver = graphic->getVariableString("$ACADVER", "");
        acadver.replace(QRegularExpression("[a-zA-Z]"), "");
        return Py_BuildValue("s", qUtf8Printable(acadver));
    }

    else if (getvar.toUpper() == "ANGBASE")
    {
        return Py_BuildValue("d", graphic->getVariableDouble("$ANGBASE", 0.0));
    }

    else if (getvar.toUpper() == "ANGDIR")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$ANGDIR", 0));
    }

    else if (getvar.toUpper() == "AUNITS")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$AUNITS", 0));
    }

    else if (getvar.toUpper() == "AUPREC")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$AUPREC", 4));
    }

    else if (getvar.toUpper() == "CLAYER")
    {
        return Py_BuildValue("s", qUtf8Printable(graphic->getVariableString("$CLAYER", "0")));
    }

    else if (getvar.toUpper() == "DIMSTYLE")
    {
        return Py_BuildValue("s", qUtf8Printable(graphic->getVariableString("$DIMSTYLE", "Standard")));
    }

    else if (getvar.toUpper() == "DIMSCALE")
    {
        return Py_BuildValue("d", graphic->getVariableDouble("$DIMSCALE", 1.0));
    }

    else if (getvar.toUpper() == "DIMASZ")
    {
        return Py_BuildValue("d", graphic->getVariableDouble("$DIMASZ", 2.5));
    }

    else if (getvar.toUpper() == "DIMEXO")
    {
        return Py_BuildValue("d", graphic->getVariableDouble("$DIMEXO", 0.625));
    }

    else if (getvar.toUpper() == "DIMEXE")
    {
        return Py_BuildValue("d", graphic->getVariableDouble("$DIMEXE", 1.25));
    }

    else if (getvar.toUpper() == "DIMFXL")
    {
        return Py_BuildValue("d", graphic->getVariableDouble("$DIMFXL", 1.0));
    }

    else if (getvar.toUpper() == "DIMTXT")
    {
        return Py_BuildValue("d", graphic->getVariableDouble("$DIMTXT", 2.5));
    }

    else if (getvar.toUpper() == "DIMTSZ")
    {
        return Py_BuildValue("d", graphic->getVariableDouble("$DIMTSZ", 2.5));
    }

    else if (getvar.toUpper() == "DIMLFAC")
    {
        return Py_BuildValue("d", graphic->getVariableDouble("$DIMLFAC", 1.0));
    }

    else if (getvar.toUpper() == "DIMGAP")
    {
        return Py_BuildValue("d", graphic->getVariableDouble("$DIMGAP", 0.625));
    }

    else if (getvar.toUpper() == "DIMTIH")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$DIMTIH", 2));
    }

    else if (getvar.toUpper() == "DIMZIN")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$DIMZIN", 1));
    }

    else if (getvar.toUpper() == "DIMAZIN")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$DIMAZIN", 0));
    }

    else if (getvar.toUpper() == "DIMCLRD")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$DIMCLRD", 0));
    }

    else if (getvar.toUpper() == "DIMCLRE")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$DIMCLRE", 0));
    }

    else if (getvar.toUpper() == "DIMCLRT")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$DIMCLRT", 0));
    }

    else if (getvar.toUpper() == "DIMADEC")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$DIMADEC", 0));
    }

    else if (getvar.toUpper() == "DIMDEC")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$DIMDEC", 2));
    }

    else if (getvar.toUpper() == "DIMAUNI")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$DIMAUNIT", 0));
    }

    else if (getvar.toUpper() == "DIMLUNIT")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$DIMLUNIT", 2));
    }

    else if (getvar.toUpper() == "DIMDSEP")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$DIMDSEP", 0));
    }

    else if (getvar.toUpper() == "DIMFXLON")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$DIMFXLON", 0));
    }

    else if (getvar.toUpper() == "DIMTXSTY")
    {
        return Py_BuildValue("s", qUtf8Printable(graphic->getVariableString("$DIMTXSTY", "standard")));
    }

    else if (getvar.toUpper() == "DIMLWD")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$DIMLWD", -2));
    }

    else if (getvar.toUpper() == "DIMLWE")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$DIMLWE", -2));
    }

    else if (getvar.toUpper() == "DWGCODEPAGE")
    {
        return Py_BuildValue("s", qUtf8Printable(graphic->getVariableString("$DWGCODEPAGE", "ANSI_1252")));
    }

    else if (getvar.toUpper() == "EXTMIN")
    {
    const RS_Vector ext = graphic->getVariableVector("$EXTMIN", RS_Vector(0.0, 0.0, 0.0));
    const QString value = QString::number(ext.x) + "," + QString::number(ext.y);

        return Py_BuildValue("s", qUtf8Printable(value));
    }

    else if (getvar.toUpper() == "EXTMAX")
    {
    const RS_Vector ext = graphic->getVariableVector("$EXTMAX", RS_Vector(0.0, 0.0, 0.0));
    const QString value = QString::number(ext.x) + "," + QString::number(ext.y);

        return Py_BuildValue("s", qUtf8Printable(value));
    }

    else if (getvar.toUpper() == "GRIDMODE")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$GRIDMODE" , 1));
    }

    else if (getvar.toUpper() == "GRIDUNIT")
    {
    const RS_Vector spacing = graphic->getVariableVector("$GRIDUNIT" , RS_Vector(0.0,0.0));
    const QString value = QString::number(spacing.x) + "," + QString::number(spacing.y);

        return Py_BuildValue("s", qUtf8Printable(value));
    }

    else if (getvar.toUpper() == "INSUNITS")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$INSUNITS", 0));
    }

    #if 0
    else if (getvar.toUpper() == "JOINSTYLE")
    {
        return Py_BuildValue("i", graphic->getVariableDouble("$JOINSTYLE", -999.9));
    }
    #endif

    else if (getvar.toUpper() == "LUNITS")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$LUNITS", 2));
    }

    else if (getvar.toUpper() == "LUPREC")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$LUPREC", 4));
    }

    else if (getvar.toUpper() == "PDMODE")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$PDMODE" , LC_DEFAULTS_PDMode));
    }

    else if (getvar.toUpper() == "PDSIZE")
    {
        return Py_BuildValue("i", graphic->getVariableDouble("$PDSIZE", LC_DEFAULTS_PDSize));
    }

    else if (getvar.toUpper() == "PSVPSCALE")
    {
        return Py_BuildValue("d", graphic->getVariableDouble("$PSVPSCALE", 1.0));
    }

    else if (getvar.toUpper() == "UCSNAME")
    {
        return Py_BuildValue("s", qUtf8Printable(graphic->getVariableString("$UCSNAME", "")));
    }

    else if (getvar.toUpper() == "UCSORG")
    {
    const RS_Vector origin = graphic->getVariableVector("$UCSORG" , RS_Vector(0.0,0.0));
    const QString value = QString::number(origin.x) + "," + QString::number(origin.y);

        return Py_BuildValue("s", qUtf8Printable(value));
    }

    else if (getvar.toUpper() == "UCSORTHOVIEW")
    {
        return Py_BuildValue("i", graphic->getVariableDouble("$UCSORTHOVIEW", 0));
    }

    else if (getvar.toUpper() == "UCSXDIR")
    {
    const RS_Vector xAxis = graphic->getVariableVector("$UCSXDIR" , RS_Vector(0.0,0.0));
    const QString value = QString::number(xAxis.x) + "," + QString::number(xAxis.y);

        return Py_BuildValue("s", qUtf8Printable(value));
    }

    else if (getvar.toUpper() == "UCSYDIR")
    {
    RS_Vector xAxis = graphic->getVariableVector("$UCSXDIR" , RS_Vector(0.0,0.0));
    const RS_Vector yAxis = graphic->getVariableVector("$UCSYDIR" , xAxis.rotate(M_PI_2));
    const QString value = QString::number(yAxis.x) + "," + QString::number(yAxis.y);

        return Py_BuildValue("s", qUtf8Printable(value));
    }

    else if (getvar.toUpper() == "SNAPSTYL")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$SNAPSTYLE", 0));
    }

    else if (getvar.toUpper() == "SNAPISOPAIR")
    {
        return Py_BuildValue("i", graphic->getVariableInt("$SNAPISOPAIR", 0));
    }

    else if (getvar.toUpper() == "TEXTSTYLE")
    {
        return Py_BuildValue("s", qUtf8Printable(graphic->getVariableString("$TEXTSTYLE", "Standard")));
    }

    else
    {}

    Py_RETURN_NONE;
}

bool fitTolerance(PyObject *pList, RS_ScriptingApiData &apiData)
{
    int gc;
    PyObject *pTuple;
    PyObject *pGc;
    PyObject *pValue;
    Py_ssize_t n = PyList_Size(pList);

    for (int i=0; i<n; i++) {
        pTuple = PyList_GetItem(pList, i);
        if(!PyTuple_Check(pTuple)) {
            PyErr_SetString(PyExc_TypeError, "list items must be a tuple.");
            return false;
        }
        pGc = PyTuple_GetItem(pTuple, 0);
        if(!PyLong_Check(pGc)) {
            PyErr_SetString(PyExc_TypeError, "first tuple item must be an integer.");
            return false;
        }
        gc = PyLong_AsLong(pGc);
        qDebug() << "[RS_PythonCore::entmake] i:" << i << "GC:" << gc;

        switch (gc)
        {
        case -1:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyLong_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "first tuple item must be an integer.");
                return false;
            }
            apiData.id.push_back({ RS_SCRIPTINGAPI->getEntityId(PyUnicode_AsUTF8(pValue)) });
        }
        break;
        case 0:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyUnicode_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a string.");
                return false;
            }
            apiData.etype = PyUnicode_AsUTF8(pValue);
        }
            break;
        case 1:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyUnicode_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a string.");
                return false;
            }
            apiData.text.push_back({ PyUnicode_AsUTF8(pValue) });
        }
        break;
        case 2:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyUnicode_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a string.");
                return false;
            }
            apiData.block = PyUnicode_AsUTF8(pValue);
        }
        break;
        case 6:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyUnicode_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a string.");
                return false;
            }

            apiData.pen.setLineType(RS_FilterDXFRW::nameToLineType(PyUnicode_AsUTF8(pValue)));
        }
        break;
        case 7:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyUnicode_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a string.");
                return false;
            }
            apiData.style.push_back({ PyUnicode_AsUTF8(pValue) });
        }
        break;
        case 8:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyUnicode_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a string.");
                return false;
            }
            apiData.layer = PyUnicode_AsUTF8(pValue);
        }
        break;
        case 10:
        {
            double xVal;
            double yVal;
            double zVal = 0.0;

            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }

            xVal = PyFloat_AsDouble(pValue);

            pValue = PyTuple_GetItem(pTuple, 2);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }

            yVal = PyFloat_AsDouble(pValue);

            if (PyTuple_Size(pTuple) > 3)
            {
                pValue = PyTuple_GetItem(pTuple, 3);
                if(!PyFloat_Check(pValue)) {
                    PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                    return false;
                }
                zVal = PyFloat_AsDouble(pValue);
            }
            apiData.gc_10.push_back({ xVal, yVal, zVal });
        }
        break;
        case 11:
        {
            double xVal;
            double yVal;
            double zVal = 0.0;

            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }

            xVal = PyFloat_AsDouble(pValue);

            pValue = PyTuple_GetItem(pTuple, 2);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }

            yVal = PyFloat_AsDouble(pValue);

            if (PyTuple_Size(pTuple) > 3)
            {
                pValue = PyTuple_GetItem(pTuple, 3);
                if(!PyFloat_Check(pValue)) {
                    PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                    return false;
                }
                zVal = PyFloat_AsDouble(pValue);
            }
            apiData.gc_11.push_back({ xVal, yVal, zVal });
        }
        break;
        case 12:
        {
            double xVal;
            double yVal;
            double zVal = 0.0;

            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }

            xVal = PyFloat_AsDouble(pValue);

            pValue = PyTuple_GetItem(pTuple, 2);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }

            yVal = PyFloat_AsDouble(pValue);

            if (PyTuple_Size(pTuple) > 3)
            {
                pValue = PyTuple_GetItem(pTuple, 3);
                if(!PyFloat_Check(pValue)) {
                    PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                    return false;
                }
                zVal = PyFloat_AsDouble(pValue);
            }
            apiData.gc_12.push_back({ xVal, yVal, zVal });
        }
        break;
        case 13:
        {
            double xVal;
            double yVal;
            double zVal = 0.0;

            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }

            xVal = PyFloat_AsDouble(pValue);

            pValue = PyTuple_GetItem(pTuple, 2);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }

            yVal = PyFloat_AsDouble(pValue);

            if (PyTuple_Size(pTuple) > 3)
            {
                pValue = PyTuple_GetItem(pTuple, 3);
                if(!PyFloat_Check(pValue)) {
                    PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                    return false;
                }
                zVal = PyFloat_AsDouble(pValue);
            }
            apiData.gc_13.push_back({ xVal, yVal, zVal });
        }
        break;
        case 14:
        {
            double xVal;
            double yVal;
            double zVal = 0.0;

            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }

            xVal = PyFloat_AsDouble(pValue);

            pValue = PyTuple_GetItem(pTuple, 2);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }

            yVal = PyFloat_AsDouble(pValue);

            if (PyTuple_Size(pTuple) > 3)
            {
                pValue = PyTuple_GetItem(pTuple, 3);
                if(!PyFloat_Check(pValue)) {
                    PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                    return false;
                }
                zVal = PyFloat_AsDouble(pValue);
            }
            apiData.gc_14.push_back({ xVal, yVal, zVal });
        }
        break;
        case 15:
        {
            double xVal;
            double yVal;
            double zVal = 0.0;

            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }

            xVal = PyFloat_AsDouble(pValue);

            pValue = PyTuple_GetItem(pTuple, 2);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }

            yVal = PyFloat_AsDouble(pValue);

            if (PyTuple_Size(pTuple) > 3)
            {
                pValue = PyTuple_GetItem(pTuple, 3);
                if(!PyFloat_Check(pValue)) {
                    PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                    return false;
                }
                zVal = PyFloat_AsDouble(pValue);
            }
            apiData.gc_15.push_back({ xVal, yVal, zVal });
        }
        break;
        case 16:
        {
            double xVal;
            double yVal;
            double zVal = 0.0;

            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }

            xVal = PyFloat_AsDouble(pValue);

            pValue = PyTuple_GetItem(pTuple, 2);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }

            yVal = PyFloat_AsDouble(pValue);

            if (PyTuple_Size(pTuple) > 3)
            {
                pValue = PyTuple_GetItem(pTuple, 3);
                if(!PyFloat_Check(pValue)) {
                    PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                    return false;
                }
                zVal = PyFloat_AsDouble(pValue);
            }
            apiData.gc_16.push_back({ xVal, yVal, zVal });
        }
        break;
        case 40:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }
            apiData.gc_40.push_back({ PyFloat_AsDouble(pValue) });
        }
        break;
        case 41:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }
            apiData.gc_41.push_back({ PyFloat_AsDouble(pValue) });
        }
        break;
        case 42:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }
            apiData.gc_42.push_back({ PyFloat_AsDouble(pValue) });
        }
        break;
        case 44:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }
            apiData.gc_44.push_back({ PyFloat_AsDouble(pValue) });
        }
        break;
        case 45:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }
            apiData.gc_45.push_back({ PyFloat_AsDouble(pValue) });
        }
        break;
        case 48:
        {
            int width = 0;
            pValue = PyTuple_GetItem(pTuple, 1);

            if(PyFloat_Check(pValue))
            {
                width = static_cast<int>(PyFloat_AsDouble(pValue));
            }

            else if(PyLong_Check(pValue))
            {
                width = PyFloat_AsDouble(pValue);
            }

            if (width >= 0)
            {
                width *= 100;
            }

            apiData.pen.setWidth(RS2::intToLineWidth(width));
        }
        break;
        case 50:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }
            apiData.gc_50.push_back({ PyFloat_AsDouble(pValue) });
        }
        break;
        case 51:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyFloat_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a float.");
                return false;
            }
            apiData.gc_51.push_back({ PyFloat_AsDouble(pValue) });
        }
        break;
        case 62:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyLong_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "first tuple item must be an integer.");
                return false;
            }
            apiData.pen.setColor(RS_FilterDXFRW::numberToColor(PyLong_AsLong(pValue)));
        }
        break;
        case 70:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyLong_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "first tuple item must be an integer.");
                return false;
            }
            apiData.gc_70.push_back({ static_cast<int>(PyLong_AsLong(pValue)) });
        }
        break;
        case 71:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyLong_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "first tuple item must be an integer.");
                return false;
            }
            apiData.gc_71.push_back({ static_cast<int>(PyLong_AsLong(pValue)) });
        }
        break;
        case 72:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyLong_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "first tuple item must be an integer.");
                return false;
            }
            apiData.gc_72.push_back({ static_cast<int>(PyLong_AsLong(pValue)) });
        }
        break;
        case 73:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyLong_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "first tuple item must be an integer.");
                return false;
            }
            apiData.gc_73.push_back({ static_cast<int>(PyLong_AsLong(pValue)) });
        }
        break;
        case 100:
        {
            pValue = PyTuple_GetItem(pTuple, 1);
            if(!PyUnicode_Check(pValue)) {
                PyErr_SetString(PyExc_TypeError, "tuple item must be a string.");
                return false;
            }
            apiData.gc_100.push_back({ PyUnicode_AsUTF8(pValue) });
        }
        break;
        default:
            break;
        }
    }
    return true;
}
