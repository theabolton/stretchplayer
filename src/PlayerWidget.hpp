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

#include <QWidget>
#include <memory>

class QPushButton;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;
class QSlider;
class QSpinBox;

namespace StretchPlayer
{

class Engine;

class PlayerWidget : public QWidget
{
    Q_OBJECT
public:
    PlayerWidget(QWidget *parent = 0);
    ~PlayerWidget();

    void load_song(const QString& filename);

public slots:
    void play();
    void stop();
    void update_time();
    void locate(int);
    void stretch(int);
    void pitch(int);

private:
    QVBoxLayout *_vbox;
    QHBoxLayout *_hbox;
    QLabel *_location;
    QSlider *_slider;
    QSlider *_stretch;
    QPushButton *_play;
    QSpinBox *_pitch;

    std::auto_ptr<Engine> _engine;

}; // PlayerWidget

} // namespace StretchPlayer

#endif // PLAYERWIDGET_HPP
