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


#ifndef RS_ENTITYCONTAINER_H
#define RS_ENTITYCONTAINER_H

#include <QList>
#include "rs_entity.h"

/**
 * Class representing a tree of entities.
 * Typical entity containers are graphics, polylines, groups, texts, ...)
 *
 * @author Andrew Mustun
 */
class RS_EntityContainer : public RS_Entity {

public:
    using value_type = RS_Entity * ;

    struct RefInfo{
        RS_Vector ref;
        RS_Entity* entity = nullptr;
    };

    struct LC_SelectionInfo{
        unsigned count = 0;
        double length = 0.0;
    };

    RS_EntityContainer(RS_EntityContainer* parent=nullptr, bool owner=true);
    RS_EntityContainer(const RS_EntityContainer& other);
    RS_EntityContainer& operator = (const RS_EntityContainer& other);
    RS_EntityContainer(RS_EntityContainer&& other);
    RS_EntityContainer& operator = (RS_EntityContainer&& other);
    //RS_EntityContainer(const RS_EntityContainer& ec);

    ~RS_EntityContainer() override;

    RS_Entity* clone() const override;
    virtual void detach();

    /** @return RS2::EntityContainer */
    RS2::EntityType rtti() const override{
        return RS2::EntityContainer;
    }

    void reparent(RS_EntityContainer* parent) override;

    /**
     * @return true: because entities made from this class
	 *         and subclasses are containers for other entities.
     */
    bool isContainer() const override{
        return true;
    }

    /**
     * @return false: because entities made from this class
     *         and subclasses are containers for other entities.
     */
    bool isAtomic() const override{
        return false;
    }

    double getLength() const override;

    void setVisible(bool v) override;

    bool setSelected(bool select=true) override;
    bool toggleSelected() override;

    void setHighlighted(bool on) override;

/*virtual void selectWindow(RS_Vector v1, RS_Vector v2,
   bool select=true, bool cross=false);*/
    virtual void selectWindow(enum RS2::EntityType typeToSelect, RS_Vector v1, RS_Vector v2,
                              bool select=true, bool cross=false);
    virtual void selectWindow(const QList<RS2::EntityType> &typesToSelect, RS_Vector v1, RS_Vector v2,
                              bool select=true, bool cross=false);
    virtual void addEntity(RS_Entity* entity);
    virtual void appendEntity(RS_Entity* entity);
    virtual void prependEntity(RS_Entity* entity);
    virtual void moveEntity(int index, QList<RS_Entity *>& entList);
    virtual void insertEntity(int index, RS_Entity* entity);
    virtual bool removeEntity(RS_Entity* entity);

//!
//! \brief addRectangle add four lines to form a rectangle by
//! the diagonal vertices v0,v1
//! \param v0,v1 diagonal vertices of the rectangle
//!
    void addRectangle(RS_Vector const& v0, RS_Vector const& v1);
    void addRectangle(RS_Vector const& v0, RS_Vector const& v1,RS_Vector const& v2, RS_Vector const& v3);

    virtual RS_Entity* firstEntity(RS2::ResolveLevel level=RS2::ResolveNone) const;
    virtual RS_Entity* lastEntity(RS2::ResolveLevel level=RS2::ResolveNone) const;
    virtual RS_Entity* nextEntity(RS2::ResolveLevel level=RS2::ResolveNone) const;
    virtual RS_Entity* prevEntity(RS2::ResolveLevel level=RS2::ResolveNone) const;
    virtual RS_Entity* entityAt(int index) const;
    virtual void setEntityAt(int index,RS_Entity* en);
    virtual int findEntity(RS_Entity const* const entity);
    virtual void clear();

    //virtual unsigned long int count() {
    //	return count(false);
    //}
    virtual bool isEmpty() const {
        return count()==0;
    }
    bool empty() const {
        return isEmpty();
    }
    unsigned count() const override;
    unsigned countDeep() const override;
    size_t size() const
    {
        return m_entities.size();
    }
//virtual unsigned long int countLayerEntities(RS_Layer* layer);
/** \brief countSelected number of selected
* @param deep count sub-containers, if true
* @param types if is not empty, only counts by types listed
*/
    virtual unsigned countSelected(bool deep=true, QList<RS2::EntityType> const& types = {});
    virtual void collectSelected(std::vector<RS_Entity*> &collect, bool deep, QList<RS2::EntityType> const &types = {});
    virtual double totalSelectedLength();
    LC_SelectionInfo getSelectionInfo(/*bool deep, */QList<RS2::EntityType> const& types = {});

    /**
     * Enables / disables automatic update of borders on entity removals
     * and additions. By default this is turned on.
     */
    void setAutoUpdateBorders(bool enable) {
        m_autoUpdateBorders = enable;
    }
    bool getAutoUpdateBorders() const {
        return m_autoUpdateBorders;
    }
    virtual void adjustBorders(RS_Entity* entity);
    void calculateBorders() override;
    void forcedCalculateBorders();
    void updateDimensions( bool autoText=true);
    virtual void updateInserts();
    virtual void updateSplines();
    void update() override;
    virtual void renameInserts(const QString& oldName,
                               const QString& newName);

    RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                 double* dist = nullptr)const override;
    RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                 double* dist, RS_Entity** pEntity ) const;

    RS_Entity* getNearestEntity(const RS_Vector& point,
                                double* dist = nullptr,
                                RS2::ResolveLevel level=RS2::ResolveAll) const;

    RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
                                      bool onEntity = true,
                                      double* dist = nullptr,
                                      RS_Entity** entity=nullptr)const override;

    RS_Vector getNearestCenter(const RS_Vector& coord,
                               double* dist = nullptr)const override;
    RS_Vector getNearestMiddle(const RS_Vector& coord,
                               double* dist = nullptr,
                               int middlePoints = 1
    )const override;
    RS_Vector getNearestDist(double distance,
                             const RS_Vector& coord,
                             double* dist = nullptr) const override;
    RS_Vector getNearestIntersection(const RS_Vector& coord,
                                     double* dist = nullptr);
    RS_Vector getNearestVirtualIntersection(const RS_Vector& coord,
                                            const double& angle,
                                            double* dist);
    RS_Vector getNearestRef(const RS_Vector& coord,
                            double* dist = nullptr) const override;
    RS_Vector getNearestSelectedRef(const RS_Vector& coord,
                                    double* dist = nullptr) const override;

    RefInfo getNearestSelectedRefInfo(const RS_Vector& coord,
                                      double* dist = nullptr) const;

    double getDistanceToPoint(const RS_Vector& coord,
                              RS_Entity** entity,
                              RS2::ResolveLevel level=RS2::ResolveNone,
                              double solidDist = RS_MAXDOUBLE) const override;

    virtual bool optimizeContours();

    bool hasEndpointsWithinWindow(const RS_Vector& v1, const RS_Vector& v2) const override;

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2a) override;
    RS_Entity& shear(double k) override;

    void stretch(const RS_Vector& firstCorner,
                 const RS_Vector& secondCorner,
                 const RS_Vector& offset) override;
    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;
    void moveSelectedRef(const RS_Vector& ref, const RS_Vector& offset) override;
    void revertDirection() override;

    void draw(RS_Painter* painter) override;

    friend std::ostream& operator << (std::ostream& os, RS_EntityContainer& ec);

    bool isOwner() const {return autoDelete;}
    void setOwner(bool owner) {autoDelete=owner;}
    /**
     * @brief areaLineIntegral, line integral for contour area calculation by Green's Theorem
     * Contour Area =\oint x dy
     * @return line integral \oint x dy along the entity
     * returns absolute value
     */
    double areaLineIntegral() const override;
    /**
	 * @brief ignoreForModification ignore this entity for entity catch for certain actions
     * like catching circles to create tangent circles
     * @return, true, indicate this entity container should be ignored
     */
    bool ignoredOnModification() const;

    void push_back(RS_Entity* entity) {
        m_entities.push_back(entity);
    }
    void pop_back()
    {
        if (!isEmpty())
            m_entities.pop_back();
    }

/**
 * @brief begin/end to support range based loop
 * @return iterator
 */
    QList<RS_Entity *>::const_iterator begin() const;
    QList<RS_Entity *>::const_iterator end() const;
    QList<RS_Entity *>::const_iterator cbegin() const;
    QList<RS_Entity *>::const_iterator cend() const;
    QList<RS_Entity *>::iterator begin() ;
    QList<RS_Entity *>::iterator end() ;
//! \{
//! first and last without resolving into children, assume the container is
//! not empty
    RS_Entity* last() const;
    RS_Entity* first() const;
//! \}

    const QList<RS_Entity*>& getEntityList();

    inline RS_Entity* unsafeEntityAt(int index) const {return m_entities.at(index);}

    void drawAsChild(RS_Painter *painter) override;

    RS_Entity *cloneProxy() const override;

protected:
    /**
     * @brief getLoops for hatch, split closed loops into single simple loops. All returned containers are owned by
     * the returned object.
     * @return std::vector<std::unique_ptr<RS_EntityContainer>> - each container contains edge entities of a single
     * closed loop. Each loop is assumed to be simply closed, and loops never cross each other.
     */
    virtual std::vector<std::unique_ptr<RS_EntityContainer>> getLoops() const;


    /** sub container used only temporarily for iteration. */
    mutable RS_EntityContainer* subContainer = nullptr;


private:
/**
 * @brief ignoredSnap whether snapping is ignored
 * @return true when entity of this container won't be considered for snapping points
 */
    bool ignoredSnap() const;

    /** m_entities in the container */
    QList<RS_Entity *> m_entities;
    /**
     * Automatically update the borders of the container when entities
     * are added or removed.
     */
    bool m_autoUpdateBorders = true;
    mutable int entIdx = 0;
    bool autoDelete = false;


};

#endif
