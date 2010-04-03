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
#ifndef PLAYERCOLORS_HPP
#define PLAYERCOLORS_HPP

#include <QColor>

namespace StretchPlayer
{

/**
 * \brief Class that manages basic widget colors.
 */
class PlayerColors
{
public:
    PlayerColors(int profile = 0);
    ~PlayerColors();

    void set_profile(int prof);
    const QColor& border() const { return _border; }
    const QColor& background() const { return _background; }
    const QColor& foreground() const { return _foreground; }

private:
    QColor _border;
    QColor _background;
    QColor _foreground;
};

} // namespace StretchPlayer

#endif // PLAYERCOLORS_HPP
