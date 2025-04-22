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

#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#ifdef DEVELOPER

#include <QPlainTextEdit>
#include <QFileInfo>
#include "dclhighlighter.h"
#include "lisphighlighter.h"
#include "pythonhighlighter.h"

class LineNumberWidget;
class TextEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    TextEditor(QWidget *parent, const QString& fileName);
    ~TextEditor();

    void lineNumberPaintEvent(QPaintEvent *e);

    void load(QString fileName);
    void reload();
    void save();
    void saveAs();
    void printer();
    bool firstSave() const { return m_firstSave; }

    QString path() const { return m_fileName; }

    QString fileName() const
    {
        QFileInfo info(m_fileName);
        return info.fileName();
    }

signals:
    void documentChanged();

public slots:
    void updateLineNumber(const QRect &rect, int dy);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void highlightCurrentLine();
    void updateLineNumberMargin();
    int getLineNumberWidth();

private:
    LineNumberWidget *m_lineNumberWidget;
    QString m_fileName;
    bool m_firstSave;

    DclHighlighter *m_dclHighlighter = nullptr;
    LispHighlighter *m_lispHighlighter = nullptr;
    PythonHighlighter *m_pythonHighlighter = nullptr;

    void initHighlighter();
    void removeHighlighter();

    void setFirstSave(bool state) { m_firstSave = state; }
    void saveFileContent(const QByteArray &fileContent, const QString &fileNameHint);
};

class LineNumberWidget : public QWidget
{
public:
    LineNumberWidget(TextEditor *editor);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    TextEditor *m_editor;
};

#endif // DEVELOPER

#endif   // TEXTEDITOR_H
