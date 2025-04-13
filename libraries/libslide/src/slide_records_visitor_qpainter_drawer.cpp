/**
 *   AutoCAD slide library
 *
 *   Copyright (C) 2023-2024 Emanuel Strobel
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "slide_colors.hpp"
#include "slide_records.hpp"
#include "slide_records_visitor_qpainter_drawer.hpp"

#include <QPolygon>

namespace libslide {

SlideRecordsVisitorQPainterDrawer::SlideRecordsVisitorQPainterDrawer(QPainter *painter,
                                                               unsigned src_width,
                                                               unsigned src_height,
                                                               double   src_ratio,
                                                               unsigned dst_x,
                                                               unsigned dst_y,
                                                               unsigned dst_width,
                                                               unsigned dst_height,
                                                               bool dark_background)
    : _qpainter{painter},
      _dst_x{dst_x},
      _dst_y{dst_y},
      _dst_height{dst_height},
      _last_x{0},
      _last_y{0},
      _color{0},
      _dark{dark_background}
{
    //
    // Calculate new_width and new_height taking
    // into account source aspect ratio.
    //
    // https://math.stackexchange.com/questions/1620366/
    // how-to-keep-aspect-ratio-and-position-of-rectangle-inside-another-rectangle
    //
    double dst_ratio = 1.0 * dst_width / dst_height;

    unsigned new_width = dst_width;
    unsigned new_height = dst_height;

    if (src_ratio > dst_ratio) {
        new_height = dst_width / src_ratio;
    } else if (src_ratio < dst_ratio) {
        new_width = dst_height * src_ratio;
    }

    _scale_x = 1.0 * new_width  / src_width;
    _scale_y = 1.0 * new_height / src_height;
}

inline
double SlideRecordsVisitorQPainterDrawer::adjust_x(unsigned x) const {
    // Scale & Move
    return x * _scale_x + _dst_x;
}

inline
double SlideRecordsVisitorQPainterDrawer::adjust_y(unsigned y) const {
    // Point (0,0) is located at the lower-left corner.
    // Scale & Move
    return _dst_height - y * _scale_y + _dst_y;
}

void SlideRecordsVisitorQPainterDrawer::accept(SlideRecordVector& r)
{
    int x0 = r.x0(), y0 = r.y0();
    int x1 = r.x1(), y1 = r.y1();

    _qpainter->drawLine(adjust_x(x0), adjust_y(y0), adjust_x(x1), adjust_y(y1));

    // The from point is saved as the last point.
    _last_x = x0;
    _last_y = y0;
}

void SlideRecordsVisitorQPainterDrawer::accept(SlideRecordOffsetVector& r)
{
    int x0 = _last_x + r.dx0(), y0 = _last_y + r.dy0();
    int x1 = _last_x + r.dx1(), y1 = _last_y + r.dy1();

    _qpainter->drawLine(adjust_x(x0), adjust_y(y0), adjust_x(x1), adjust_y(y1));

    // The adjusted from point is saved as the last point.
    _last_x = x0;
    _last_y = y0;
}

void SlideRecordsVisitorQPainterDrawer::accept(SlideRecordCommonEndpoint& r)
{
    // This is a vector starting at the last point.
    int x0 = _last_x,           y0 = _last_y;
    int x1 = _last_x + r.dx0(), y1 = _last_y + r.dy0();

    _qpainter->drawLine(adjust_x(x0), adjust_y(y0), adjust_x(x1), adjust_y(y1));

    // The adjusted to point is saved as the last point.
    _last_x = x1;
    _last_y = y1;
}

void SlideRecordsVisitorQPainterDrawer::accept(SlideRecordSolidFillPolygon& r)
{
    QPolygon *polygon = new QPolygon;
    auto& vs = r.vertices();
    if (vs.size() > 0) {
        auto it = vs.cbegin();
        auto end = vs.cend();

        auto [x0, y0] = *it;
        polygon->append(QPoint(x0, y0));
        ++it;

        for (; it != end; ++it) {
            auto [x1, y1] = *it;
            polygon->append(QPoint(x1, y1));
        }
        _qpainter->drawPolygon(*polygon);
    }
    delete polygon;
}

void SlideRecordsVisitorQPainterDrawer::accept(SlideRecordColor& r)
{
    uint8_t color = r.color();

    if (!_dark && color == 7)
    {
        color = 0;
    }

    RGB rgb = AutoCAD::colors[color];
    _qpainter->setPen(QColor(rgb.red, rgb.green, rgb.blue));
}

void SlideRecordsVisitorQPainterDrawer::accept(SlideRecordEndOfFile&)
{

}

} // namespace libslide
