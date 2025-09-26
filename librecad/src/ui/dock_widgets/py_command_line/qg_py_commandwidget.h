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
#ifndef QG_PY_COMMANDWIDGET_H
#define QG_PY_COMMANDWIDGET_H

#ifdef DEVELOPER

#include "ui_qg_py_commandwidget.h"

class QG_ActionHandler;
class QAction;

class QG_Py_CommandWidget : public QWidget, public Ui::QG_Py_CommandWidget
{
    Q_OBJECT

public:
    QG_Py_CommandWidget(QG_ActionHandler *action_handler, QWidget *parent = nullptr, const char *name = nullptr, Qt::WindowFlags fl = {});
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
    void setCommand( const QString & cmd );
    void appendHistory( const QString & msg );
    void handleCommand(QString cmd);
    void handleKeycode(QString code);
    void spacePressed();
    void tabPressed();
    void escape();
    void setActionHandler( QG_ActionHandler * ah );
    void setCommandMode();
    void setNormalMode();
    static QString getRootCommand( const QStringList & cmdList, const QString & typed );
    void setKeycodeMode(bool state);
protected slots:
    void languageChange();
    void chooseCommandFile();
private slots:
    void dockingButtonTriggered(bool);
private:
    QG_ActionHandler* m_actionHandler = nullptr;
    QAction* m_docking = nullptr;
};

#endif // DEVELOPER

#endif // QG_PY_COMMANDWIDGET_H
