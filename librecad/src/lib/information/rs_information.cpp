/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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


#include "rs_information.h"

#include "lc_containertraverser.h"
#include "lc_parabola.h"
#include "lc_quadratic.h"
#include "lc_rect.h"
#include "lc_splinepoints.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_polyline.h"

class LC_Parabola;

namespace {

// The tolerance in finding tangent point
// Tangent point is assumed, if the gap between to curves is less than this factor
// times the curve length
constexpr double g_tangentTolerance = 1e-7;

// whether the entity is circular (circle or arc)
bool isArc(RS_Entity const* e) {
    if (e==nullptr)
        return false;
    switch(e->rtti()) {
    case RS2::EntityArc:
    case RS2::EntityCircle:
        return true;
    default:
        return false;
    }
}

// whether the entity is a line
bool isLine(RS_Entity const* e) {
    return e != nullptr && e->rtti() == RS2::EntityLine;
}

// whether the entity is an ellipse
bool isEllipse(RS_Entity const* e) {
    return e != nullptr && e->rtti() == RS2::EntityEllipse;
}

// whether the entity is an ellipse
bool isParabola(RS_Entity const* e) {
    return e != nullptr && e->rtti() == RS2::EntityParabola;
}

/**
 * @brief The TangentFinder class, a helper class for tangent point finding
 * Assuming there's no intersection between the two curves (e1, e2)
 */
class TangentFinder {
    const RS_Entity* m_e1=nullptr;
    const RS_Entity* m_e2=nullptr;
public:
    TangentFinder(const RS_Entity* e1, const RS_Entity* e2):
        m_e1{e1}
      , m_e2{e2}
    {}

    // Find tangent point
    RS_VectorSolutions GetTangent() const;
};

// Type specific algorithms for tangent point finding
class TangentFinderAlgo {
public:

    virtual ~TangentFinderAlgo() = default;

    /**
     * @brief operator () main API to find a tangent point
     * @param e1 - the first entity
     * @param e2 - the second entity
     * @return std::pair<RS_VectorSolutions, bool> -
     *      RS_VectorSolutions, the tangent point found, could be empty
     *      bool, whether the algorithm has been applied. If true is returned,
     *      the algorithm has been applied, no extra algorithm is necessary;
     *      if false is applied, the current algorithm is not applicable due to
     *      mismatched entity types.
     */
    virtual std::pair<RS_VectorSolutions, bool> operator () (const RS_Entity* e1, const RS_Entity* e2) const = 0;
protected:

    // Find tangent points by trying offsets of two sides
    RS_VectorSolutions FindTangent(const RS_Entity* e1, const RS_Entity* e2) const {
        const double tol = g_tangentTolerance * std::min(e1->getLength(), e2->getLength());

        RS_VectorSolutions ret = FindOffsetIntersections(e1, e2, tol);

        if (ret.empty()) {
            ret = FindOffsetIntersections(e1, e2, -tol);
        }
        return ret;
    }

private:
    // Find tangent point by bisection of the offset distance
    // If two intersections are found, keep reducing the offset distance, until the two intersections
    // are close enough
    RS_VectorSolutions FindOffsetIntersections(const RS_Entity* e1, const RS_Entity* e2, double aOffsetValue) const {
        double offsetValue = aOffsetValue;
        std::unique_ptr<RS_Entity> offset = CreateOffset(e1, offsetValue);
        RS_VectorSolutions ret = LC_Quadratic::getIntersection(offset->getQuadratic(), e2->getQuadratic());
        if (ret.size() <= 1)
            return ret;

        // invariant: offset: low (zero intersection), high: 2 intersection
        double low = 0., high = offsetValue;
        while( ret.at(0).distanceTo(ret.at(1)) > 1e-7 && high - low > RS_TOLERANCE) {
            offsetValue = (low + high) * 0.5;
            offset = CreateOffset(e1, offsetValue);
            RS_VectorSolutions retMid = LC_Quadratic::getIntersection(offset->getQuadratic(), e2->getQuadratic());
            switch(retMid.size()) {
            case 0:
                low = offsetValue;
                break;
            case 1:
                retMid.setTangent(true);
                return retMid;
            default:
                ret = std::move(retMid);
                high = offsetValue;
            }
        }
        RS_Vector tangent = (ret.at(0) + ret.at(1)) * 0.5;
        tangent = e2->getNearestPointOnEntity(tangent, false);
        ret = RS_VectorSolutions{tangent};
        ret.setTangent(true);
        return ret;
    }

    // Create an offset curve, according to the distance factor
    // The sign of the factor is used to indicate the two sides of offsets
    virtual std::unique_ptr<RS_Entity> CreateOffset([[maybe_unused]] const RS_Entity* e1, [[maybe_unused]] double offsetValue) const
    {
        return {};
    }
};

// Algorithm for cases with one entity being a line, and assuming the other entity is not a line
class TangentFinderAlgoLine : public TangentFinderAlgo
{
public:
    std::pair<RS_VectorSolutions, bool> operator () (const RS_Entity* e1, const RS_Entity* e2) const override
    {
        if (isLine(e2))
            std::swap(e1, e2);
        if (!isLine(e1))
            return {};
        // Line-Line doesn't have any tangent point
        if (isLine(e2))
            return {};

        RS_VectorSolutions ret = FindTangent(e1, e2);
        return {ret, true};
    }

    std::unique_ptr<RS_Entity> CreateOffset(const RS_Entity* e1, double offsetValue) const override {
        std::unique_ptr<RS_Entity> line{e1->clone()};
        auto normal = RS_Vector{e1->getDirection1() + M_PI/2.};
        line->move(normal * offsetValue);
        return line;
    }
};

// Algorithm for cases with one entity being circular, i.e. an arc or circle
class TangentFinderAlgoArc : public TangentFinderAlgo
{
public:
    std::pair<RS_VectorSolutions, bool> operator () (const RS_Entity* e1, const RS_Entity* e2) const override
    {
        if (isArc(e2))
            std::swap(e1, e2);
        if (!isArc(e1))
            return {};

        RS_VectorSolutions ret = FindTangent(e1, e2);

        return {ret, true};
    }

    std::unique_ptr<RS_Entity> CreateOffset(const RS_Entity* e1, double offsetValue) const override {
        std::unique_ptr<RS_Entity> circle{ e1->clone()};
        circle->setRadius(e1->getRadius()*(1. + offsetValue));
        return circle;
    }
};

// Algorithm for cases with one entity being an ellipse
class TangentFinderAlgoEllipse : public TangentFinderAlgo
{
public:
    std::pair<RS_VectorSolutions, bool> operator () (const RS_Entity* e1, const RS_Entity* e2) const override
    {
        if (isEllipse(e2))
            std::swap(e1, e2);
        if (!isEllipse(e1))
            return {};
        auto ellipse = static_cast<const RS_Ellipse*>(e1);
        RS_Circle c1{nullptr, {ellipse->getCenter(), ellipse->getMinorRadius() * (1.0 + g_tangentTolerance)}};
        std::unique_ptr<RS_Entity> other{e2->clone()};
        other->rotate(ellipse->getCenter(), -ellipse->getAngle());
        other->scale(ellipse->getCenter(), {ellipse->getRatio(), 1.});

        RS_VectorSolutions ret;
        bool processed=false;
       std::tie(ret, processed) = TangentFinderAlgoArc{}(&c1, other.get());
        ret.scale(ellipse->getCenter(), {1./ellipse->getRatio(), 1.});
        ret.rotate(ellipse->getCenter(), ellipse->getAngle());
        return {ret, true};
    }
};

// Algorithm for cases with one entity being a parabola
class TangentFinderAlgoParabola : public TangentFinderAlgo
{
public:
    std::pair<RS_VectorSolutions, bool> operator () (const RS_Entity* e1, const RS_Entity* e2) const override
    {
        if (isParabola(e2))
            std::swap(e1, e2);
        if (!isParabola(e1))
            return {};

        RS_VectorSolutions ret = FindTangent(e1, e2);

        return {ret, true};
    }

    std::unique_ptr<RS_Entity> CreateOffset(const RS_Entity* e1, double offsetValue) const override {

        auto parabola = static_cast<const LC_Parabola*>(e1);
        return parabola->approximateOffset(offsetValue);
    }
};

RS_VectorSolutions TangentFinder::GetTangent() const
{
    // TODO: handle splinepoints, parabola, and hyperbola
    std::unique_ptr<TangentFinderAlgo> algos[] = {
        std::make_unique<TangentFinderAlgoLine>(),
        std::make_unique<TangentFinderAlgoArc>(),
        std::make_unique<TangentFinderAlgoEllipse>(),
        std::make_unique<TangentFinderAlgoParabola>()
    };

    for(const std::unique_ptr<TangentFinderAlgo>& algo: algos) {
        const auto& [points, processed] = (*algo)(m_e1, m_e2);
        if (processed) {
            RS_VectorSolutions ret = points;
            //
            if (points.size() == 2) {
                RS_Vector center = (points.at(0) + points.at(1)) * 0.5;
                RS_Vector projection = m_e1->getNearestPointOnEntity(center, false);
                projection = m_e2->getNearestPointOnEntity(projection, false);
                ret = {projection};
            }
            if (!ret.empty())
                ret.setTangent(true);

            return ret;
        }
    }
    return {};
}
}

/**
 * Default constructor.
 *
 * @param container The container to which we will add
 *        entities. Usually that's an RS_Graphic entity but
 *        it can also be a polyline, text, ...
 */
RS_Information::RS_Information(RS_EntityContainer& container):
	container(&container)
{
}



/**
 * @return true: if the entity is a dimensioning entity.
 *         false: otherwise
 */
bool RS_Information::isDimension(RS2::EntityType type) {
	switch(type){
	case RS2::EntityDimAligned:
	case RS2::EntityDimLinear:
	case RS2::EntityDimOrdinate:
	case RS2::EntityDimRadial:
	case RS2::EntityDimDiametric:
	case RS2::EntityDimAngular:
	case RS2::EntityDimArc:
	case RS2::EntityTolerance:
		return true;
	default:
		return false;
	}
}

/**
 * @retval true the entity can be trimmed.
 * i.e. it is in a graphic or in a polyline.
 */
bool RS_Information::isTrimmable(RS_Entity *e){
    if (e){
        if (e->getParent()){
            switch (e->getParent()->rtti()) {
                case RS2::EntityPolyline:
                case RS2::EntityContainer:
                case RS2::EntityGraphic:
                case RS2::EntityBlock:
                    return true;
                default:
                    return false;
            }
        }
    }

    return false;
}
// fixme - return more meaningful information regarding why two entities are not trimmable and show it in UI by action
/**
 * @retval true the two entities can be trimmed to each other;
 * i.e. they are in a graphic or in the same polyline.
 */
bool RS_Information::isTrimmable(RS_Entity *e1, RS_Entity *e2){
    if (e1 && e2){
        if (e1->getParent() && e2->getParent()){
            if (e1->getParent()->rtti() == RS2::EntityPolyline &&
                e2->getParent()->rtti() == RS2::EntityPolyline &&
                e1->getParent() == e2->getParent()){

                // in the same polyline
                RS_Polyline* pl = static_cast<RS_Polyline *>(e1->getParent());
                int idx1 = pl->findEntity(e1);
                int idx2 = pl->findEntity(e2);
                RS_DEBUG->print("RS_Information::isTrimmable: "
                                "idx1: %d, idx2: %d", idx1, idx2);
                bool adjacentSegments = abs(idx1 - idx2) == 1;
                if (adjacentSegments ||
                    (pl->isClosed() && abs(idx1 - idx2) == int(pl->count() - 1))){
                    // directly following entities
                    return true;
                } else {
                    // not directly following entities
                    return false;
                }
            } else if ((e1->getParent()->rtti() == RS2::EntityContainer ||
                        e1->getParent()->rtti() == RS2::EntityGraphic ||
                        e1->getParent()->rtti() == RS2::EntityBlock) &&
                       (e2->getParent()->rtti() == RS2::EntityContainer ||
                        e2->getParent()->rtti() == RS2::EntityGraphic ||
                        e2->getParent()->rtti() == RS2::EntityBlock)){

                // normal entities:
                return true;
            }
        } else {
            // independent entities with the same parent:
            return (e1->getParent() == e2->getParent());
        }
    }

    return false;
}


/**
 * Gets the nearest end point to the given coordinate.
 *
 * @param coord Coordinate (typically a mouse coordinate)
 *
 * @return the coordinate found or an invalid vector
 * if there are no elements at all in this graphics
 * container.
 */
RS_Vector RS_Information::getNearestEndpoint(const RS_Vector& coord,double* dist) const {
    return container->getNearestEndpoint(coord, dist);
}


/**
 * Gets the nearest point to the given coordinate which is on an entity.
 *
 * @param coord Coordinate (typically a mouse coordinate)
 * @param dist Pointer to a double which will contain the
 *        measured distance after return or nullptr
 * @param entity Pointer to a pointer which will point to the
 *        entity on which the point is or nullptr
 *
 * @return the coordinate found or an invalid vector
 * if there are no elements at all in this graphics
 * container.
 */
RS_Vector RS_Information::getNearestPointOnEntity(const RS_Vector& coord,
        bool onEntity,
        double* dist,
        RS_Entity** entity) const {

    return container->getNearestPointOnEntity(coord, onEntity, dist, entity);
}


/**
 * Gets the nearest entity to the given coordinate.
 *
 * @param coord Coordinate (typically a mouse coordinate)
 * @param dist Pointer to a double which will contain the
 *             masured distance after return
 * @param level Level of resolving entities.
 *
 * @return the entity found or nullptr if there are no elements
 * at all in this graphics container.
 */
RS_Entity* RS_Information::getNearestEntity(const RS_Vector& coord,
        double* dist,
        RS2::ResolveLevel level) const {

    return container->getNearestEntity(coord, dist, level);
}

/**
 * Calculates the intersection point(s) between two entities.
 *
 * @param onEntities true: only return intersection points which are
 *                   on both entities.
 *                   false: return all intersection points.
 *
 * @todo support more entities
 *
 * @return All intersections of the two entities. The tangent flag in
 * RS_VectorSolutions is set if one intersection is a tangent point.
 */
RS_VectorSolutions RS_Information::getIntersection(RS_Entity const* e1,
                                                   RS_Entity const* e2, bool onEntities) {

    RS_VectorSolutions ret;
    const double tol = 1.0e-4;

    if (!(e1 && e2) ) {
        RS_DEBUG->print("RS_Information::getIntersection() for nullptr entities");
        return ret;
    }
    if (e1->getId() == e2->getId()) {
        RS_DEBUG->print("RS_Information::getIntersection() of the same entity");
        return ret;
    }

    // unsupported entities / entity combinations:
    RS2::EntityType e1Type = e1->rtti();
    RS2::EntityType e2Type = e2->rtti();
    if (
        e1Type == RS2::EntityMText || e2Type == RS2::EntityMText ||
        e1Type == RS2::EntityText || e2Type == RS2::EntityText ||
        isDimension(e1Type) || isDimension(e2Type)) {
        return ret;
    }

    if (onEntities && !(e1->isConstruction() || e2->isConstruction() || e1Type == RS2::EntityConstructionLine || e2Type == RS2::EntityConstructionLine)) {
// a little check to avoid doing unneeded intersections, an attempt to avoid O(N^2) increasing of checking two-entity information
        LC_Rect const rect1{e1->getMin(), e1->getMax()};
        LC_Rect const rect2{e2->getMin(), e2->getMax()};

        if (onEntities && !rect1.intersects(rect2, RS_TOLERANCE)) {
            return ret;
        }
    }

    //avoid intersections between line segments the same spline
    /* ToDo: 24 Aug 2011, Dongxu Li, if rtti() is not defined for the parent, the following check for splines may still cause segfault */
    if ( e1->getParent() && e1->getParent() == e2->getParent()) {
        if ( e1->getParent()->rtti()==RS2::EntitySpline ) {
            //do not calculate intersections from neighboring lines of a spline
            if ( abs(e1->getParent()->findEntity(e1) - e1->getParent()->findEntity(e2)) <= 1 ) {
                return ret;
            }
        }
    }

    if(e1Type == RS2::EntitySplinePoints || e2Type == RS2::EntitySplinePoints){
        ret = LC_SplinePoints::getIntersection(e1, e2);
    }
    else {
// issue #484 , quadratic intersection solver is not robust enough for quadratic-quadratic
// TODO, implement a robust algorithm for quadratic based solvers, and detecting entity type
// circles/arcs can be removed
        // issue #523: TangentFinder cannot handle line-line
        bool firstIsLine = e1Type == RS2::EntityLine || e1Type == RS2::EntityConstructionLine;
        bool secondIsLine = e2Type == RS2::EntityLine || e2Type == RS2::EntityConstructionLine;
        bool isLineLine = firstIsLine && secondIsLine;
        if (isLineLine) {
            ret = getIntersectionLineLine(e1, e2);
        } else if (isArc(e1)) {
            std::swap(e1, e2);
            if (isArc(e1)) {
//use specialized arc-arc intersection solver
                ret = getIntersectionArcArc(e1, e2);
            } else if (e1Type == RS2::EntityLine || e1Type == RS2::EntityConstructionLine) {
                ret = getIntersectionLineArc(static_cast<const RS_Line *>(e1), static_cast<const RS_Arc *>(e2));
            }
        }

        if (ret.empty()) {
            ret = LC_Quadratic::getIntersection(e1->getQuadratic(), e2->getQuadratic());
            if (!isLineLine && ret.empty()) {
                // TODO: the following tangent point recovery only works if there's no other intersection
                // Issue #523: direct intersection finding may fail for tangent points
                // Accept small distance gap as tangent points. While this will allow some false tangent
                // points from close but non-contacting curves, it's more important to reliably find all
                // tangent points with rounding errors.
                ret = TangentFinder{e1, e2}.GetTangent();
            }
        }

    }
    RS_VectorSolutions ret2;
	for(const RS_Vector& vp: ret){
		if (!vp.valid) continue;
		if (onEntities) {
            //ignore intersections not on entity
            if (!(
                        (e1->isConstruction(true) || e1->isPointOnEntity(vp, tol)) &&
                        (e2->isConstruction(true) || e2->isPointOnEntity(vp, tol))
                        )
                    ) {
//				std::cout<<"Ignored intersection "<<vp<<std::endl;
//				std::cout<<"because: e1->isPointOnEntity(ret.get(i), tol)="<<e1->isPointOnEntity(vp, tol)
//					<<"\t(e2->isPointOnEntity(ret.get(i), tol)="<<e2->isPointOnEntity(vp, tol)<<std::endl;
                continue;
            }
        }
        // need to test whether the intersection is tangential
		RS_Vector direction1=e1->getTangentDirection(vp);
		RS_Vector direction2=e2->getTangentDirection(vp);
        if( direction1.valid && direction2.valid && fabs(fabs(direction1.dotP(direction2)) - sqrt(direction1.squared()*direction2.squared())) < sqrt(tol)*tol )
            ret2.setTangent(true);
        //TODO, make the following tangential test, nearest test work for all entity types

//        RS_Entity   *lpLine = nullptr,
//                    *lpCircle = nullptr;
//        if( RS2::EntityLine == e1->rtti() && RS2::EntityCircle == e2->rtti()) {
//            lpLine = e1;
//            lpCircle = e2;
//        }
//        else if( RS2::EntityCircle == e1->rtti() && RS2::EntityLine == e2->rtti()) {
//            lpLine = e2;
//            lpCircle = e1;
//        }
//        if( nullptr != lpLine && nullptr != lpCircle) {
//            double dist = 0.0;
//            RS_Vector nearest = lpLine->getNearestPointOnEntity( lpCircle->getCenter(), false, &dist);

//            // special case: line touches circle tangent
//            if( nearest.valid && fabs( dist - lpCircle->getRadius()) < tol) {
//                ret.set(i,nearest);
//                ret2.setTangent(true);
//            }
//        }
        ret2.push_back(vp);
    }

    return ret2;
}



/**
 * @return Intersection between two lines.
 */
RS_VectorSolutions RS_Information::getIntersectionLineLine(const RS_Entity* e1,
                                                           const RS_Entity* e2)
{
    if (e1 == nullptr || e2 == nullptr)
        return {};

    if (e1->rtti() != RS2::EntityLine || e2->rtti() != RS2::EntityLine)
        return {};

    if (!(e1 && e2)) {
        RS_DEBUG->print("RS_Information::getIntersectionLineLin() for nullptr entities");
        return {};
    }

    RS_Vector p1 = e1->getStartpoint();
    RS_Vector p2 = e1->getEndpoint();
    RS_Vector p3 = e2->getStartpoint();
    RS_Vector p4 = e2->getEndpoint();

    double num = ((p4.x-p3.x)*(p1.y-p3.y) - (p4.y-p3.y)*(p1.x-p3.x));
    double div = ((p4.y-p3.y)*(p2.x-p1.x) - (p4.x-p3.x)*(p2.y-p1.y));

    // parallel condition
    const double dAngle = static_cast<const RS_Line*>(e1)->getAngle1() - static_cast<const RS_Line*>(e2)->getAngle1();
    if (std::abs(div)>RS_TOLERANCE &&
            std::abs(std::remainder(dAngle, M_PI))>=RS_TOLERANCE_ANGLE) {
        double u = num / div;

        double xs = p1.x + u * (p2.x-p1.x);
        double ys = p1.y + u * (p2.y-p1.y);
        return {RS_Vector{xs, ys}};
    }

    // handle zero-length lines
    if (e1->getLength() < e2->getLength())
        std::swap(e1, e2);
    if (e1->getLength() <= RS_TOLERANCE) {
        // both are zero length lines. Still check distance from points
        if (p1.squaredTo(p3) <= RS_TOLERANCE2)
            return {p1};
        return {};
    }
    if (e2->getLength() <= RS_TOLERANCE) {
        // one line is zero length, only consider if it's close enough to the non-zero line
        RS_Vector projection = e1->getNearestPointOnEntity(e2->getStartpoint(), true);
        if (projection.squaredTo(e2->getStartpoint()) <= RS_TOLERANCE2)
            return {projection};
    }
    // lines are parallel
    return {};
}



/**
 * @return One or two intersection points between given entities.
 */
RS_VectorSolutions RS_Information::getIntersectionLineArc(const RS_Entity* line,
                                                          const RS_Entity* arc)
{

    if (line == nullptr || arc == nullptr)
        return {};

    if(line->rtti() != RS2::EntityLine || !isArc(arc))
        return {};

    RS_Vector p = line->getStartpoint();
    RS_Vector d = line->getEndpoint() - line->getStartpoint();
    const double d2=d.squared();
    RS_Vector c = arc->getCenter();
    const double r = arc->getRadius();
    RS_Vector delta = p - c;
    if (d2<RS_TOLERANCE2) {
        //line too short, still check the whether the line touches the arc
        if ( std::abs(delta.squared() - r*r) < 2.*RS_TOLERANCE*r ){
            return RS_VectorSolutions({line->getMiddlePoint()});
        }
        return {};
    }

    // Arc center projection on line
    double dist=0.;
    RS_Vector projection = line->getNearestPointOnEntity(c, false, &dist);
    RS_Vector dP = projection - c;
    dP -= d *(d.dotP(dP)/d2); // reduce rounding errors
    projection = c + dP;

    const double dr = dP.magnitude() - r;
    const double tol = 1e-5 * r;
    if (dr > tol )
        return {};

    if (dr < - tol) {
        // two solutions
        const double dt = std::sqrt(r*r - dP.squared());
        const RS_Vector dT = d*(dt/d.magnitude());
        return RS_VectorSolutions({ projection + dT, projection - dT});
    }

    // Tangential
    RS_VectorSolutions ret{projection};
    ret.setTangent(true);
//        std::cout<<"Tangential point: "<<ret<<std::endl;
    return ret;
}



/**
 * @return One or two intersection points between given entities.
 */
RS_VectorSolutions RS_Information::getIntersectionArcArc(RS_Entity const* e1,
                                                         RS_Entity const* e2) {

    if (!(e1 && e2))
        return {};

    if(!isArc(e1) || !isArc(e2))
        return {};

    RS_Vector c1 = e1->getCenter();
    RS_Vector c2 = e2->getCenter();

    double r1 = e1->getRadius();
    double r2 = e2->getRadius();

    RS_Vector u = c2 - c1;

    // concentric
    if (u.magnitude()<1.0e-7*(r1 + r2))
        return {};

    // perpendicular to the line passing both centers
    auto v = RS_Vector{u.y, -u.x};

    const double r12 = r1*r1;
    const double r22 = r2*r2;
    // r1/|u|*cos(angle): the angle is at the arc center 1, between an intersection and the line passing arc centers
    double s = 0.5 * ((r12 - r22)/u.squared() + 1.0);
    // term = (r12/|u|^2)*sin^2(angle)
    double term = r12/u.squared() - s*s;

    // no intersection:
    if (term<- RS_TOLERANCE) {
        return {};
    }

    // one or two intersections:
    double t1 = std::sqrt(std::max(0., term));

    RS_Vector sol1 = c1 + u*s + v*t1;
    RS_Vector sol2 = c1 + u*s - v*t1;

    if (sol1.distanceTo(sol2)<1.0e-5*(r1+r2)) {
        RS_VectorSolutions ret{sol1};
        ret.setTangent(true);
        return ret;
    }

    return {sol1, sol2};
}

// find intersections between ellipse/arc/circle using quartic equation solver
//
// @author Dongxu Li <dongxuli2011@gmail.com>
//

RS_VectorSolutions RS_Information::getIntersectionEllipseEllipse(
		RS_Ellipse const* e1, RS_Ellipse const* e2) {
    RS_VectorSolutions ret;

	if (!(e1 && e2) ) {
        return ret;
    }
    if (
        (e1->getCenter() - e2 ->getCenter()).squared() < RS_TOLERANCE2 &&
        (e1->getMajorP() - e2 ->getMajorP()).squared() < RS_TOLERANCE2 &&
        fabs(e1->getRatio() - e2 ->getRatio()) < RS_TOLERANCE
    ) { // overlapped ellipses, do not do overlap
        return ret;
    }
	RS_Ellipse ellipse01(nullptr,e1->getData());

    RS_Ellipse *e01= & ellipse01;
    if( e01->getMajorRadius() < e01->getMinorRadius() ) e01->switchMajorMinor();
	RS_Ellipse ellipse02(nullptr,e2->getData());
    RS_Ellipse *e02= &ellipse02;
    if( e02->getMajorRadius() < e02->getMinorRadius() ) e02->switchMajorMinor();
    //transform ellipse2 to ellipse1's coordinates
    RS_Vector shiftc1=- e01->getCenter();
    double shifta1=-e01->getAngle();
    e02->move(shiftc1);
    e02->rotate(shifta1);
//    RS_Vector majorP2=e02->getMajorP();
    double a1=e01->getMajorRadius();
    double b1=e01->getMinorRadius();
    double x2=e02->getCenter().x,
           y2=e02->getCenter().y;
    double a2=e02->getMajorRadius();
    double b2=e02->getMinorRadius();

    if( e01->getMinorRadius() < RS_TOLERANCE || e01 -> getRatio()< RS_TOLERANCE) {
        // treat e01 as a line
        RS_Line line{e1->getParent(), {{-a1,0.}, {a1,0.}}};
        ret= getIntersectionEllipseLine(&line, e02);
        ret.rotate(-shifta1);
        ret.move(-shiftc1);
        return ret;
    }
    if( e02->getMinorRadius() < RS_TOLERANCE || e02 -> getRatio()< RS_TOLERANCE) {
        // treat e02 as a line
        RS_Line line{e1->getParent(), {{-a2,0.}, {a2,0.}}};
        line.rotate({0.,0.}, e02->getAngle());
        line.move(e02->getCenter());
        ret = getIntersectionEllipseLine(&line, e01);
        ret.rotate(-shifta1);
        ret.move(-shiftc1);
        return ret;
    }

    //ellipse01 equation:
    //	x^2/(a1^2) + y^2/(b1^2) - 1 =0
    double t2= - e02->getAngle();
    //ellipse2 equation:
    // ( (x - u) cos(t) - (y - v) sin(t))^2/a^2 + ( (x - u) sin(t) + (y-v) cos(t))^2/b^2 =1
    // ( cos^2/a^2 + sin^2/b^2) x^2 +
    // ( sin^2/a^2 + cos^2/b^2) y^2 +
    //  2 sin cos (1/b^2 - 1/a^2) x y +
    //  ( ( 2 v sin cos - 2 u cos^2)/a^2 - ( 2v sin cos + 2 u sin^2)/b^2) x +
    //  ( ( 2 u sin cos - 2 v sin^2)/a^2 - ( 2u sin cos + 2 v cos^2)/b^2) y +
    //  (u cos - v sin)^2/a^2 + (u sin + v cos)^2/b^2 -1 =0
    // detect whether any ellipse radius is zero
    double cs=cos(t2),si=sin(t2);
    double ucs=x2*cs,usi=x2*si,
           vcs=y2*cs,vsi=y2*si;
    double cs2=cs*cs,si2=1-cs2;
    double tcssi=2.*cs*si;
    double ia2=1./(a2*a2),ib2=1./(b2*b2);
    std::vector<double> m(0,0.);
    m.push_back( 1./(a1*a1)); //ma000
    m.push_back( 1./(b1*b1)); //ma011
    m.push_back(cs2*ia2 + si2*ib2); //ma100
    m.push_back(cs*si*(ib2 - ia2)); //ma101
    m.push_back(si2*ia2 + cs2*ib2); //ma111
    m.push_back(( y2*tcssi - 2.*x2*cs2)*ia2 - ( y2*tcssi+2*x2*si2)*ib2); //mb10
    m.push_back( ( x2*tcssi - 2.*y2*si2)*ia2 - ( x2*tcssi+2*y2*cs2)*ib2); //mb11
    m.push_back((ucs - vsi)*(ucs-vsi)*ia2+(usi+vcs)*(usi+vcs)*ib2 -1.); //mc1
	auto vs0=RS_Math::simultaneousQuadraticSolver(m);
    shifta1 = - shifta1;
    shiftc1 = - shiftc1;
	for(RS_Vector vp: vs0){
        vp.rotate(shifta1);
        vp.move(shiftc1);
        ret.push_back(vp);
    }
    return ret;
}

//wrapper to do Circle-Ellipse and Arc-Ellipse using Ellipse-Ellipse intersection
RS_VectorSolutions RS_Information::getIntersectionCircleEllipse(RS_Circle* c1,
        RS_Ellipse* e1) {
    RS_VectorSolutions ret;
	if (!(c1 && e1)) return ret;

	RS_Ellipse const e2{c1->getParent(),
			{c1->getCenter(), {c1->getRadius(),0.},
			1.0,
			0., 2.*M_PI,
			false}};
	return getIntersectionEllipseEllipse(e1, &e2);
}

RS_VectorSolutions RS_Information::getIntersectionArcEllipse(RS_Arc * a1,
        RS_Ellipse* e1) {
    RS_VectorSolutions ret;
	if (!(a1 && e1)) {
        return ret;
	}
	RS_Ellipse const e2{a1->getParent(),
			{a1->getCenter(),
			{a1->getRadius(), 0.},
			1.0,
			a1->getAngle1(), a1->getAngle2(),
			a1->isReversed()}};
	return getIntersectionEllipseEllipse(e1, &e2);
}





/**
 * @return One or two intersection points between given entities.
 */
RS_VectorSolutions RS_Information::getIntersectionEllipseLine(RS_Line* line,
        RS_Ellipse* ellipse) {

    RS_VectorSolutions ret;

	if (!(line && ellipse)) return ret;

    // rotate into normal position:

    double rx = ellipse->getMajorRadius();
    if(rx<RS_TOLERANCE) {
        //zero radius ellipse
        RS_Vector vp(line->getNearestPointOnEntity(ellipse->getCenter(), true));
        if((vp-ellipse->getCenter()).squared() <RS_TOLERANCE2){
            //center on line
            ret.push_back(vp);
        }
        return ret;
    }
    RS_Vector angleVector = ellipse->getMajorP().scale(RS_Vector(1./rx,-1./rx));
    double ry = rx*ellipse->getRatio();
    RS_Vector center = ellipse->getCenter();
    RS_Vector a1 = line->getStartpoint().rotate(center, angleVector);
    RS_Vector a2 = line->getEndpoint().rotate(center, angleVector);
//    RS_Vector origin = a1;
    RS_Vector dir = a2-a1;
    RS_Vector diff = a1 - center;
    RS_Vector mDir = RS_Vector(dir.x/(rx*rx), dir.y/(ry*ry));
    RS_Vector mDiff = RS_Vector(diff.x/(rx*rx), diff.y/(ry*ry));

    double a = RS_Vector::dotP(dir, mDir);
    double b = RS_Vector::dotP(dir, mDiff);
    double c = RS_Vector::dotP(diff, mDiff) - 1.0;
    double d = b*b - a*c;

//    std::cout<<"RS_Information::getIntersectionEllipseLine(): d="<<d<<std::endl;
    if (d < - 1.e3*RS_TOLERANCE*sqrt(RS_TOLERANCE)) {
        RS_DEBUG->print("RS_Information::getIntersectionLineEllipse: outside 0");
        return ret;
    }
    if( d < 0. ) d=0.;
    double root = sqrt(d);
    double t_a = -b/a;
    double t_b = root/a;
    //        double t_b = (-b + root) / a;

    ret.push_back(a1.lerp(a2,t_a+t_b));
    RS_Vector vp(a1.lerp(a2,t_a-t_b));
    if ( (ret.get(0)-vp).squared()>RS_TOLERANCE2) {
        ret.push_back(vp);
    }
    angleVector.y *= -1.;
    ret.rotate(center, angleVector);
//    std::cout<<"found Ellipse-Line intersections: "<<ret.getNumber()<<std::endl;
//    std::cout<<ret<<std::endl;
    RS_DEBUG->print("RS_Information::getIntersectionEllipseLine(): done");
    return ret;
}


/**
 * Checks if the given coordinate is inside the given contour.
 *
 * @param point Coordinate to check.
 * @param contour One or more entities which shape a contour.
 *         If the given contour is not closed, the result is undefined.
 *         The entities don't need to be in a specific order.
 * @param onContour Will be set to true if the given point it exactly
 *         on the contour.
 */
bool RS_Information::isPointInsideContour(const RS_Vector& point,
        RS_EntityContainer* contour, bool* onContour) {

	if (!contour) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
						"RS_Information::isPointInsideContour: contour is nullptr");
        return false;
    }

    if (point.x < contour->getMin().x || point.x > contour->getMax().x ||
            point.y < contour->getMin().y || point.y > contour->getMax().y) {
        return false;
    }

    double width = contour->getSize().x+1.0;

    bool sure;
    int counter;
    int tries = 0;
    double rayAngle = 0.0;
    do {
        sure = true;

        // create ray:
		RS_Vector v = RS_Vector::polar(width*10.0, rayAngle);
		RS_Line ray{point, point+v};
        counter = 0;
        RS_VectorSolutions sol;

		if (onContour) {
            *onContour = false;
        }

        for(RS_Entity* e: lc::LC_ContainerTraverser{*contour, RS2::ResolveAll}.entities()) {
            // intersection(s) from ray with contour entity:
            sol = RS_Information::getIntersection(&ray, e, true);

            for (int i=0; i<=1; ++i) {
                RS_Vector p = sol.get(i);

                if (p.valid) {
                    // point is on the contour itself
                    if (p.distanceTo(point)<1.0e-5) {
						if (onContour) {
                            *onContour = true;
                        }
                    } else {
                        if (e->rtti()==RS2::EntityLine) {
                            RS_Line* line = (RS_Line*)e;

                            // ray goes through startpoint of line:
                            if (p.distanceTo(line->getStartpoint())<1.0e-4) {
                                if (RS_Math::correctAngle(line->getAngle1())<M_PI) {
                                    sure = false;
                                }
                            }

                            // ray goes through endpoint of line:
                            else if (p.distanceTo(line->getEndpoint())<1.0e-4) {
                                if (RS_Math::correctAngle(line->getAngle2())<M_PI) {
                                    sure = false;
                                }
                            }
                            // else: ray goes through the line


                                counter++;
                            
                        } else if (e->rtti()==RS2::EntityArc) {
                            RS_Arc* arc = (RS_Arc*)e;

                            if (p.distanceTo(arc->getStartpoint())<1.0e-4) {
                                double dir = arc->getDirection1();
                                if ((dir<M_PI && dir>=1.0e-5) ||
                                        ((dir>2*M_PI-1.0e-5 || dir<1.0e-5) &&
                                         arc->getCenter().y>p.y)) {
                                    counter++;
                                    sure = false;
                                }
                            }
                            else if (p.distanceTo(arc->getEndpoint())<1.0e-4) {
                                double dir = arc->getDirection2();
                                if ((dir<M_PI && dir>=1.0e-5) ||
                                        ((dir>2*M_PI-1.0e-5 || dir<1.0e-5) &&
                                         arc->getCenter().y>p.y)) {
                                    counter++;
                                    sure = false;
                                }
                            } else {
                                counter++;
                            }
                        } else if (e->rtti()==RS2::EntityCircle) {
                            // tangent:
                            if (i==0 && sol.get(1).valid==false) {
                                if (!sol.isTangent()) {
                                    counter++;
                                } else {
                                    sure = false;
                                }
                            } else if (i==1 || sol.get(1).valid==true) {
                                counter++;
                            }
                        } else if (e->rtti()==RS2::EntityEllipse) {
                            RS_Ellipse* ellipse=static_cast<RS_Ellipse*>(e);
                            if(ellipse->isArc()){
                                if (p.distanceTo(ellipse->getStartpoint())<1.0e-4) {
                                    double dir = ellipse->getDirection1();
                                    if ((dir<M_PI && dir>=1.0e-5) ||
                                            ((dir>2*M_PI-1.0e-5 || dir<1.0e-5) &&
                                             ellipse->getCenter().y>p.y)) {
                                        counter++;
                                        sure = false;
                                    }
                                }
                                else if (p.distanceTo(ellipse->getEndpoint())<1.0e-4) {
                                    double dir = ellipse->getDirection2();
                                    if ((dir<M_PI && dir>=1.0e-5) ||
                                            ((dir>2*M_PI-1.0e-5 || dir<1.0e-5) &&
                                             ellipse->getCenter().y>p.y)) {
                                        counter++;
                                        sure = false;
                                    }
                                } else {
                                    counter++;
                                }
                            }else{
                                // tangent:
                                if (i==0 && sol.get(1).valid==false) {
                                    if (!sol.isTangent()) {
                                        counter++;
                                    } else {
                                        sure = false;
                                    }
                                } else if (i==1 || sol.get(1).valid==true) {
                                    counter++;
                                }
                            }
                        }
                    }
                }
            }
        }

        rayAngle+=0.02;
        tries++;
    }
    while (!sure && rayAngle<2*M_PI && tries<6);

    // remove double intersections:
    /*
       QList<RS_Vector> is2;
       bool done;
    RS_Vector* av;
       do {
           done = true;
           double minDist = RS_MAXDOUBLE;
           double dist;
		av = nullptr;
		   for (RS_Vector* v = is.first(); v; v = is.next()) {
               dist = point.distanceTo(*v);
               if (dist<minDist) {
                   minDist = dist;
                   done = false;
                        av = v;
               }
           }

		if (!done && av) {
                is2.append(*av);
        }
       } while (!done);
    */

    return ((counter%2)==1);
}


RS_VectorSolutions RS_Information::createQuadrilateral(const RS_EntityContainer& container)
{
	RS_VectorSolutions ret;
	if(container.count()!=4) return ret;
	RS_EntityContainer c(container);
	std::vector<RS_Line*> lines;
	for(auto e: c){
		if(e->rtti()!=RS2::EntityLine) return ret;
		lines.push_back(static_cast<RS_Line*>(e));
	}
	if(lines.size()!=4) return ret;

	//find intersections
	std::vector<RS_Vector> vertices;
	for(auto it=lines.begin()+1; it != lines.end(); ++it){
		for(auto jt=lines.begin(); jt != it; ++jt){
			RS_VectorSolutions const& sol=RS_Information::getIntersectionLineLine(*it, *jt);
			if(sol.size()){
				vertices.push_back(sol.at(0));
			}
		}
	}

//	std::cout<<"vertices.size()="<<vertices.size()<<std::endl;

	switch (vertices.size()){
	default:
		return ret;
	case 4:
		break;
	case 5:
	case 6:
		for(RS_Line* pl: lines){
			const double a0=pl->getDirection1();
			std::vector<std::vector<RS_Vector>::iterator> left;
			std::vector<std::vector<RS_Vector>::iterator> right;
			for(auto it=vertices.begin(); it != vertices.end(); ++it){
				RS_Vector const& dir=*it - pl->getNearestPointOnEntity(*it, false);
				if(dir.squared()<RS_TOLERANCE15) continue;
//				std::cout<<"angle="<<remainder(dir.angle() - a0, 2.*M_PI)<<std::endl;
				if(remainder(dir.angle() - a0, 2.*M_PI) > 0.)
					left.push_back(it);
				else
					right.push_back(it);

				if(left.size()==2 && right.size()==1){
					vertices.erase(right[0]);
					break;
				} else if(left.size()==1 && right.size()==2){
					vertices.erase(left[0]);
					break;
				}
			}
			if(vertices.size()==4) break;
		}
		break;
	}

	//order vertices
	RS_Vector center{0., 0.};
	for(const RS_Vector& vp: vertices)
		center += vp;
	center *= 0.25;
	std::sort(vertices.begin(), vertices.end(), [&center](const RS_Vector& a,
			  const RS_Vector&b)->bool{
		return center.angleTo(a)<center.angleTo(b);
	}
	);
	for(const RS_Vector& vp: vertices){
		ret.push_back(vp);
//		std::cout<<"vp="<<vp<<std::endl;
	}
	return ret;
}
