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
#include "StatusWidget.hpp"

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
#include <iostream>

namespace StretchPlayer
{
    namespace Details
    {
	class PlayerWidgetMessageCallback : public EngineMessageCallback
	{
	public:
	    PlayerWidgetMessageCallback(PlayerWidget* w) : _widget(w)
		{}
	    virtual ~PlayerWidgetMessageCallback() {}

	    virtual void operator()(const QString& msg) {
		_widget->status_message(msg);
	    }
	private:
	    PlayerWidget* _widget;
	};

    }

    PlayerWidget::PlayerWidget(QWidget *parent)
	: QWidget(parent),
	  _colors(1)
    {
	setWindowFlags( Qt::Window
			| Qt::FramelessWindowHint );

	setAttribute( Qt::WA_TranslucentBackground );

	_vlay = new QVBoxLayout(this);
	QHBoxLayout *top_hbox = new QHBoxLayout;
	QVBoxLayout *top_right_vbox = new QVBoxLayout;
	QHBoxLayout *hbox_ctl = new QHBoxLayout;

	_status = new StatusWidget(this, &_sizes);

	_stretch = new QSlider(Qt::Horizontal, this);
	_play = new QPushButton(this);
	_stop = new QPushButton(this);
	_ab = new QPushButton(this);
	_open = new QPushButton(this);
	_pitch = new QSpinBox(this);
	_quit = new QPushButton(this);

	_play->setText("P");
	_stop->setText("S");
	_ab->setText("AB");
	_open->setText("O");
	_quit->setText("X");

	_stretch->setMinimum(0);
	_stretch->setMaximum(1000);
	_pitch->setMinimum(-12);
	_pitch->setMaximum(12);

	_vlay->addLayout(top_hbox);
	_vlay->addLayout(hbox_ctl);

	top_hbox->addWidget(_status);
	top_hbox->addLayout(top_right_vbox);

	top_right_vbox->addWidget(_quit);
	top_right_vbox->addWidget(new QSlider(Qt::Vertical, this));

	hbox_ctl->addWidget(_play);
	hbox_ctl->addWidget(_stop);
	hbox_ctl->addWidget(_ab);
	hbox_ctl->addWidget(_stretch);
	hbox_ctl->addWidget(_pitch);
	hbox_ctl->addWidget(_open);

	QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	policy.setHeightForWidth(true);
	setSizePolicy(policy);

	_engine_callback.reset(new Details::PlayerWidgetMessageCallback(this));
	_engine.reset(new Engine);
	_engine->subscribe_errors(_engine_callback.get());
	_engine->subscribe_messages(_engine_callback.get());

	connect(_play, SIGNAL(clicked()),
		this, SLOT(play_pause()));
	connect(_stop, SIGNAL(clicked()),
		this, SLOT(stop()));
	connect(_ab, SIGNAL(clicked()),
		this, SLOT(ab()));
	connect(_open, SIGNAL(clicked()),
		this, SLOT(open_file()));
	connect(_stretch, SIGNAL(sliderMoved(int)),
		this, SLOT(stretch(int)));
	connect(_pitch, SIGNAL(valueChanged(int)),
		this, SLOT(pitch(int)));
	connect(_quit, SIGNAL(clicked()),
		this, SLOT(close()));

	QTimer* timer = new QTimer(this);
	timer->setSingleShot(false);
	timer->setInterval(200);
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

    void PlayerWidget::play_pause()
    {
	_engine->play_pause();
    }

    void PlayerWidget::stop()
    {
	_engine->stop();
	_engine->locate(0);
    }

    void PlayerWidget::ab()
    {
	_engine->loop_ab();
    }

    void PlayerWidget::open_file()
    {
	_engine->stop();
	QString filename = QFileDialog::getOpenFileName(
	    this,
	    "Open song file..."
	    );
	if( ! filename.isNull() ) {
	    load_song(filename);
	}	
    }

    void PlayerWidget::status_message(const QString& msg) {
	_status->message(msg);
    }

    void PlayerWidget::locate(int pos)
    {
	double frac = double(pos) / 1000.0;
	double s = frac * _engine->get_length();
	_engine->locate(s);
    }

    void PlayerWidget::pitch(int pitch)
    {
	_engine->set_pitch(pitch);
    }

    void PlayerWidget::stretch(int pos)
    {
	_engine->set_stretch( 0.5 + double(pos)/1000.0 );
    }

    void PlayerWidget::update_time()
    {
	float pos = _engine->get_position();
	_status->time(pos);

	float len = _engine->get_length();
	_status->position(pos/len);

	float sch = _engine->get_stretch();
	_status->speed(sch);

	int pit = _engine->get_pitch();
	_status->pitch(pit);

	float cpu = _engine->get_cpu_load();
	_status->cpu(cpu);

	_stretch->setValue( (sch-0.5) * 1000 );
	_pitch->setValue( pit );
	update();
    }

    int PlayerWidget::heightForWidth(int w) const
    {
	return w * 175.0 / 420.0;
    }

    void PlayerWidget::paintEvent(QPaintEvent * event)
    {
	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing);

	float scale = width()/450.0;
	_sizes.scale(scale);

	float thickline = _sizes.thicker_line();
	float border_rad = thickline * 4.0;
	float margin = thickline * 2.5;

	float w = width();
	float h = height();

	_vlay->setContentsMargins(margin, margin, margin, margin);

	QImage mask_img(width(), height(), QImage::Format_Mono);
	mask_img.fill(0xff);
	QPainter mask_ptr(&mask_img);
	mask_ptr.setBrush( QBrush( QColor(0, 0, 0) ) );
	mask_ptr.drawRoundedRect( QRectF( 0, 0, w, h),
				  border_rad+thickline/2.0,
				  border_rad+thickline/2.0 );
	QBitmap bmp = QBitmap::fromImage(mask_img);
	setMask( bmp );

	QBrush bg_brush( _colors.background() );
	QPen border_pen( _colors.border() );

	border_pen.setWidthF(thickline);
	border_pen.setJoinStyle(Qt::RoundJoin);
	painter.setBrush(bg_brush);
	painter.setPen(border_pen);
	painter.drawRoundedRect( QRectF( thickline/2.0,
					 thickline/2.0,
					 w-thickline,
					 h-thickline ),
				 border_rad,
				 border_rad );

	QWidget::paintEvent(event);
    }

} // namespace StretchPlayer
