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
#ifndef PLAYERSIZES_HPP
#define PLAYERSIZES_HPP

namespace StretchPlayer
{

/**
 * \brief Class that manages basic widget sizes.
 */
class PlayerSizes
{
public:
    float scale();
    void scale(float val);

    float ppi_setting();
    void ppi_setting(float val);

    /**
     * Returns number of pixels for grid size.
     */
    float widget_grid_size();
    void widget_grid_size(float inches);

    float text_size();
    void text_size(float inches_tall);

    float thicker_line();
    float thick_line();
    float line();
    float thin_line();

private:
    float _scale; //< Raw scaling factor
    float _grid;  //< inches
    float _text;  //< inches, tall
    float _ppi;   //< pixels per inch

public:
    PlayerSizes() :
	_scale(1.0),
	_grid(.375),
	_text(.125),
	_ppi(120.)
	{}

    ~PlayerSizes() {}
};

} // namespace StretchPlayer

#endif // PLAYERSIZES_HPP
