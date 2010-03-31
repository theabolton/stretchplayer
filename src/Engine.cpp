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

#include "Engine.hpp"
#include <iostream>
#include <stdexcept>

namespace StretchPlayer
{
    Engine::Engine()
	: _jack_client(0)
    {
	QMutexLocker lk(&_audio_lock);

	_jack_client = jack_client_open("StretchPlayer", JackNullOption, 0);
	if(!_jack_client)
	    throw std::runtime_error("Could not set up JACK");

	_port_left = jack_port_register( _jack_client,
					 "left",
					 JACK_DEFAULT_AUDIO_TYPE,
					 JackPortIsOutput,
					 0 );
	if(!_port_left)
	    throw std::runtime_error("Could not set up left out");

	_port_right = jack_port_register( _jack_client,
					  "right",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput,
					  0 );
	if(!_port_right)
	    throw std::runtime_error("Could not set up right out");

	int rv = jack_set_process_callback( _jack_client,
					    Engine::static_jack_callback,
					    this );
	if(rv)
	    throw std::runtime_error("Could not set up jack callback.");

	jack_activate(_jack_client);
    }

    Engine::~Engine()
    {
	QMutexLocker lk(&_audio_lock);

	if(_jack_client) {
	    jack_deactivate(_jack_client);

	    jack_client_close(_jack_client);
	    _jack_client = 0;
	}
    }

    int Engine::jack_callback(jack_nframes_t nframes)
    {
	bool locked = false;

	try {
	    locked = _audio_lock.tryLock();
	    if(locked) {
		void *buf_L = 0, *buf_R = 0;
		if(_port_left) {
		    buf_L = jack_port_get_buffer(_port_left, nframes);
		}
		if(buf_L) {
		    memset(buf_L, 0, nframes * sizeof(float));
		}
		if(_port_right) {
		    buf_R = jack_port_get_buffer(_port_right, nframes);
		}
		if(buf_R) {
		    memset(buf_R, 0, nframes * sizeof(float));
		}
	    } else if (_jack_client) {
		// Just zero the buffers
		void *buf_L = 0, *buf_R = 0;
		if(_port_left) {
		    buf_L = jack_port_get_buffer(_port_left, nframes);
		}
		if(buf_L) {
		    memset(buf_L, 0, nframes * sizeof(float));
		}
		if(_port_right) {
		    buf_R = jack_port_get_buffer(_port_right, nframes);
		}
		if(buf_R) {
		    memset(buf_R, 0, nframes * sizeof(float));
		}
	    } else {
		// Nothing to do.
	    }
	} catch (...) {
	}

	if(locked) _audio_lock.unlock();

	return 0;
    }

    void Engine::load_song(const QString& filename)
    {
	std::cerr << "Opening " << filename.toStdString() << std::endl;
    }

    void Engine::play()
    {
	_playing = true;
    }

    void Engine::stop()
    {
	_playing = false;
    }

    void Engine::set_stretch(float factor)
    {
	std::cerr << "Stretch = " << factor << std::endl;
    }

    float Engine::get_position()
    {
	if(_left.size() > 0) {
	    return float(_position) / (_sample_rate * _left.size());
	}
	return 0;
    }

} // namespace StretchPlayer
