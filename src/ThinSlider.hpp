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
#ifndef THINSLIDER_HPP
#define THINSLIDER_HPP

#include <QAbstractSlider>

class QWidget;

namespace StretchPlayer
{

namespace Widgets
{
    class ThinSlider : public QAbstractSlider
    {
    public:
	ThinSlider(QWidget* parent = 0);
	virtual ~ThinSlider();

	/**
	 * Set the line widths in pixels.  This sets the preferred
	 * widget size... but the widget is still expandable.
	 */
	void set_line_widths(float thin, float thick);

    private:
	void init();
	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const;
	virtual void paintEvent(QPaintEvent *event);
	
	virtual void mousePressEvent(QMouseEvent *ev);
	virtual void mouseMoveEvent(QMouseEvent *ev);
	virtual void mouseReleaseEvent(QMouseEvent *ev);

    private:
	float _thin;
	float _thick;
    };

} // namespace Widgets

} // namespace StretchPlayer

#endif // THINSLIDER_HPP
