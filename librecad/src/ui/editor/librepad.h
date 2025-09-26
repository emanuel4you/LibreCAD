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

#ifndef NOTEPAD_H
#define NOTEPAD_H

#ifdef DEVELOPER

#include <QMainWindow>
#include <QLineEdit>
#include <QCloseEvent>
#include <QToolBar>
#include <QStringList>
#include <QComboBox>

#include "texteditor.h"
#include "lpsearchbar.h"

#include "ui_librepad.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Librepad;
}
QT_END_NAMESPACE

class LpSearchBar;

class Librepad : public QMainWindow
{
    Q_OBJECT
public:
    QString m_editorName;

    explicit Librepad(QWidget *parent = nullptr, const QString& name="Librepad", const QString& fileName="");
    ~Librepad();

    void showScriptToolBar();
    void hideScriptToolBar();

    void writePowerMatchCase(bool enabled);
    void writeIncMatchCase(bool enabled);
    void writePowerMode(int index);
    void writePowerSearch(bool enabled);
    void writeSearchHistory(const QString &path, const QStringList &list);
    void writeIncFind(QStringList list) { writeSearchHistory("/incfind", list); }
    void writePowerFind(QStringList list) { writeSearchHistory("/powerfind", list); }
    void writePowerReplace(QStringList list) { writeSearchHistory("/powerreplace", list); }
    void writeLispTrace(QStringList list) { writeSearchHistory("/lisptrace", list); }
    void message(const QString &msg);
    void freeTrace() { m_debugCombo->setCurrentText(""); }

    TextEditor *editor() const { return dynamic_cast<TextEditor *>(ui->tabWidget->widget(ui->tabWidget->currentIndex())); }

    virtual void setName() { m_editorName = "Librepad"; }
    virtual void setHistoryHeight() {}
    virtual int historyHeight() const { return 0; }

    bool incMatchCase();
    bool powerMatchCase();
    bool powerSearch();
    int powerMatchMode();

    bool debugging() { return ui->actionDebug->isChecked(); }
    bool tracing() { return ui->actionTrace->isChecked(); }
    QString debugFunc() { return m_debugFunc; }

    QStringList searchHistory(const QString &path);

    QStringList incFind() { return searchHistory("/incfind"); }
    QStringList powerFind() { return searchHistory("/powerfind"); }
    QStringList powerReplace() { return searchHistory("/powerreplace"); }
    QStringList lispTrace() { return searchHistory("/lisptrace"); }

    void enableIDETools();
    void setCmdWidgetChecked(bool val);

    QString path() const { return m_fileName; }
    QString toPlainText() const;
    QString editorNametolower() const { return m_editorName.toLower(); }
    QString editorName() const { return m_editorName; }

    QTabWidget *tabWidget() { return ui->tabWidget; }

public slots:
    void save();
    void find();
    void replace();
    void hideSearch();
    virtual void run() {}
    virtual void loadScript() {}
    virtual void cmdDock() {}
    virtual void help();
    virtual void trace();
    virtual void untrace() {}
    virtual void debug() {}
    virtual void traceFuncChanged(const QString &text) { m_debugFunc = text; }

private slots:
    void slotTabChanged(int index);
    void slotTabClose(int index);
    void newDocument();
    void open();
    void openRecent();
    void saveAs();
    void selectAll();
    void deselect();
    void reload();
    void print();
    void undo();
    void redo();
    void copy();
    void paste();
    void setFont();
    void about();
    void aboutIde();
    void licence();
    void toolBarMain();
    void toolBarBuild();
    void toolBarSearch();
    void freeTraceHistory();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QString m_fileName;
    QFont m_font;
    Ui::Librepad *ui;
    LpSearchBar *m_searchWidget;

    const unsigned int m_maxFileNr;
    QList<QAction*> m_recentFileActionList;
    QLineEdit *m_debugEdit;
    QString m_debugFunc;
    QComboBox *m_debugCombo;

    void addNewTab(const QString& path);
    QFont font() const { return m_font; }

    void writeSettings();
    void writeFontSettings();
    void readSettings();
    void recentMenu();
    void writeRecentSettings(const QString &filePath);
    void updateRecentActionList();
    bool firstSave() const;
};

#endif // DEVELOPER

#endif // NOTEPAD_H
