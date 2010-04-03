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

#include "PlayerSizes.hpp"

namespace StretchPlayer
{

    float PlayerSizes::scale()
    {
	return _scale;
    }

    void PlayerSizes::scale(float val)
    {
	if( val < .1 ) {
	    _scale = .1;
	} else {
	    _scale = val;
	}
    }    

    float PlayerSizes::ppi_setting()
    {
	return _ppi;
    }

    void PlayerSizes::ppi_setting(float val)
    {
	if( val < 60 ) {
	    _ppi = 60;
	} else {
	    _ppi = val;
	}
    }

    float PlayerSizes::widget_grid_size()
    {
	return _grid * _ppi * _scale;
    }

    void PlayerSizes::widget_grid_size(float inches)
    {
	if( inches < .09 ) {
	    _grid = .09;
	} else {
	    _grid = inches;
	}
    }

    float PlayerSizes::thicker_line()
    {
	return line() * 2.0;
    }

    float PlayerSizes::thick_line()
    {
	return line() * 1.5;
    }

    float PlayerSizes::line() 
    {
	return widget_grid_size() / 16.0;
    }

    float PlayerSizes::thin_line()
    {
	return line() / 2.0;
    }

} // namespace StretchPlayer

