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
#ifndef STATUS_HPP
#define STATUS_HPP

#include <QWidget>
#include <memory>
#include <QString>
#include <QTimer>

class QPushButton;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;
class Slider;
class QSpinBox;
class QPaintEvent;

namespace StretchPlayer
{

class PlayerSizes;

    namespace Widgets
    {
	class ThinSlider;
    }

class StatusWidget : public QWidget
{
    Q_OBJECT
public:
    StatusWidget(QWidget *parent, PlayerSizes *sizes);
    ~StatusWidget();

public slots:
    void position(float);
    void time(float);
    void speed(float);
    void pitch(int);
    void volume(float);
    void cpu(float);
    void message(QString);
    void clear_message();

signals:
    void locate(float); // [0.0, 1.0]

private slots:
    void _changing_position(int);

private:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void paintEvent(QPaintEvent *event);
    void _update_palette();

private:
    QString _time;
    QString _speed;
    QString _pitch;
    QString _volume;
    QString _cpu;
    QString _status;
    QTimer _status_scroll_timer;

    QFont _large_font;
    QFont _small_font;
    QFont _message_font;

    QRect _bg_zone;
    QRect _time_zone;
    QRect _stats_zone;
    QRect _message_zone;

    Widgets::ThinSlider *_position;
    PlayerSizes *_sizes;

}; // StatusWidget

} // namespace StretchPlayer

#endif // PLAYERWIDGET_HPP
