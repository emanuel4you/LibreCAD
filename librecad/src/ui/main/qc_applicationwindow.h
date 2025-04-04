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

// Changes: https://github.com/LibreCAD/LibreCAD/commits/master/librecad/src/main/qc_applicationwindow.h

#ifndef QC_APPLICATIONWINDOW_H
#define QC_APPLICATIONWINDOW_H

#include <memory>

#include <QMap>
#include <QSettings>

#include "rs.h"
#include "rs_pen.h"
#include "rs_snapper.h"
#include "mainwindowx.h"
#include "lc_penpalettewidget.h"
#include "lc_quickinfowidget.h"
#include "lc_mdiapplicationwindow.h"
#include "lc_releasechecker.h"
#include "lc_qtstatusbarmanager.h"
#include "lc_namedviewslistwidget.h"
#include "lc_ucslistwidget.h"
#include "lc_ucsstatewidget.h"
#include "lc_anglesbasiswidget.h"
#include "lc_workspacesmanager.h"
class LC_MenuFactory;
class LC_ActionGroupManager;
class LC_CustomToolbar;
class LC_PenWizard;
class LC_PenPaletteWidget;
class LC_SimpleTests;
class QC_DialogFactory;
class QC_MDIWindow;
class QC_PluginInterface;
class QG_ActionHandler;
class QG_ActiveLayerName;
class QG_BlockWidget;
class QG_CommandWidget;
class QG_CoordinateWidget;
class QG_LayerWidget;
class LC_LayerTreeWidget;
class LC_RelZeroCoordinatesWidget;
class QG_LibraryWidget;
class QG_MouseWidget;
class QG_PenToolBar;
class QG_RecentFiles;
class QG_SelectionWidget;
class QG_SnapToolBar;
class QMdiArea;
class QMdiSubWindow;
class RS_Block;
class RS_Document;
class RS_GraphicView;
class RS_Pen;
class TwoStackedLabels;
struct RS_SnapMode;

struct DockAreas
{
    QAction* left {nullptr};
    QAction* right {nullptr};
    QAction* top {nullptr};
    QAction* bottom {nullptr};
    QAction* floating {nullptr};
};

/**
 * Main application window. Hold together document, view and controls.
 *
 * @author Andrew Mustun
 */
class QC_ApplicationWindow: public LC_MDIApplicationWindow
{
    Q_OBJECT
public:

    enum
    {
        DEFAULT_STATUS_BAR_MESSAGE_TIMEOUT = 2000
    };

    ~QC_ApplicationWindow();

    void initSettings();
    void storeSettings();

    bool queryExit(bool force);

    /** Catch hotkey for giving focus to command line. */
     void keyPressEvent(QKeyEvent* e) override;
    void setRedoEnable(bool enable);
    void setUndoEnable(bool enable);
    static bool loadStyleSheet(QString path);

    bool eventFilter(QObject *obj, QEvent *event) override;
    QAction* getAction(const QString& name) const;

    void activateWindow(QMdiSubWindow* w){
        if (w != nullptr) {
            doActivate(w);
        }
    }

    void fireIconsRefresh();
    void fireWidgetSettingsChanged();
    void fireWorkspacesChanged();
public slots:
    void relayAction(QAction* q_action);
    void slotFocus();
    void slotBack();
    void slotKillAllActions();
    void slotEnter();
    void slotFocusCommandLine();
    void slotFocusOptionsWidget();
    void slotError(const QString& msg);
    void slotShowDrawingOptions();
    void slotShowDrawingOptionsUnits();

    void slotWindowActivated(QMdiSubWindow* w, bool forced=false) override;
    void slotWorkspacesMenuAboutToShow();
    void slotWindowsMenuActivated(bool);

    void slotPenChanged(RS_Pen p);
    //void slotSnapsChanged(RS_SnapMode s);
    void slotEnableActions(bool enable);

    /** generates a new document for a graphic. */
    QC_MDIWindow* slotFileNew(RS_Document* doc=nullptr);
    /** generates a new document based in predefined template */
    void slotFileNewNew();
    /** generates a new document based in selected template */
    void slotFileNewTemplate();
    /** opens a document */
    void slotFileOpen();

    /**
     * opens the given file.
     */
    void slotFileOpen(const QString& fileName, RS2::FormatType type);
    void slotFileOpen(const QString& fileName); // Assume Unknown type
    void slotFileOpenRecent(QAction* action);
    /** saves a document */
    void slotFileSave();
    /** saves a document under a different filename*/
    void slotFileSaveAs();
	/** saves all open documents; return false == operation cancelled **/
    bool slotFileSaveAll();
    /** auto-save document */
    void slotFileAutoSave();
    /** exports the document as bitmap */
    void slotFileExport();
    bool slotFileExport(const QString& name,
                        const QString& format,
                        QSize size,
                        QSize borders,
                        bool black,
                        bool bw=true);
    /** closing the current file */
    void slotFileClosing(QC_MDIWindow*);
	/** close all files; return false == operation cancelled */
	   bool slotFileCloseAll();
    /** prints the current file */
    void slotFilePrint(bool printPDF=false);
    void slotFilePrintPDF();
    /** shows print preview of the current file */
    void slotFilePrintPreview(bool on);
    /** exits the application */
    void slotFileQuit();

    /** toggle the grid */
    void slotViewGrid(bool toggle);
    /** toggle the draft mode */
    void slotViewDraft(bool toggle);
    void slotViewDraftLines(bool toggle);
    /** toggle the statusbar */
    void slotViewStatusBar(bool toggle);
    void slotViewAntialiasing(bool toggle);

    void slotViewGridOrtho(bool toggle);
    void slotViewGridIsoLeft(bool toggle);
    void slotViewGridIsoRight(bool toggle);
    void slotViewGridIsoTop(bool toggle);

    void slotOptionsGeneral();
    void slotOptionsShortcuts();

    void slotImportBlock();

    /** shows an about dlg*/
    void showAboutWindow();

    /**
     * @brief slotUpdateActiveLayer
     * update layer name when active layer changed
     */
    void slotUpdateActiveLayer();
    void execPlug();

    //void invokeLinkList();

    void toggleFullscreen(bool checked);

    void setPreviousZoomEnable(bool enable);

    void hideOptions(QC_MDIWindow*);

    void widgetOptionsDialog();

    void modifyCommandTitleBar(Qt::DockWidgetArea area);
    void reloadStyleSheet();

    void updateGridStatus(const QString&);

    void showDeviceOptions();

    void updateDevice(QString);

    void invokeMenuCreator();
    void invokeToolbarCreator();
    void saveNamedView();
    void saveWorkspace(bool on);
    void removeWorkspace(bool on);
    void restoreWorkspace(bool on);
    void restoreNamedView1();
    void restoreNamedView2();
    void restoreNamedView3();
    void restoreNamedView4();
    void restoreNamedView5();
    void restoreNamedViewCurrent();
    void restoreNamedView(const QString& viewName);
    void createToolbar(const QString& toolbar_name);
    void destroyToolbar(const QString& toolbar_name);
    void destroyMenu(const QString& activator);
    void unassignMenu(const QString& activator, const QString& menu_name);
    void assignMenu(const QString& activator, const QString& menu_name);
    void invokeMenuAssigner(const QString& menu_name);
    void updateMenu(const QString& menu_name);

    void invokeLicenseWindow();
    void onNewVersionAvailable();
    void checkForNewVersion();
    void forceCheckForNewVersion();
    void slotRedockWidgets();
    void slotShowEntityDescriptionOnHover(bool toggle);
    void slotInfoCursorSetting(bool toggle);
signals:
    void gridChanged(bool on);
    void draftChanged(bool on);
    void draftLinesChanged(bool on);
    void antialiasingChanged(bool on);
    void printPreviewChanged(bool on);
    void windowsChanged(bool windowsLeft);
    void signalEnableRelativeZeroSnaps(const bool);
    void showEntityDescriptionOnHoverChanged(bool show);
    void showInfoCursorSettingChanged(bool enabled);
    void iconsRefreshed();
    void widgetSettingsChanged();
    void workspacesChanged(bool hasWorkspaces);
public:
    /**
     * @return Pointer to application window.
     */
    static std::unique_ptr<QC_ApplicationWindow>&  getAppWindow();


    /**
     * Creates a new document. Implementation from RS_MainWindowInterface.
     */
    void createNewDocument(const QString& fileName = QString(), RS_Document* doc=nullptr);

    QG_PenToolBar* getPenToolBar() {return penToolBar;};


    void updateGrids();

    QG_BlockWidget* getBlockWidget(void){
        return blockWidget;
    }

    QG_SnapToolBar* getSnapToolBar(void){
        return snapToolBar;
    }

    QG_SnapToolBar const* getSnapToolBar(void) const{
        return snapToolBar;
    }

    LC_PenPaletteWidget* getPenPaletteWidget(void) const{ return penPaletteWidget;};

    DockAreas& getDockAreas(){
        return dock_areas;
    }

    LC_QuickInfoWidget* getEntityInfoWidget(void) const {return quickInfoWidget;};
    LC_AnglesBasisWidget* getAnglesBasisWidget() const {return anglesBasisWidget;};

    // Highlight the active block in the block widget
    void showBlockActivated(const RS_Block* block);

    // Auto-save
    void startAutoSave(bool enabled);

    int showCloseDialog(QC_MDIWindow* w, bool showSaveAll = false);
    bool doSave(QC_MDIWindow* w, bool forceSaveAs = false);
    void doClose(QC_MDIWindow* w, bool activateNext = true);
    void updateActionsAndWidgetsForPrintPreview(bool printPreviewOn);
    void updateGridViewActions(bool isometric, RS2::IsoGridViewType type);

    void  fillWorkspacesList(QList<QPair<int, QString>> &list);
    void  applyWorkspaceById(int id);
    void  rebuildMenuIfNecessary();
protected:
    void closeEvent(QCloseEvent*) override;
    //! \{ accept drop files to open
    void dropEvent(QDropEvent* e) override;
    void dragEnterEvent(QDragEnterEvent * event) override;
    void changeEvent(QEvent* event) override;
    //! \}
private:
    QC_ApplicationWindow();

    QMenu* createPopupMenu() override;

    QString format_filename_caption(const QString &qstring_in);
    /** Helper function for Menu file -> New & New.... */
	   bool slotFileNewHelper(QString fileName, QC_MDIWindow* w = nullptr);
	// more helpers

	   void doActivate(QMdiSubWindow* w) override;
    void enableFileActions(QC_MDIWindow* w);

    /**
     * @brief updateWindowTitle, for draft mode, add "Draft Mode" to window title
     * @param w, pointer to window widget
     */
    void updateWindowTitle(QWidget* w);

    //Plugin support
    void loadPlugins();

#ifdef LC_DEBUGGING
        LC_SimpleTests* m_pSimpleTest {nullptr};
    #endif

//    QMap<QString, QAction*> a_map; // todo - move actionmap to ActionManager
    LC_ActionGroupManager* ag_manager {nullptr};

    LC_WorkspacesManager m_workspacesManager;
    LC_MenuFactory* m_menuFactory = nullptr;

    /** Pointer to the application window (this). */
    static QC_ApplicationWindow* appWindow;
    std::unique_ptr<QTimer> m_autosaveTimer;

    QG_ActionHandler* actionHandler {nullptr};

    /** Dialog factory */
    QC_DialogFactory* dialogFactory {nullptr};

    /** Recent files list */
    QG_RecentFiles* recentFiles {nullptr};

    // --- Dockwidgets ---
    //! toggle actions for the dock areas
    DockAreas dock_areas;

    /** Layer list widget */
    QG_LayerWidget* layerWidget {nullptr};

    /** Layer tree widget */
    LC_LayerTreeWidget* layerTreeWidget {nullptr};

    /** Entity info widget */
    LC_QuickInfoWidget* quickInfoWidget {nullptr};

    /** Block list widget */
    QG_BlockWidget* blockWidget {nullptr};
    /** Library browser widget */
    QG_LibraryWidget* libraryWidget {nullptr};
    /** Command line */
    QG_CommandWidget* commandWidget {nullptr};

    LC_PenWizard* pen_wiz {nullptr};
    LC_PenPaletteWidget* penPaletteWidget {nullptr};
    LC_NamedViewsListWidget* namedViewsWidget {nullptr};
    LC_UCSListWidget* ucsListWidget {nullptr};

    // --- Statusbar ---
    /** Coordinate widget */
    QG_CoordinateWidget* coordinateWidget {nullptr};
    LC_RelZeroCoordinatesWidget* relativeZeroCoordinatesWidget {nullptr};
    /** Mouse widget */
    QG_MouseWidget* mouseWidget {nullptr};
    /** Selection Status */
    QG_SelectionWidget* selectionWidget {nullptr};
    QG_ActiveLayerName* m_pActiveLayerName {nullptr};
    TwoStackedLabels* grid_status {nullptr};
    LC_UCSStateWidget* ucsStateWidget {nullptr};
    LC_AnglesBasisWidget* anglesBasisWidget{nullptr};


    LC_QTStatusbarManager* statusbarManager {nullptr};


    // --- Toolbars ---
    QG_SnapToolBar* snapToolBar {nullptr};
    QG_PenToolBar* penToolBar {nullptr}; //!< for selecting the current pen
    QToolBar* optionWidget {nullptr}; //!< for individual tool options

    // --- Actions ---
    QAction* previousZoom {nullptr};
    QAction* undoButton {nullptr};
    QAction* redoButton {nullptr};

    QAction* scriptOpenIDE {nullptr};
    QAction* scriptRun {nullptr};
    QAction* helpAboutApp {nullptr};

    // --- Flags ---
    bool previousZoomEnable{false};
    bool undoEnable{false};
    bool redoEnable{false};

    // --- Lists ---
    QList<QC_PluginInterface*> loadedPlugins;
    QList<QAction*> toolbar_view_actions;
    QList<QAction*> dockwidget_view_actions;
    QList<QAction*> recentFilesAction;

    QStringList openedFiles;

    // --- Strings ---
    QString style_sheet_path;

    QList<QAction*> actionsToDisableInPrintPreview;

    LC_ReleaseChecker* releaseChecker;

    void enableWidgets(bool enable);

    friend class LC_WidgetFactory;

    void setGridView(bool toggle, bool isometric, RS2::IsoGridViewType isoGridType);

    void doRestoreNamedView(int i) const;


};

#ifdef _WINDOWS
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif

#endif
