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

#include "StatusWidget.hpp"
#include "ThinSlider.hpp"
#include "PlayerSizes.hpp"
#include "Marquee.hpp"

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QFont>
#include <QTimer>
#include <QSpinBox>
#include <QFileDialog>
#include <QPainter>
#include <QBitmap>

namespace StretchPlayer
{

    StatusWidget::StatusWidget(QWidget *parent, PlayerSizes *sizes)
	: QWidget(parent),
	  _sizes(sizes)
    {
	// Using REVERSE colors of parent.
	_update_palette();

	_position = new Widgets::ThinSlider(this);
	_position->setMinimum(0);
	_position->setMaximum(1000);
	_position->setOrientation(Qt::Horizontal);

	_message = new Widgets::Marquee(this);
	_message->set_permanent("I am the very model of a modern major general.  I've information vegetable animal and mineral.");

	QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setSizePolicy(policy);

	connect(_position, SIGNAL(sliderMoved(int)),
		this, SLOT(_changing_position(int)));
    }

    StatusWidget::~StatusWidget()
    {
    }

    void StatusWidget::position(float pos)
    {
	_position->setValue( pos * 1000.0 );
	_position->update();
    }

    void StatusWidget::time(float time)
    {
	int min = (int)(time/60.0);
	float sec = time - min*60.0;
	_time = QString("%1:%2")
	    .arg(int(min), 2, 10, QChar('0'))
	    .arg(double(sec), 4, 'f', 1, QChar('0'));
    }

    void StatusWidget::speed(float val)
    {
	val *= 100.0;
	_speed = QString("SPEED: %1%")
	    .arg(val, 3, 'f', 0);
    }

    void StatusWidget::pitch(int p)
    {
	_pitch = QString("PITCH: %1")
	    .arg(int(p));
    }

    void StatusWidget::volume(float g)
    {
	g *= 100.0;
	_volume = QString("VOL: %1%")
	    .arg(g, 3, 'f', 0, ' ');
    }

    void StatusWidget::cpu(float c)
    {
	c *= 100.0;
	_cpu = QString("CPU: %1%")
	    .arg(c, 3, 'f', 0, ' ');
    }

    void StatusWidget::message(QString msg)
    {
	_message->set_temporary( msg );
    }

    void StatusWidget::song_name(QString msg)
    {
	_message->set_permanent( msg );
    }

    void StatusWidget::_changing_position(int pos)
    {
	float p = float(pos) / 1000.0;
	emit locate(p);
    }

    void StatusWidget::resizeEvent(QResizeEvent * /*event*/)
    {
	float w, h, margin, radius;

	w = width();
	h = height();
	radius = _sizes->thicker_line() * 2.0;
	margin = radius;

	_bg_zone.setRect( 0, 0, w, h );

	_position->set_line_widths( _sizes->thin_line(), _sizes->thicker_line() );
	QSize pos_sz = _position->sizeHint();
	_position->setGeometry( margin,
				h - margin - pos_sz.height(),
				w - 2*margin,
				pos_sz.height() );

	// Since that's been drawn... discout it from the
	// height and calculate all the text.
	h -= pos_sz.height();

	_message_zone.setRect( margin, h - h/5 - margin, w-2*margin, h/5 );
	_message->setGeometry(_message_zone);

	h -= _message_zone.height();

	_time_zone.setRect( margin, margin, w*2/3, _message_zone.y() );
	_stats_zone.setRect( _time_zone.right() + 2*margin,
			     _time_zone.y(),
			     w - 3*margin - _time_zone.right(),
			     _time_zone.height() );

	// Size the fonts...
	_large_font.setPixelSize( _time_zone.height() * 9 / 10 );
	_large_font.setStretch( 100 );
	_small_font.setPixelSize( _stats_zone.height() * 7 / 10 / 4 );
	_small_font.setStretch( 100 );
	_small_font.setWeight(QFont::Bold);

	QFontMetrics large_m( _large_font );
	QFontMetrics small_m( _small_font );

	int stretch;
	stretch = 100.0 * _time_zone.width() / large_m.width("99:99.9");
	_large_font.setStretch(stretch);
	stretch = 100.0 * _stats_zone.width() / small_m.width(".SPEED: 100%");
	_small_font.setStretch(stretch);

	_message_font = _small_font;
	_message_font.setStretch(100);
	_message->set_font(_message_font);
    }

    void StatusWidget::paintEvent(QPaintEvent * /*event*/)
    {
	// Using REVERSE colors of parent.
	_update_palette();

	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing);
	painter.setBackgroundMode(Qt::TransparentMode);

	QBrush brush( palette().color(QPalette::Active, QPalette::Window ) );
	QPen pen( palette().color(QPalette::Active, QPalette::Dark) );

	int radius = _sizes->thicker_line() * 2.0;

	painter.setBrush(brush);
	painter.setPen(pen);
	painter.drawRoundedRect( _bg_zone, radius, radius );

        // Change pen color to draw text.
	pen.setColor( palette().color(QPalette::Active, QPalette::WindowText) );
	painter.setPen(pen);
	brush.setColor( palette().color(QPalette::Active, QPalette::WindowText) );
	painter.setBrush(brush);

	// Audit the font sizes.  Only change them if there is a problem.
	QFontMetrics large_m( _large_font );
	QFontMetrics small_m( _small_font );

	int stretch;
	int twid;
	twid = large_m.width(_time);
	if( twid > 0 ) {
	    stretch = 100.0 * _time_zone.width() / large_m.width(_time);
	    if(stretch < _large_font.stretch() && stretch > 10 ) {
		_large_font.setStretch(stretch);
	    }
	}
	twid = small_m.width(_speed);
	if( twid > 0 ) {
	    stretch = 100.0 * _stats_zone.width() / small_m.width(_speed);
	    if(stretch < _small_font.stretch() && stretch > 10 ) {
		_small_font.setStretch(stretch);
	    }
	}

	painter.setFont(_large_font);
	painter.drawText(_time_zone, _time);

	painter.setFont(_small_font);
	painter.setRenderHints(QPainter::TextAntialiasing, false);
	QRect stat = _stats_zone;
	stat.setHeight( stat.height() / 4 );
	painter.drawText(stat, _speed);
	stat.moveTo( stat.x(), stat.bottom() );
	painter.drawText(stat, _pitch);
	stat.moveTo( stat.x(), stat.bottom() );
	painter.drawText(stat, _cpu);
	stat.moveTo( stat.x(), stat.bottom() );
	painter.drawText(stat, _volume);

	_message->update();
	//painter.setFont(_message_font);
	//painter.drawText(_message_zone, _message);
    }

    void StatusWidget::_update_palette()
    {
	QPalette p(parentWidget()->palette());

	QColor base, bright, light, mid, dark;
	base = p.color(QPalette::Active, QPalette::Dark);
	bright = p.color(QPalette::Active, QPalette::BrightText);
	light = p.color(QPalette::Active, QPalette::Light);
	mid = p.color(QPalette::Active, QPalette::Mid);
	dark = p.color(QPalette::Active, QPalette::Dark);

	p.setColorGroup( QPalette::Active,
			 bright, // Window Text
			 dark, // Button
			 light, // light
			 dark, // dark
			 mid, // mid
			 light, // text
			 light, // bright text
			 base, // base
			 dark // window
	    );
	setPalette(p);
	
    }

} // namespace StretchPlayer
