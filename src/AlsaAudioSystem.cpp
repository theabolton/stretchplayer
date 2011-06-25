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
#include <cstdio> // For snprintf
#include <QString>
#include <alsa/asoundlib.h>
#include <sys/time.h>
#include <cmath>

#include "bams_format.h"
#include <endian.h>

#include <iostream>
using namespace std;

/* Formats supported by this class, in order or preference
 */
static const snd_pcm_format_t aas_supported_formats[] = {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    SND_PCM_FORMAT_FLOAT_LE,
    SND_PCM_FORMAT_S16_LE,
    SND_PCM_FORMAT_FLOAT_BE,
    SND_PCM_FORMAT_S16_BE,
    SND_PCM_FORMAT_U16_LE,
    SND_PCM_FORMAT_U16_BE,
#elif __BYTE_ORDER == __BIG_ENDIAN
    SND_PCM_FORMAT_FLOAT_BE,
    SND_PCM_FORMAT_S16_BE,
    SND_PCM_FORMAT_FLOAT_LE,
    SND_PCM_FORMAT_S16_LE,
    SND_PCM_FORMAT_U16_BE,
    SND_PCM_FORMAT_U16_LE,
#else
#error Unsupport byte order.
#endif
    SND_PCM_FORMAT_UNKNOWN
};

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
	_period_nframes(512),
	_active(false),
	_playback_handle(0),
	_left_root(0),
	_right_root(0),
	_left(0),
	_right(0),
	_callback(0),
	_callback_arg(0),
	_dsp_load_pos(0),
	_dsp_load(0.0f),
	_d(0)
    {
	memset(&_dsp_a, 0, sizeof(timeval));
	memset(&_dsp_b, 0, sizeof(timeval));
	memset(_dsp_idle_time, 0, sizeof(_dsp_idle_time));
	memset(_dsp_work_time, 0, sizeof(_dsp_work_time));

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
	QString emsg;
	unsigned nfrags = 3;
	int err;
	snd_pcm_format_t format = SND_PCM_FORMAT_UNKNOWN;
	int k;

	if(app_name) {
	    name = *app_name;
	}

	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;
	int nfds;
	struct pollfd *pfds;

	if((err = snd_pcm_open(&_playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
	    emsg = QString("cannot open default ALSA audio device (%1)")
		.arg(snd_strerror(err));
	    goto init_bail;
	}
	assert(_playback_handle);

	if((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
	    emsg = QString("cannot allocate hardware parameter structure (%1)")
		.arg( snd_strerror(err) );
	    goto init_bail;
	}

	if((err = snd_pcm_hw_params_any(_playback_handle, hw_params)) < 0) {
	    emsg = QString("cannot initialize hardware parameter structure (%1)")
		.arg( snd_strerror(err) );
	    goto init_bail;
	}

	if((err = snd_pcm_hw_params_set_access(_playback_handle, hw_params,
					       SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
	    emsg = QString("cannot set access type (%1)")
		.arg( snd_strerror(err) );
	    goto init_bail;
	}

	for(k = 0 ; aas_supported_formats[k] != SND_PCM_FORMAT_UNKNOWN ; ++k) {
	    format = aas_supported_formats[k];
	    if(snd_pcm_hw_params_test_format(_playback_handle, hw_params, format)) {
		format = SND_PCM_FORMAT_UNKNOWN;
	    } else {
		break;
	    }
	}

	switch(format) {
	case SND_PCM_FORMAT_FLOAT_LE:
	    _type = FLOAT;
	    _bits = 32;
	    _little_endian = true;
	    break;
	case SND_PCM_FORMAT_S16_LE:
	    _type = INT;
	    _bits = 16;
	    _little_endian = true;
	    break;
	case SND_PCM_FORMAT_FLOAT_BE:
	    _type = FLOAT;
	    _bits = 32;
	    _little_endian = false;
	    break;
	case SND_PCM_FORMAT_S16_BE:
	    _type = INT;
	    _bits = 16;
	    _little_endian = false;
	    break;
	case SND_PCM_FORMAT_U16_LE:
	    _type = UINT;
	    _bits = 32;
	    _little_endian = true;
	    break;
	case SND_PCM_FORMAT_U16_BE:
	    _type = UINT;
	    _bits = 32;
	    _little_endian = false;
	    break;
	case SND_PCM_FORMAT_UNKNOWN:
	    emsg = QString("The audio card does not support any PCM audio formats"
			   " that StretchPlayer supports");
	    goto init_bail;
	    break;
	default:
	    assert(false);
	}

	if((err = snd_pcm_hw_params_set_format(_playback_handle, hw_params, format)) < 0) {
	    emsg = QString("cannot set sample format (%1)")
		.arg( snd_strerror(err) );
	    goto init_bail;
	}

	if((err = snd_pcm_hw_params_set_rate_near(_playback_handle, hw_params, &_sample_rate, 0)) < 0) {
	    emsg = QString("cannot set sample rate (%1)")
		.arg( snd_strerror(err) );
	    goto init_bail;
	}

	if((err = snd_pcm_hw_params_set_channels(_playback_handle, hw_params, 2)) < 0) {
	    emsg = QString("cannot set channel count (%1)")
		.arg( snd_strerror(err) );
	    goto init_bail;
	}

	if((err = snd_pcm_hw_params_set_periods_near(_playback_handle, hw_params, &nfrags, 0)) < 0) {
	    emsg = QString("cannot set the period count (%1)")
		.arg( snd_strerror(err) );
	    goto init_bail;
	}

	if((err = snd_pcm_hw_params_set_buffer_size(_playback_handle, hw_params, _period_nframes * nfrags)) < 0) {
	    emsg = QString("cannot set the buffer size to %1 x %2 (%3)")
		.arg( nfrags )
		.arg( _period_nframes )
		.arg( snd_strerror(err) );
	    goto init_bail;
	}

	if((err = snd_pcm_hw_params(_playback_handle, hw_params)) < 0) {
	    emsg = QString("cannot set parameters (%1)")
		.arg( snd_strerror(err) );
	    goto init_bail;
	}

	snd_pcm_hw_params_free(hw_params);

	/* Tell ALSA to wake us up whenever _period_nframes or more frames
	 * of playback data can be delivered.  Also, tell ALSA
	 * that we'll start the device ourselves.
	 */

	if((err = snd_pcm_sw_params_malloc(&sw_params)) < 0) {
	    emsg = QString("cannot allocate software parameters structure (%1)")
		.arg( snd_strerror(err) );
	    goto init_bail;
	}

	if((err = snd_pcm_sw_params_current(_playback_handle, sw_params)) < 0) {
	    emsg = QString("cannot initialize software parameters structure (%1)")
		.arg( snd_strerror(err) );
	    goto init_bail;
	}

	if((err = snd_pcm_sw_params_set_avail_min(_playback_handle, sw_params, _period_nframes)) < 0) {
	    emsg = QString("cannot set minimum available count (%1)")
		.arg( snd_strerror(err) );
	    goto init_bail;
	}

	if((err = snd_pcm_sw_params_set_start_threshold(_playback_handle, sw_params, 0U)) < 0) {
	    emsg = QString("cannot set start mode (%1)")
		.arg( snd_strerror(err) );
	    goto init_bail;
	}
	if((err = snd_pcm_sw_params(_playback_handle, sw_params)) < 0) {
	    emsg = QString("cannot set software parameters (%1)")
		.arg( snd_strerror(err) );
	    goto init_bail;
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
	    *err_msg = emsg;
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

    int AlsaAudioSystem::set_segment_size_callback(process_callback_t, void*, QString*)
    {
	// This API never changes the segment size automatically
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
	return _dsp_load;
    }

    uint32_t AlsaAudioSystem::time_stamp()
    {
	return 0;
    }

    uint32_t AlsaAudioSystem::segment_start_time_stamp()
    {
	return 0;
    }

    uint32_t AlsaAudioSystem::current_segment_size()
    {
	return _period_nframes;
    }

    static inline unsigned long calc_elapsed(const timeval& a, const timeval& b)
    {
	unsigned long ans;
	if(b.tv_sec < a.tv_sec)
	    return 0;
	ans = (b.tv_sec - a.tv_sec) * 100000 + b.tv_usec;
	if(ans >= a.tv_usec)
	    ans -= a.tv_usec;
	return ans;
    }

    void AlsaAudioSystem::_stopwatch_init()
    {
	gettimeofday(&_dsp_a, 0);
    }

    void AlsaAudioSystem::_stopwatch_start_idle()
    {
	gettimeofday(&_dsp_b, 0);
	_dsp_work_time[_dsp_load_pos] = calc_elapsed(_dsp_a, _dsp_b);
	_dsp_load_update();
	_dsp_a = _dsp_b;
	++_dsp_load_pos;
	if(_dsp_load_pos > DSP_AVG_SIZE)
	    _dsp_load_pos = 0;
    }

    void AlsaAudioSystem::_stopwatch_start_work()
    {
	gettimeofday(&_dsp_b, 0);
	_dsp_idle_time[_dsp_load_pos] = calc_elapsed(_dsp_a, _dsp_b);
	_dsp_a = _dsp_b;
    }

    void AlsaAudioSystem::_dsp_load_update()
    {
	int k = DSP_AVG_SIZE;
	unsigned long work = 0, idle = 0, tot;
	while(k--) {
	    idle += _dsp_idle_time[k];
	    work += _dsp_work_time[k];
	}
	tot = work + idle;
	if(tot)
	    _dsp_load = float(work) / float(tot);
	else
	    _dsp_load = 0.0f;
	assert(_dsp_load <= 1.0f);
	assert(_dsp_load >= 0.0f);
	assert( !isnan(_dsp_load) );
    }

    void AlsaAudioSystem::_run()
    {
	int err;
	snd_pcm_sframes_t frames_to_deliver;
	uint32_t f;
	const char *err_msg, *str_err;
	const int misc_msg_size = 256;
	char misc_msg[misc_msg_size] = "";

	assert(_active);

	// Set RT priority
	sched_param thread_sched_param;
	thread_sched_param.sched_priority = 80;
	pthread_setschedparam( pthread_self(), SCHED_FIFO, &thread_sched_param );

	err = 0;

	/* the interface will interrupt the kernel every
	 * _period_nframes frames, and ALSA will wake up this program
	 * very soon after that.
	 */
	if((err = snd_pcm_prepare(_playback_handle)) < 0) {
	    err_msg = "Cannot prepare audio interface for use [snd_pcm_prepare()].";
	    str_err = snd_strerror(err);
	    goto run_bail;
	}

	_stopwatch_init();
	while(_active) {
	    assert(_callback);

	    _stopwatch_start_idle();
	    if((err = snd_pcm_wait(_playback_handle, 1000)) < 0) {
		err_msg = "Audio poll failed [snd_pcm_wait()].";
		str_err = strerror(errno);
		goto run_bail;
	    }

	    _stopwatch_start_work();
	    if((frames_to_deliver = snd_pcm_avail_update(_playback_handle)) < 0) {
		if(frames_to_deliver == -EPIPE) {
		    /* An XRUN Occurred.  Ignoring. */
		} else {
		    err_msg = "Unknown ALSA snd_pcm_avail_update return value [snd_pcm_avail_update()].";
		    snprintf(misc_msg, misc_msg_size, "%ld", frames_to_deliver);
		    str_err = misc_msg;
		    goto run_bail;
		}
	    }

	    if(frames_to_deliver < _period_nframes) continue;

	    frames_to_deliver = frames_to_deliver > _period_nframes ? _period_nframes : frames_to_deliver;

	    assert( 0 == ((frames_to_deliver-1)&frames_to_deliver) );  // is power of 2.

	    if( _callback(frames_to_deliver, _callback_arg) != 0 ) {
		err_msg = "Application's audio callback failed.";
		str_err = 0;
		goto run_bail;
	    }

	    _convert_to_output(frames_to_deliver);

	    if((err = snd_pcm_writei(_playback_handle, _buf, frames_to_deliver)) < 0) {
		err_msg = "Write to audio card failed [snd_pcm_writei()].";
		str_err = snd_strerror(err);
		goto run_bail;
	    }

	}

	_active = false;

	return;

    run_bail:

	_active = false;
	thread_sched_param.sched_priority = 0;
	pthread_setschedparam( pthread_self(), SCHED_OTHER, &thread_sched_param );

	cerr << "ERROR: " << err_msg;
	if(str_err)
	    cerr << " (" << str_err << ")";
	cerr << endl;
	cerr << "Aborting audio driver." << endl;

	return;
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
	switch(_bits) {
	case 16: {
	    bams_sample_s16le_t *dst = (bams_sample_s16le_t*)_buf;
#if __BYTE_ORDER == __LITTLE_ENDIAN
	    if(_little_endian) {
		bams_copy_s16le_floatle(dst, 2, &_left[0], 1, nframes);
		bams_copy_s16le_floatle(dst+1, 2, &_right[0], 1, nframes);
	    } else {
		bams_copy_s16be_floatle(dst, 2, &_left[0], 1, nframes);
		bams_copy_s16be_floatle(dst+1, 2, &_right[0], 1, nframes);
	    }
#else
	    if(_little_endian) {
		bams_copy_s16le_floatbe(dst, 2, &_left[0], 1, nframes);
		bams_copy_s16le_floatbe(dst+1, 2, &_right[0], 1, nframes);
	    } else {
		bams_copy_s16be_floatbe(dst, 2, &_left[0], 1, nframes);
		bams_copy_s16be_floatbe(dst+1, 2, &_right[0], 1, nframes);
	    }
#endif
	}   break;
	case 8:
	case 24:
	case 32:
	default:
	    assert(false);
	}
    }

    void AlsaAudioSystem::_convert_to_output_uint(uint32_t nframes)
    {
	switch(_bits) {
	case 16: {
	    bams_sample_u16le_t *dst = (bams_sample_u16le_t*)_buf;
#if __BYTE_ORDER == __LITTLE_ENDIAN
	    if(_little_endian) {
		bams_copy_u16le_floatle(dst, 2, &_left[0], 1, nframes);
		bams_copy_u16le_floatle(dst+1, 2, &_right[0], 1, nframes);
	    } else {
		bams_copy_u16be_floatle(dst, 2, &_left[0], 1, nframes);
		bams_copy_u16be_floatle(dst+1, 2, &_right[0], 1, nframes);
	    }
#else
	    if(_little_endian) {
		bams_copy_u16le_floatbe(dst, 2, &_left[0], 1, nframes);
		bams_copy_u16le_floatbe(dst+1, 2, &_right[0], 1, nframes);
	    } else {
		bams_copy_u16be_floatbe(dst, 2, &_left[0], 1, nframes);
		bams_copy_u16be_floatbe(dst+1, 2, &_right[0], 1, nframes);
	    }
#endif
	}   break;
	case 8:
	case 24:
	case 32:
	default:
	    assert(false);
	}
    }

    void AlsaAudioSystem::_convert_to_output_float(uint32_t nframes)
    {
	float *out, *l, *r;
	uint32_t f, count;
	assert(_bits == 32);
	assert(_little_endian);
	out = (float*)_buf;
	l = &_left[0];
	r = &_right[0];
	count = nframes;
	while(count--) {
	    (*out++) = (*l++);
	    (*out++) = (*r++);
	}
	/* Check for non-native byte ordering */
#if __BYTE_ORDER == __LITTLE_ENDIAN
	if(!_little_endian) {
	    bams_byte_reorder_in_place(_buf, 4, 1, 2*nframes);
	}
#else
	if(_little_endian) {
	    bams_byte_reorder_in_place(_buf, 4, 1, 2*nframes);
	}	    
#endif
    }

} // namespace StretchPlayer
