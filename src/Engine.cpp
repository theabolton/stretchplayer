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
#include <QFileInfo>
#include <AudioSystem.hpp>
#include <JackAudioSystem.hpp>
#include <QString>

using RubberBand::RubberBandStretcher;
using namespace std;

namespace StretchPlayer
{
    Engine::Engine()
	: _playing(false),
	  _state_changed(false),
	  _position(0),
	  _loop_a(0),
	  _loop_b(0),
	  _sample_rate(48000.0),
	  _stretch(1.0),
	  _pitch(0),
	  _gain(1.0)
    {
	QString err;
	QMutexLocker lk(&_audio_lock);

	_audio_system.reset( new JackAudioSystem );
	QString app_name("StretchPlayer");
	_audio_system->init( &app_name, &err);
	_audio_system->set_process_callback(Engine::static_process_callback, this);

	if( ! err.isNull() )
	    throw std::runtime_error(err.toLocal8Bit().data());


	uint32_t sample_rate = _audio_system->sample_rate();

	_stretcher.reset(
	    new RubberBandStretcher( sample_rate,
				     2,
				     RubberBandStretcher::OptionProcessRealTime
				     | RubberBandStretcher::OptionThreadingAuto
		)
	    );
	_stretcher->setMaxProcessSize(16384);
	_stretcher->reset();

	if( _audio_system->activate(&err) )
	    throw std::runtime_error(err.toLocal8Bit().data());

    }

    Engine::~Engine()
    {
	QMutexLocker lk(&_audio_lock);

	_audio_system->deactivate();
	_audio_system->cleanup();

	callback_seq_t::iterator it;
	QMutexLocker lk_cb(&_callback_lock);
	for( it=_error_callbacks.begin() ; it!=_error_callbacks.end() ; ++it ) {
	    (*it)->_parent = 0;
	}
	for( it=_message_callbacks.begin() ; it!=_message_callbacks.end() ; ++it ) {
	    (*it)->_parent = 0;
	}
    }

    void Engine::_zero_buffers(uint32_t nframes)
    {
	// MUTEX MUST ALREADY BE LOCKED

	// Just zero the buffers
	void *buf_L = 0, *buf_R = 0;
	buf_L = _audio_system->output_buffer(0);
	if(buf_L) {
	    memset(buf_L, 0, nframes * sizeof(float));
	}
	buf_R = _audio_system->output_buffer(1);
	if(buf_R) {
	    memset(buf_R, 0, nframes * sizeof(float));
	}
    }

    int Engine::process_callback(uint32_t nframes)
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
	    } else {
		_zero_buffers(nframes);
	    }
	} catch (...) {
	}

	if(locked) _audio_lock.unlock();

	return 0;
    }

    void Engine::_process_playing(uint32_t nframes)
    {
	// MUTEX MUST ALREADY BE LOCKED
	float *buf_L = 0, *buf_R = 0;

	buf_L = _audio_system->output_buffer(0);
	buf_R = _audio_system->output_buffer(1);
	float* rb_buf_in[2];
	float* rb_buf_out[2] = { buf_L, buf_R };

	uint32_t srate = _audio_system->sample_rate();

	_stretcher->setTimeRatio( srate / _sample_rate / _stretch );
	_stretcher->setPitchScale( ::pow(2.0, double(_pitch)/12.0) * _sample_rate / srate );

	uint32_t frame;
	uint32_t reqd, gend, zeros, feed;

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
	    buf_R[frame] *= _gain;
	}

	if(_position >= _left.size()) {
	    _playing = false;
	    _position = 0;
	}
    }

    /**
     * Load a file
     *
     * \return Name of song
     */
    QString Engine::load_song(const QString& filename)
    {
	QMutexLocker lk(&_audio_lock);
	stop();
	_left.clear();
	_right.clear();
	_position = 0;

	SNDFILE *sf = 0;
	SF_INFO sf_info;
	memset(&sf_info, 0, sizeof(sf_info));

	_message( QString("Opening file...") );
	sf = sf_open(filename.toLocal8Bit().data(), SFM_READ, &sf_info);
	if( !sf ) {
	    _error( QString("Error opening file '%1': %2")
		    .arg(filename)
		    .arg( sf_strerror(sf) ) );
	    return QString();
	}

	_sample_rate = sf_info.samplerate;
	_left.reserve( sf_info.frames );
	_right.reserve( sf_info.frames );

	if(sf_info.frames == 0) {
	    _error( QString("Error opening file '%1': File is empty")
		    .arg(filename) );
	    sf_close(sf);
	    return QString();
	}

	_message( QString("Opening file...") );
	std::vector<float> buf(4096, 0.0f);
	sf_count_t read, k;
	unsigned mod;
	while(true) {
	    read = sf_read_float(sf, &buf[0], buf.size());
	    if( read < 1 ) break;
	    for(k=0 ; k<read ; ++k) {
		mod = k % sf_info.channels;
		if( mod == 0 ) {
		    _left.push_back( buf[k] );
		} else if( mod == 1 ) {
		    _right.push_back( buf[k] );
		} else {
		    // remaining channels ignored
		}
	    }
	}

	if( _left.size() != sf_info.frames ) {
	    _error( QString("Warning: not all of the file data was read.") );
	}

	sf_close(sf);
	QFileInfo f_info(filename);
	return f_info.fileName();
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
	return _audio_system->dsp_load();
    }

} // namespace StretchPlayer
