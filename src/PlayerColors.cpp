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

#include "PlayerColors.hpp"

namespace StretchPlayer
{

    PlayerColors::PlayerColors(int profile)
    {
	set_profile(profile);
    }

    PlayerColors::~PlayerColors()
    {
    }

    void PlayerColors::set_profile(int profile)
    {
	switch(profile) {
	case 0: // Yellow
	    _border.setRgb(0, 0, 0, 0xff); // black
	    _background.setRgb(0xe5, 0xd7, 0x3a, 0xff); // yellow
	    _foreground = _border;
	    break;
	case 1: // Blue
	    _border.setRgb(0x12, 0x55, 0x85, 0xff); // dark blue
	    _background.setRgb(0x76, 0xc6, 0xf5, 0xff); // light blue
	    _foreground = _border;
	    break;
	default: // Black and White
	    _border.setRgb(0, 0, 0, 0xff); // black
	    _background.setRgb(0xff, 0xff, 0xff, 0xff); // white
	    _foreground = _border;
	}
    }



} // namespace StretchPlayer

