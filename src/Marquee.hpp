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
#ifndef MARQUEE_HPP
#define MARQUEE_HPP

#include <QWidget>
#include <QString>
#include <QRect>
#include <QTimer>
#include <memory>

class QImage;

namespace StretchPlayer
{

namespace Widgets
{
    class Marquee : public QWidget
    {
	Q_OBJECT
    public:
	Marquee(QWidget* parent = 0);
	virtual ~Marquee();

	void set_font(const QFont& font);

    public slots:
	void set_temporary(QString);
	void set_permanent(QString);

    private:
	virtual void resizeEvent(QResizeEvent *event);
	virtual void paintEvent(QPaintEvent *event);

    private slots:
	void _scroll_incr();
	void _wait_over();

    private:
	void _draw_text(const QString& txt);

    private:
	QString _temporary;
	QString _permanent;
	std::auto_ptr<QPixmap> _canvas;
	QTimer _scroll_timer;
	bool _scroll;
	QTimer _wait_timer;
	int _pos;
	int _incr;
	int _show_temporary;
	const int _show_temporary_first;
	QFont _font;
    };

} // namespace Widgets

} // namespace StretchPlayer

#endif // MARQUEE_HPP
