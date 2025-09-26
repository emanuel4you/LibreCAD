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

#ifndef COMMANDEDIT_H
#define COMMANDEDIT_H

#ifdef DEVELOPER

#include <QLineEdit>
#include <QString>
#include <QFile>

/**
 * A python command line edit with some typical console features
 * (uparrow for the history, tab, ..).
 */
class CommandEdit: public QLineEdit {
    Q_OBJECT

public:
    CommandEdit(QWidget* parent=nullptr);
    virtual ~CommandEdit() { writeHistoryFile(); }

    virtual QString text() const;
    virtual void reset() {}
    virtual void setCurrent() {}
    virtual void processInput(QString) {}

    void setPrompt(const QString &p) { m_prom = p; doProcess(false); prompt(); setFocus(); }
    void prompt() { QLineEdit::setText(m_prom); }
    void doProcess(bool proc) { m_doProcess = proc; }
    void doProcessLc(bool proc) { m_doProcessLc = proc; }

    QString dockName() const { return parentWidget()->objectName(); }
    QString cmdLang() const { return dockName().contains("Python") ? "py" : "lisp"; }

    /* for input mode */
    bool m_doProcess;
    bool m_doProcessLc;
    bool m_keycode_mode;

    QStringList historyList;
    QStringList::const_iterator it = historyList.cbegin();

signals:
    void spacePressed();
    void tabPressed();
    void escape();
    void focusIn();
    void focusOut();
    void clearCommandsHistory();
    void command(QString cmd);
    void message(QString msg);
    void keycode(QString code);

protected:
    void keyPressEvent(QKeyEvent* e) override;
    bool event(QEvent* e) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    bool isForeignCommand(QString input);
    void processVariable(QString input);

    QMap<QString, QString> variables;

private:
    /*save history for next session*/
    QString m_path;
    QString m_prom;
    QFile m_histFile;
    QTextStream  m_histFileStream;

    int promptSize() const { return (int) m_prom.size(); }
    void readHistoryFile();
    void writeHistoryFile();

public slots:
    void modifiedPaste();
};

#endif // DEVELOPER

#endif // COMMANDEDIT_H

