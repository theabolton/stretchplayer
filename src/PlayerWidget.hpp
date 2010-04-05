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
#ifndef PLAYERWIDGET_HPP
#define PLAYERWIDGET_HPP

#include <QMainWindow>
#include <memory>
#include <QIcon>
#include "PlayerSizes.hpp"

class QToolButton;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;
class QSlider;
class QSpinBox;
class QPaintEvent;
class QStyle;

namespace StretchPlayer
{

class Engine;
class EngineMessageCallback;
class StatusWidget;

class PlayerWidget : public QMainWindow
{
    Q_OBJECT
public:
    PlayerWidget(QWidget *parent = 0);
    ~PlayerWidget();

    void load_song(const QString& filename);
    virtual int heightForWidth(int w);

public slots:
    void play_pause();
    void stop();
    void ab();
    void open_file();
    void update_time();
    void locate(float); // [0.0, 1.0]
    void stretch(int);
    void volume(int);
    void pitch_inc();
    void pitch_dec();
    void speed_inc();
    void speed_dec();
    void status_message(const QString&);
    void reset();

protected:
    virtual void resizeEvent(QResizeEvent* event);
    virtual void paintEvent(QPaintEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    //virtual void mouseReleaseEvent(QMouseEvent* event);

private:
    void _setup_color_scheme(int profile);
    void _load_icons();
    void _setup_actions();
    void _setup_widgets();
    void _layout_widgets();
    void _setup_signals_and_slots();
    Qt::CursorShape _which_cursor(const QPoint& pos);
    void _drag_resize(Qt::CursorShape cur, QMouseEvent* event);

    // Encode/decode volume fader
    float _from_fader(int val);
    int _to_fader(float val);

    float _margin();

private:
    struct icons_t {
	QIcon play;
	QIcon stop;
	QIcon ab;
	QIcon help;
	QIcon quit;
	QIcon plus;
	QIcon minus;
	QIcon open;
    } _ico;

    struct actions_t {
	QAction *play_pause;
	QAction *stop;
	QAction *ab;
	QAction *open;
	QAction *quit;
	QAction *pitch_inc;
	QAction *pitch_dec;
	QAction *speed_inc;
	QAction *speed_dec;
	QAction *reset;
    } _act;

    struct buttons_t {
	QToolButton *play;
	QToolButton *stop;
	QToolButton *ab;
	QToolButton *open;
	QToolButton *quit;
	QToolButton *pitch_inc;
	QToolButton *pitch_dec;
    } _btn;

    // Misc widgets
    QStyle *_style;
    StatusWidget *_status;
    QSlider *_stretch;
    QSlider *_volume;
    PlayerSizes _sizes;

    std::auto_ptr<Engine> _engine;
    std::auto_ptr<EngineMessageCallback> _engine_callback;

    // State variables
    QPoint _anchor; // for window moves

}; // PlayerWidget

} // namespace StretchPlayer

#endif // PLAYERWIDGET_HPP
