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
#ifndef QG_PY_COMMANDWIDGET_H
#define QG_PY_COMMANDWIDGET_H

#include "ui_qg_py_commandwidget.h"

#ifdef DEVELOPER

class QG_ActionHandler;
class QAction;

class QG_Py_CommandWidget : public QWidget, public Ui::QG_Py_CommandWidget
{
    Q_OBJECT

public:
    QG_Py_CommandWidget(QWidget *parent = nullptr, const char *name = nullptr, Qt::WindowFlags fl = {});
    ~QG_Py_CommandWidget();

    bool eventFilter(QObject *obj, QEvent *event) override;
    QAction* getDockingAction() const {
        return m_docking;
    }

    void runFile(const QString &path) { leCommand->runFile(path); }
    void setInput(const QString &cmd);
    void processInput(QString input) { leCommand->processInput(input); }

public slots:
    virtual void setFocus();
    virtual void setCommand( const QString & cmd );
    virtual void appendHistory( const QString & msg );
    virtual void handleCommand(QString cmd);
    virtual void handleKeycode(QString code);
    virtual void spacePressed();
    virtual void tabPressed();
    virtual void escape();
    virtual void setActionHandler( QG_ActionHandler * ah );
    virtual void setCommandMode();
    virtual void setNormalMode();
    static QString getRootCommand( const QStringList & cmdList, const QString & typed );
    void setKeycodeMode(bool state);

protected slots:
    virtual void languageChange();
    virtual void chooseCommandFile();

private slots:
    virtual void dockingButtonTriggered(bool);

private:
    QG_ActionHandler* actionHandler = nullptr;
    QAction* m_docking = nullptr;

};

#endif // DEVELOPER

#endif // QG_PY_COMMANDWIDGET_H