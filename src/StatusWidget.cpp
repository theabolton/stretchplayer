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
	_vlay = new QVBoxLayout(this);
	QHBoxLayout *top_lay = new QHBoxLayout;
	QVBoxLayout *top_right_lay = new QVBoxLayout;

	_time = new QLabel(this);
	_speed = new QLabel(this);
	_pitch = new QLabel(this);
	_volume = new QLabel(this);
	_cpu = new QLabel(this);
	_status = new QLabel(this);
	_position = new Widgets::ThinSlider(this);

	QFont font = _time->font();
	font.setPointSize(32);
	_time->setFont(font);
	_time->setText("00:00:00.0");
	_time->setScaledContents(true);

	_speed->setText("SPEED: 100%");
	_speed->setAlignment(Qt::AlignRight);
	_pitch->setText("PITCH:    0");
	_pitch->setAlignment(Qt::AlignRight);
	_volume->setText("VOL: 100%");
	_volume->setAlignment(Qt::AlignRight);
	_cpu->setText("CPU:   0%");
	_cpu->setAlignment(Qt::AlignRight);

	_position->setMinimum(0);
	_position->setMaximum(1000);
	_position->setOrientation(Qt::Horizontal);

	_status->setWordWrap(true);

	_vlay->addLayout(top_lay);
	_vlay->addWidget(_status);
	_vlay->addWidget(_position);

	top_lay->addWidget(_time);
	top_lay->addLayout(top_right_lay);

	top_right_lay->addWidget(_speed);
	top_right_lay->addWidget(_pitch);
	top_right_lay->addWidget(_volume);
	top_right_lay->addWidget(_cpu);
	top_right_lay->addStretch();

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
	_time->setText(QString("%1:%2")
		       .arg(int(min), 2, 10, QChar('0'))
		       .arg(double(sec), 4, 'f', 1, QChar('0'))
	    );
    }

    void StatusWidget::speed(float val)
    {
	val *= 100.0;
	_speed->setText( QString("SPEED: %1%")
			 .arg(val, 3, 'f', 0) );
    }

    void StatusWidget::pitch(int p)
    {
	_pitch->setText( QString("PITCH: %1")
			 .arg(int(p)) );
    }

    void StatusWidget::volume(float g)
    {
	g *= 100.0;
	_volume->setText( QString("VOL: %1%")
			  .arg(g, 3, 'f', 0) );
    }

    void StatusWidget::cpu(float c)
    {
	c *= 100.0;
	_cpu->setText( QString("CPU: %1%")
		       .arg(c, 3, 'f', 0) );
    }

    void StatusWidget::message(QString msg)
    {
	_status->setText( msg );
	QTimer::singleShot(10000, _status, SLOT(clear()));
    }

    void StatusWidget::_changing_position(int pos)
    {
	float p = float(pos) / 1000.0;
	emit locate(p);
    }

    void StatusWidget::paintEvent(QPaintEvent *event)
    {
	// Using REVERSE colors of parent.
	_update_palette();

	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing);
	painter.setBackgroundMode(Qt::TransparentMode);

	QBrush brush( palette().color(QPalette::Active, QPalette::Window ) );
	QPen pen( palette().color(QPalette::Active, QPalette::Dark) );

	float x, y, w, h;
	float radius, margin;

	x = 0.0;
	y = 0.0;
	w = width();
	h = height();
	radius = _sizes->thicker_line() * 2.0;
	margin = radius * .75;

	_vlay->setContentsMargins(margin, margin, margin, margin);
	painter.setBrush(brush);
	painter.setPen(pen);
	painter.drawRoundedRect( x, y, w, h, radius, radius );

	QWidget::paintEvent(event);
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
