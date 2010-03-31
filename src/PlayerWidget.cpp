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

#include <QWidget>
#include <QPushButton>
#include <QLCDNumber>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>

namespace StretchPlayer
{
    PlayerWidget::PlayerWidget(QWidget *parent)
	: QWidget(parent)
    {
	_vbox = new QVBoxLayout;
	_hbox = new QHBoxLayout;

	_location = new QLCDNumber(10, this);
	_slider = new QSlider(Qt::Horizontal, this);
	_play = new QPushButton(this);

	_location->display("00:00:00.0");
	_play->setText("P");

	_vbox->addWidget(_location);
	_vbox->addWidget(_slider);
	_vbox->addLayout(_hbox);

	_hbox->addWidget(_play);
	_hbox->addStretch();

	setLayout(_vbox);
    }

    PlayerWidget::~PlayerWidget()
    {
    }

} // namespace StretchPlayer
