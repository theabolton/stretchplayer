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
#include <rubberband/RubberBandStretcher.h>
#include <stdexcept>
#include <cassert>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iostream>

using RubberBand::RubberBandStretcher;
using namespace std;

namespace StretchPlayer
{
    Engine::Engine()
	: _jack_client(0),
	  _port_left(0),
	  _port_right(0),
	  _playing(false),
	  _state_changed(false),
	  _position(0),
	  _loop_a(0),
	  _loop_b(0),
	  _sample_rate(48000.0),
	  _stretch(1.0),
	  _pitch(0),
	  _gain(1.0)
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

	jack_nframes_t sample_rate = jack_get_sample_rate(_jack_client);

	_stretcher.reset(
	    new RubberBandStretcher( sample_rate,
				     2,
				     RubberBandStretcher::OptionProcessRealTime
				     | RubberBandStretcher::OptionThreadingAuto
		)
	    );
	_stretcher->setMaxProcessSize(8192);

	jack_activate(_jack_client);

	// Autoconnection to first two ports we find.
	const char** ports = jack_get_ports( _jack_client,
					     0,
					     JACK_DEFAULT_AUDIO_TYPE,
					     JackPortIsInput
	    );
	int k;
	for( k=0 ; ports && ports[k] != 0 ; ++k ) {
	    if(k==0) {
		rv = jack_connect( _jack_client,
				   jack_port_name(_port_left),
				   ports[k] );
	    } else if (k==1) {
		rv = jack_connect( _jack_client,
				   jack_port_name(_port_right),
				   ports[k] );
	    } else {
		break;
	    }
	    if( rv ) {
		_error("Could not connect output ports");
	    }
	}
	if(k==0) {
	    _error("There were no output ports to connect to.");
	}
	if(ports) {
	    free(ports);
	}
    }

    Engine::~Engine()
    {
	QMutexLocker lk(&_audio_lock);

	if(_jack_client) {
	    jack_deactivate(_jack_client);

	    jack_client_close(_jack_client);
	    _jack_client = 0;
	}
	callback_seq_t::iterator it;
	QMutexLocker lk_cb(&_callback_lock);
	for( it=_error_callbacks.begin() ; it!=_error_callbacks.end() ; ++it ) {
	    (*it)->_parent = 0;
	}
	for( it=_message_callbacks.begin() ; it!=_message_callbacks.end() ; ++it ) {
	    (*it)->_parent = 0;
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
	    if(_state_changed) {
		_state_changed = false;
		_stretcher->reset();
	    }
	    if(locked) {
		if(_playing) {
		    if(_left.size()) {
			_process_playing(nframes);
		    } else {
			_playing = false;
		    }
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
	float* rb_buf_in[2];
	float* rb_buf_out[2] = { buf_L, buf_R };

	jack_nframes_t srate = jack_get_sample_rate(_jack_client);

	_stretcher->setTimeRatio( srate / _sample_rate / _stretch );
	_stretcher->setPitchScale( ::pow(2.0, double(_pitch)/12.0) * _sample_rate / srate );

	jack_nframes_t frame;
	size_t reqd, gend, zeros, feed;

	frame = 0;
	while( frame < nframes ) {
	    reqd = _stretcher->getSamplesRequired();
	    int avail = _stretcher->available();
	    if( avail <= 0 ) avail = 0;
	    if( unsigned(avail) >= nframes ) reqd = 0;
	    zeros = 0;
	    feed = reqd;
	    if( looping() && ((_position + reqd) >=_loop_b) ) {
		assert( _loop_b >= _position );
		reqd = _loop_b - _position;
	    }
	    if( _position + reqd > _left.size() ) {
		feed = _left.size() - _position;
		zeros = reqd - feed;
	    }
	    rb_buf_in[0] = &_left[_position];
	    rb_buf_in[1] = &_right[_position];
	    if( reqd == 0 ) {
		feed = 0;
		zeros = 0;
	    }
	    _stretcher->process( rb_buf_in, feed, false);
	    if(reqd && zeros) {
		float l[zeros], r[zeros];
		float* z[2] = { l, r };
		memset(l, 0, zeros * sizeof(float));
		memset(r, 0, zeros * sizeof(float));
		_stretcher->process(z, zeros, false);
	    }
	    gend = 1;
	    while( gend && frame < nframes ) {
		gend = _stretcher->retrieve(rb_buf_out, (nframes-frame));
		rb_buf_out[0] += gend;
		rb_buf_out[1] += gend;
		frame += gend;
	    }
	    _position += feed;
	    if( looping() && _position >= _loop_b ) {
		_position = _loop_a;
	    }
	}

	// Apply gain and clip
	for( frame=0 ; frame<nframes ; ++frame ) {
	    buf_L[frame] *= _gain;
	    if(buf_L[frame] > 1.0) buf_L[frame] = 1.0;
	    if(buf_L[frame] < -1.0) buf_L[frame] = -1.0;
	    buf_R[frame] *= _gain;
	    if(buf_R[frame] > 1.0) buf_R[frame] = 1.0;
	    if(buf_R[frame] < -1.0) buf_R[frame] = -1.0;
	}

	if(_position >= _left.size()) {
	    _playing = false;
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
	memset(&sf_info, 0, sizeof(sf_info));

	_message( QString("Opening file '%1'")
		  .arg(filename) );
	sf = sf_open(filename.toLocal8Bit().data(), SFM_READ, &sf_info);
	if( !sf ) {
	    _error( QString("Error opening file '%1': %2")
		    .arg(filename)
		    .arg( sf_strerror(sf) ) );
	    return;
	}

	_sample_rate = sf_info.samplerate;
	_left.reserve( sf_info.frames );
	_right.reserve( sf_info.frames );

	if(sf_info.frames == 0) {
	    _error( QString("Error opening file '%1': File is empty")
		    .arg(filename) );
	    sf_close(sf);
	    return;
	}

	std::vector<float> buf(sf_info.frames * sf_info.channels, 0.0f);
	sf_count_t read;
	read = sf_read_float(sf, &buf[0], buf.size());
	if( read < 1 ) {
	    _error( QString("Error reading from file '%1'")
		    .arg(filename) );
	    sf_close(sf);
	    return;
	}

	if( read != (sf_info.frames * sf_info.channels)) {
	    _error( QString("Warning: not all of the file data was read.") );
	}

	sf_count_t j;
	unsigned mod;
	for( j=0 ; j<read ; ++j ) {
	    mod = j % sf_info.channels;
	    if( mod == 0 ) {
		_left.push_back( buf[j] );
	    } else if ( mod == 1 ) {
		_right.push_back( buf[j] );
	    } else {
		// remaining channels ignored
	    }
	}

	sf_close(sf);	
    }

    void Engine::play()
    {
	if( ! _playing ) {
	    _state_changed = true;
	    _playing = true;
	}
    }

    void Engine::play_pause()
    {
	_playing = (_playing) ? false : true;
	_state_changed = true;
    }

    void Engine::stop()
    {
	if( _playing ) {
	    _playing = false;
	    _state_changed = true;
	}
    }

    float Engine::get_position()
    {
	if(_left.size() > 0) {
	    return float(_position) / _sample_rate;
	}
	return 0;
    }

    void Engine::loop_ab()
    {
	if( _loop_b > _loop_a ) {
	    _loop_b = 0;
	    _loop_a = 0;
	} else if( _loop_a == 0 ) {
	    _loop_a = _position;
	    if(_position == 0) {
		_loop_a = 1;
	    }
	} else if( _loop_a != 0 ) {
	    if( _position > _loop_a ) {
		_loop_b = _position;
	    } else {
		_loop_a = _position;
	    }
	} else {
	    assert(false);  // invalid state
	}
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
	_state_changed = true;
    }

    void Engine::_dispatch_message(const Engine::callback_seq_t& seq, const QString& msg) const
    {
	QMutexLocker lk(&_callback_lock);
	Engine::callback_seq_t::const_iterator it;
	for( it=seq.begin() ; it!=seq.end() ; ++it ) {
	    (**it)(msg);
	}
    }

    void Engine::_subscribe_list(Engine::callback_seq_t& seq, EngineMessageCallback* obj)
    {
	if( obj == 0 ) return;
	QMutexLocker lk(&_callback_lock);
	obj->_parent = this;
	seq.insert(obj);
    }

    void Engine::_unsubscribe_list(Engine::callback_seq_t& seq, EngineMessageCallback* obj)
    {
	if( obj == 0 ) return;
	QMutexLocker lk(&_callback_lock);
	obj->_parent = 0;
	seq.erase(obj);
    }

    float Engine::get_cpu_load()
    {
	return jack_cpu_load(_jack_client)/100.0;
    }

} // namespace StretchPlayer
