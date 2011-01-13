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

#include "Marquee.hpp"

#include <QPainter>
#include <QPen>
#include <QMouseEvent>

namespace StretchPlayer
{

namespace Widgets
{

    Marquee::Marquee(QWidget* parent) :
	QWidget(parent),
	_pos(0),
	_incr(5),
	_show_temporary(0),
	_show_temporary_first(3)
    {
	_scroll_timer.setInterval(42);
	_scroll_timer.setSingleShot(false);
	_wait_timer.setInterval(1500);
	_wait_timer.setSingleShot(true);

	connect( &_scroll_timer, SIGNAL(timeout()),
		 this, SLOT(_scroll_incr()) );
	connect( &_wait_timer, SIGNAL(timeout()),
		 this, SLOT(_wait_over()) );

	_draw_text("");
	_scroll = false;
    }

    Marquee::~Marquee()
    {
	_scroll_timer.stop();
	_wait_timer.stop();
    }

    void Marquee::set_font(const QFont& font) {
	_font = font;
    }

    void Marquee::set_temporary(QString txt) {
	_scroll_timer.stop();
	_temporary = txt;
	_show_temporary = _show_temporary_first;
	_draw_text(txt);
	_wait_timer.start();
    }

    void Marquee::set_permanent(QString txt) {
	_permanent = txt;
	if(_show_temporary == 0) {
	    _draw_text(txt);
	}
    }

    void Marquee::_scroll_incr()
    {
	_pos += _incr;
	if(_pos > _canvas->width()) {
	    _scroll_timer.stop();
	    _pos = 0;
	    _wait_timer.start(1500);
	}
    }

    void Marquee::_wait_over()
    {
	if( _show_temporary == 1 ) {
	    _draw_text(_permanent);
	    _pos = 0;
	    _show_temporary = 0;
	} else if( _show_temporary > 0 ) {
	    --_show_temporary;
	}
	if(_scroll == false) {
	    _draw_text(_permanent);
	    _pos = 0;
	    _show_temporary = 0;
	    return;
	} else {
	    _scroll_timer.start();
	}
    }

    void Marquee::_draw_text(const QString& txt)
    {
	QFontMetrics fm(_font);
	QSize size = fm.size(0, txt);
	int w = size.width();
	int h = height();
	_incr = fm.maxWidth() / 8;
	if( _incr <= 0 ) _incr = 1;

	if( width() > w ) {
	    w = width();
	    _scroll = false;
	    _scroll_timer.stop();
	    _wait_timer.start(3000);
	    _pos = 0;
	} else {
	    // w = w
	    _scroll = true;
	    _scroll_timer.stop();
	    _wait_timer.start(1500);
	    _pos = 0;
	}

	_canvas.reset(new QPixmap(w*6/5, h) );
	QPainter painter(_canvas.get());

	painter.setBrush( palette().color(QPalette::Active, QPalette::Window) );
	painter.setPen( palette().color(QPalette::Active, QPalette::Window) );
	painter.drawRect( 0, 0, _canvas->width(), _canvas->height() );

	painter.setBrush( palette().color(QPalette::Active, QPalette::WindowText) );
	painter.setPen( palette().color(QPalette::Active, QPalette::WindowText) );
	painter.setFont(_font);

	painter.drawText( 0, 0, w, h, Qt::TextSingleLine, txt );
    }

    void Marquee::resizeEvent(QResizeEvent * /*event*/)
    {
	if( _show_temporary > 0 ) {
	    _draw_text( _temporary );
	} else {
	    _draw_text( _permanent );
	}
	if( _pos > _canvas->width() ) _pos = 0;
    }

    void Marquee::paintEvent(QPaintEvent * /*event*/)
    {
	QRect viewport(0, 0, width(), height());
	QRect target;
	viewport.translate(_pos, 0);
	int clipped = 0;

	if( viewport.right() > _canvas->width() ) {
	    clipped = viewport.right() - _canvas->width();
	    viewport.setRight( _canvas->width() );
	}
	target = viewport;
	target.moveTo(0, 0);

	QPainter painter(this);
	painter.drawPixmap(target, *_canvas, viewport);
	if(clipped) {
	    viewport.setRect(0, 0, clipped, height());
	    target.translate( target.width(), 0 );
	    target.setWidth( clipped );
	    painter.drawPixmap(target, *_canvas, viewport);
	}
    }


} // namespace Widgets

} // namespace StretchPlayer
