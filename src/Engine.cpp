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
#include "AudioSystem.hpp"
#include "RubberBandServer.hpp"
#include "Configuration.hpp"
#include <sndfile.h>
#include <mpg123.h>
#include <stdexcept>
#include <cassert>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <QFileInfo>
#include <QString>

#include "config.h"

using RubberBand::RubberBandStretcher;

namespace StretchPlayer
{
    Engine::Engine(Configuration *config)
	: _config(config),
	  _playing(false),
	  _hit_end(false),
	  _state_changed(false),
	  _position(0),
	  _loop_a(0),
	  _loop_b(0),
	  _sample_rate(48000.0),
	  _stretch(1.0),
	  _pitch(0),
	  _gain(1.0),
	  _output_position(0)
    {
	QString err;

	QMutexLocker lk(&_audio_lock);

	Configuration::driver_t pref_driver;

	if(_config) {
	    pref_driver = _config->driver();
	}

	_audio_system.reset( audio_system_factory(pref_driver) );

	QString app_name("StretchPlayer");
	_audio_system->init( &app_name, _config, &err );
	_audio_system->set_process_callback(Engine::static_process_callback, this);
	_audio_system->set_segment_size_callback(Engine::static_segment_size_callback, this);

	if( ! err.isNull() ) {
	    char msg[513];
	    strncpy(msg, err.toLocal8Bit().data(), 512); 
	    throw std::runtime_error(msg);
	}


	uint32_t sample_rate = _audio_system->sample_rate();

	_stretcher.reset( new RubberBandServer(sample_rate) );
	_stretcher->set_segment_size( _audio_system->current_segment_size() );
	_stretcher->start();

	if( _audio_system->activate(&err) )
	    throw std::runtime_error(err.toLocal8Bit().data());

    }

    Engine::~Engine()
    {
	QMutexLocker lk(&_audio_lock);

	_stretcher->go_idle();
	_stretcher->shutdown();

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

	_stretcher->wait();
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

    int Engine::segment_size_callback(uint32_t nframes)
    {
	_stretcher->set_segment_size(nframes);
    }

    int Engine::process_callback(uint32_t nframes)
    {
	bool locked = false;

	if( _loop_ab_pressed > 0 ) {
	    _handle_loop_ab();
	}

	try {
	    locked = _audio_lock.tryLock();
	    if(_state_changed) {
		_state_changed = false;
		_stretcher->reset();
		float left[64], right[64];
		while( _stretcher->available_read() > 0 )
		    _stretcher->read_audio(left, right, 64);
		assert( 0 == _stretcher->available_read() );
		_position = _output_position;
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

    static void apply_gain_to_buffer(float *buf, uint32_t frames, float gain);

    void Engine::_process_playing(uint32_t nframes)
    {
	// MUTEX MUST ALREADY BE LOCKED
	float *buf_L = 0, *buf_R = 0;

	buf_L = _audio_system->output_buffer(0);
	buf_R = _audio_system->output_buffer(1);

	uint32_t srate = _audio_system->sample_rate();
	float time_ratio = srate / _sample_rate / _stretch;

	_stretcher->time_ratio( time_ratio );
	_stretcher->pitch_scale( ::pow(2.0, double(_pitch)/12.0) * _sample_rate / srate );

	uint32_t frame;
	uint32_t reqd, gend, zeros, feed;

	assert( _stretcher->is_running() );

	// Determine how much data to push into the stretcher
	int32_t write_space, written, input_frames;
	write_space = _stretcher->available_write();
	written = _stretcher->written();
	if(written < _stretcher->feed_block_min()
	   && write_space >= _stretcher->feed_block_max() ) {
	    input_frames = _stretcher->feed_block_max();
	} else {
	    input_frames = 0;
	}

	// Push data into the stretcher, observing A/B loop points
	while( input_frames > 0 ) {
	    feed = input_frames;
	    if( looping() && ((_position + feed) >= _loop_b) ) {
		if( _position >= _loop_b ) {
		    _position = _loop_a;
		    if( _loop_a + feed > _loop_b ) {
			assert(_loop_b > _loop_a );
			feed = _loop_b - _loop_a;
		    }
		} else {
		    assert( _loop_b >= _position );
		    feed = _loop_b - _position;
		}
	    }
	    if( _position + feed > _left.size() ) {
		feed = _left.size() - _position;
		input_frames = feed;
	    }
	    _stretcher->write_audio( &_left[_position], &_right[_position], feed );
	    _position += feed;
	    assert( input_frames >= feed );
	    input_frames -= feed;
	    if( looping() && _position >= _loop_b ) {
		_position = _loop_a;
	    }
	}

	// Pull generated data off the stretcher
	uint32_t read_space;
	read_space = _stretcher->available_read();

	if( read_space >= nframes ) {
	    _stretcher->read_audio(buf_L, buf_R, nframes);
	} else if ( (read_space > 0) && _hit_end ) {
	    _zero_buffers(nframes);
	    _stretcher->read_audio(buf_L, buf_R, read_space);
	} else {
	    _zero_buffers(nframes);
	}

	// Update our estimation of the output position.
	unsigned n_feed_buf = _stretcher->latency();
	if(_position > n_feed_buf) {
	    _output_position = _position - n_feed_buf;
	} else {
	    _output_position = 0;
	}
	assert( (_output_position > _position) ? (_output_position - _position) <= n_feed_buf : true );
	assert( (_output_position < _position) ? (_position - _output_position) <= n_feed_buf : true );

	// Apply gain... unroll loop manually so GCC will use SSE
	if(nframes & 0xf) {  // nframes < 16
	    unsigned f = nframes;
	    while(f--) {
		(*buf_L++) *= _gain;
		(*buf_R++) *= _gain;
	    }
	} else {
	    apply_gain_to_buffer(buf_L, nframes, _gain);
	    apply_gain_to_buffer(buf_R, nframes, _gain);
	}

	if(_position >= _left.size()) {
	    _hit_end = true;
	}
	if( (_hit_end == true) && (read_space == 0) ) {
	    _hit_end = false;
	    _playing = false;
	    _position = 0;
	    _stretcher->reset();
	}

	// Wake up, lazybones!
	_stretcher->nudge();
    }

    /**
     * Attempt to load a file via libsndfile
     *
     * \return true on success
     */
    bool Engine::_load_song_using_libsndfile(const QString& filename)
    {
	SNDFILE *sf = 0;
	SF_INFO sf_info;
	memset(&sf_info, 0, sizeof(sf_info));

	_message( QString("Opening file...") );
	sf = sf_open(filename.toLocal8Bit().data(), SFM_READ, &sf_info);
	if( !sf ) {
	    _error( QString("Error opening file '%1': %2")
		    .arg(filename)
		    .arg( sf_strerror(sf) ) );
	    return false;
	}

	_sample_rate = sf_info.samplerate;
	_left.reserve( sf_info.frames );
	_right.reserve( sf_info.frames );

	if(sf_info.frames == 0) {
	    _error( QString("Error opening file '%1': File is empty")
		    .arg(filename) );
	    sf_close(sf);
	    return false;
	}

	_message( QString("Reading file...") );
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
	return true;
    }

    /**
     * Attempt to load an MP3 file via libmpg123
     *
     * adapted by Sean Bolton from mpg123_to_wav.c
     *
     * \return true on success
     */
    bool Engine::_load_song_using_libmpg123(const QString& filename)
    {
	mpg123_handle *mh = 0;
	int err, channels, encoding;
	long rate;

	_message( QString("Opening file...") );
	if ((err = mpg123_init()) != MPG123_OK ||
	    (mh = mpg123_new(0, &err)) == 0 ||
	    mpg123_open(mh, filename.toLocal8Bit().data()) != MPG123_OK ||
	    mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {

	    _error( QString("Error opening file '%1': %2")
		    .arg(filename)
		    .arg(mh == NULL ? mpg123_plain_strerror(err) : mpg123_strerror(mh)) );

	  mpg123error:
	    mpg123_close(mh);
	    mpg123_delete(mh);
	    mpg123_exit();
	    return false;
	}
	if (encoding != MPG123_ENC_SIGNED_16) {
	    _error( QString("Error: unsupported encoding format.") );
	    goto mpg123error;
	}
	/* lock the output format */
	mpg123_format_none(mh);
	mpg123_format(mh, rate, channels, encoding);

	off_t length = mpg123_length(mh);
	if (length == MPG123_ERR || length == 0) {
	    _error( QString("Error: file is empty or length unknown.") );
	    goto mpg123error;
	}

	_sample_rate = rate;
	_left.reserve( length );
	_right.reserve( length );
	
	_message( QString("Reading file...") );
	std::vector<signed short> buffer(4096, 0);
	size_t read = 0, k;

	while (1) {
	    err = mpg123_read(mh, (unsigned char*)&buffer[0], buffer.size(), &read);
	    if (err != MPG123_OK && err != MPG123_DONE)
		break;
	    if (read > 0) {
		read /= sizeof(signed short);
		for(k = 0; k < read ; k++) {
		    unsigned int mod = k % channels;
		    if( mod == 0 ) {
			_left.push_back( (float)buffer[k] / 32768.0f );
		    }
		    if( mod == 1 || channels == 1 ) {
			_right.push_back( (float)buffer[k] / 32768.0f );
		    }
		    /* remaining channels ignored */
		}
	    }
	    if (err == MPG123_DONE)
		break;
	};

	if (err == MPG123_NEED_MORE) {
	    _error( QString("Warning: premature end of MP3 stream"));
	    /* allow user to play what we did manage to read */
	} else if (err != MPG123_DONE) {
	    _error( QString("Error decoding file: %1.")
		    .arg(err == MPG123_ERR ? mpg123_strerror(mh) : mpg123_plain_strerror(err)));
	    goto mpg123error;
	}

	mpg123_close(mh);
	mpg123_delete(mh);
	mpg123_exit();
	return true;
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
	_output_position = 0;
	_stretcher->reset();

	if( ! _load_song_using_libsndfile(filename) &&
	    ! _load_song_using_libmpg123(filename) )
	    return QString();

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
	    return float(_output_position) / _sample_rate;
	}
	return 0;
    }

    void Engine::loop_ab()
    {
	_loop_ab_pressed.fetchAndAddRelaxed(1);
    }

    void Engine::_handle_loop_ab()
    {
	while( _loop_ab_pressed > 0 ) {
	    uint32_t pos, lat;
	    uint32_t pressed_frame, seg_frame;

	    assert( _stretcher->time_ratio() > 0 );
	    pos = _output_position;

	    if(pos > lat) pos -= lat;

	    if( _loop_b > _loop_a ) {
		_loop_b = 0;
		_loop_a = 0;
	    } else if( _loop_a == 0 ) {
		_loop_a = pos;
		if(pos == 0) {
		    _loop_a = 1;
		}
	    } else if( _loop_a != 0 ) {
		if( pos > _loop_a ) {
		    _loop_b = pos;
		} else {
		    _loop_a = pos;
		}
	    } else {
		assert(false);  // invalid state
	    }
	    _loop_ab_pressed.fetchAndAddOrdered(-1);
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
	_output_position = _position = pos;
	_state_changed = true;
	_stretcher->reset();
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
	float audio_load, worker_load;

	audio_load = _audio_system->dsp_load();
	if(_playing) {
	    worker_load = _stretcher->cpu_load();
	} else {
	    worker_load = 0.0;
	}
	return  audio_load + worker_load;
    }

    /* SIMD code for optimizing the gain application.
     *
     * Below is vectorized (SSE, SIMD) code for applying
     * the gain.  If you enable >= SSE2 optimization, then
     * this will calculate 4 floats at a time.  If you do
     * not, then it will still work.
     *
     * This syntax is a GCC extension, but more portable
     * than writing x86 assembly.
     */

    typedef float __vf4 __attribute__((vector_size(16)));
    typedef union {
	float f[4];
	__vf4 v;
    } vf4;

    /**
     * \brief Multiply each element in a buffer by a scalar.
     *
     * For each element in buf[0..nframes-1], buf[i] *= gain.
     *
     * This function detects the buffer alignment, and if it's 4-byte
     * aligned and SSE optimization is enabled, it will use the
     * optimized code-path.  However, it will still work with 1-byte
     * aligned buffers.
     *
     * \param buf - Pointer to a buffer of floats.
     */
    static void apply_gain_to_buffer(float *buf, uint32_t nframes, float gain)
    {
	vf4* opt;
	vf4 gg = {gain, gain, gain, gain};
	int alignment;
	unsigned ctr = nframes/4;

	alignment = reinterpret_cast<uintptr_t>(buf) & 0x0F;

	switch(alignment) {
	case 4: (*buf++) *= gain;
	case 8: (*buf++) *= gain;
	case 12:(*buf++) *= gain;
	    --ctr;
	case 0:
	    break;
	default:
	  goto LAME;
	}

	assert( (reinterpret_cast<uintptr_t>(buf) & 0x0F) == 0 );
	opt = (vf4*) buf;
	while(ctr--) {
	    opt->v *= gg.v; ++opt;
	}

	buf = (float*) opt;
	switch(alignment) {
	case 12: (*buf++) *= gain;
	case 8:  (*buf++) *= gain;
	case 4:  (*buf++) *= gain;
	}

	return;

    LAME:
	// If it's not even 4-byte aligned
	// then this is is the un-optimized code.
	while(nframes--) {
	  (*buf++) *= gain;
	}
    }

} // namespace StretchPlayer
