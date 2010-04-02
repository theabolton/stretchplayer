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
#include <QSpinBox>
#include <QFileDialog>
#include <QPainter>
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
	: QWidget(parent)
    {
	QVBoxLayout *vbox = new QVBoxLayout;
	QHBoxLayout *hbox_ctl = new QHBoxLayout;
	QHBoxLayout *hbox_stretch = new QHBoxLayout;

	_location = new QLabel(this);
	_position = new QSlider(Qt::Horizontal, this);
	_stretch = new QSlider(Qt::Horizontal, this);
	_play = new QPushButton(this);
	_stop = new QPushButton(this);
	_ab = new QPushButton(this);
	_open = new QPushButton(this);
	_pitch = new QSpinBox(this);
	_status = new QLabel(this);

	QFont font = _location->font();
	font.setPointSize(32);
	_location->setFont(font);
	_location->setText("00:00:00.0");
	_location->setScaledContents(true);
	_play->setText("P");
	_stop->setText("S");
	_ab->setText("AB");
	_open->setText("O");
	_status->setWordWrap(true);

	_position->setMinimum(0);
	_position->setMaximum(1000);
	_stretch->setMinimum(0);
	_stretch->setMaximum(1000);
	_pitch->setMinimum(-12);
	_pitch->setMaximum(12);

	vbox->addWidget(_location);
	vbox->addWidget(_position);
	vbox->addLayout(hbox_ctl);
	vbox->addLayout(hbox_stretch);
	vbox->addWidget(_status);

	hbox_ctl->addWidget(_play);
	hbox_ctl->addWidget(_stop);
	hbox_ctl->addWidget(_ab);
	hbox_ctl->addStretch();
	hbox_ctl->addWidget(_open);

	hbox_stretch->addWidget(_stretch);
	hbox_stretch->addWidget(_pitch);

	setLayout(vbox);

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
	connect(_position, SIGNAL(sliderMoved(int)),
		this, SLOT(locate(int)));
	connect(_stretch, SIGNAL(sliderMoved(int)),
		this, SLOT(stretch(int)));
	connect(_pitch, SIGNAL(valueChanged(int)),
		this, SLOT(pitch(int)));
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
	_status->setText(msg);
	QTimer::singleShot(10000, _status, SLOT(clear()));
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
	float len = _engine->get_length();
	float sch = _engine->get_stretch();
	int pit = _engine->get_pitch();

	int min = (int)(pos/60.0);
	float sec = pos - min*60.0;
	_location->setText(QString("%1:%2")
			   .arg(int(min), 2, 10, QChar('0'))
			   .arg(double(sec), 4, 'f', 1, QChar('0'))
	    );

	if( len > 0 ) {
	    float prog = 1000.0 * pos / len;
	    _position->setValue( prog );
	} else {
	    _position->setValue(0);
	}
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
	float w = 420.0;
	float h = 175.0;
	float thickline = 5.0;
	float border_rad = 20.0;

	float scale = width() / w;

	w = width();
	h *= scale;
	thickline *= scale;
	border_rad *= scale;

	QBrush bg_brush( QColor(0xe5, 0xd7, 0x3a) );
	QPen border_pen( QColor(0, 0, 0) );

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
