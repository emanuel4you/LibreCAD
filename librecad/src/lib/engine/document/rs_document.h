/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
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


#ifndef RS_DOCUMENT_H
#define RS_DOCUMENT_H

#include "lc_ucslist.h"
#include "lc_viewslist.h"
#include "rs_entitycontainer.h"
#include "rs_graphicview.h"
#include "rs_pen.h"
#include "rs_undo.h"

class RS_BlockList;
class RS_LayerList;

/**
 * Base class for documents. Documents can be either graphics or
 * blocks and are typically shown in graphic views. Documents hold
 * an active pen for drawing in the Document, a file name and they
 * know whether they have been modified or not.
 *
 * @author Andrew Mustun
 */
class RS_Document : public RS_EntityContainer,
    public RS_Undo {
public:
	RS_Document(RS_EntityContainer* parent=nullptr);

    virtual RS_LayerList* getLayerList()= 0;
    virtual RS_BlockList* getBlockList() = 0;
    virtual LC_ViewList* getViewList() { return nullptr;}
    virtual LC_UCSList* getUCSList() {return nullptr;}

    virtual void newDoc() = 0;
    virtual bool save(bool isAutoSave = false) = 0;
    virtual bool saveAs(const QString &filename, RS2::FormatType type, bool force) = 0;
    virtual bool open(const QString &filename, RS2::FormatType type) = 0;
    virtual bool loadTemplate(const QString &filename, RS2::FormatType type) = 0;


    /**
     * @return true for all document entities (e.g. Graphics or Blocks).
     */
    bool isDocument() const override {return true;}

    /**
     * Removes an entity from the entity container. Implementation
     * from RS_Undo.
     */
    void removeUndoable(RS_Undoable* u) override {
        if (u && u->undoRtti()==RS2::UndoableEntity && u->isUndone()) {
            removeEntity(static_cast<RS_Entity*>(u));
        }
    }

    /**
     * @return Currently active drawing pen.
     */
    RS_Pen getActivePen() const {return activePen;}

    /**
     * Sets the currently active drawing pen to p.
     */
    void setActivePen(const RS_Pen& p) {activePen = p;}

    /**
     * @return File name of the document currently loaded.
     * Note, that the default file name is empty.
     */
    QString getFilename() const {return filename;}

    /**
     * @return Auto-save file name of the document currently loaded.
     */
    QString getAutoSaveFilename() const {return autosaveFilename;}

    /**
     * Sets file name for the document currently loaded.
     */
    void setFilename(QString fn) {filename = std::move(fn);}

	/**
	 * Sets the documents modified status to 'm'.
	 */
    virtual void setModified(bool m) {
//std::cout << "RS_Document::setModified: %d" << (int)m << std::endl;
        modified = m;
    }

	/**
	 * @retval true The document has been modified since it was last saved.
	 * @retval false The document has not been modified since it was last saved.
	 */
    virtual bool isModified() const {return modified;}

    /**
     * Overwritten to set modified flag when undo cycle finished with undoable(s).
     */
     void endUndoCycle() override;

    void setGraphicView(RS_GraphicView * g) {gv = g;}
    RS_GraphicView* getGraphicView() {return gv;} // fixme - sand -- REALLY BAD DEPENDANCE TO UI here, REWORK!

protected:
    /** Flag set if the document was modified and not yet saved. */
    bool modified = false;
    /** Active pen. */
    RS_Pen activePen;
    /** File name of the document or empty for a new document. */
    QString filename;
	/** Auto-save file name of document. */
    QString autosaveFilename;
	/** Format type */
    RS2::FormatType formatType = RS2::FormatUnknown;
    //used to read/save current view
    RS_GraphicView * gv = nullptr; // fixme - sand -- REALLY BAD DEPENDANCE TO UI here, REWORK!

};
#endif
