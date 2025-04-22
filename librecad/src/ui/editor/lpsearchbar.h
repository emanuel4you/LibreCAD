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

#ifndef LPSEARCHBAR_H
#define LPSEARCHBAR_H

#ifdef DEVELOPER

#include <QWidget>

#include "librepad.h"

class QVBoxLayout;
class QToolButton;
class QComboBox;
class Librepad;

namespace Ui
{
class IncrementalSearchBar;
class PowerSearchBar;
}

class LpSearchBar : public QWidget
{
    Q_OBJECT

public:
    enum SearchMode {
        // NOTE: Concrete values are important here
        // to work with the combobox index!
        MODE_PLAIN_TEXT = 0,
        MODE_WHOLE_WORDS = 1,
        MODE_REGEX = 2
    };

public:
    explicit LpSearchBar(QWidget *parent = nullptr, Librepad *lpad = nullptr);
    ~LpSearchBar();

    bool isPower() const;

    QString searchPattern() const;
    QString replacementPattern() const;

    bool selectionOnly() const;
    bool matchCase() const;

public slots:
    void setSelectionOnly(bool selectionOnly);

    // Called by buttons and typically <F3>/<Shift>+<F3> shortcuts
    void findNext();
    void findPrevious();

    // PowerMode stuff
    void findAll();

    void replaceNext();
    void replaceAll();

    // Also used by KTextEditor::ViewPrivate
    void enterPowerMode();
    void enterIncrementalMode();

private slots:
    void onReturnPressed();
    void onMatchCaseToggled(bool matchCase);

    void onPowerPatternChanged(const QString &pattern);
    void onPowerModeChanged(int index);
    void onPowerPatternContextMenuRequest();
    void onPowerPatternContextMenuRequest(const QPoint &);
    void onPowerReplacmentContextMenuRequest();
    void onPowerReplacmentContextMenuRequest(const QPoint &);

protected:
    // Overridden
    bool eventFilter(QObject *obj, QEvent *event) override;

    /**
     * @return widget that should be used to add controls to bar widget
     */
    QWidget *centralWidget()
    {
        return m_centralWidget;
    }

    /**
     * @return close button, if there
     */
    QToolButton *closeButton()
    {
        return m_closeButton;
    }

private:
    Librepad *m_librePad;
    QWidget *m_centralWidget = nullptr;
    QVBoxLayout *const m_layout;

    // Shared by both dialogs
    QWidget *m_widget;

    // Incremental search related
    Ui::IncrementalSearchBar *m_incUi;

    // Power search related
    Ui::PowerSearchBar *m_powerUi = nullptr;

    int m_powerMode;
    QToolButton *m_closeButton;
    QString m_unfinishedSearchText;
    QString m_replacement;

    uint m_matchCounter = 0;

    // Status backup
    bool m_powerMatchCase;
    bool m_incMatchCase;
    bool m_replaceMode = false;

    void addCurrentTextToHistory(QComboBox *combo);
    void backupConfig(bool ofPower);
    void showExtendedContextMenu(bool forPattern, const QPoint &pos);
    void givePatternFeedback();
    void showResultMessage();

    bool find(bool direction, bool reset);
    bool isPatternValid() const;

    SearchMode searchMode() const;

    QList<QString> getCapturePatterns(const QString &pattern) const;
};

#endif // DEVELOPER
#endif // LPSEARCHBAR_H
