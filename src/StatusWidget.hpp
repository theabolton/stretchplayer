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
class PlayerColors;

    namespace Widgets
    {
	class ThinSlider;
    }

class StatusWidget : public QWidget
{
    Q_OBJECT
public:
    StatusWidget(QWidget *parent, PlayerSizes *sizes, PlayerColors *colors);
    ~StatusWidget();

public slots:
    void position(float);
    void time(float);
    void speed(float);
    void pitch(int);
    void volume(float);
    void cpu(float);
    void message(QString);

signals:
    void locate(float);

private slots:
    void _changing_position(int);

private:
    virtual void paintEvent(QPaintEvent *event);

private:
    QLabel *_time;
    QLabel *_speed;
    QLabel *_pitch;
    QLabel *_volume;
    QLabel *_cpu;
    QLabel *_status;
    Widgets::ThinSlider *_position;
    PlayerSizes *_sizes;
    PlayerColors *_colors;

}; // StatusWidget

} // namespace StretchPlayer

#endif // PLAYERWIDGET_HPP
