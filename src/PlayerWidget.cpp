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

#include "PlayerWidget.hpp"
#include "Engine.hpp"

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QFont>
#include <QTimer>

namespace StretchPlayer
{
    PlayerWidget::PlayerWidget(QWidget *parent)
	: QWidget(parent)
    {
	_vbox = new QVBoxLayout;
	_hbox = new QHBoxLayout;

	_location = new QLabel(this);
	_slider = new QSlider(Qt::Horizontal, this);
	_stretch = new QSlider(Qt::Horizontal, this);
	_play = new QPushButton(this);

	QFont font = _location->font();
	font.setPointSize(32);
	_location->setFont(font);
	_location->setText("00:00:00.0");
	_location->setScaledContents(true);
	_play->setText("P");

	_slider->setMinimum(0);
	_slider->setMaximum(1000);

	_vbox->addWidget(_location);
	_vbox->addWidget(_slider);
	_vbox->addLayout(_hbox);

	_hbox->addWidget(_play);
	_hbox->addStretch();
	_hbox->addWidget(_stretch);

	setLayout(_vbox);

	_engine.reset(new Engine);

	connect(_play, SIGNAL(clicked()),
		this, SLOT(play()));
	connect(_slider, SIGNAL(sliderMoved(int)),
		this, SLOT(locate(int)));
	QTimer* timer = new QTimer(this);
	timer->setSingleShot(false);
	timer->setInterval(60);
	connect(timer, SIGNAL(timeout()),
		this, SLOT(update_time()));
	timer->start();
    }

    PlayerWidget::~PlayerWidget()
    {
    }

    void PlayerWidget::load_song(const QString& filename)
    {
	_engine->load_song(filename);
    }

    void PlayerWidget::play()
    {
	_engine->play();
	disconnect(_play, SIGNAL(clicked()),
		   this, SLOT(play()));
	connect(_play, SIGNAL(clicked()),
		   this, SLOT(stop()));
    }

    void PlayerWidget::stop()
    {
	_engine->stop();
	disconnect(_play, SIGNAL(clicked()),
		   this, SLOT(stop()));
	connect(_play, SIGNAL(clicked()),
		   this, SLOT(play()));
    }

    void PlayerWidget::locate(int pos)
    {
	double frac = double(pos) / 1000.0;
	double s = frac * _engine->get_length();
	_engine->locate(s);
    }

    void PlayerWidget::update_time()
    {
	float pos = _engine->get_position();
	float len = _engine->get_length();
	_location->setText(QString("%1").arg(pos));
	if( len > 0 ) {
	    float prog = 1000.0 * pos / len;
	    _slider->setValue( prog );
	} else {
	    _slider->setValue(0);
	}
	update();
    }

} // namespace StretchPlayer
