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
#include <sndfile.h>
#include <samplerate.h>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include <cstring>

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

	_sample_rate = jack_get_sample_rate(_jack_client);

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

    void Engine::_zero_buffers(jack_nframes_t nframes)
    {
	// MUTEX MUST ALREADY BE LOCKED
	if (_jack_client) {
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
	}
    }

    int Engine::jack_callback(jack_nframes_t nframes)
    {
	bool locked = false;

	try {
	    locked = _audio_lock.tryLock();
	    if(locked) {
		if(_playing) {
		    _process_playing(nframes);
		} else {
		    _zero_buffers(nframes);
		}
	    } else if (_jack_client) {
		_zero_buffers(nframes);
	    } else {
		// Nothing to do.
	    }
	} catch (...) {
	}

	if(locked) _audio_lock.unlock();

	return 0;
    }

    void Engine::_process_playing(jack_nframes_t nframes)
    {
	// MUTEX MUST ALREADY BE LOCKED
	assert(_jack_client);
	assert(_port_left);
	assert(_port_right);

	float *buf_L = 0, *buf_R = 0;
	buf_L = static_cast<float*>( jack_port_get_buffer(_port_left, nframes) );
	buf_R = static_cast<float*>( jack_port_get_buffer(_port_right, nframes) );

	jack_nframes_t leftover;
	leftover = _left.size() - _position;
	if(leftover < nframes) {
	    _zero_buffers(nframes);
	    _playing = false;
	} else {
	    leftover = nframes;
	}

	if(buf_L)
	    memcpy(buf_L, &_left[_position], sizeof(float) * leftover);
	if(buf_R)
	    memcpy(buf_R, &_right[_position], sizeof(float) * leftover);

	_position += nframes;
	if(_position > _left.size()) {
	    _position = 0;
	}
    }

    void Engine::load_song(const QString& filename)
    {
	QMutexLocker lk(&_audio_lock);
	stop();
	_left.clear();
	_right.clear();
	_position = 0;

	SNDFILE *sf = 0;
	SF_INFO sf_info;
	sf_info.format = 0;

	std::cout << "Opening " << filename.toStdString() << std::endl;
	sf = sf_open(filename.toLocal8Bit().data(), SFM_READ, &sf_info);

	_left.reserve( sf_info.frames );
	_right.reserve( sf_info.frames );

	std::vector<float> buf(sf_info.frames * sf_info.channels, 0.0f);
	sf_count_t read;
	read = sf_read_float(sf, &buf[0], buf.size());
	std::vector<float> *ptr = &buf;
	assert(read == (sf_info.frames * sf_info.channels));

	std::vector<float> out;
	if( sf_info.samplerate != _sample_rate ) {
	    // Resample
	    SRC_DATA data;
	    data.src_ratio = double(_sample_rate) / double(sf_info.samplerate);
	    std::cout << data.src_ratio << std::endl;
	    data.data_in = &buf[0];
	    out.clear();
	    out.insert(out.end(), size_t(data.src_ratio * buf.size()), 0.0);
	    data.data_out = &out[0];
	    ptr = &out;
	    data.input_frames = sf_info.frames;
	    data.output_frames = out.size()/sf_info.channels;
	    int rv = src_simple(&data, SRC_SINC_BEST_QUALITY, sf_info.channels);
	    if(rv != 0) {
		std::cerr << src_strerror(rv) << std::endl;
	    }
	    assert(rv == 0); // XXX TODO: Handle error
	    read = data.output_frames_gen * sf_info.channels;
	}

	sf_count_t j;
	unsigned mod;
	for( j=0 ; j<read ; ++j ) {
	    mod = j % sf_info.channels;
	    if( mod == 0 ) {
		_left.push_back( (*ptr)[j] );
	    } else if ( mod == 1 ) {
		_right.push_back( (*ptr)[j] );
	    } else {
		// remaining channels ignored
	    }
	}

	sf_close(sf);	
    }

    void Engine::play()
    {
	std::cout << "Playing..." << std::endl;
	_playing = true;
    }

    void Engine::stop()
    {
	std::cout << "Stopping..." << std::endl;
	_playing = false;
    }

    void Engine::set_stretch(float factor)
    {
	std::cerr << "Stretch = " << factor << std::endl;
    }

    float Engine::get_position()
    {
	if(_left.size() > 0) {
	    return float(_position) / _sample_rate;
	}
	return 0;
    }

    float Engine::get_length()
    {
	if(_left.size() > 0) {
	    return float(_left.size()) / _sample_rate;
	}
	return 0;
    }

    void Engine::locate(double secs)
    {
	unsigned long pos = secs * _sample_rate;
	QMutexLocker lk(&_audio_lock);
	_position = pos;
    }

} // namespace StretchPlayer
