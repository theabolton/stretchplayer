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

#include "config.h"
#include "Configuration.hpp"
#include <stdexcept>

#ifdef AUDIO_SUPPORT_JACK
#include "JackAudioSystem.hpp"
#endif
#ifdef AUDIO_SUPPORT_ALSA
#include "AlsaAudioSystem.hpp"
#endif

namespace StretchPlayer
{

    AudioSystem* audio_system_factory(int driver)
    {
	AudioSystem* d = 0;
	switch(driver) {
#ifdef AUDIO_SUPPORT_JACK
	case Configuration::JackDriver:
	    d = new JackAudioSystem;
	    break;
#endif
#ifdef AUDIO_SUPPORT_ALSA
	case Configuration::AlsaDriver:
	    d = new AlsaAudioSystem;
	    break;
#endif
	default:
	    throw std::runtime_error("Unsupported driver requested");
	}
	return d;
    }

} // namespace StretchPlayer

