/*
 * Copyright(c) 2009 by Gabriel M. Beddingfield <gabriel@teuton.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "ThinSlider.hpp"

#include <QPainter>
#include <QPen>

namespace StretchPlayer
{

namespace Widgets
{

    ThinSlider::ThinSlider(QWidget* parent) :
	QAbstractSlider(parent),
	_thin(1.0),
	_thick(3.0)
    {
    }

    ThinSlider::~ThinSlider()
    {}

    void ThinSlider::init()
    {
	QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Fixed, QSizePolicy::Slider);
	setSizePolicy(sp);
    }

    QSize ThinSlider::sizeHint() const
    {
	if(orientation() == Qt::Horizontal) {
	    return QSize(85, _thick);
	}
	return QSize(_thick, 85);
    }

    QSize ThinSlider::minimumSizeHint() const
    {
	return sizeHint();
    }

    void ThinSlider::set_line_widths(float thin, float thick)
    {
	if(thin < .1) thin = .1;
	if(thick < .1) thick = .1;

	_thin = thin;
	_thick = thick;
	if( orientation() == Qt::Horizontal ) {
	    setMinimumHeight(_thick);
	} else {
	    setMinimumWidth(_thick);
	}
    }

    void ThinSlider::paintEvent(QPaintEvent * /*event*/)
    {
	QPointF start, end, pos;
	float pos_val;

	pos_val = float(value() - minimum()) / maximum();

	if( orientation() == Qt::Horizontal ) {
	    // All Y coords are the same
	    start.setY( height()/2.0 );
	    end.setY( start.y() );
	    pos.setY( start.y() );

	    start.setX( _thick/2.0 );
	    end.setX( width() - _thick/2.0 );
	    pos.setX( _thick/2.0 + pos_val * (width()-_thick) );
	} else { // Qt::Vertical
	    // All X coords are the same
	    start.setX( width() / 2.0 );
	    end.setX( start.x() );
	    pos.setX( start.x() );

	    start.setY( _thick/2.0 );
	    end.setY( height() - _thick );
	    pos.setY( pos_val * height() );
	}

	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing);
	painter.setBackgroundMode(Qt::TransparentMode);

	QColor thin_color( palette().color(QPalette::Active, QPalette::WindowText) );
	thin_color.setAlphaF(0.25);
	QColor thick_color( palette().color(QPalette::Active, QPalette::WindowText) );

	QPen line_pen( thin_color );
	line_pen.setWidthF(_thin);
	line_pen.setJoinStyle(Qt::RoundJoin);
	line_pen.setCapStyle(Qt::RoundCap);
	painter.setPen(line_pen);
	painter.drawLine(start, end);

	line_pen.setColor(thick_color);
	line_pen.setWidthF(_thick);
	painter.setPen(line_pen);
	painter.drawPoint(start);
	painter.drawLine(start, pos);	
    }


} // namespace Widgets

} // namespace StretchPlayer
