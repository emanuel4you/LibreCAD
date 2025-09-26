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

#include <string>

#include "rs_python.h"
#include "rs_lisp_main.h"

#include "rs_graphicview.h"
#include "rs_scriptingapi.h"
#include "rs_scripting_inputhandle.h"
#include "rs_entity.h"
#include "rs_insert.h"
#include "rs_entitycontainer.h"
#include "rs_eventhandler.h"
#include "rs_dialogs.h"
#include "rs_settings.h"
#include "rs_filterdxfrw.h"

#include "rs_line.h"
#include "rs_circle.h"
#include "rs_arc.h"
#include "rs_ellipse.h"
#include "rs_point.h"
#include "rs_spline.h"
#include "rs_polyline.h"
#include "rs_text.h"
#include "rs_mtext.h"
#include "rs_layer.h"

#include "lc_defaults.h"
#include "lc_undosection.h"
#include "lc_actioncontext.h"
#include "lc_defaultactioncontext.h"

#include "qc_applicationwindow.h"

#include "qg_actionhandler.h"
#include "qg_colordlg.h"

#include "intern/qc_actiongetrad.h"
#include "intern/qc_actiongetdist.h"
#include "intern/qc_actiongetpoint.h"
#include "intern/qc_actiongetcorner.h"
#include "intern/qc_actiongrdraw.h"
#include "intern/qc_actionentsel.h"
#include "intern/qc_actionselectset.h"
#include "intern/qc_actionsingleset.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QEventLoop>
#include <QStatusBar>

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

#include "rs_lisp_lcl.h"
#include "rs_lisp_types.h"
#include "rs_lisp_env.h"

#include <iostream>
#include <sstream>

/**
 * helper function getEntityIdbyName
 */
RS2::EntityType getEntityIdbyName(const QString &name)
{
    RS2::EntityType type = RS2::EntityUnknown;

    for (auto e : entityIds)
    {
       if (name.compare(e.name) == 0)
       {
           type = e.id;
           break;
       }
    }
    return type;
}

/**
 * static instance to class RS_ScriptingApi
 */
RS_ScriptingApi* RS_ScriptingApi::unique = NULL;

RS_ScriptingApi* RS_ScriptingApi::instance() {
    qInfo() << "[RS_ScriptingApi] RS_ScriptingApi::instance requested";
    if (unique == NULL) {
        unique = new RS_ScriptingApi();
    }
    return unique;
}

void RS_ScriptingApi::command(const QString &cmd)
{
    QG_ActionHandler* actionHandler = NULL;
    actionHandler = QC_ApplicationWindow::getAppWindow()->getActionHandler();

    if (actionHandler)
    {
        actionHandler->command(cmd.simplified());
    }
}

void RS_ScriptingApi::prompt(CommandEdit *cmdline, const char *prompt)
{
    if (cmdline)
    {
        cmdline->setPrompt(prompt);
        const std::string result = RS_Scripting_InputHandle::readLine(QObject::tr(prompt), cmdline).toStdString();
        Q_UNUSED(result);
        cmdline->reset();
    }
    else
    {
        msgInfo(qUtf8Printable(QObject::tr(prompt)));
    }
}

void RS_ScriptingApi::initGet(int bit, const char *str)
{
    shadowEnv->set("initget_bit", lcl::integer(bit));
    shadowEnv->set("initget_string", lcl::string(str));
}

void RS_ScriptingApi::help(const QString &tag)
{
    QDir directory(QDir::currentPath());
    QString librebrowser = directory.absoluteFilePath("librebrowser");

    if(QFile::exists(librebrowser))
    {
        if (!tag.isEmpty())
        {
            librebrowser += " '";
            librebrowser += tag;
            librebrowser += "' &";
        }
        system(qUtf8Printable(librebrowser));
    }
    else
    {
        msgInfo("poor Help call 'SOS' :-b");
    }

}

void RS_ScriptingApi::msgInfo(const char *msg)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("LibreCAD");
    msgBox.setText(msg);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}

unsigned int RS_ScriptingApi::getEntityId(const std::string &name)
{
    std::string ename = name.substr(name.find(": ") + 2);
    ename.pop_back();

    return static_cast<unsigned int>(stoi(ename, 0, 16));
}

unsigned int RS_ScriptingApi::getSelectionId(const std::string &name)
{
    return getEntityId(name);
}

int RS_ScriptingApi::getIntDlg(const char *prompt)
{
    return QInputDialog::getInt(NULL,
            "LibreCAD",
            QObject::tr(prompt),
            // , int value = 0, int min = -2147483647, int max = 2147483647, int step = 1, bool *ok = NULL, Qt::WindowFlags flags = Qt::WindowFlags())
            0, -2147483647, 2147483647, 1, NULL, Qt::WindowFlags());
}

double RS_ScriptingApi::getDoubleDlg(const char *prompt)
{
    return QInputDialog::getDouble(NULL,
            "LibreCAD",
            QObject::tr(prompt),
            // double value = 0, double min = -2147483647, double max = 2147483647, int decimals = 1, bool *ok = NULL, Qt::WindowFlags flags = Qt::WindowFlags(), double step = 1)
            0.0, -2147483647.0, 2147483647.0, 1, NULL, Qt::WindowFlags(), 1);
}

std::string RS_ScriptingApi::copyright() const
{
    QFile f(":/readme.md");
    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        return "";
    }
    QTextStream in(&f);
    return in.readAll().toStdString();
}

std::string RS_ScriptingApi::credits() const
{
    return "Thanks to all the people supporting LibreCAD development. See https://dokuwiki.librecad.org for more information.\n";
}

std::string RS_ScriptingApi::getStrDlg(const char *prompt) const
{
    return QInputDialog::getText(NULL,
            "LibreCAD",
            QObject::tr(prompt),
            //QLineEdit::EchoMode mode = QLineEdit::Normal, const QString &text = QString(), bool *ok = NULL, Qt::WindowFlags flags = Qt::WindowFlags(), Qt::InputMethodHints inputMethodHints = Qt::ImhNone)
            QLineEdit::Normal, "", NULL, Qt::WindowFlags(), Qt::ImhNone).toStdString();
}

std::string RS_ScriptingApi::getFileNameDlg(const char *title, const char *filename, const char *ext) const
{
    return QFileDialog::getOpenFileName(NULL,
                                        QObject::tr(title),
                                        filename,
                                        QObject::tr(ext)).toStdString();
}

std::string RS_ScriptingApi::getEntityName(unsigned int id) const
{
    std::string ename = "<Entity name: ";
    std::stringstream ss;
    ss << std::uppercase << std::hex << id;
    ename += ss.str();
    ename += ">";
    return ename;
}

std::string RS_ScriptingApi::getEntityHndl(unsigned int id) const
{
    std::stringstream ss;
    ss << std::uppercase << std::hex << id;
    std::string hndl = ss.str();
    return hndl;
}

std::string RS_ScriptingApi::getSelectionName(unsigned int id) const
{
    std::string ename = "<Selection set: ";
    std::stringstream ss;
    ss << std::uppercase << std::hex << id;
    ename += ss.str();
    ename += ">";
    return ename;
}

char RS_ScriptingApi::readChar()
{
    return RS_InputDialog::readChar();
}

bool RS_ScriptingApi::getFiled(const char *title, const char *def, const char *ext, int flags, std::string &filename)
{
    QString path = def;
    QString fileExt = "(*.";
    fileExt += ext;
    fileExt += ")";

    if (flags & 1)
    {
        if (flags & 4) {
            path += ".";
            path += ext;
        }

        QFileDialog saveFile;
        if (flags & 32) {
            saveFile.setAcceptMode(QFileDialog::AcceptSave);
            saveFile.setOptions(QFileDialog::DontConfirmOverwrite);
        }
        filename = saveFile.getSaveFileName(NULL, title, path, fileExt).toStdString();

        if (filename.size())
        {
            if (flags & 4) {
                filename += ".";
                filename += ext;
            }
            return true;
        }
    }

    if (flags & 2)
    {
        if (!(flags & 4)) {
            fileExt = "(*.dxf)";
        }
        if (!(flags & 16)) {
            // pfad abschliesen
        }
        if (fileExt.size() == 0) {
            fileExt = "(*)";
        }
        filename = QFileDialog::getOpenFileName(NULL, title, path, fileExt).toStdString();
        if (filename.size())
        {
            return true;
        }
    }

    /*
     * not implemented yet
     *
     * 8 (bit 3) -- If this bit is set and bit 0 is not set, getfiled performs a library
     *  search for the file name entered. If it finds the file and its directory in the library search path,
     *  it strips the path and returns only the file name.
     *  (It does not strip the path name if it finds that a file of the same name is in a different directory.)
     *
     * 64  (bit 6) -- Do not transfer the remote file if the user specifies a URL.
     *
     * 128 (bit 7) -- Do not allow URLs at all.
     *
     */

    return false;
}

RS_Vector RS_ScriptingApi::getCorner(CommandEdit *cmdline, const char *msg, const RS_Vector &basePoint) const
{
    double x=0.0, y=0.0;
    QString prompt = QObject::tr("Enter a point: ");

    if (strcmp(msg, ""))
    {
        prompt = msg;
    }

    if (getGraphicView() == NULL || getGraphic() == NULL){
        qDebug() << "graphicView == NULL";
        return RS_Vector();
    }

    auto ctx = QC_ApplicationWindow::getAppWindow()->getActionContext();

    auto a = std::make_shared<QC_ActionGetCorner>(ctx);
    if (a)
    {
        QPointF *point = new QPointF;
        QPointF *base;
        bool status = false;

        if (!(prompt.isEmpty()))
        {
            a->setMessage(prompt);
        }

        ctx->getGraphicView()->killAllActions();
        ctx->getGraphicView()->setCurrentAction(a);

        base = new QPointF(basePoint.x, basePoint.y);
        a->setBasepoint(base);

        QEventLoop ev;

        cmdline->setPrompt(QObject::tr(qUtf8Printable(prompt)));

        while (!a->isCompleted())
        {
            ev.processEvents ();

            if (!ctx->getGraphicView()->hasAction())
                break;
        }
        if (a->isCompleted() && !a->wasCanceled())
        {
            a->getPoint(point);
            status = true;
        }

        cmdline->reset();

        ctx->getGraphicView()->killAllActions();

        if(status)
        {
            x = point->x();
            y = point->y();
            delete point;
            delete base;

            return RS_Vector(x, y);
        }
        else
        {
            delete point;
            delete base;
        }
    }

    return RS_Vector();
}

RS_Vector RS_ScriptingApi::getPoint(CommandEdit *cmdline, const char *msg, const RS_Vector basePoint) const
{
    double x=0.0, y=0.0, z=0.0;
    QString prompt = QObject::tr("Enter a point: ");

    if (strcmp(msg, ""))
    {
        prompt = msg;
    }

    if (getGraphicView() == NULL || getGraphic() == NULL){
        qDebug() << "graphicView == NULL";
        return RS_Vector();
    }

    auto ctx = QC_ApplicationWindow::getAppWindow()->getActionContext();

    auto a = std::make_shared<QC_ActionGetPoint>(ctx);
    if (a)
    {
        QPointF *point = new QPointF;
        QPointF *base;
        bool status = false;

        if (!(prompt.isEmpty()))
        {
            a->setMessage(prompt);
        }

        ctx->getGraphicView()->killAllActions();
        ctx->getGraphicView()->setCurrentAction(a);

        if (basePoint.valid)
        {
            base = new QPointF(basePoint.x, basePoint.y);
            z = basePoint.z;
            a->setBasepoint(base);
        }

        QEventLoop ev;

        cmdline->setPrompt(QObject::tr(qUtf8Printable(prompt)));
        cmdline->doProcessLc(true);

        while (!a->isCompleted())
        {
            ev.processEvents ();
            if (!ctx->getGraphicView()->hasAction())
                break;
        }
        if (a->isCompleted() && !a->wasCanceled())
        {
            a->getPoint(point);
            status = true;
        }

        cmdline->reset();

        ctx->getGraphicView()->killAllActions();

        if(status)
        {
            x = point->x();
            y = point->y();
            delete point;
            if (basePoint.valid)
            {
                delete base;
            }
            return RS_Vector(x, y, z);
        }
        else
        {
            delete point;
            if (basePoint.valid)
            {
                delete base;
            }
        }
    }

    return RS_Vector();
}

bool RS_ScriptingApi::getDist(CommandEdit *cmdline, const char *msg, const RS_Vector &basePoint, double &distance)
{
    QString prompt = msg;
    RS_Vector start = basePoint;
    bool finished = false;

    if (getGraphicView() == NULL || getGraphic() == NULL){
        qDebug() << "graphicView == NULL";
        return false;
    }

    auto ctx = QC_ApplicationWindow::getAppWindow()->getActionContext();

    QEventLoop ev;

    if(!start.valid)
    {
        auto startAction = std::make_shared<QC_ActionGetPoint>(ctx);
        if (startAction)
        {
            QPointF *startPoint = new QPointF;
            bool status = false;

            startAction->setMessage(prompt);
            ctx->getGraphicView()->killAllActions();
            cmdline->setPrompt(QObject::tr(qUtf8Printable(prompt)));
            ctx->getGraphicView()->setCurrentAction(startAction);

            QObject::connect(cmdline, &QLineEdit::returnPressed, static_cast<QC_ActionGetPoint*>(startAction.get()), [cmdline, startAction, prompt, &distance, &finished]
            {
                const QString result = cmdline->text();
                static const QRegularExpression floatRegExpr(QStringLiteral("^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)$"));
                QRegularExpressionMatch match = floatRegExpr.match(result);

                if (result.isEmpty())
                {
                    startAction->trigger();
                }

                if (match.hasMatch())
                {
                    distance = result.toDouble();
                    startAction->trigger();
                    finished = true;
                }
                else
                {
                    cmdline->setPrompt(QObject::tr(qUtf8Printable(prompt)));
                }
            } );

            while (!startAction->isCompleted())
            {
                ev.processEvents();
                if (!ctx->getGraphicView()->hasAction())
                    break;
            }

            if(finished)
            {
                QObject::disconnect(cmdline, &QLineEdit::returnPressed, static_cast<QC_ActionGetPoint*>(startAction.get()), NULL);
                ctx->getGraphicView()->killAllActions();
                cmdline->reset();
                return true;
            }

            if (startAction->isCompleted() && !startAction->wasCanceled())
            {
                startAction->getPoint(startPoint);
                status = true;
            }

            if(status)
            {
                start.x = startPoint->x();
                start.y = startPoint->y();
                delete startPoint;
            }
            else
            {
                cmdline->reset();
                ctx->getGraphicView()->killAllActions();
                delete startPoint;
                return false;
            }
        }
    }

    prompt = QObject::tr("Enter second point: ");
    cmdline->setPrompt(QObject::tr(qUtf8Printable(prompt)));

    auto endAction = std::make_shared<QC_ActionGetDist>(ctx);

    if (endAction)
    {
        QPointF *base = new QPointF(start.x, start.y);

        ctx->getGraphicView()->setCurrentAction(endAction);
        endAction->setBasepoint(base);

        QObject::connect(cmdline, &QLineEdit::returnPressed, static_cast<QC_ActionGetDist*>(endAction.get()), [cmdline, endAction, prompt, &distance, &finished]
        {
            const QString result = cmdline->text();
            static const QRegularExpression floatRegExpr(QStringLiteral("^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)$"));
            QRegularExpressionMatch match = floatRegExpr.match(result);

            if (result.isEmpty())
            {
                endAction->trigger();
            }

            if (match.hasMatch())
            {
                distance = result.toDouble();
                endAction->trigger();
                finished = true;
            }
            else
            {
                cmdline->setPrompt(QObject::tr(qUtf8Printable(prompt)));
            }
        } );

        while (!endAction->isCompleted())
        {
            ev.processEvents();

            if (!ctx->getGraphicView()->hasAction())
            {
                break;
            }
        }

        QObject::disconnect(cmdline, &QLineEdit::returnPressed, static_cast<QC_ActionGetDist*>(endAction.get()), NULL);
        ctx->getGraphicView()->killAllActions();
        ctx->getGraphicView()->redraw();
        cmdline->reset();
        delete base;

        if(finished)
        {
            return true;
        }

        if (endAction->isCompleted() && !endAction->wasCanceled())
        {
            distance = endAction->getDist();
            return true;
        }
    }

    return false;
}

bool RS_ScriptingApi::getAngle(CommandEdit *cmdline, const char *msg, const RS_Vector &basePoint, double &rad)
{
    getOrient(cmdline, msg, basePoint, rad);
    rad = rad + RS_SCRIPTINGAPI->getGraphic()->getVariableDouble("$ANGBASE", 0.0);

    return false;
}

bool RS_ScriptingApi::getOrient(CommandEdit *cmdline, const char *msg, const RS_Vector &basePoint, double &rad)
{
    QString prompt = msg;
    RS_Vector start = basePoint;
    bool finished = false;
    double distance;

    if (getGraphicView() == NULL || getGraphic() == NULL){
        qDebug() << "graphicView == NULL";
        return false;
    }

    auto ctx = QC_ApplicationWindow::getAppWindow()->getActionContext();

    QEventLoop ev;

    if(!start.valid)
    {
        auto startAction = std::make_shared<QC_ActionGetPoint>(ctx);
        if (startAction)
        {
            QPointF *startPoint = new QPointF;
            bool status = false;

            startAction->setMessage(prompt);

            ctx->getGraphicView()->killAllActions();
            cmdline->setPrompt(QObject::tr(qUtf8Printable(prompt)));
            ctx->getGraphicView()->setCurrentAction(startAction);

            QObject::connect(cmdline, &QLineEdit::returnPressed, static_cast<QC_ActionGetPoint*>(startAction.get()), [cmdline, startAction, prompt, &distance, &finished]
            {
                const QString result = cmdline->text();
                static const QRegularExpression floatRegExpr(QStringLiteral("^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)$"));
                QRegularExpressionMatch match = floatRegExpr.match(result);

                if (result.isEmpty())
                {
                    startAction->trigger();
                }

                if (match.hasMatch())
                {
                    distance = result.toDouble();
                    startAction->trigger();
                    finished = true;
                }
                else
                {
                    cmdline->setPrompt(QObject::tr(qUtf8Printable(prompt)));
                }
            } );

            while (!startAction->isCompleted())
            {
                ev.processEvents();
                if (!ctx->getGraphicView()->hasAction())
                    break;
            }

            if(finished)
            {
                QObject::disconnect(cmdline, &QLineEdit::returnPressed, static_cast<QC_ActionGetPoint*>(startAction.get()), NULL);
                ctx->getGraphicView()->killAllActions();
                cmdline->reset();
                return true;
            }

            if (startAction->isCompleted() && !startAction->wasCanceled())
            {
                startAction->getPoint(startPoint);
                status = true;
            }

            if(status)
            {
                start.x = startPoint->x();
                start.y = startPoint->y();
                delete startPoint;
            }
            else
            {
                cmdline->reset();
                ctx->getGraphicView()->killAllActions();
                delete startPoint;
                return false;
            }
        }
    }

    prompt = QObject::tr("Enter second point: ");
    cmdline->setPrompt(QObject::tr(qUtf8Printable(prompt)));

    auto endAction = std::make_shared<QC_ActionGetRad>(ctx);
    if (endAction)
    {
        QPointF *base = new QPointF(start.x, start.y);

        ctx->getGraphicView()->setCurrentAction(endAction);
        endAction->setBasepoint(base);

        QObject::connect(cmdline, &QLineEdit::returnPressed, static_cast<QC_ActionGetRad*>(endAction.get()), [cmdline, endAction, prompt, &rad, &finished]
        {
            const QString result = cmdline->text();
            static const QRegularExpression floatRegExpr(QStringLiteral("^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)$"));
            QRegularExpressionMatch match = floatRegExpr.match(result);

            if (result.isEmpty())
            {
                endAction->trigger();
            }

            if (match.hasMatch())
            {
                rad = result.toDouble();
                endAction->trigger();
                finished = true;
            }
            else
            {
                cmdline->setPrompt(QObject::tr(qUtf8Printable(prompt)));
            }
        } );

        while (!endAction->isCompleted())
        {
            ev.processEvents();

            if (!ctx->getGraphicView()->hasAction())
            {
                break;
            }
        }

        QObject::disconnect(cmdline, &QLineEdit::returnPressed, static_cast<QC_ActionGetRad*>(endAction.get()), NULL);
        ctx->getGraphicView()->killAllActions();
        ctx->getGraphicView()->redraw();
        cmdline->reset();
        delete base;

        if(finished)
        {
            return true;
        }

        if (endAction->isCompleted() && !endAction->wasCanceled())
        {
            rad = endAction->getRad();
            return true;
        }
    }
    return false;
}

bool RS_ScriptingApi::getReal(CommandEdit *cmdline, const char *msg, double &res)
{
    QString prompt = "Enter a floating point number: ";
    QString result;

    if (strcmp(msg, ""))
    {
        prompt = msg;
    }

    if (cmdline)
    {
        while (1)
        {
            cmdline->setPrompt(QObject::tr(qUtf8Printable(prompt)));
            result = RS_Scripting_InputHandle::readLine(QObject::tr(qUtf8Printable(prompt)), cmdline);

            if (result.isEmpty())
            {
                cmdline->reset();
                return false;
            }

            static const QRegularExpression floatRegExpr(QStringLiteral("^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)$"));
            QRegularExpressionMatch match = floatRegExpr.match(result);

            if (match.hasMatch())
            {
                res = result.toDouble();
                break;
            }
        }

        cmdline->reset();
        return true;
    }
    else
    {
        res = getDoubleDlg(qUtf8Printable(prompt));
        return true;
    }
    return false;
}

bool RS_ScriptingApi::getInteger(CommandEdit *cmdline, const char *msg, int &res)
{
    QString prompt = "Enter an integer: ";
    QString result;

    if (strcmp(msg, ""))
    {
        prompt = msg;
    }

    if (cmdline)
    {
        while (1)
        {
            cmdline->setPrompt(QObject::tr(qUtf8Printable(prompt)));
            result = RS_Scripting_InputHandle::readLine(QObject::tr(qUtf8Printable(prompt)), cmdline);

            if (result.isEmpty())
            {
                cmdline->reset();
                return false;
            }

            static const QRegularExpression intRegExpr(QStringLiteral("^[+-]?[0-9]+|[+-]?0[xX][0-9A-Fa-f]"));
            QRegularExpressionMatch match = intRegExpr.match(result);

            if (match.hasMatch())
            {
                res = result.toInt();
                break;
            }
        }

        cmdline->reset();
        return true;
    }
    else
    {
        res = getIntDlg(qUtf8Printable(prompt));
        return true;
    }
    return false;
}

bool RS_ScriptingApi::getString(CommandEdit *cmdline, bool cr, const char *msg, std::string &res)
{
    QString prompt = "Enter a text: ";
    QString result;

    Q_UNUSED(cr);

    //fixme 'cr' true no return on space input

    if (strcmp(msg, ""))
    {
        prompt = msg;
    }

    if (cmdline)
    {
        while (1)
        {
            cmdline->setPrompt(QObject::tr(qUtf8Printable(prompt)));
            result = RS_Scripting_InputHandle::readLine(QObject::tr(qUtf8Printable(prompt)), cmdline);

            if (result.isEmpty())
            {
                cmdline->reset();
                return false;
            }
            else
            {
                res = qUtf8Printable(result);
                break;
            }

        }

        cmdline->reset();
        return true;
    }
    else
    {
        res = getStrDlg(qUtf8Printable(prompt));
        return true;
    }
    return false;
}

bool RS_ScriptingApi::getKeyword(CommandEdit *cmdline, const char *msg, std::string &res)
{
    const lclInteger* bit = VALUE_CAST(lclInteger, shadowEnv->get("initget_bit"));
    const lclString* pat = VALUE_CAST(lclString, shadowEnv->get("initget_string"));

    std::vector<std::string> StringList;
    std::string del = " ";
    std::string pattern = pat->value();

    auto pos = pattern.find(del);
    while (pos != std::string::npos)
    {
        StringList.push_back(pattern.substr(0, pos));
        pattern.erase(0, pos + del.length());
        pos = pattern.find(del);
    }
    StringList.push_back(pattern);

    if (cmdline)
    {
        while (1)
        {
            cmdline->setPrompt(QObject::tr(msg));
            res = RS_Scripting_InputHandle::readLine(QObject::tr(msg), cmdline).toStdString();

            for (auto &it : StringList) {
                if (it == res)
                {
                    cmdline->reset();
                    return true;
                }
            }

            if ((bit->value() & 1) != 1)
            {
                cmdline->reset();
                return false;
            }
        }

        cmdline->reset();
    }
    else
    {
        while (1) {
            res = getStrDlg(qUtf8Printable(QObject::tr(msg)));

            for (auto &it : StringList) {
                if (it == res)
                {
                    return true;
                }
            }
            if ((bit->value() & 1) != 1)
            {
                return false;
            }
        }
    }
    return false;
}

bool RS_ScriptingApi::colorDialog(int color, bool by, int &res)
{
    return QG_ColorDlg::getIndexColor(res, NULL, color, by);
}

bool RS_ScriptingApi::trueColorDialog(int &tres, int &res, int tcolor, int color, bool by, int tbycolor, int bycolor)
{
    return QG_ColorDlg::getTrueColor(tres, res, NULL, tcolor, color, by, tbycolor, bycolor);
}

bool RS_ScriptingApi::getSelection(unsigned int &id)
{
    bool status = false;
    std::vector<unsigned int> ssget;

    if (getGraphicView() == NULL || getGraphic() == NULL){
        qDebug() << "graphicView == NULL";
        return false;
    }

    auto ctx = QC_ApplicationWindow::getAppWindow()->getActionContext();

    auto actionSelect = std::make_shared<QC_ActionSelectSet>(ctx);
    if (actionSelect)
    {
        ctx->getGraphicView()->killAllActions();
        ctx->getGraphicView()->setCurrentAction(actionSelect);

        QEventLoop ev;
        while (!actionSelect->isCompleted())
        {
            ev.processEvents ();
            if (!ctx->getGraphicView()->hasAction())
                break;
        }
    }

    if (actionSelect->isCompleted() && !actionSelect->wasCanceled())
    {
        actionSelect->getSelected(ssget);
        status = true;
    }

    ctx->getGraphicView()->killAllActions();

    int length = ssget.size();
    if (length == 0)
    {
        return false;
    }

    lclValueVec* items = new lclValueVec(length);
    for (int i = 0; i < length; i++) {
        (*items)[i] = lcl::integer(ssget.at(i));
    }

    id = getNewSelectionId();
    shadowEnv->set(getSelectionName(getNewSelectionId()), lcl::list(items));

    qDebug() << "[RS_ScriptingApi::getSelected] status:" << status;
    return status;
}

bool RS_ScriptingApi::getSingleSelection(std::vector<unsigned int> &selection_set)
{
    qDebug() << "[RS_ScriptingApi::getSingleSelection] start";
    bool status = false;

    if (getGraphicView() == NULL || getGraphic() == NULL){
        qDebug() << "graphicView == NULL";
        return false;
    }

    auto ctx = QC_ApplicationWindow::getAppWindow()->getActionContext();

    auto actionSelect = std::make_shared<QC_ActionSingleSet>(ctx);
    if (actionSelect)
    {
        ctx->getGraphicView()->killAllActions();
        ctx->getGraphicView()->setCurrentAction(actionSelect);

        QEventLoop ev;
        while (!actionSelect->isCompleted())
        {

            ev.processEvents();
            if (!ctx->getGraphicView()->hasAction())
            {
                break;
            }
        }
    }

    if (actionSelect->isCompleted())
    {
        actionSelect->getSelected(selection_set);
        status = true;
    }

    ctx->getGraphicView()->killAllActions();

    int length = selection_set.size();
    if (length == 0)
    {
        return false;
    }

    qDebug() << "[RS_ScriptingApi::getSingleSelection] status:" << status;
    return status;
}

bool RS_ScriptingApi::filterByData(RS_Entity *entity, const RS_ScriptingApiData &apiData)
{
    bool hasData = false;

    if(!apiData.etype.isEmpty())
    {
        hasData = false;
        if(entity->rtti() == getEntityIdbyName(apiData.etype))
        {
            hasData = true;
        }
    }

    if(!apiData.layer.isEmpty())
    {
        hasData = false;
        if(entity->getLayer()->getName() == apiData.layer)
        {
            hasData = true;
        }
    }

    if(!apiData.gc_lineWidth.empty())
    {
        hasData = false;
        if(entity->getPen(false).getWidth() == apiData.pen.getWidth())
        {
            hasData = true;
        }
    }

    if(!apiData.linetype.isEmpty())
    {
        hasData = false;
        if(entity->getPen(false).getLineType() == apiData.pen.getLineType())
        {
            hasData = true;
        }
    }

    if(!apiData.gc_62.empty())
    {
        hasData = false;
        const RS_Color color = entity->getPen(false).getColor();
        int exact_rgb;

        if(RS_FilterDXFRW::colorToNumber(color, &exact_rgb) == apiData.gc_62.front())
        {
            hasData = true;
        }
    }

    if(!apiData.gc_402.empty())
    {
        hasData = false;
        const RS_Color color = entity->getPen(false).getColor();
        int exact_rgb;
        RS_FilterDXFRW::colorToNumber(color, &exact_rgb);

        if(exact_rgb == apiData.gc_402.front())
        {
            hasData = true;
        }
    }

    return hasData;
}

bool RS_ScriptingApi::getSelectionByData(const RS_ScriptingApiData &apiData, std::vector<unsigned int> &selection_set)
{
    RS_EntityContainer* entityContainer = getContainer();

    if(entityContainer && entityContainer->count())
    {
        for (auto e: *entityContainer)
        {
            if(filterByData(e, apiData))
            {
                selection_set.push_back(e->getId());
            }
        }

        return true;
    }

    return false;
}

bool RS_ScriptingApi::selectAll(std::vector<unsigned int> &selection_set)
{
    RS_EntityContainer* entityContainer = getContainer();

    if(entityContainer && entityContainer->count())
    {
        for (auto e: *entityContainer)
        {
            selection_set.push_back(e->getId());
        }

        return true;
    }

    return false;
}

bool RS_ScriptingApi::entdel(unsigned int id)
{
    RS_Document* doc = getDocument();
    RS_GraphicView* graphicView = getGraphicView();
    RS_EntityContainer* entityContainer = getContainer();
    LC_UndoSection undo(doc, graphicView->getViewPort());

    if(entityContainer)
    {
        for (auto e: *entityContainer) {
            if (e->getId() == id)
            {
                e->setSelected(false);
                e->changeUndoState();
                undo.addUndoable(e);
                graphicView->redraw(RS2::RedrawDrawing);
                return true;
            }
        }
    }

    return false;
}

unsigned int RS_ScriptingApi::entlast()
{
    RS_EntityContainer* entityContainer = getContainer();
    unsigned int id = 0;

    if(entityContainer && entityContainer->count())
    {
        for (auto e: *entityContainer)
        {
            switch(e->rtti())
            {
            case RS2::EntityContainer:
                qDebug() << "[entlast] rtti: RS2::EntityContainer";
                break;
            case RS2::EntityBlock:
                qDebug() << "[entlast] rtti: RS2::EntityBlock";
                break;
            case RS2::EntityFontChar:
                qDebug() << "[entlast] rtti: RS2::EntityFontChar";
                break;
            case RS2::EntityInsert:
                qDebug() << "[entlast] rtti: RS2::EntityInsert";
                break;
            case RS2::EntityGraphic:
                qDebug() << "[entlast] rtti: RS2::EntityGraphic";
                break;
            case RS2::EntityPoint:
                qDebug() << "[entlast] rtti: RS2::EntityPoint";
                break;
            case RS2::EntityLine:
                qDebug() << "[entlast] rtti: RS2::EntityLine";
                break;
            case RS2::EntityPolyline:
                qDebug() << "[entlast] rtti: RS2::EntityPolyline";
                break;
            case RS2::EntityVertex:
                qDebug() << "[entlast] rtti: RS2::EntityVertex";
                break;
            case RS2::EntityArc:
                qDebug() << "[entlast] rtti: RS2::EntityArc";
                break;
            case RS2::EntityCircle:
                qDebug() << "[entlast] rtti: RS2::EntityCircle";
                break;
            case RS2::EntityEllipse:
                qDebug() << "[entlast] rtti: RS2::EntityEllipse";
                break;
            case RS2::EntityHyperbola:
                qDebug() << "[entlast] rtti: RS2::EntityHyperbola";
                break;
            case RS2::EntitySolid:
                qDebug() << "[entlast] rtti: RS2::EntitySolid";
                break;
            case RS2::EntityConstructionLine:
                qDebug() << "[entlast] rtti: RS2::EntityConstructionLine";
                break;
            case RS2::EntityMText:
                qDebug() << "[entlast] rtti: RS2::EntityMText";
                break;
            case RS2::EntityText:
                qDebug() << "[entlast] rtti: RS2::EntityText";
                break;
            case RS2::EntityDimAligned:
                qDebug() << "[entlast] rtti: RS2::EntityDimAligned";
                break;
            case RS2::EntityDimLinear:
                qDebug() << "[entlast] rtti: RS2::EntityDimLinear";
                break;
            case RS2::EntityDimRadial:
                qDebug() << "[entlast] rtti: RS2::EntityDimRadial";
                break;
            case RS2::EntityDimDiametric:
                qDebug() << "[entlast] rtti: RS2::EntityDimDiametric";
                break;
            case RS2::EntityDimAngular:
                qDebug() << "[entlast] rtti: RS2::EntityDimAngular";
                break;
            case RS2::EntityDimArc:
                qDebug() << "[entlast] rtti: RS2::EntityDimArc";
                break;
            case RS2::EntityDimLeader:
                qDebug() << "[entlast] rtti: RS2::EntityDimLeader";
                break;
            case RS2::EntityHatch:
                qDebug() << "[entlast] rtti: RS2::EntityHatch";
                break;
            case RS2::EntityImage:
                qDebug() << "[entlast] rtti: RS2::EntityImage";
                break;
            case RS2::EntitySpline:
                qDebug() << "[entlast] rtti: RS2::EntitySpline";
                break;
            case RS2::EntitySplinePoints:
                qDebug() << "[entlast] rtti: RS2::EntitySplinePoints";
                break;
            case RS2::EntityParabola:
                qDebug() << "[entlast] rtti: RS2::EntityParabola";
                break;
            case RS2::EntityOverlayBox:
                qDebug() << "[entlast] rtti: RS2::EntityOverlayBox";
                break;
            case RS2::EntityPreview:
                qDebug() << "[entlast] rtti: RS2::EntityPreview";
                break;
            case RS2::EntityPattern:
                qDebug() << "[entlast] rtti: RS2::EntityPattern";
                break;
            case RS2::EntityOverlayLine:
                qDebug() << "[entlast] rtti: RS2::EntityOverlayLine";
                break;
            case RS2::EntityRefPoint:
                qDebug() << "[entlast] rtti: RS2::EntityRefPoint";
                break;
            case RS2::EntityRefLine:
                qDebug() << "[entlast] rtti: RS2::EntityRefLine";
                break;
            case RS2::EntityRefConstructionLine:
                qDebug() << "[entlast] rtti: RS2::EntityRefConstructionLine";
                break;
            case RS2::EntityRefArc:
                qDebug() << "[entlast] rtti: RS2::EntityRefArc";
                break;
            case RS2::EntityRefCircle:
                qDebug() << "[entlast] rtti: RS2::EntityRefCircle";
                break;
            case RS2::EntityRefEllipse:
                qDebug() << "[entlast] rtti: RS2::EntityRefEllipse";
                break;

            default:
                qDebug() << "[entlast] rtti: RS2::EntityUnknown";
                break;
            }

            qDebug() << "[entlast] id:" << e->getId();
            qDebug() << "[entlast] Flags:" << e->getFlags();
            qDebug() << "[entlast] -----------------------------";

            if (e->getFlags() == 2 && e->getId() > id)
            {
                id = e->getId();
            }
        }
    }
    return id;
}

unsigned int RS_ScriptingApi::entnext(unsigned int current)
{
    RS_EntityContainer* entityContainer = getContainer();
    unsigned int maxId = 0;
    unsigned int id = 0;

    if(entityContainer && entityContainer->count())
    {
        for (auto e: *entityContainer)
        {
            if (e->getFlags() == 2 && maxId < e->getId())
            {
                maxId = e->getId();
            }
        }
        id = maxId;

        if (current == 0)
        {
            for (auto e: *entityContainer)
            {
                if (e->getFlags() == 2 && e->getId() < id)
                {
                    id = e->getId();
                }
            }
            return id <= maxId ? id : 0;
        }
        else
        {
            for (auto e: *entityContainer)
            {
                if (e->getFlags() == 2 && e->getId() > current)
                {
                    if (id > e->getId())
                    {
                        id = e->getId();
                    }
                }
            }
            return id > current ? id : 0;
        }
    }

    return 0;
}

unsigned int RS_ScriptingApi::sslength(const std::string &name)
{
    unsigned int result = 0;
    lclValuePtr value = shadowEnv->get(name);

    if (value)
    {
        const lclList* list = DYNAMIC_CAST(lclList, value);
        if (!list || (list->count() == 0)) {
            return result;
        }
        result = static_cast<unsigned int>(list->count());
    }

    return result;
}

bool RS_ScriptingApi::ssname(unsigned int ss, unsigned int idx, unsigned int &id)
{
    lclValuePtr value = shadowEnv->get(getSelectionName(ss));

    if (value)
    {
        const lclList* list = DYNAMIC_CAST(lclList, value);

        if (!list || list->count() == 0)
        {
            return false;
        }

        if (static_cast<int>(idx) < list->count())
        {
            const lclInteger *idVal = VALUE_CAST(lclInteger, list->item(idx));
            id = idVal->value();
            return true;
        }
    }

    return false;
}

bool RS_ScriptingApi::ssadd(unsigned int id, unsigned int ss, unsigned int &newss)
{
    if (ss+id == 0)
    {
        newss = RS_SCRIPTINGAPI->getNewSelectionId();
        shadowEnv->set(RS_SCRIPTINGAPI->getSelectionName(newss), lcl::list(new lclValueVec(0)));
        return true;
    }
    else
    {
        // check db for entity or nil

        lclValuePtr value = shadowEnv->get(getSelectionName(ss));

        if (value)
        {
            const lclList* list = DYNAMIC_CAST(lclList, value);

            if (!list)
            {
                return false;
            }

            lclValueVec* items = new lclValueVec(list->count());
            std::copy(list->begin(), list->end(), items->begin());
            items->push_back(lcl::integer(id));

            shadowEnv->set(RS_SCRIPTINGAPI->getSelectionName(ss), lcl::list(items));
            newss = ss;

            return true;
        }
    }
    return false;
}

bool RS_ScriptingApi::ssdel(unsigned int id, unsigned int ss)
{
    bool found = false;

    lclValuePtr value = shadowEnv->get(getSelectionName(ss));
    if (value)
    {
        const lclList* list = DYNAMIC_CAST(lclList, value);

        if (!list || list->count() == 0)
        {
            return false;
        }

        lclValueVec* items = new lclValueVec(0);
        for (auto it = list->begin(), end = list->end(); it != end; it++)
        {
            const lclInteger *idVal = DYNAMIC_CAST(lclInteger, *it);
            const unsigned int curr = idVal->value();

            if (curr == id)
            {
                found = true;
            }
            else
            {
                items->push_back(lcl::integer(id));
            }
        }

        if (found)
        {
            shadowEnv->set(getSelectionName(ss), lcl::list(items));
            return true;
        }
        else
        {
            delete items;
        }
    }

    return false;
}

bool RS_ScriptingApi::entsel(CommandEdit *cmdline, const QString &prompt, unsigned long &id, RS_Vector &point)
{
    QString prom = "Select object: ";

    if (!prompt.isEmpty())
    {
        prom = prompt;
    }

    if (getGraphicView() == NULL || getGraphic() == NULL){
        qDebug() << "graphicView == NULL";
        return false;
    }

    auto ctx = QC_ApplicationWindow::getAppWindow()->getActionContext();

    auto a = std::make_shared<QC_ActionEntSel>(ctx);
    if (a)
    {
        a->setMessage(prom);
        ctx->getGraphicView()->killAllActions();
        ctx->getGraphicView()->setCurrentAction(a);

        QEventLoop ev;

        cmdline->setPrompt(QObject::tr(qUtf8Printable(prom)));

        while (!a->isCompleted())
        {
            ev.processEvents ();
            if (!ctx->getGraphicView()->hasAction())
                break;
        }

        cmdline->reset();

        if (a->isCompleted())
        {
            ctx->getGraphicView()->killAllActions();

            if (a->wasCanceled())
            {
                return false;
            }

            point.x = a->getPoint().x;
            point.y = a->getPoint().y;
            point.z = a->getPoint().z;
            id = static_cast<unsigned long>(a->getEntityId());

            return true;
        }
    }

    ctx->getGraphicView()->killAllActions();
    return false;
}

int RS_ScriptingApi::loadDialog(const char *filename)
{
    std::string path = filename;
    const std::filesystem::path p(path.c_str());
    if (!p.has_extension()) {
        path += ".dcl";
    }
    if (!std::filesystem::exists(path.c_str())) {
        return -1;
    }

    lclValuePtr dcl = loadDcl(path);
    const lclGui *container = VALUE_CAST(lclGui, dcl);

    if (dcl) {
        int uniq = container->value().dialog_Id;
        dclEnv->set(STRF("#builtin-gui(%d)", uniq), dcl);
        dclEnv->set("load_dialog_id", lcl::integer(uniq));
        return uniq;
    }
    return -1;
}

bool RS_ScriptingApi::newDialog(const char *name, int id)
{
    const lclGui*     gui     = DYNAMIC_CAST(lclGui, dclEnv->get(STRF("#builtin-gui(%d)", id)));
    lclValueVec*      items   = new lclValueVec(gui->value().tiles->size());
    std::copy(gui->value().tiles->begin(), gui->value().tiles->end(), items->begin());

    for (auto it = items->begin(), end = items->end(); it != end; it++) {
        const lclGui* dlg = DYNAMIC_CAST(lclGui, *it);
        qDebug() << "Dialog: " << dlg->value().name.c_str();
        if (dlg->value().name == name) {
            openTile(dlg);
            return true;
        }
    }
    return false;
}

int RS_ScriptingApi::startDialog()
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    if(dialogId)
    {
        for (auto & tile : dclTiles)
        {
            if (tile->value().dialog_Id == dialogId->value())
            {
                const lclDialog* dlg = static_cast<const lclDialog*>(tile);
                dlg->dialog()->show();
                dlg->dialog()->setFixedSize(dlg->dialog()->geometry().width(),
                                            dlg->dialog()->geometry().height());
                if(tile->value().initial_focus != "")
                {
                    for (auto & child : dclTiles)
                    {
                        if (child->value().dialog_Id == dialogId->value() &&
                            child->value().key == tile->value().initial_focus)
                        {
                            switch (child->value().id)
                            {
                                case BUTTON:
                                {
                                    const lclButton* b = static_cast<const lclButton*>(tile);
                                    b->button()->setFocus();
                                }
                                    break;
                                case EDIT_BOX:
                                {
                                    const lclEdit* edit = static_cast<const lclEdit*>(tile);
                                    edit->edit()->setFocus();
                                }
                                    break;
                                case IMAGE_BUTTON:
                                {
                                    const lclImageButton* ib = static_cast<const lclImageButton*>(tile);
                                    ib->button()->setFocus();
                                }
                                    break;
                                case LIST_BOX:
                                {
                                    const lclListBox* l = static_cast<const lclListBox*>(tile);
                                    l->list()->setFocus();
                                }
                                    break;
                                case POPUP_LIST:
                                {
                                    const lclPopupList* pl = static_cast<const lclPopupList*>(tile);
                                    pl->list()->setFocus();
                                }
                                    break;
                                case RADIO_BUTTON:
                                {
                                    const lclRadioButton* rb = static_cast<const lclRadioButton*>(tile);
                                    rb->button()->setFocus();
                                }
                                    break;
                                case SCROLL:
                                {
                                    const lclScrollBar* sb = static_cast<const lclScrollBar*>(tile);
                                    sb->slider()->setFocus();
                                }
                                    break;
                                case SLIDER:
                                {
                                    const lclSlider* sl = static_cast<const lclSlider*>(tile);
                                    sl->slider()->setFocus();
                                }
                                    break;
                                case DIAL:
                                {
                                    const lclDial* sc = static_cast<const lclDial*>(tile);
                                    sc->slider()->setFocus();
                                }
                                    break;
                                case TOGGLE:
                                {
                                    const lclToggle* tb = static_cast<const lclToggle*>(tile);
                                    tb->toggle()->setFocus();
                                }
                                    break;
                                case TAB:
                                {
                                    const lclWidget* w = static_cast<const lclWidget*>(tile);
                                    w->widget()->setFocus();
                                }
                                break;
                                default:
                                    break;
                            }
                        }
                    }
                }
                return dlg->dialog()->exec();
            }
        }
    }
    return -1;
}

void RS_ScriptingApi::unloadDialog(int id)
{
    if(id)
    {
        for (auto & tile : dclTiles)
        {
            if (tile->value().dialog_Id == id)
            {
                const lclDialog* dlg = static_cast<const lclDialog*>(tile);
                delete dlg->dialog();

                break;
            }
        }
    }

    for (int i = 0; i < (int) dclTiles.size(); i++)
    {
        if(dclTiles.at(i)->value().dialog_Id == id)
        {
            dclTiles.erase(dclTiles.begin()+i);
        }
    }

    dclEnv->set(STRF("#builtin-gui(%d)", id), lcl::nilValue());
    dclEnv->set("load_dialog_id", lcl::nilValue());
}

void RS_ScriptingApi::termDialog()
{
    for (int i = dclTiles.size() - 1; i >= 0; i--)
    {
        if (dclTiles.at(i)->value().id == DIALOG)
        {
            const lclDialog* dlg = static_cast<const lclDialog*>(dclTiles.at(i));
            dlg->dialog()->done(0);
            dlg->dialog()->deleteLater();
            dclEnv->set(STRF("#builtin-gui(%d)", dclTiles.at(i)->value().dialog_Id), lcl::nilValue());
        }
    }
    dclTiles.clear();
    dclEnv->set("load_dialog_id", lcl::nilValue());
}

bool RS_ScriptingApi::doneDialog(int res, int &x, int &y)
{
    int result = -1;
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    if(dialogId)
    {
        for (auto & tile : dclTiles)
        {
            if (tile->value().dialog_Id == dialogId->value())
            {
                const lclDialog* dlg = static_cast<const lclDialog*>(tile);
                const lclInteger *dlg_result = VALUE_CAST(lclInteger, dclEnv->get(std::to_string(dialogId->value()) + "_dcl_result"));

                result = dlg_result->value();

                //qDebug() << "RS_ScriptingApi::doneDialog] result:" << result;
                if (res > 1)
                {
                    result = res;
                }
                //qDebug() << "RS_ScriptingApi::doneDialog] res:" << res;
                dlg->dialog()->done(result);
                x = dlg->dialog()->x();
                y = dlg->dialog()->y();
                return true;
            }
        }
    }
    return false;
}

bool RS_ScriptingApi::setTile(const char *key, const char *val)
{
    static const QRegularExpression intRegExpr(QStringLiteral("^[-+]?\\d+$"));
    QRegularExpressionMatch match = intRegExpr.match(val);

    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));
    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key)
        {
            switch (tile->value().id)
            {
            case EDIT_BOX:
            {
                const lclEdit* edit = static_cast<const lclEdit*>(tile);
                edit->edit()->setText(val);
            }
                break;
            case TEXT:
            case ERRTILE:
            {
                const lclLabel* l = static_cast<const lclLabel*>(tile);
                l->label()->setText(val);
            }
                break;
            case BUTTON:
            {
                const lclButton* b = static_cast<const lclButton*>(tile);
                b->button()->setText(val);
            }
                break;
            case RADIO_BUTTON:
            {
                const lclRadioButton* rb = static_cast<const lclRadioButton*>(tile);
                rb->button()->setText(val);
            }
                break;
            case TOGGLE:
            {
                const lclToggle* tb = static_cast<const lclToggle*>(tile);
                if(std::string("0") == val)
                {
                    tb->toggle()->setChecked(false);
                }
                if(std::string("1") == val)
                {
                    tb->toggle()->setChecked(true);
                }
            }
                break;
            case OK_CANCEL_HELP_ERRTILE:
            {
                const lclOkCancelHelpErrtile* err = static_cast<const lclOkCancelHelpErrtile*>(tile);
                err->errtile()->setText(val);
            }
                break;
            case DIAL:
            {
                const lclDial* sc = static_cast<const lclDial*>(tile);

                if (match.hasMatch())
                {
                    sc->slider()->setValue(atoi(val));
                }
            }
             break;
            case SCROLL:
            {
                const lclScrollBar* sc = static_cast<const lclScrollBar*>(tile);
                if (match.hasMatch())
                {
                    sc->slider()->setValue(atoi(val));
                }
            }
                break;
            case SLIDER:
            {
                const lclSlider* sc = static_cast<const lclSlider*>(tile);
                if (match.hasMatch())
                {
                    sc->slider()->setValue(atoi(val));
                }
            }
                break;
            default:
                return false;
            }
            return true;
        }
    }
    return false;
}

bool RS_ScriptingApi::actionTile(const char *id, const char *action)
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    lclValuePtr value = dclEnv->get(std::to_string(dialogId->value()) + "_" + id);
    qDebug() << "value->print(true)" << value->print(true).c_str();
    if (value->print(true).compare("nil") == 0) {
        dclEnv->set(std::to_string(dialogId->value()) + "_" + id, lcl::string(action));
        return true;
    }
    return false;
}

bool RS_ScriptingApi::addLayer(const char *name, const RS_Pen &pen, int state)
{
    RS_Graphic* graphic = getGraphic();
    RS_LayerList* layerList = graphic->getLayerList();
    QString layer_name = name;
    QString newLayerName;

    if (NULL != layerList) {
        newLayerName = layer_name;

        QString sBaseLayerName( layer_name);
        QString sNumLayerName;
        int nlen {1};
        int i {0};
        static const QRegularExpression re(QStringLiteral("^(.*\\D+|)(\\d*)$"));
        QRegularExpressionMatch match(re.match(layer_name));
        if (match.hasMatch()) {
            sBaseLayerName = match.captured(1);
            if( 1 < match.lastCapturedIndex()) {
                sNumLayerName = match.captured(2);
                nlen = sNumLayerName.length();
                i = sNumLayerName.toInt();
            }
        }

        while (layerList->find(newLayerName)) {
            newLayerName = QString("%1%2").arg(sBaseLayerName).arg( ++i, nlen, 10, QChar('0'));
        }

        RS_Layer *l = new RS_Layer(newLayerName);
        l->setPen(pen);

        if(state & 1)
        {
            l->freeze(true);
        }

        if(state & 2)
        {
            l->freeze(true);
        }

        if(state & 4)
        {
            l->lock(true);
        }

        graphic->addLayer(l);
        return true;
    }

    return false;
}

void RS_ScriptingApi::addLine(const RS_Vector &start, const RS_Vector &end, const RS_Pen &pen)
{
    RS_EntityContainer* container = getContainer();
    RS_Line *line  = new RS_Line(container, start, end);

    line->setPen(pen);
    container->addEntity(line);

    LC_UndoSection undo(getDocument(), getGraphicView()->getViewPort());
    undo.addUndoable(line);
}

void RS_ScriptingApi::addCircle(const RS_Vector &pnt, double rad, const RS_Pen &pen)
{
    RS_EntityContainer* container = getContainer();
    RS_Circle *circle = new RS_Circle(container, RS_CircleData(pnt, rad));

    circle->setPen(pen);
    container->addEntity(circle);

    LC_UndoSection undo(getDocument(), getGraphicView()->getViewPort());
    undo.addUndoable(circle);
}

void RS_ScriptingApi::addArc(const RS_Vector &pnt, double rad, double ang1, double ang2, const RS_Pen &pen)
{
    RS_EntityContainer* container = getContainer();
    RS_Arc *arc = new RS_Arc(container, RS_ArcData(pnt, rad, ang1, ang2, false));
    arc->setPen(pen);
    container->addEntity(arc);

    LC_UndoSection undo(getDocument(), getGraphicView()->getViewPort());
    undo.addUndoable(arc);
}

void RS_ScriptingApi::addBlock(const RS_InsertData &data, const RS_Pen &pen)
{
    RS_EntityContainer* container = getContainer();
    RS_Insert *block = new RS_Insert(container, data);
    block->setPen(pen);
    container->addEntity(block);

    LC_UndoSection undo(getDocument(), getGraphicView()->getViewPort());
    undo.addUndoable(block);
}

void RS_ScriptingApi::addEllipse(const RS_Vector &center, const RS_Vector &majorP, double rad, const RS_Pen &pen)
{
    RS_EntityContainer* container = getContainer();
    RS_EllipseData data;
    data.center = center;
    data.majorP = majorP;
    data.ratio = rad;
    RS_Ellipse *ellipse = new RS_Ellipse(container, data);
    ellipse->setPen(pen);
    container->addEntity(ellipse);

    LC_UndoSection undo(getDocument(), getGraphicView()->getViewPort());
    undo.addUndoable(ellipse);
}

void RS_ScriptingApi::addPoint(const RS_Vector &pnt, const RS_Pen &pen)
{
    RS_EntityContainer* container = getContainer();
    RS_Point *point = new RS_Point(container, RS_PointData(pnt));
    point->setPen(pen);
    container->addEntity(point);

    LC_UndoSection undo(getDocument(), getGraphicView()->getViewPort());
    undo.addUndoable(point);
}

void RS_ScriptingApi::addLwPolyline(const std::vector<Plug_VertexData> &points, bool closed, const RS_Pen &pen)
{
    RS_EntityContainer* container = getContainer();
    RS_PolylineData data;

    if(closed)
    {
        data.setFlag(RS2::FlagClosed);
    }

    RS_Polyline *polyline = new RS_Polyline(container, data);

    for(auto const& pt: points){
        polyline->addVertex(RS_Vector(pt.point.x(), pt.point.y()), pt.bulge);
    }

    polyline->setPen(pen);
    container->addEntity(polyline);

    LC_UndoSection undo(getDocument(), getGraphicView()->getViewPort());
    undo.addUndoable(polyline);
}

void RS_ScriptingApi::addText(const RS_Vector &pnt, double height, double width, double angle, int valign, int halign, int generation, const QString &txt, const QString &style, const RS_Pen &pen)
{
    RS_EntityContainer* container = getContainer();

    RS_TextData::VAlign val = static_cast <RS_TextData::VAlign>(valign);
    RS_TextData::HAlign hal = static_cast <RS_TextData::HAlign>(halign);

    RS_TextData::TextGeneration tGen;
    switch (generation) {
    case 2:
        tGen = RS_TextData::Backward;
        break;
    case 4:
        tGen = RS_TextData::UpsideDown;
        break;
    default:
        tGen = RS_TextData::None;
        break;
    }

    RS_TextData data(pnt, pnt, height, width, val, hal, tGen, txt, style, angle, RS2::Update);

    RS_Text* text = new RS_Text(container, data);
    text->setPen(pen);
    container->addEntity(text);

    LC_UndoSection undo(getDocument(), getGraphicView()->getViewPort());
    undo.addUndoable(text);
}

void RS_ScriptingApi::addMText(const RS_Vector &pnt, double height, double width, double angle, int spacing, int direction, int attach, const QString &txt, const QString &style, const RS_Pen &pen)
{
    RS_EntityContainer* container = getContainer();
    RS_MTextData::MTextDrawingDirection dir;

    switch (direction) {
    case 1:
        dir = RS_MTextData::LeftToRight;
        break;
    case 2:
        dir = RS_MTextData::RightToLeft;
        break;
    case 3:
        dir = RS_MTextData::TopToBottom;
        break;
    case 5:
        dir = RS_MTextData::ByStyle;
        break;
    default:
        dir = RS_MTextData::ByStyle;
        break;
    }

    RS_MTextData data(pnt, height, width, RS_MTextData::VATop, RS_MTextData::HALeft, dir, spacing == 2 ? RS_MTextData::Exact : RS_MTextData::AtLeast, 0.0, txt, style, angle, RS2::Update);

    RS_MText* mtext = new RS_MText(container, data);
    mtext->setPen(pen);
    mtext->setAlignment(attach);
    container->addEntity(mtext);

    LC_UndoSection undo(getDocument(), getGraphicView()->getViewPort());
    undo.addUndoable(mtext);
}

void RS_ScriptingApi::addSolid(const RS_SolidData &data, const RS_Pen &pen)
{
    RS_EntityContainer* container = getContainer();
    RS_Solid *solid = new RS_Solid(container, data);
    solid->setPen(pen);
    container->addEntity(solid);

    LC_UndoSection undo(getDocument(), getGraphicView()->getViewPort());
    undo.addUndoable(solid);
}

void RS_ScriptingApi::addSpline(const RS_SplineData data, const std::vector<RS_Vector> &points, const RS_Pen &pen)
{
    RS_EntityContainer* container = getContainer();
    RS_Spline* spline = new RS_Spline(container, data);

    int count = points.size();

    for (int i = 0; i < count; i++) {
        RS_Vector point = points.at(i);
        spline->addControlPoint(point);
    }
    spline->update();
    spline->setPen(pen);
    container->addEntity(spline);

    LC_UndoSection undo(getDocument(), getGraphicView()->getViewPort());
    undo.addUndoable(spline);
}

void RS_ScriptingApi::addDimAligned(const RS_DimensionData &data, const RS_DimAlignedData &edata, const RS_Pen &pen)
{
    RS_EntityContainer* container = getContainer();
    RS_DimAligned *dim = new RS_DimAligned(container, data, edata);
    dim->updateDim(true);
    dim->setPen(pen);
    container->addEntity(dim);

    LC_UndoSection undo(getDocument(), getGraphicView()->getViewPort());
    undo.addUndoable(dim);
}

void RS_ScriptingApi::addDimAngular(const RS_DimensionData &data, const RS_DimAngularData &edata, const RS_Pen &pen)
{
    RS_EntityContainer* container = getContainer();
    RS_DimAngular *dim = new RS_DimAngular(container, data, edata);
    dim->updateDim(true);
    dim->setPen(pen);
    container->addEntity(dim);

    LC_UndoSection undo(getDocument(), getGraphicView()->getViewPort());
    undo.addUndoable(dim);
}

void RS_ScriptingApi::addDimDiametric(const RS_DimensionData &data, const RS_DimDiametricData &edata, const RS_Pen &pen)
{
    RS_EntityContainer* container = getContainer();
    RS_DimDiametric *dim = new RS_DimDiametric(container, data, edata);
    dim->updateDim(true);
    dim->setPen(pen);
    container->addEntity(dim);

    LC_UndoSection undo(getDocument(), getGraphicView()->getViewPort());
    undo.addUndoable(dim);
}

void RS_ScriptingApi::addDimLinear(const RS_DimensionData &data, const RS_DimLinearData &edata, const RS_Pen &pen)
{
    RS_EntityContainer* container = getContainer();
    RS_DimLinear *dim = new RS_DimLinear(container, data, edata);
    dim->updateDim(true);
    dim->setPen(pen);
    container->addEntity(dim);

    LC_UndoSection undo(getDocument(), getGraphicView()->getViewPort());
    undo.addUndoable(dim);
}

void RS_ScriptingApi::addDimRadial(const RS_DimensionData &data, const RS_DimRadialData &edata, const RS_Pen &pen)
{
    RS_EntityContainer* container = getContainer();
    RS_DimRadial *dim = new RS_DimRadial(container, data, edata);
    dim->updateDim(true);
    dim->setPen(pen);
    container->addEntity(dim);

    LC_UndoSection undo(getDocument(), getGraphicView()->getViewPort());
    undo.addUndoable(dim);
}

void RS_ScriptingApi::grdraw(const RS_Vector &start, const RS_Vector &end, int color, bool highlight)
{
    if (getGraphicView() == NULL || getGraphic() == NULL){
        qDebug() << "graphicView == NULL";
        return;
    }

    auto ctx = QC_ApplicationWindow::getAppWindow()->getActionContext();

    auto a = std::make_shared<QC_ActionGrDraw>(ctx);
    if (a)
    {
        ctx->getGraphicView()->killAllActions();
        ctx->getGraphicView()->setCurrentAction(a);

        a->drawLine(start, end, color, highlight);
        QEventLoop ev;

        while (a->getStatus() > -1)
        {
            ev.processEvents ();
            if (!ctx->getGraphicView()->hasAction())
                break;
        }
    }
}

void RS_ScriptingApi::grvecs(const std::vector<grdraw_line_t> &lines)
{
    if (getGraphicView() == NULL || getGraphic() == NULL){
        qDebug() << "graphicView == NULL";
        return;
    }

    auto ctx = QC_ApplicationWindow::getAppWindow()->getActionContext();

    auto a = std::make_shared<QC_ActionGrDraw>(ctx);
    if (a)
    {
        ctx->getGraphicView()->killAllActions();
        ctx->getGraphicView()->setCurrentAction(a);

        for (unsigned i = 0; i < lines.size(); i++)
        {
            a->drawLine(lines.at(i).start, lines.at(i).end, lines.at(i).color, lines.at(i).highlight);
        }

        QEventLoop ev;

        while (a->getStatus() > -1)
        {
            ev.processEvents ();
            if (!ctx->getGraphicView()->hasAction())
                break;
        }
    }
}

bool RS_ScriptingApi::grtext(int flag, const char *msg)
{
    if (flag == -1)
    {
        QC_ApplicationWindow::getAppWindow()->statusBar()->showMessage(msg);
        return true;
    }
    return false;
}

bool RS_ScriptingApi::entmake(const RS_ScriptingApiData &apiData)
{
    RS_Graphic* graphic = getGraphic();

    if (graphic)
    {
        if (apiData.etype == "LAYER" && apiData.layer != "")
        {
            return addLayer(qUtf8Printable(apiData.layer), apiData.pen, apiData.gc_70.empty() ? 0.0 : apiData.gc_70.front()) ? true : false;
        }

        else if (apiData.etype == "LINE"
                 && !apiData.gc_10.empty()
                 && !apiData.gc_11.empty())
        {
            addLine(apiData.gc_10.front(),
                    apiData.gc_11.front(),
                    apiData.pen);
        }

        else if (apiData.etype == "CIRCLE"
                 && !apiData.gc_10.empty()
                 && !apiData.gc_40.empty())
        {
            addCircle(apiData.gc_10.front(),
                      apiData.gc_40.front(),
                      apiData.pen);
        }

        else if (apiData.etype == "ARC"
                 && !apiData.gc_10.empty()
                 && !apiData.gc_40.empty()
                 && !apiData.gc_50.empty()
                 && !apiData.gc_51.empty())
        {
            addArc(apiData.gc_10.front(),
                   apiData.gc_40.front(),
                   apiData.gc_50.front(),
                   apiData.gc_51.front(),
                   apiData.pen);
        }

        else if (apiData.etype == "ELLIPSE"
                 && !apiData.gc_10.empty()
                 && !apiData.gc_11.empty()
                 && !apiData.gc_40.empty())
        {
            addEllipse(apiData.gc_10.front(),
                       apiData.gc_11.front(),
                       apiData.gc_40.front(),
                       apiData.pen);
        }

        else if (apiData.etype == "POINT" && !apiData.gc_10.empty())
        {
            addPoint(apiData.gc_10.front(), apiData.pen);
        }

        else if (apiData.etype == "LWPOLYLINE" && !apiData.gc_10.empty())
        {
            std::vector<Plug_VertexData> vertex;

            if (apiData.gc_42.size() == apiData.gc_10.size())
            {
                for (unsigned int i = 0; i < apiData.gc_10.size(); i++)
                {
                    vertex.push_back(Plug_VertexData(QPointF(apiData.gc_10.at(i).x,
                                            apiData.gc_10.at(i).y),
                                            apiData.gc_42.at(i)));
                }
            }
            else
            {
                for(auto const& pt: apiData.gc_10){
                    vertex.push_back(Plug_VertexData(QPointF(pt.x,
                                            pt.y),
                                            0.0));
                }
            }

            addLwPolyline(vertex, apiData.gc_70.empty() ? 0.0 : apiData.gc_70.front(), apiData.pen);
        }

        else if (apiData.etype == "MTEXT" && !apiData.gc_10.empty())
        {
            double height = 0.0;
            double width = 100.0;
            double angle = 0.0;

            if(!apiData.gc_40.empty())
            {
                height = apiData.gc_40.front();
            }

            if(!apiData.gc_41.empty())
            {
                width = apiData.gc_41.front();
            }

            if(!apiData.gc_50.empty())
            {
                angle = apiData.gc_50.front();
            }

            addMText(apiData.gc_10.front(),
                     height,
                     width,
                     angle,
                     apiData.gc_73.empty() ? 0.0 : apiData.gc_73.front(),
                     apiData.gc_72.empty() ? 0.0 : apiData.gc_72.front(),
                     apiData.gc_71.empty() ? 0.0 : apiData.gc_71.front(),
                     apiData.text.isEmpty() ? "" : apiData.text,
                     apiData.style.isEmpty() ? "STANDARD" : apiData.style,
                     apiData.pen);
        }

        else if (apiData.etype == "TEXT" && !apiData.gc_10.empty())
        {
            double height = 0.0;
            double width = 1.0;
            double angle = 0.0;

            if(!apiData.gc_40.empty())
            {
                height = apiData.gc_40.front();
            }

            if(!apiData.gc_41.empty())
            {
                width = apiData.gc_41.front();
            }

            if(!apiData.gc_50.empty())
            {
                angle = apiData.gc_50.front();
            }

            addText(apiData.gc_10.front(),
                    height,
                    width,
                    angle,
                    apiData.gc_73.empty() ? 0.0 : apiData.gc_73.front(),
                    apiData.gc_72.empty() ? 0.0 : apiData.gc_72.front(),
                    apiData.gc_71.empty() ? 0.0 : apiData.gc_71.front(),
                    apiData.text.isEmpty() ? "" : apiData.text,
                    apiData.style.isEmpty() ? "STANDARD" : apiData.style,
                    apiData.pen);
        }

        else if (apiData.etype == "SOLID"
                 && !apiData.gc_10.empty()
                 && !apiData.gc_11.empty()
                 && !apiData.gc_12.empty())
        {
            if(!apiData.gc_13.empty())
            {
                RS_SolidData data(apiData.gc_10.front(),
                                  apiData.gc_11.front(),
                                  apiData.gc_12.front(),
                                  apiData.gc_13.front());

                addSolid(data, apiData.pen);
            }
            else
            {
                RS_SolidData data(apiData.gc_10.front(),
                                  apiData.gc_11.front(),
                                  apiData.gc_12.front());

                addSolid(data, apiData.pen);
            }
        }

        else if (apiData.etype == "SPLINE" && !apiData.gc_10.empty())
        {
            bool closed = false;
            bool enoughPoints = false;
            int splineDegree = 0;
            int count = 0;

            std::vector<RS_Vector> controlPoints;

            if(!apiData.gc_70.empty())
            {
                if (apiData.gc_70.front() & 1)
                {
                    closed = true;
                }
            }

            if(!apiData.gc_71.empty())
            {
                splineDegree = apiData.gc_71.front();
            }

            for(auto const& pt: apiData.gc_10)
            {
                controlPoints.push_back({ pt });
            }

            count = controlPoints.size();

            switch (splineDegree) {
                case 1: {
                    enoughPoints = count > 2;
                    break;
                }
                case 2: {
                    enoughPoints = count > 3;
                    break;
                }
                case 3: {
                    enoughPoints = count > 3;
                    break;
                }
                default:
                    enoughPoints = true;
            }

            if(count > 2 && enoughPoints)
            {
                RS_SplineData data = RS_SplineData(splineDegree, closed);
                addSpline(data, controlPoints, apiData.pen);
            }
            else
            {
                return false;
            }
        }

        else if (apiData.etype == "DIMENSION"
                 && !apiData.gc_100.empty()
                 && !apiData.gc_10.empty())
        {
            qDebug() << "[DIMENSION] - start";

            RS_Vector middleOfText;
            RS_MTextData::MTextLineSpacingStyle lineSpacingStyle = RS_MTextData::AtLeast;
            double lineSpacingFactor = 1.0;
            double angle = 0.0;
            QString text = "";
            QString style = "Standard";

            if(!apiData.text.isEmpty())
            {
                text = apiData.text.front();
            }

            if(!apiData.block.size())
            {
                style = apiData.block;
            }

            if(!apiData.gc_11.empty())
            {
                middleOfText = apiData.gc_11.front();
            }

            if(!apiData.gc_41.empty())
            {
                lineSpacingFactor = apiData.gc_41.front();
            }

            if(!apiData.gc_53.empty())
            {
                angle = apiData.gc_53.front();
            }

            if(!apiData.gc_72.empty())
            {
                if (apiData.gc_72.front() == 2)
                {
                    lineSpacingStyle = RS_MTextData::Exact;
                }
            }

            auto* dimStyle = new LC_DimStyle();

            RS_DimensionData data(apiData.gc_10.front(),
                                  middleOfText,
                                  RS_MTextData::VAMiddle,
                                  RS_MTextData::HACenter,
                                  lineSpacingStyle,
                                  lineSpacingFactor,
                                  text,
                                  style,
                                  angle,
                                  0.0 /* double hdir */,
                                  true /* bool autoTextLocation */,
                                  dimStyle   /* LC_DimStyle* dimStyleOverride */,
                                  false /* flipArr1 */,
                                  false /* flipArr2 */
                                );


            if(apiData.gc_100.front().toUpper() == "ACDBROTATEDDIMENSION") // posible eSubtype = 3!!
            {
                qDebug() << "[DIMENSION] - exit";
                if (apiData.gc_13.empty()
                        || apiData.gc_13.empty()
                        || apiData.gc_14.empty())
                {
                    qDebug() << "[DIMENSION] - exit";
                    return false;
                }

                double angle = 0.0, oblique = 0.0;

                if (!apiData.gc_50.empty())
                {
                    angle = apiData.gc_50.front();
                }

                if (!apiData.gc_52.empty())
                {
                    oblique = apiData.gc_52.front();
                }

                RS_DimLinearData edata(apiData.gc_13.front(),
                                       apiData.gc_14.front(),
                                       angle,
                                       oblique);

                addDimLinear(data, edata, apiData.pen);
            }

            qDebug() << "[DIMENSION] - start2";
            if(apiData.gc_100.front().toUpper() == "ACDBALIGNEDDIMENSION")
            {
                if (apiData.gc_13.empty()
                        || apiData.gc_13.empty()
                        || apiData.gc_14.empty())
                {
                    qDebug() << "[DIMENSION] - exit";
                    return false;
                }

                RS_DimAlignedData edata(apiData.gc_13.front(),
                                        apiData.gc_14.front());

                addDimAligned(data, edata, apiData.pen);
            }

            else if(apiData.gc_100.front().toUpper() == "ACDB3POINTANGULARDIMENSION")
            {
                qDebug() << "[DIMENSION] - exit";
                if (apiData.gc_13.empty()
                        || apiData.gc_14.empty()
                        || apiData.gc_15.empty()
                        || apiData.gc_16.empty())
                {
                    qDebug() << "[DIMENSION] - exit";
                    return false;
                }

                RS_DimAngularData edata(apiData.gc_13.front(),
                                        apiData.gc_14.front(),
                                        apiData.gc_15.front(),
                                        apiData.gc_16.front());

                addDimAngular(data, edata, apiData.pen);
            }

            else if(apiData.gc_100.front().toUpper() == "ACDBRADIALDIMENSION")
            {
                qDebug() << "[DIMENSION] - ACDBRADIALDIMENSION";
                if (apiData.gc_40.empty() || apiData.gc_15.empty())
                {
                    qDebug() << "[DIMENSION] - exit";
                    return false;
                }

                RS_DimRadialData edata(apiData.gc_15.front(),
                                       apiData.gc_40.front());

                addDimRadial(data, edata, apiData.pen);
            }

            else if(apiData.gc_100.front().toUpper() == "ACDBDIAMETRICDIMENSION")
            {
                qDebug() << "[DIMENSION] - exit";
                if (apiData.gc_40.empty() || apiData.gc_15.empty())
                {
                    qDebug() << "[DIMENSION] - exit";
                    return false;
                }

                RS_DimDiametricData edata(apiData.gc_15.front(),
                                          apiData.gc_40.front());

                addDimDiametric(data, edata, apiData.pen);
            }
        }

        else if (apiData.etype == "HATCH" && !apiData.gc_10.empty())
        {
            return false;
        }

        else if (apiData.etype == "INSERT" && !apiData.gc_10.empty() && !apiData.block.isEmpty())
        {
            RS_Vector scaleFactor(1.0, 1.0, 1.0);
            RS_Vector spacing(0.0, 0.0);
            double angle = 0.0;
            double cols = 1.0;
            double rows = 1.0;

            if (!apiData.gc_41.empty())
            {
                scaleFactor.x = apiData.gc_41.front();
            }

            if (!apiData.gc_42.empty())
            {
                scaleFactor.y = apiData.gc_42.front();
            }

            if (!apiData.gc_43.empty())
            {
                scaleFactor.z = apiData.gc_43.front();
            }

            if (!apiData.gc_44.empty())
            {
                spacing.x = apiData.gc_44.front();
            }

            if (!apiData.gc_45.empty())
            {
                spacing.y =  apiData.gc_45.front();
            }

            if (!apiData.gc_50.empty())
            {
                angle = apiData.gc_50.front();
            }

            if (!apiData.gc_70.empty())
            {
                cols = apiData.gc_70.front();
            }

            if (!apiData.gc_71.empty())
            {
                rows = apiData.gc_71.front();
            }

            RS_InsertData data(apiData.block,
                               apiData.gc_10.front(),
                                scaleFactor,
                                angle,
                                cols,
                                rows,
                                spacing,
                                nullptr,
                                RS2::Update);

            addBlock(data, apiData.pen);
        }

        else if (apiData.etype == "IMAGE"
                 && !apiData.gc_10.empty()
                 && !apiData.gc_11.empty()
                 && !apiData.gc_12.empty()
                 && !apiData.gc_13.empty())
        {
#if 0
            int brightness = 50;
            int contrast = 50;
            int fade = 0;
            //QString file;

            if (!apiData.gc_281.empty())
            {
                brightness = apiData.gc_281.front();
            }

            if (!apiData.gc_282.empty())
            {
                contrast = apiData.gc_282.front();
            }

            if (!apiData.gc_283.empty())
            {
                fade = apiData.gc_283.front();
            }

            RS_ImageData edata(handle,
                               apiData.gc_10.front(),
                               apiData.gc_11.front(),
                               apiData.gc_12.front(),
                               apiData.gc_13.front()),
                         file,
                         brightness,
                         contrast,
                         fade);
#endif
            return false;
        }

        else
        {
            return false;
        }

        RS_GraphicView* v = getGraphicView();
        if (v) {
            v->redraw();
        }

        return true;
    }

    return false;
}

bool RS_ScriptingApi::entmod(RS_Entity *entity, const RS_ScriptingApiData &apiData)
{
    switch (entity->rtti())
    {
        case RS2::EntityPoint:
            {
                if (!apiData.gc_10.empty())
                {
                    RS_Point* p = (RS_Point*)entity;
                    p->setPos(apiData.gc_10.front());
                }
            }
            break;
        case RS2::EntityLine:
        {
            RS_Line* l = (RS_Line*)entity;
            if (!apiData.gc_10.empty())
            {
                l->setStartpoint(apiData.gc_10.front());
            }

            if (!apiData.gc_11.empty())
            {
                l->setEndpoint(apiData.gc_11.front());
            }
        }
            break;
        case RS2::EntityArc:
        {
            RS_Arc* a = (RS_Arc*)entity;

            if (!apiData.gc_10.empty())
            {
                a->setCenter(apiData.gc_10.front());
            }

            if (!apiData.gc_40.empty())
            {
                a->setRadius(apiData.gc_40.front());
            }

            if (!apiData.gc_50.empty())
            {
                a->setAngle1(apiData.gc_50.front());
            }

            if (!apiData.gc_51.empty())
            {
                a->setAngle2(apiData.gc_51.front());
            }
        }
            break;
        case RS2::EntityCircle:
        {
            RS_Circle* c = (RS_Circle*)entity;
            if (!apiData.gc_10.empty())
            {
                c->setCenter(apiData.gc_10.front());
            }

            if (!apiData.gc_40.empty())
            {
                c->setRadius(apiData.gc_40.front());
            }
        }
            break;
        case RS2::EntityEllipse:
        {
            RS_Ellipse* ellipse=static_cast<RS_Ellipse*>(entity);

            if (!apiData.gc_10.empty())
            {
                ellipse->setCenter(apiData.gc_10.front());
            }

            if (!apiData.gc_11.empty())
            {
                ellipse->setMajorP(apiData.gc_11.front());
            }

            if (!apiData.gc_40.empty())
            {
                ellipse->setRatio(apiData.gc_40.front());
            }
        }
            break;
        case RS2::EntityInsert:
        {
            RS_Insert* i = (RS_Insert*)entity;

            if (!apiData.gc_10.empty())
            {
                i->setInsertionPoint(apiData.gc_10.front());
            }

            if (!apiData.gc_41.empty())
            {
                RS_Vector scale(i->getScale());
                scale.x = apiData.gc_41.front();
                i->setScale(scale);
            }

            if (!apiData.gc_42.empty())
            {
                RS_Vector scale(i->getScale());
                scale.y = apiData.gc_42.front();
                i->setScale(scale);
            }

            if (!apiData.gc_43.empty())
            {
                RS_Vector scale(i->getScale());
                scale.z = apiData.gc_43.front();
                i->setScale(scale);
            }

            if (!apiData.gc_44.empty())
            {
                RS_Vector spacing(i->getSpacing());
                spacing.x = apiData.gc_44.front();
                i->setSpacing(spacing);
            }

            if (!apiData.gc_45.empty())
            {
                RS_Vector spacing(i->getSpacing());
                spacing.y = apiData.gc_45.front();
                i->setSpacing(spacing);
            }

            if (!apiData.gc_50.empty())
            {
                i->setAngle(apiData.gc_50.front());
            }

            if (!apiData.gc_70.empty())
            {
                i->setCols(apiData.gc_70.front());
            }

            if (!apiData.gc_71.empty())
            {
                i->setRows(apiData.gc_71.front());
            }
        }
            break;
        case RS2::EntityMText:
        {
            RS_MText* mt = (RS_MText*)entity;

            if (!apiData.gc_10.empty())
            {
                mt->moveRef(mt->getInsertionPoint(), apiData.gc_10.front());
            }

            if (!apiData.text.isEmpty())
            {
                mt->setText(apiData.text.front());
            }

            if (!apiData.style.isEmpty())
            {
                mt->setStyle(apiData.style.front());
            }
        }
            break;
        case RS2::EntityText:
        {
            RS_Text* t = (RS_Text*)entity;

            if (!apiData.gc_10.empty())
            {
                t->moveRef(t->getInsertionPoint(), apiData.gc_10.front());
            }

            if (!apiData.text.isEmpty())
            {
                t->setText(apiData.text.front());
            }

            if (!apiData.style.isEmpty())
            {
                t->setStyle(apiData.style.front());
            }
        }
        entity->setPen(apiData.pen);
            break;
        default:
        {
            return false;
        }
        return true;
    }
    return false;
}

bool RS_ScriptingApi::getTile(const char *key, std::string &result)
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key)
        {
            switch (tile->value().id)
            {
                case EDIT_BOX:
                {
                    const lclEdit* e = static_cast<const lclEdit*>(tile);
                    result = qUtf8Printable(e->edit()->text());
                }
                    break;
                case LIST_BOX:
                {
                    const lclListBox* lb = static_cast<const lclListBox*>(tile);
                    result = std::to_string(lb->list()->currentRow());
                }
                    break;
                case BUTTON:
                {
                    const lclButton* b = static_cast<const lclButton*>(tile);
                    result = qUtf8Printable(b->button()->text());
                }
                    break;
                case RADIO_BUTTON:
                {
                    const lclButton* rb = static_cast<const lclButton*>(tile);
                    result = qUtf8Printable(rb->button()->text());
                }
                    break;
                case TEXT:
                {
                    const lclLabel* l = static_cast<const lclLabel*>(tile);
                    result = qUtf8Printable(l->label()->text());
                }
                    break;
                case POPUP_LIST:
                {
                    const lclPopupList* pl = static_cast<const lclPopupList*>(tile);
                    result = std::to_string(pl->list()->currentIndex());
                }
                    break;
                default:
                {
                    return false;
                }
                return true;
            }
            break;
        }
    }
    return false;
}

bool RS_ScriptingApi::getAttr(const char *key, const char *attr, std::string &result)
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key)
        {
            switch (getDclAttributeId(attr)) {
            case ACTION:
                result = tile->value().action;
                break;
            case ALIGNMENT:
                result = std::to_string((int)tile->value().alignment);
                break;
            case ALLOW_ACCEPT:
                result = boolToString(tile->value().allow_accept);
                break;
            case ASPECT_RATIO:
                result = std::to_string(tile->value().aspect_ratio);
                break;
            case BIG_INCREMENT:
                result = std::to_string(tile->value().big_increment);
                break;
            case CHILDREN_ALIGNMENT:
            {
                for (int i = 0; i < MAX_DCL_POS; i++)
                {
                    if (tile->value().children_alignment == dclPosition[i].pos)
                    {
                        result = dclPosition[i].name;
                        break;
                    }
                }
            }
                break;
            case CHILDREN_FIXED_HEIGHT:
                result = boolToString(tile->value().children_fixed_height);
                break;
            case CHILDREN_FIXED_WIDTH:
                result = boolToString(tile->value().children_fixed_width);
                break;
            case COLOR:
            {
                for (int i = 0; i < MAX_DCL_COLOR; i++)
                {
                    if (tile->value().color == dclColor[i].color)
                    {
                        result = dclColor[i].name;
                        break;
                    }
                }
                result = std::to_string((int)tile->value().color);
                break;
            }
            case EDIT_LIMIT:
                result = std::to_string(tile->value().edit_limit);
                break;
            case EDIT_WIDTH:
                result = std::to_string(tile->value().edit_width);
                break;
            case FIXED_HEIGHT:
                result = boolToString(tile->value().fixed_height);
                break;
            case FIXED_WIDTH:
                result = boolToString(tile->value().fixed_width);
                break;
            case FIXED_WIDTH_FONT:
                result = boolToString(tile->value().fixed_width_font);
                break;
            case HEIGHT:
                result = std::to_string(tile->value().height);
                break;
            case INITIAL_FOCUS:
                result = tile->value().initial_focus;
                break;
            case IS_BOLD:
                result = boolToString(tile->value().is_bold);
                break;
            case IS_CANCEL:
                result = boolToString(tile->value().is_cancel);
                break;
            case IS_DEFAULT:
                result = boolToString(tile->value().is_default);
                break;
            case IS_ENABLED:
                result = boolToString(tile->value().is_enabled);
                break;
            case IS_TAB_STOP:
                result = boolToString(tile->value().is_tab_stop);
                break;
            case KEY:
                result = tile->value().key;
                break;
            case LABEL:
                result = tile->value().label;
                break;
            case LAYOUT:
            {
                for (int i = 0; i < MAX_DCL_POS; i++)
                {
                    if (tile->value().layout == dclPosition[i].pos)
                    {
                        result = dclPosition[i].name;
                        break;
                    }
                }
            }
                break;
            case LIST:
                result = tile->value().list;
                break;
            case MAX_VALUE:
                result = std::to_string(tile->value().max_value);
                break;
            case MIN_VALUE:
                result = std::to_string(tile->value().min_value);
                break;
            case MNEMONIC:
                result = tile->value().mnemonic;
                break;
            case MULTIPLE_SELECT:
                result = boolToString(tile->value().multiple_select);
                break;
            case PASSWORD_CHAR:
                result = tile->value().password_char;
                break;
            case SMALL_INCREMENT:
                result = std::to_string(tile->value().small_increment);
                break;
            case TABS:
                result = tile->value().tabs;
                break;
            case TAB_TRUNCATE:
                result = boolToString(tile->value().tab_truncate);
                break;
            case VALUE:
                result = tile->value().value;
                break;
            case WIDTH:
                result = std::to_string(tile->value().width);
                break;
            default:
                return false;
            }
            return true;
        }
    }
    return false;
}

bool RS_ScriptingApi::modeTile(const char *key, int mode)
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }

        if (noQuotes(tile->value().key) == key)
        {
            switch (tile->value().id)
            {
            case EDIT_BOX:
            {
                qDebug() << "modeTile EDIT_BOX";
                const lclEdit* e = static_cast<const lclEdit*>(tile);
                switch (mode)
                {
                    case 0:
                    {
                        e->edit()->setEnabled(true);
                    }
                        break;
                    case 1:
                    {
                        e->edit()->setEnabled(false);
                    }
                        break;
                    case 2:
                    {
                        e->edit()->setFocus();
                    }
                        break;
                    case 3:
                    {
                        e->edit()->selectAll();
                    }
                        break;
                    default:
                        return false;
                }
            }
            break;
            case LIST_BOX:
            {
                qDebug() << "modeTile LIST_BOX";
                const lclListBox* e = static_cast<const lclListBox*>(tile);
                switch (mode)
                {
                    case 0:
                    {
                        e->list()->setEnabled(true);
                    }
                        break;
                    case 1:
                    {
                        e->list()->setEnabled(false);
                    }
                        break;
                    case 2:
                    {
                        e->list()->setFocus();
                    }
                        break;
                    case 3:
                    {
                        e->list()->selectAll();
                    }
                        break;
                    default:
                        return false;
                }
            }
            break;
            case BUTTON:
            {
                qDebug() << "modeTile BUTTON";
                const lclButton* b = static_cast<const lclButton*>(tile);
                switch (mode)
                {
                    case 0:
                    {
                        b->button()->setEnabled(true);
                    }
                        break;
                    case 1:
                    {
                        b->button()->setEnabled(false);
                    }
                        break;
                    case 2:
                    {
                        b->button()->setFocus();
                    }
                        break;
                    default:
                        return false;
                }
            }
            break;
            case RADIO_BUTTON:
            {
                qDebug() << "modeTile RADIO_BUTTON";
                const lclButton* rb = static_cast<const lclButton*>(tile);
                switch (mode)
                {
                    case 0:
                    {
                        rb->button()->setEnabled(true);
                    }
                        break;
                    case 1:
                    {
                        rb->button()->setEnabled(false);
                    }
                        break;
                    case 2:
                    {
                        rb->button()->setFocus();
                    }
                        break;
                    default:
                        return false;
                }
            }
            break;
            case TEXT:
            {
                qDebug() << "modeTile TEXT";
                const lclLabel* l = static_cast<const lclLabel*>(tile);
                switch (mode)
                {
                    case 0:
                    {
                        l->label()->setEnabled(true);
                    }
                        break;
                    case 1:
                    {
                        l->label()->setEnabled(false);
                    }
                        break;
                    case 2:
                    {
                        l->label()->setFocus();
                    }
                        break;
                    default:
                        return false;
                }
            }
            break;
            case POPUP_LIST:
            {
                qDebug() << "modeTile POPUP_LIST";
                const lclPopupList* pl = static_cast<const lclPopupList*>(tile);
                switch (mode)
                {
                    case 0:
                    {
                        pl->list()->setEnabled(true);
                    }
                       break;
                    case 1:
                    {
                        pl->list()->setEnabled(false);
                    }
                        break;
                    case 2:
                    {
                        pl->list()->setFocus();
                    }
                        break;
                    default:
                        return false;
                }
            }
                break;
            default:
                return false;
            }
            return true;
        }
    }
    return false;
}

bool RS_ScriptingApi::startList(const char *key, int operation, int index)
{
    const lclInteger *dialogId  = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key)
        {
            if (operation == -1)
            {
                dclEnv->set("start_list_operation", lcl::integer(2));
            }
            else
            {
                dclEnv->set("start_list_operation", lcl::integer(operation));
            }

            if (index == -1)
            {
                dclEnv->set("start_list_index", lcl::nilValue());
            }
            else
            {
                dclEnv->set("start_list_index", lcl::integer(index));
            }

            dclEnv->set("start_list_key", lcl::string(key));
            return true;
        }
    }
    return false;
}

bool RS_ScriptingApi::addList(const char *val, std::string &result)
{
    const lclString  *key       = VALUE_CAST(lclString, dclEnv->get("start_list_key"));
    const lclInteger *operation = VALUE_CAST(lclInteger, dclEnv->get("start_list_operation"));
    const lclInteger *dialogId  = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    if (key)
    {
        qDebug() << "addList key: " << key->value().c_str();
        for (auto & tile : dclTiles)
        {
            if(tile->value().dialog_Id != dialogId->value())
            {
                continue;
            }
            if (noQuotes(tile->value().key) == key->value())
            {
                if (tile->value().id == LIST_BOX)
                {
                    qDebug() <<  "addList got LIST_BOX";
                    const lclListBox *lb = static_cast<const lclListBox*>(tile);
                    if(operation->value() == 1)
                    {
                        if(dclEnv->get("start_list_index").ptr()->print(true).compare("nil") == 0)
                        {
                            return false;
                        }
                        const lclInteger *index = VALUE_CAST(lclInteger, dclEnv->get("start_list_index"));
                        QListWidgetItem *item = lb->list()->item(index->value());
                        item->setText(val);
                        result = std::string(val);
                        return true;
                    }
                    if(operation->value() == 3)
                    {
                        lb->list()->clear();
                    }
                    if(operation->value() == 2 ||
                        operation->value() == 3)
                    {
                        lb->list()->addItem(new QListWidgetItem(val, lb->list()));
                        if(noQuotes(tile->value().value) != "")
                        {
                            bool ok;
                            int i = QString::fromStdString(noQuotes(tile->value().value)).toInt(&ok);

                            if (ok && lb->list()->count() == i)
                            {
                                lb->list()->setCurrentRow(i-1);
                            }
                        }
                        result = std::string(val);
                        return true;
                    }
                }
                if (tile->value().id == POPUP_LIST)
                {
                    qDebug() <<  "addList got POPUP_LIST";
                    const lclPopupList *pl = static_cast<const lclPopupList*>(tile);
                    if(operation->value() == 1)
                    {
                        if(dclEnv->get("start_list_index").ptr()->print(true).compare("nil") == 0)
                        {
                            return false;
                        }
                        const lclInteger *index = VALUE_CAST(lclInteger, dclEnv->get("start_list_index"));
                        pl->list()->setItemText(index->value(), val);
                        result = std::string(val);
                        return true;
                    }
                    if(operation->value() == 3)
                    {
                        pl->list()->clear();
                    }
                    if(operation->value() == 2 ||
                        operation->value() == 3)
                    {
                        pl->list()->addItem(val);
                        if(noQuotes(tile->value().value) != "")
                        {
                            bool ok;
                            int i = QString::fromStdString(noQuotes(tile->value().value)).toInt(&ok);

                            if (ok && pl->list()->count() == i)
                            {
                                pl->list()->setCurrentIndex(i-1);
                            }
                        }
                        result = std::string(val);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void RS_ScriptingApi::endList()
{
    dclEnv->set("start_list_operation", lcl::nilValue());
    dclEnv->set("start_list_index", lcl::nilValue());
    dclEnv->set("start_list_key", lcl::nilValue());
}

bool RS_ScriptingApi::dimxTile(const char *key, int &x)
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key)
        {
            switch (tile->value().id)
            {
                case IMAGE:
                case IMAGE_BUTTON:
                {
                    x = int(tile->value().width);
                    return true;
                }
                    break;
                default:
                    return false;
            }
        }
    }
    return false;
}

bool RS_ScriptingApi::dimyTile(const char *key, int &y)
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key)
        {
            switch (tile->value().id)
            {
                case IMAGE:
                case IMAGE_BUTTON:
                {
                    y = int(tile->value().height);
                    return true;
                }
                    break;
                default:
                    return false;
            }
        }
    }
    return false;
}

bool RS_ScriptingApi::fillImage(int x, int y, int width, int height, int color)
{
    const lclString *key = VALUE_CAST(lclString, dclEnv->get("start_image_key"));
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key->value())
        {
            switch (tile->value().id)
            {
                case IMAGE:
                {
                    const lclImage* img = static_cast<const lclImage*>(tile);
                    img->image()->addRect(x, y, width, height, color);
                }
                    break;
                case IMAGE_BUTTON:
                {
                    const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                    img->button()->addRect(x, y, width, height, color);
                }
                    break;
                default:
                    return false;
            }
            return true;
        }
    }
    return false;
}

bool RS_ScriptingApi::vectorImage(int x1, int y1, int x2, int y2, int color)
{
    const lclString *key = VALUE_CAST(lclString, dclEnv->get("start_image_key"));
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key->value())
        {
            switch (tile->value().id)
            {
                case IMAGE:
                {
                    const lclImage* img = static_cast<const lclImage*>(tile);
                    img->image()->addLine(x1, y1, x2, y2, color);
                }
                break;
                case IMAGE_BUTTON:
                {
                    const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                    img->button()->addLine(x1, y1, x2, y2, color);
                }
                    break;
                default:
                    return false;
            }
            return true;
        }
    }
    return false;
}

bool RS_ScriptingApi::pixImage(int x, int y, int width, int height, const char *path)
{
    const lclString *key = VALUE_CAST(lclString, dclEnv->get("start_image_key"));
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key->value())
        {
            switch (tile->value().id)
            {
                case IMAGE:
                {
                    const lclImage* img = static_cast<const lclImage*>(tile);
                    img->image()->addPicture(x, y, width, height, tile->value().aspect_ratio, path);
                }
                    break;
                case IMAGE_BUTTON:
                {
                    const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                    img->button()->addPicture(x, y, width, height, tile->value().aspect_ratio, path);
                }
                    break;
                default:
                    return false;
            }
            return true;
        }
    }
    return false;
}

bool RS_ScriptingApi::slideImage(int x, int y, int width, int height, const char *path)
{
    const lclString *key = VALUE_CAST(lclString, dclEnv->get("start_image_key"));
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key->value())
        {
            switch (tile->value().id)
            {
                case IMAGE:
                {
                    const lclImage* img = static_cast<const lclImage*>(tile);
                    img->image()->addSlide(x, y, width, height, tile->value().aspect_ratio, path);
                }
                    break;
                case IMAGE_BUTTON:
                {
                    const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                    img->button()->addSlide(x, y, width, height, tile->value().aspect_ratio, path);
                }
                    break;
                default:
                    return false;
            }
            return true;
        }
    }
    return false;
}

bool RS_ScriptingApi::textImage(int x, int y, int width, int height, const char *text, int color)
{
    const lclString *key = VALUE_CAST(lclString, dclEnv->get("start_image_key"));
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key->value())
        {
            switch (tile->value().id)
            {
                case IMAGE:
                {
                    const lclImage* img = static_cast<const lclImage*>(tile);
                    img->image()->addText(x, y, width, height, text, color);
                }
                    break;
                case IMAGE_BUTTON:
                {
                    const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                    img->button()->addText(x, y, width, height, text, color);
                }
                    break;
                default:
                    return false;
            }
            return true;
        }
    }
    return false;
}

void RS_ScriptingApi::endImage()
{
    const lclString *key = VALUE_CAST(lclString, dclEnv->get("start_image_key"));
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key->value())
        {
            switch (tile->value().id)
            {
                case IMAGE:
                {
                    const lclImage* img = static_cast<const lclImage*>(tile);
                    img->image()->repaint();
                }
                    break;
                case IMAGE_BUTTON:
                {
                    const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                    img->button()->repaint();
                }
                    break;
                default: {}
                    break;
            }
        }
    }
}

bool RS_ScriptingApi::startImage(const char *key)
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key)
        {
            dclEnv->set("start_image_key", lcl::string(key));
            return true;
        }
    }

    return false;
}

bool RS_ScriptingApi::setvar(const char *var, double v1, double v2, const char *str_value)
{
    RS_Graphic *graphic = RS_SCRIPTINGAPI->getGraphic();

    if(!graphic)
    {
        return false;
    }

    const QString sysvar = var;
    int int_value = static_cast<int>(v1);

    if (sysvar.toUpper() == "$ANGBASE")
    {
        graphic->addVariable("$ANGBASE", v1, 50);
        return true;
    }

    else if (sysvar.toUpper() == "ANGDIR")
    {
        if (int_value == 1 || int_value == 0)
        {
            graphic->addVariable("$ANGDIR", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "AUNITS")
    {
        if (int_value > -1 && int_value < 5)
        {
            graphic->addVariable("$AUNITS", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "AUPREC")
    {
        if (int_value > -1 && int_value < 9)
        {
            graphic->addVariable("$AUPREC", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "CLAYER")
    {
        graphic->addVariable("$CLAYER", str_value, 8);
        return true;
    }

    else if (sysvar.toUpper() == "DIMSTYLE")
    {
        graphic->addVariable("$DIMSTYLE", str_value, 2);
        return true;
    }

    else if (sysvar.toUpper() == "DIMSCALE")
    {
        graphic->addVariable("$DIMSCALE", v1, 40);
        return true;
    }

    else if (sysvar.toUpper() == "DIMASZ")
    {
        graphic->addVariable("$DIMASZ", v1, 40);
        return true;
    }

    else if (sysvar.toUpper() == "DIMEXO")
    {
        graphic->addVariable("$DIMEXO", v1, 40);
        return true;
    }

    else if (sysvar.toUpper() == "DIMEXE")
    {
        graphic->addVariable("$DIMEXE", v1, 40);
        return true;
    }

    else if (sysvar.toUpper() == "DIMFXL")
    {
        graphic->addVariable("$DIMFXL", v1, 40);
        return true;
    }

    else if (sysvar.toUpper() == "DIMTXT")
    {
        graphic->addVariable("$DIMTXT", v1, 40);
        return true;
    }

    else if (sysvar.toUpper() == "DIMTSZ")
    {
        graphic->addVariable("$DIMTSZ", v1, 40);
        return true;
    }

    else if (sysvar.toUpper() == "DIMLFAC")
    {
        graphic->addVariable("$DIMLFAC", v1, 40);
        return true;
    }

    else if (sysvar.toUpper() == "DIMGAP")
    {
        graphic->addVariable("$DIMGAP", v1, 40);
        return true;
    }

    else if (sysvar.toUpper() == "DIMTIH")
    {
        if (int_value == 1 || int_value == 0)
        {
            graphic->addVariable("$DIMTIH", int_value, 70);
            return true;
        }
    }

    else if (sysvar.toUpper() == "DIMZIN")
    {
        if (int_value > -1 && int_value < 13)
        {
            graphic->addVariable("$DIMZIN", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "DIMAZIN")
    {
        if (int_value > -1 && int_value < 4)
        {
            graphic->addVariable("$DIMAZIN", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "DIMCLRD")
    {
        if (int_value > -1 && int_value < 257)
        {
            graphic->addVariable("$DIMCLRD", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "DIMCLRE")
    {
        if (int_value > -1 && int_value < 257)
        {
            graphic->addVariable("$DIMCLRE", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "DIMCLRT")
    {
        graphic->addVariable("$DIMCLRT", int_value, 70);
        return true;
    }

    else if (sysvar.toUpper() == "DIMADEC")
    {
        if (int_value > -2 && int_value < 9)
        {
            graphic->addVariable("$DIMADEC", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "DIMDEC")
    {
        if (int_value > -2 && int_value < 5)
        {
            graphic->addVariable("$DIMDEC", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "DIMAUNIT")
    {
        if (int_value > -2 && int_value < 4)
        {
            graphic->addVariable("$DIMAUNIT", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "DIMLUNIT")
    {
        if (int_value > -1 && int_value < 7)
        {
            graphic->addVariable("$DIMLUNIT", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "DIMDSEP")
    {
        if (int_value == 1 || int_value == 0)
        {
            graphic->addVariable("$DIMLUNIT", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "DIMFXLON")
    {
        if (int_value == 1 || int_value == 0)
        {
            graphic->addVariable("$DIMFXLON", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "DIMTXSTY")
    {
        graphic->addVariable("$DIMTXSTY", str_value, 2);
        return true;
    }

    else if (sysvar.toUpper() == "DIMLWD")
    {
        switch (int_value)
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
            graphic->addVariable("$DIMLWD", int_value, 70);
            break;
        }

        default:
        {
            return false;
        }
        }
        return true;
    }

    else if (sysvar.toUpper() == "DIMLWE")
    {
        switch (int_value)
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
            graphic->addVariable("$DIMLWE", int_value, 70);
            break;
        }

        default:
        {
            return false;
        }
        }
        return true;
    }

    else if (sysvar.toUpper() == "DWGCODEPAGE")
    {
        graphic->addVariable("$DWGCODEPAGE", str_value, 7);
        return true;
    }

    else if (sysvar.toUpper() == "GRIDMODE")
    {
        if (int_value == 1 || int_value == 0)
        {
            graphic->addVariable("$GRIDMODE", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "GRIDUNIT")
    {

        graphic->addVariable("GRIDUNIT", RS_Vector(v1, v2), 10);
        return true;
    }

    else if (sysvar.toUpper() == "INPUTHISTORYMODE")
    {
        switch (int_value)
        {
        case 0:
        case 1:
        case 2:
        case 4:
        case 8:
        {
            LC_SET("INPUTHISTORYMODE", int_value);
            break;
        }
        default:
        {
            return false;
        }
        }

        return true;
    }

    else if (sysvar.toUpper() == "INSUNITS")
    {
        if (int_value > -1 && int_value < 22)
        {
            graphic->addVariable("$INSUNITS", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "LUNITS")
    {
        if (int_value > -1 && int_value < 6)
        {
            graphic->addVariable("$LUNITS", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "LUPREC")
    {
        if (int_value > -1 && int_value < 8)
        {
            graphic->addVariable("$LUPREC", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "PDMODE")
    {
        switch (int_value)
        {
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
            graphic->addVariable("$PDMODE", int_value, DXF_FORMAT_GC_PDMode);
            break;
        }
        default:
        {
            return false;
        }
        }

        return true;
    }

    else if (sysvar.toUpper() == "PDSIZE")
    {
        graphic->addVariable("$PDSIZE", v1, DXF_FORMAT_GC_PDSize);
        return true;
    }

    else if (sysvar.toUpper() == "PSVPSCALE")
    {
        graphic->addVariable("PSVPSCALE", v1, 40);
        return true;
    }

    else if (sysvar.toUpper() == "SNAPSTYL")
    {
        if (int_value == 1 || int_value == 0)
        {
            graphic->addVariable("$SNAPSTYLE", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "SNAPISOPAIR")
    {
        if (int_value > -1 && int_value < 3)
        {
            graphic->addVariable("$SNAPISOPAIR", int_value, 70);
        }
        return true;
    }

    else if (sysvar.toUpper() == "TEXTSTYLE")
    {
        graphic->addVariable("$TEXTSTYLE", str_value, 7);
        return true;
    }

    else
    {}

    return false;
}

RS_ScriptingApi::SysVarResult RS_ScriptingApi::getvar(const char *var, int &int_result, double &v1_result, double &v2_result, double &v3_result, std::string &str_result)
{

    RS_Graphic *graphic = getGraphic();

    if(!graphic)
    {
        return RS_ScriptingApi::None;
    }

    const QString sysvar = var;

    if (sysvar.toUpper() == "ACADVER")
    {
        QString acadver = graphic->getVariableString("$ACADVER", "");
        static const QRegularExpression regexp(QStringLiteral("[a-zA-Z]"));
        acadver.replace(regexp, "");
        str_result = acadver.toStdString();
        return RS_ScriptingApi::String;
    }

    else if (sysvar.toUpper() == "ANGBASE")
    {
        v1_result = graphic->getVariableDouble("$ANGBASE", 0.0);
        return RS_ScriptingApi::Double;
    }

    else if (sysvar.toUpper() == "ANGDIR")
    {
        int_result = graphic->getVariableInt("$ANGDIR", 0);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "AUNITS")
    {
        int_result = graphic->getVariableInt("$AUNITS", 0);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "AUPREC")
    {
        int_result = graphic->getVariableInt("$AUPREC", 4);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "CLAYER")
    {
        str_result = graphic->getVariableString("$CLAYER", "0").toStdString();
        return RS_ScriptingApi::String;
    }

    else if (sysvar.toUpper() == "DIMSTYLE")
    {
        str_result = graphic->getVariableString("$DIMSTYLE", "Standard").toStdString();
        return RS_ScriptingApi::String;
    }

    else if (sysvar.toUpper() == "DIMSCALE")
    {
        v1_result = graphic->getVariableDouble("$DIMSCALE", 1.0);
        return RS_ScriptingApi::Double;
    }

    else if (sysvar.toUpper() == "DIMASZ")
    {
        v1_result = graphic->getVariableDouble("$DIMASZ", 2.5);
        return RS_ScriptingApi::Double;
    }

    else if (sysvar.toUpper() == "DIMEXO")
    {
        v1_result = graphic->getVariableDouble("$DIMEXO", 0.625);
        return RS_ScriptingApi::Double;
    }

    else if (sysvar.toUpper() == "DIMEXE")
    {
        v1_result = graphic->getVariableDouble("$DIMEXE", 1.25);
        return RS_ScriptingApi::Double;
    }

    else if (sysvar.toUpper() == "DIMFXL")
    {
        v1_result = graphic->getVariableDouble("$DIMFXL", 1.0);
        return RS_ScriptingApi::Double;
    }

    else if (sysvar.toUpper() == "DIMTXT")
    {
        v1_result = graphic->getVariableDouble("$DIMTXT", 2.5);
        return RS_ScriptingApi::Double;
    }

    else if (sysvar.toUpper() == "DIMTSZ")
    {
        v1_result = graphic->getVariableDouble("$DIMTSZ", 2.5);
        return RS_ScriptingApi::Double;
    }

    else if (sysvar.toUpper() == "DIMLFAC")
    {
        v1_result = graphic->getVariableDouble("$DIMLFAC", 1.0);
        return RS_ScriptingApi::Double;
    }

    else if (sysvar.toUpper() == "DIMGAP")
    {
        v1_result = graphic->getVariableDouble("$DIMGAP", 0.625);
        return RS_ScriptingApi::Double;
    }

    else if (sysvar.toUpper() == "DIMTIH")
    {
        int_result = graphic->getVariableInt("$DIMTIH", 2);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "DIMZIN")
    {
        int_result = graphic->getVariableInt("$DIMZIN", 1);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "DIMAZIN")
    {
        int_result = graphic->getVariableInt("$DIMAZIN", 0);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "DIMCLRD")
    {
        int_result = graphic->getVariableInt("$DIMCLRD", 0);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "DIMCLRE")
    {
        int_result = graphic->getVariableInt("$DIMCLRE", 0);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "DIMCLRT")
    {
        int_result = graphic->getVariableInt("$DIMCLRT", 0);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "DIMADEC")
    {
        int_result = graphic->getVariableInt("$DIMADEC", 0);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "DIMDEC")
    {
        int_result = graphic->getVariableInt("$DIMDEC", 2);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "DIMAUNI")
    {
        int_result = graphic->getVariableInt("$DIMAUNIT", 0);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "DIMLUNIT")
    {
        int_result = graphic->getVariableInt("$DIMLUNIT", 2);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "DIMDSEP")
    {
        int_result = graphic->getVariableInt("$DIMDSEP", 0);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "DIMFXLON")
    {
        int_result = graphic->getVariableInt("$DIMFXLON", 0);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "DIMTXSTY")
    {
        str_result = graphic->getVariableString("$DIMTXSTY", "standard").toStdString();
        return RS_ScriptingApi::String;
    }

    else if (sysvar.toUpper() == "DIMLWD")
    {
        int_result = graphic->getVariableInt("$DIMLWD", -2);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "DIMLWE")
    {
        int_result = graphic->getVariableInt("$DIMLWE", -2);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "DWGCODEPAGE")
    {
        str_result = graphic->getVariableString("$DWGCODEPAGE", "ANSI_1252").toStdString();
        return RS_ScriptingApi::String;
    }

    else if (sysvar.toUpper() == "EXTMIN")
    {
#if 0
        const RS_Vector ext = graphic->getVariableVector("$EXTMIN", RS_Vector(0.0, 0.0,0.0));
#else
        const RS_Vector extMin = graphic->getMin();
#endif
        v1_result = extMin.x;
        v2_result = extMin.y;
        v3_result = extMin.z;
        return RS_ScriptingApi::Vector3D;
    }

    else if (sysvar.toUpper() == "EXTMAX")
    {
#if 0
        const RS_Vector ext = graphic->getVariableVector("$EXTMAX" , RS_Vector(0.0, 0.0,0.0));
#else
        const RS_Vector extMax = graphic->getMax();
#endif
        v1_result = extMax.x;
        v2_result = extMax.y;
        v3_result = extMax.z;
        return RS_ScriptingApi::Vector3D;
    }

    else if (sysvar.toUpper() == "GRIDMODE")
    {
        int_result = graphic->getVariableInt("$GRIDMODE" , 1);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "GRIDUNIT")
    {
        const RS_Vector spacing = graphic->getVariableVector("$GRIDUNIT" , RS_Vector(0.0,0.0));

        v1_result = spacing.x;
        v2_result = spacing.y;
        return RS_ScriptingApi::Vector2D;
    }

    else if ("INPUTHISTORYMODE")
    {
        int_result = LC_GET_INT("INPUTHISTORYMODE", 1);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "INSUNITS")
    {
        int_result = graphic->getVariableInt("$INSUNITS", 0);
        return RS_ScriptingApi::Int;
    }

#if 0
    else if (sysvar.toUpper() == "JOINSTYLE")
    {
        int_result = graphic->getVariableDouble("$JOINSTYLE", -999.9);
        return RS_ScriptingApi::Int;
    }
#endif

    else if (sysvar.toUpper() == "LUNITS")
    {
        int_result = graphic->getVariableInt("$LUNITS", 2);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "LUPREC")
    {
        int_result = graphic->getVariableInt("$LUPREC", 4);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "PDMODE")
    {
        int_result = graphic->getVariableInt("$PDMODE" , LC_DEFAULTS_PDMode);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "PDSIZE")
    {
        int_result = graphic->getVariableDouble("$PDSIZE", LC_DEFAULTS_PDSize);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "PSVPSCALE")
    {
        v1_result = graphic->getVariableDouble("$PSVPSCALE", 1.0);
        return RS_ScriptingApi::Double;
    }

    else if (sysvar.toUpper() == "UCSNAME")
    {
        str_result = graphic->getVariableString("$UCSNAME", "").toStdString();
        return RS_ScriptingApi::String;
    }

    else if (sysvar.toUpper() == "UCSORG")
    {
        const RS_Vector origin = graphic->getVariableVector("$UCSORG" , RS_Vector(0.0,0.0));
        v1_result = origin.x;
        v2_result = origin.y;
        return RS_ScriptingApi::Vector2D;
    }

    else if (sysvar.toUpper() == "UCSORTHOVIEW")
    {
        int_result = graphic->getVariableDouble("$UCSORTHOVIEW", 0);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "UCSXDIR")
    {
        const RS_Vector xAxis = graphic->getVariableVector("$UCSXDIR" , RS_Vector(0.0,0.0));
        v1_result = xAxis.x;
        v2_result = xAxis.y;
        return RS_ScriptingApi::Vector2D;
    }

    else if (sysvar.toUpper() == "UCSYDIR")
    {
        const RS_Vector yAxis = graphic->getVariableVector("$UCSYDIR" , RS_Vector(0.0,0.0));
        v1_result = yAxis.x;
        v2_result = yAxis.y;
        return RS_ScriptingApi::Vector2D;
    }

    else if (sysvar.toUpper() == "SNAPSTYL")
    {
        int_result = graphic->getVariableInt("$SNAPSTYLE", 0);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "SNAPISOPAIR")
    {
        int_result = graphic->getVariableInt("$SNAPISOPAIR", 0);
        return RS_ScriptingApi::Int;
    }

    else if (sysvar.toUpper() == "TEXTSTYLE")
    {
        str_result = graphic->getVariableString("$TEXTSTYLE", "Standard").toStdString();
        return RS_ScriptingApi::String;
    }

    else
    {}

    return RS_ScriptingApi::None;
}

RS_EntityContainer* RS_ScriptingApi::getContainer() const
{
    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_GraphicView* graphicView = appWin->getCurrentGraphicView();;
    return graphicView == NULL ? NULL : graphicView->getContainer();
}

RS_Document* RS_ScriptingApi::getDocument() const
{
    auto& appWin = QC_ApplicationWindow::getAppWindow();
    return appWin->getCurrentDocument();
}

RS_Graphic* RS_ScriptingApi::getGraphic() const
{
    auto& appWin=QC_ApplicationWindow::getAppWindow();
    RS_Document* d = appWin->getCurrentDocument();

    if (d && d->rtti()==RS2::EntityGraphic)
    {
        RS_Graphic* graphic = (RS_Graphic*)d;
        if (graphic==NULL) {
            return NULL;
        }
        return graphic;
    }
    return NULL;
}

RS_GraphicView* RS_ScriptingApi::getGraphicView() const
{
    auto& appWin=QC_ApplicationWindow::getAppWindow();
    return appWin->getCurrentGraphicView();
}

LC_GraphicViewport* RS_ScriptingApi::getViewPort() const
{
    RS_GraphicView *gv = getGraphicView();
    return gv == NULL ? NULL : gv->getViewPort();
}

#endif // DEVELOPER
