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

#ifndef QG_RECENTFILES_H
#define QG_RECENTFILES_H

#include <QObject>

class QAction;
class QMenu;
class QWidget;

/**
 * This class can store recent files in a list.
 */
class QG_RecentFiles: public QObject{
    Q_OBJECT
public:
    QG_RecentFiles(QObject* parent, int number);
    ~QG_RecentFiles() override;
    void add(const QString& filename);
    void addIfAbsent(const QString& filename);
    /**
     * @return complete path and name of the file stored in the
     * list at index i
     */
    QString get(int i) const;

    /** @return number of files currently stored in the list */
    int count() const;

    /** @return number of files that can be stored in the list at maximum */
    int getNumber() const;

    int indexOf(const QString& filename) const;
    bool contains(const QString& filename) const
    {
        return indexOf(filename) >= 0;
    }
    void addFiles(QMenu* file_menu);
    void updateRecentFilesMenu();
    bool hasMenuEntries() const
    {
        return !m_recentFilesActions.empty();
    }
private:
    /**
     * @brief saveToSettings() save the current recent file list to the "RecentFiles"
     * group of RS_Settings
     */
    void saveToSettings() const;

    int const m_maxEntries;
    QStringList m_files{};
    QList<QAction*> m_recentFilesActions{};
};

#endif
