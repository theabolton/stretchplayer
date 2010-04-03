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
#include <QToolButton>
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
#include <QAction>

#include <cmath>

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
	setWindowFlags( Qt::Window
			| Qt::FramelessWindowHint );

	setAttribute( Qt::WA_TranslucentBackground );
	QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setSizePolicy(policy);


	_setup_color_scheme(0);
	_setup_actions();
	_setup_widgets();
	_setup_layout();
	_setup_signals_and_slots();
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

    void PlayerWidget::locate(float pos)
    {
	_engine->locate(pos * _engine->get_length());
    }

    void PlayerWidget::pitch_inc()
    {
	_engine->set_pitch( _engine->get_pitch() + 1 );
    }

    void PlayerWidget::pitch_dec()
    {
	_engine->set_pitch( _engine->get_pitch() - 1);
    }

    void PlayerWidget::speed_inc()
    {
	_engine->set_stretch( _engine->get_stretch() + .05 );
    }

    void PlayerWidget::speed_dec()
    {
	_engine->set_stretch( _engine->get_stretch() - .05 );
    }

    void PlayerWidget::stretch(int pos)
    {
	_engine->set_stretch( 0.5 + double(pos)/1000.0 );
    }

    void PlayerWidget::volume(int vol)
    {
	_engine->set_volume( _from_fader(vol) );
    }

    /**
     * "Traditional" fader mapping
     */
    float PlayerWidget::_from_fader(int p_val)
    {
	float fader = float(p_val)/1000.0f;
	float gain;
	float db;

	if(fader == 0) {
	    gain = 0.0f;
	} else if(fader < .04) {
	    gain = fader * 1e-6f / .04f;
	} else if(fader < .16) {
	    db = -60.0 + 20.0f * (fader-.04) / .12f;
	    gain = exp10(db/10.0);
	} else if(fader < .52) {
	    db = -40.0 + 10.0f * (fader-.16) / .12f;
	    gain = exp10(db/10.0);
	} else {
	    db = -10.0 + 15.0f * (fader-.52) / .48f;
	    gain = exp10(db/10.0);
	}

	return gain;
    }

    /**
     * "Traditional" fader mapping
     */
    int PlayerWidget::_to_fader(float gain)
    {
	if(gain == 0.0f) return 0;

	float fader;
	float db = 10.0 * log10(gain);

	if(db < -60.0) {
	    fader = gain * .04f / 1e-6f;
	} else if(db < -40.0) {
	    fader = .04f + ((db + 60.0f) * .12f / 20.0f);
	} else if(db < -10.0) {
	    fader = .16f + ((db + 40.0f) * .12f / 10.0f);
	} else {
	    fader = .52f + ((db + 10.0f) * .48f / 15.0f);
	}

	if( fader > 1.0 ) fader = 1.0f;

        return ::round(fader * 1000.0);
    }

    void PlayerWidget::reset()
    {
	stop();
	_engine->set_pitch(0);
	_engine->set_stretch(1.0);
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

	float vol = _engine->get_volume();
	_volume->setValue( _to_fader(vol) );
	_status->volume( _volume->value() / 1000.0 );

	_stretch->setValue( (sch-0.5) * 1000 );
	update();
    }

    void PlayerWidget::paintEvent(QPaintEvent * event)
    {
	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing);

	const QPalette& pal = palette();

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

	QBrush bg_brush( pal.color(QPalette::Active, QPalette::Window) );
	QPen border_pen( pal.color(QPalette::Active, QPalette::Dark) );

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

    void PlayerWidget::_setup_color_scheme(int profile)
    {
	QPalette p;
	QColor base, bright, light, mid, dark;

	switch(profile) {
        // case 0: // default
	case 1: // Blue
	    bright.setRgb(0xff, 0xff, 0xff, 0xff); // white
	    light.setRgb(0x76, 0xc6, 0xf5, 0xff); // light blue
	    mid.setRgb(68, 141, 189, 0xff); // average
	    dark.setRgb(0x12, 0x55, 0x85, 0xff); // dark blue
	    base = light;
	    break;
	default: // Yellow
	    bright.setRgb(0xff, 0xff, 0xff, 0xff); //white
	    light.setRgb(0xe5, 0xd7, 0x3a, 0xff); // yellow
	    mid.setRgb(114, 107, 29, 0xff); // average
	    dark.setRgb(0, 0, 0, 0xff); // black
	    base = light;
	}

	p.setColorGroup(QPalette::Active,
			dark, // Window Text
			light, // button
			light, // light
			dark, // dark
			mid, // mid
			dark, // text
			bright, // bright text
			base, // base
			light // window
	    );

	setPalette(p);
    }

    void PlayerWidget::_setup_actions()
    {
	memset(&_act, 0, sizeof(_act));

	_act.play_pause = new QAction("P", this);
	_act.play_pause->setToolTip("Play/Pause [Space]");
	_act.play_pause->setShortcut(Qt::Key_Space);
	_act.play_pause->setShortcutContext(Qt::ApplicationShortcut);
	addAction(_act.play_pause);
	connect(_act.play_pause, SIGNAL(triggered()),
		this, SLOT(play_pause()));

	_act.stop = new QAction("S", this);
	_act.stop->setToolTip("Stop [S]");
	_act.stop->setShortcut(Qt::Key_S);
	_act.stop->setShortcutContext(Qt::ApplicationShortcut);
	addAction(_act.stop);
	connect(_act.stop, SIGNAL(triggered()),
		this, SLOT(stop()));

	QList<QKeySequence> ab_shortcuts;
	ab_shortcuts << Qt::Key_Enter;
	ab_shortcuts << Qt::Key_Return;
	_act.ab = new QAction("AB", this);
	_act.ab->setToolTip("AB Repeat [Enter]");
	_act.ab->setShortcuts(ab_shortcuts);
	_act.ab->setShortcutContext(Qt::ApplicationShortcut);
	addAction(_act.ab);
	connect(_act.ab, SIGNAL(triggered()),
		this, SLOT(ab()));

	_act.open = new QAction("O", this);
	_act.open->setToolTip("Open [O]");
	_act.open->setShortcut(Qt::Key_O);
	_act.open->setShortcutContext(Qt::ApplicationShortcut);
	addAction(_act.open);
	connect(_act.open, SIGNAL(triggered()),
		this, SLOT(open_file()));

	_act.quit = new QAction("X", this);
	_act.quit->setToolTip("Quit [Esc]");
	_act.quit->setShortcut(Qt::Key_Escape);
	_act.quit->setShortcutContext(Qt::ApplicationShortcut);
	addAction(_act.quit);
	connect(_act.quit, SIGNAL(triggered()),
		this, SLOT(close()));

	QList<QKeySequence> inc_shortcuts;
	inc_shortcuts << Qt::Key_Plus;
	inc_shortcuts << Qt::Key_Equal;
	_act.pitch_inc = new QAction("+", this);
	_act.pitch_inc->setToolTip("Pitch Increase [+]");
	_act.pitch_inc->setShortcuts(inc_shortcuts);
	_act.pitch_inc->setShortcutContext(Qt::ApplicationShortcut);
	addAction(_act.pitch_inc);
	connect(_act.pitch_inc, SIGNAL(triggered()),
		this, SLOT(pitch_inc()));

	_act.pitch_dec = new QAction("-", this);
	_act.pitch_dec->setToolTip("Pitch Decrease [-]");
	_act.pitch_dec->setShortcut(Qt::Key_Minus);
	_act.pitch_dec->setShortcutContext(Qt::ApplicationShortcut);
	addAction(_act.pitch_dec);
	connect(_act.pitch_dec, SIGNAL(triggered()),
		this, SLOT(pitch_dec()));

	_act.speed_inc = new QAction("Faster", this);
	_act.speed_inc->setToolTip("Play faster [Up]");
	_act.speed_inc->setShortcut(Qt::Key_Up);
	_act.speed_inc->setShortcutContext(Qt::ApplicationShortcut);
	addAction(_act.speed_inc);
	connect(_act.speed_inc, SIGNAL(triggered()),
		this, SLOT(speed_inc()));

	_act.speed_dec = new QAction("Slower", this);
	_act.speed_dec->setToolTip("Play slower [Down]");
	_act.speed_dec->setShortcut(Qt::Key_Down);
	_act.speed_dec->setShortcutContext(Qt::ApplicationShortcut);
	addAction(_act.speed_dec);
	connect(_act.speed_dec, SIGNAL(triggered()),
		this, SLOT(speed_dec()));

	_act.reset = new QAction("Reset", this);
	_act.reset->setToolTip("Reset all settings [Home]");
	_act.reset->setShortcut(Qt::Key_Home);
	_act.reset->setShortcutContext(Qt::ApplicationShortcut);
	addAction(_act.reset);
	connect(_act.reset, SIGNAL(triggered()),
		this, SLOT(reset()));
    }

    void PlayerWidget::_setup_widgets()
    {
	_btn.play = new QToolButton(this);
	_btn.play->setDefaultAction(_act.play_pause);
	_btn.play->setAutoRaise(true);

	_btn.stop = new QToolButton(this);
	_btn.stop->setDefaultAction(_act.stop);
	_btn.stop->setAutoRaise(true);

	_btn.ab = new QToolButton(this);
	_btn.ab->setDefaultAction(_act.ab);
	_btn.ab->setAutoRaise(true);

	_btn.open = new QToolButton(this);
	_btn.open->setDefaultAction(_act.open);
	_btn.open->setAutoRaise(true);

	_btn.quit = new QToolButton(this);
	_btn.quit->setDefaultAction(_act.quit);
	_btn.quit->setAutoRaise(true);

	_btn.pitch_inc = new QToolButton(this);
	_btn.pitch_inc->setDefaultAction(_act.pitch_inc);
	_btn.pitch_inc->setAutoRaise(true);

	_btn.pitch_dec = new QToolButton(this);
	_btn.pitch_dec->setDefaultAction(_act.pitch_dec);
	_btn.pitch_dec->setAutoRaise(true);

	_status = new StatusWidget(this, &_sizes);

	_stretch = new QSlider(Qt::Horizontal, this);
	_stretch->setMinimum(0);
	_stretch->setMaximum(1000);

	_volume = new QSlider(Qt::Vertical, this);
	_volume->setMinimum(0);
	_volume->setMaximum(1000);
    }

    void PlayerWidget::_setup_layout()
    {
	_vlay = new QVBoxLayout(this);
	QHBoxLayout *top_hbox = new QHBoxLayout;
	QVBoxLayout *top_right_vbox = new QVBoxLayout;
	QHBoxLayout *hbox_ctl = new QHBoxLayout;

	_vlay->addLayout(top_hbox);
	_vlay->addLayout(hbox_ctl);

	top_hbox->addWidget(_status);
	top_hbox->addLayout(top_right_vbox);

	top_right_vbox->addWidget(_btn.quit);
	top_right_vbox->addWidget(_volume);

	hbox_ctl->addWidget(_btn.play);
	hbox_ctl->addWidget(_btn.stop);
	hbox_ctl->addWidget(_btn.ab);
	hbox_ctl->addWidget(_stretch);
	hbox_ctl->addWidget(_btn.pitch_inc);
	hbox_ctl->addWidget(_btn.pitch_dec);
	hbox_ctl->addWidget(_btn.open);

    }

    void PlayerWidget::_setup_signals_and_slots()
    {
	_engine_callback.reset(new Details::PlayerWidgetMessageCallback(this));
	_engine.reset(new Engine);
	_engine->subscribe_errors(_engine_callback.get());
	_engine->subscribe_messages(_engine_callback.get());

	connect(_stretch, SIGNAL(sliderMoved(int)),
		this, SLOT(stretch(int)));
	connect(_status, SIGNAL(locate(float)),
		this, SLOT(locate(float)));
	connect(_volume, SIGNAL(sliderMoved(int)),
		this, SLOT(volume(int)));

	QTimer* timer = new QTimer(this);
	timer->setSingleShot(false);
	timer->setInterval(200);
	connect(timer, SIGNAL(timeout()),
		this, SLOT(update_time()));
	timer->start();
    }

} // namespace StretchPlayer
