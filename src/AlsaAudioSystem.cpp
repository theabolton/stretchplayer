/*
 * Copyright(c) 2009 by Gabriel M. Beddingfield <gabriel@teuton.org>
 * Copyright(c) 2002 by Paul Davis
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

/**
 * Much of this code was adapted from "A Tutorial on Using the ALSA
 * Audio API" by Paul Davis.
 * http://www.equalarea.com/paul/alsa-audio.html
 */

#include "AlsaAudioSystem.hpp"
#include "AlsaAudioSystemPrivate.hpp"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <QString>
#include <alsa/asoundlib.h>

#include <iostream>
using namespace std;

namespace StretchPlayer
{
    inline bool not_aligned_16(void* ptr) {
	return ((int)ptr) % 16;
    }

    AlsaAudioSystem::AlsaAudioSystem() :
	_channels(2),
	_type(FLOAT),
	_bits(32),
/*	_type(INT),
	_bits(16), */
	_little_endian(true),
	_sample_rate(44100),
	_period_nframes(2048),
	_active(false),
	_playback_handle(0),
	_left_root(0),
	_right_root(0),
	_left(0),
	_right(0),
	_callback(0),
	_callback_arg(0),
	_d(0)
    {
	_d = new AlsaAudioSystemPrivate();
	_d->parent(this);
	_d->run_callback( AlsaAudioSystem::run );
    }

    AlsaAudioSystem::~AlsaAudioSystem()
    {
	cleanup();
	delete _d;
	_d = 0;
    }

    int AlsaAudioSystem::init(QString * app_name, QString *err_msg)
    {
	QString name("StretchPlayer");
	int err;

	if(app_name) {
	    name = *app_name;
	}

	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;
	int nfds;
	struct pollfd *pfds;

	if((err = snd_pcm_open(&_playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
	    cerr << "cannot open default ALSA audio device"
		 << " (" << snd_strerror(err) << ")" << endl;
	    assert(false);
	}
	assert(_playback_handle);

	if((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
	    cerr << "cannot allocate hardware parameter structure"
		 << " (" << snd_strerror(err) << ")" << endl;
	    assert(false);
	}

	if((err = snd_pcm_hw_params_any(_playback_handle, hw_params)) < 0) {
	    cerr << "cannot initialize hardware parameter structure"
		 << " (" << snd_strerror(err) << ")" << endl;
	    assert(false);
	}

	if((err = snd_pcm_hw_params_set_access(_playback_handle, hw_params,
					       SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
	    cerr << "cannot set access type"
		 << " (" << snd_strerror(err) << ")" << endl;
	    assert(false);
	}

	enum _snd_pcm_format format = SND_PCM_FORMAT_S16_LE;

	if(_type == INT) {
	    switch(_bits) {
	    case 8:
		format = SND_PCM_FORMAT_S8;
		break;
	    case 16:
		format = _little_endian ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_S16_BE;
		break;
	    case 24:
		format = _little_endian ? SND_PCM_FORMAT_S24_LE : SND_PCM_FORMAT_S24_BE;
		break;
	    case 32:
		format = _little_endian ? SND_PCM_FORMAT_S32_LE : SND_PCM_FORMAT_S32_BE;
		break;
	    default:
		assert(false);
	    }
	} else if (_type == UINT) {
	    switch(_bits) {
	    case 8:
		format = SND_PCM_FORMAT_U8;
		break;
	    case 16:
		format = _little_endian ? SND_PCM_FORMAT_U16_LE : SND_PCM_FORMAT_U16_BE;
		break;
	    case 24:
		format = _little_endian ? SND_PCM_FORMAT_U24_LE : SND_PCM_FORMAT_U24_BE;
		break;
	    case 32:
		format = _little_endian ? SND_PCM_FORMAT_U32_LE : SND_PCM_FORMAT_U32_BE;
		break;
	    default:
		assert(false);
	    }
	} else if (_type == FLOAT) {
	    switch(_bits) {
	    case 32:
		format = _little_endian ? SND_PCM_FORMAT_FLOAT_LE : SND_PCM_FORMAT_FLOAT_BE;
		break;
	    default:
		assert(false);
	    }

	} else {
	    assert(false);
	}

	if((err = snd_pcm_hw_params_set_format(_playback_handle, hw_params, format)) < 0) {
	    cerr << "cannot set sample format"
		 << " (" << snd_strerror(err) << ")" << endl;
	    assert(false);
	}

	if((err = snd_pcm_hw_params_set_rate_near(_playback_handle, hw_params, &_sample_rate, 0)) < 0) {
	    cerr << "cannot set sample rate"
		 << " (" << snd_strerror(err) << ")" << endl;
	    assert(false);
	}

	if((err = snd_pcm_hw_params_set_channels(_playback_handle, hw_params, 2)) < 0) {
	    cerr << "cannot set channel count"
		 << " (" << snd_strerror(err) << ")" << endl;
	    assert(false);
	}

	if((err = snd_pcm_hw_params(_playback_handle, hw_params)) < 0) {
	    cerr << "cannot set parameters"
		 << " (" << snd_strerror(err) << ")" << endl;
	    assert(false);
	}

	snd_pcm_hw_params_free(hw_params);

	/* Tell ALSA to wake us up whenever _period_nframes or more frames
	 * of playback data can be delivered.  Also, tell ALSA
	 * that we'll start the device ourselves.
	 */

	if((err = snd_pcm_sw_params_malloc(&sw_params)) < 0) {
	    cerr << "cannot allocate software parameters structure"
		 << " (" << snd_strerror(err) << ")" << endl;
	    assert(false);
	}

	if((err = snd_pcm_sw_params_current(_playback_handle, sw_params)) < 0) {
	    cerr << "cannot initialize software parameters structure"
		 << " (" << snd_strerror(err) << ")" << endl;
	    assert(false);
	}

	if((err = snd_pcm_sw_params_set_avail_min(_playback_handle, sw_params, _period_nframes)) < 0) {
	    cerr << "cannot set minimum available count"
		 << " (" << snd_strerror(err) << ")" << endl;
	    assert(false);
	}

	if((err = snd_pcm_sw_params_set_start_threshold(_playback_handle, sw_params, 0U)) < 0) {
	    cerr << "cannot set start mode"
		 << " (" << snd_strerror(err) << ")" << endl;
	    assert(false);
	}
	if((err = snd_pcm_sw_params(_playback_handle, sw_params)) < 0) {
	    cerr << "cannot set software parameters"
		 << " (" << snd_strerror(err) << ")" << endl;
	    assert(false);
	}

	size_t data_size;

	switch(_bits) {
	case 8:  data_size = 1; break;
	case 16: data_size = 2; break;
	case 24: data_size = 4; break;
	case 32: data_size = 4; break;
	default: assert(false);
	}

	_buf = _buf_root = new unsigned short[_period_nframes * _channels * data_size + 16];
	_left = _left_root = new float[_period_nframes + 4];
	_right = _right_root = new float[_period_nframes + 4];

	assert(_buf);
	assert(_left);
	assert(_right);

	while( not_aligned_16(_buf) ) ++_buf;
	while( not_aligned_16(_left) ) ++_left;
	while( not_aligned_16(_right) ) ++_right;

	return 0;

    init_bail:
	if(err_msg) {
	    *err_msg = err;
	}
	return 0xDEADBEEF;
    }

    void AlsaAudioSystem::cleanup()
    {
	deactivate();
	if(_right_root) {
	    delete [] _right_root;
	    _right = _right_root = 0;
	}
	if(_left_root) {
	    delete [] _left_root;
	    _left = _left_root = 0;
	}
	if(_buf_root) {
	    delete [] _buf_root;
	    _buf = _buf_root = 0;
	}
	if(_playback_handle) {
	    snd_pcm_close(_playback_handle);
	    _playback_handle = 0;
	}
    }

    int AlsaAudioSystem::set_process_callback(process_callback_t cb, void* arg, QString* err_msg)
    {
	assert(cb);
	_callback = cb;
	_callback_arg = arg;
	return 0;
    }

    int AlsaAudioSystem::activate(QString *err_msg)
    {
	assert(!_active);
	assert(_d);
	assert( ! (_d->isRunning()) );
	assert(_left);
	assert(_right);

	_active = true;
	_d->start(QThread::TimeCriticalPriority);

	return 0;
    }

    int AlsaAudioSystem::deactivate(QString *err_msg)
    {
	assert(_d);
	_active = false;
	_d->wait();
	return 0;
    }

    AudioSystem::sample_t* AlsaAudioSystem::output_buffer(int index)
    {
	if(index == 0) {
	    assert(_left);
	    return _left;
	}else if(index == 1) {
	    assert(_right);
	    return _right;
	}
	return 0;
    }

    uint32_t AlsaAudioSystem::output_buffer_size(int /*index*/)
    {
	return _period_nframes;
    }

    uint32_t AlsaAudioSystem::sample_rate()
    {
	return _sample_rate;
    }

    float AlsaAudioSystem::dsp_load()
    {
	return -1;
    }

    void AlsaAudioSystem::_run()
    {
	int err;
	snd_pcm_sframes_t frames_to_deliver;
	uint32_t f;

	assert(_active);

	/* the interface will interrupt the kernel every
	 * _period_nframes frames, and ALSA will wake up this program
	 * very soon after that.
	 */
	if((err = snd_pcm_prepare(_playback_handle)) < 0) {
	    cerr << "cannot prepare audio interface for use"
		 << " (" << snd_strerror(err) << ")" << endl;
	    assert(false);
	}

	while(_active) {
	    assert(_callback);

	    if((err = snd_pcm_wait(_playback_handle, 1000)) < 0) {
		cerr << "Poll failed " << strerror(errno) << endl;
		break;
	    }

	    if((frames_to_deliver = snd_pcm_avail_update(_playback_handle)) < 0) {
		if(frames_to_deliver == -EPIPE) {
		    cerr << "an xrun occured" << endl;
		} else {
		    cerr << "unknown ALSA snd_pcm_avail_update return value"
			 << "(" << frames_to_deliver << ")" << endl;
		    break;
		}
	    }

	    if(frames_to_deliver < _period_nframes) continue;

	    frames_to_deliver = frames_to_deliver > _period_nframes ? _period_nframes : frames_to_deliver;

	    cout << frames_to_deliver << endl;
	    assert( 0 == ((frames_to_deliver-1)&frames_to_deliver) );  // is power of 2.

	    if( _callback(frames_to_deliver, _callback_arg) != 0 ) {
		cerr << "Audio callback failed" << endl;
		break;
	    }

	    _convert_to_output(frames_to_deliver);

	skippit:
	    if((err = snd_pcm_writei(_playback_handle, _buf, frames_to_deliver)) < 0) {
		cerr << "write failed"
		     << " (" << snd_strerror(err) << ")" << endl;
		assert(false);
	    }

	}
	_active = false;
    }

    /**
     * \brief Convert, copy, and interleave _left and _right to _buf;
     */
    void AlsaAudioSystem::_convert_to_output(uint32_t nframes)
    {
	switch(_type) {
	case INT: _convert_to_output_int(nframes); break;
	case UINT: _convert_to_output_uint(nframes); break;
	case FLOAT: _convert_to_output_float(nframes); break;
	default: assert(false);
	}
    }

    void AlsaAudioSystem::_convert_to_output_int(uint32_t nframes)
    {
	assert(false);
    }

    void AlsaAudioSystem::_convert_to_output_uint(uint32_t nframes)
    {
	assert(false);
    }

    void AlsaAudioSystem::_convert_to_output_float(uint32_t nframes)
    {
	float *out;
	uint32_t f;
	assert(_bits == 32);
	assert(_little_endian);
	out = (float*)_buf;
	for(f=0 ; f<nframes ; ++f) {
	    (*out++) = _left[f];
	    (*out++) = _right[f];
	}	    
    }

} // namespace StretchPlayer
