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

class QPushButton;
class QLCDNumber;
class QVBoxLayout;
class QHBoxLayout;
class QSlider;

namespace StretchPlayer
{

class PlayerWidget : public QWidget
{
    Q_OBJECT
public:
    PlayerWidget(QWidget *parent = 0);
    ~PlayerWidget();

private:
    QVBoxLayout *_vbox;
    QHBoxLayout *_hbox;
    QLCDNumber *_location;
    QSlider *_slider;
    QPushButton *_play;

}; // PlayerWidget

} // namespace StretchPlayer

#endif // PLAYERWIDGET_HPP
