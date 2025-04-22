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

#ifndef PYTHONHIGHLIGHTER_H
#define PYTHONHIGHLIGHTER_H

#ifdef DEVELOPER

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

//! [0]
class PythonHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    PythonHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        int nth = 0;
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QList<HighlightingRule> highlightingRules;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

    QTextCharFormat atFormat;
    QTextCharFormat bytesFormat;
    QTextCharFormat classValuesFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat connectFormat;
    QTextCharFormat expFormat;
    QTextCharFormat functionsFormat;
    QTextCharFormat moduleFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat placeholderFormat;
    QTextCharFormat quoteFormat;
    QTextCharFormat statementFormat;
    QTextCharFormat symbolFormat;
    QTextCharFormat valuesFormat;
    QTextCharFormat dclFormat;

    HighlightingRule triSingle;
    HighlightingRule triDouble;

    bool matchMultiline(const QString& text, const QRegularExpression& delimiter, int inState, const QTextCharFormat& format);
};

#endif // DEVELOPER

#endif // HIGHLIGHTER_H
