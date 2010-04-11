/*
 * Copyright(c) 2010 by Gabriel M. Beddingfield <gabriel@teuton.org>
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

#include "RubberBandServer.hpp"
#include <rubberband/RubberBandStretcher.h>
#include <unistd.h>
#include <cassert>

using RubberBand::RubberBandStretcher;

namespace StretchPlayer
{
    RubberBandServer::RubberBandServer(uint32_t sample_rate) :
	_running(true),
	_time_ratio_param(1.0),
	_pitch_scale_param(1.0),
	_reset_param(true)
    {
	_stretcher.reset(
	    new RubberBandStretcher( sample_rate,
				     2,
				     RubberBandStretcher::OptionProcessRealTime
				     | RubberBandStretcher::OptionThreadingAuto
		)
	    );

	const uint32_t MAXBUF = 1 << 14; // 16384

	_stretcher->setMaxProcessSize(MAXBUF); // 16384

	_inputs[0].reset( new ringbuffer_t(MAXBUF) );
	_inputs[1].reset( new ringbuffer_t(MAXBUF) );
	_outputs[0].reset( new ringbuffer_t(MAXBUF) );
	_outputs[1].reset( new ringbuffer_t(MAXBUF) );

    }

    RubberBandServer::~RubberBandServer()
    {
    }

    void RubberBandServer::start()
    {
	QThread::start();
    }

    void RubberBandServer::shutdown()
    {
	_running = false;
    }

    bool RubberBandServer::is_running()
    {
	return QThread::isRunning();
    }

    void RubberBandServer::wait()
    {
	QThread::wait();
    }

    void RubberBandServer::reset()
    {
	QMutexLocker lk(&_param_mutex);
	_reset_param = true;
    }

    void RubberBandServer::time_ratio(float val)
    {
	QMutexLocker lk(&_param_mutex);
	_time_ratio_param = val;
    }

    float RubberBandServer::time_ratio()
    {
	return _time_ratio_param;
    }

    void RubberBandServer::pitch_scale(float val)
    {
	QMutexLocker lk(&_param_mutex);
	_pitch_scale_param = val;
    }

    float RubberBandServer::pitch_scale()
    {
	return _pitch_scale_param;
    }

    void RubberBandServer::go_idle()
    {
	setPriority(QThread::IdlePriority);
    }

    void RubberBandServer::go_active()
    {
	setPriority(QThread::TimeCriticalPriority);
    }

    uint32_t RubberBandServer::latency() const
    {
	return _stretcher->getLatency();
    }

    uint32_t RubberBandServer::available_write()
    {
	uint32_t l, r;
	l = _inputs[0]->write_space();
	r = _inputs[1] ->write_space();
	return (l < r) ? l : r;
    }

    uint32_t RubberBandServer::write_audio(float* left, float* right, uint32_t count)
    {
	unsigned l, r, max = available_write();
	if( count > max ) count = max;
	l = _inputs[0]->write(left, count);
	r = _inputs[1]->write(right, count);
	// _have_new_data.wakeAll();
	assert( l == r );
	return l;
    }

    uint32_t RubberBandServer::available_read()
    {
	unsigned l, r;
	l = _outputs[0]->read_space();
	r = _outputs[1]->read_space();
	return (l<r) ? l : r;
    }

    uint32_t RubberBandServer::read_audio(float* left, float* right, uint32_t count)
    {
	unsigned l, r, max = available_read();
	if( count > max ) count = max;
	l = _outputs[0]->read(left, count);
	r = _outputs[1]->read(right, count);
	// _room_for_output.wakeAll();
	assert( l == r );
	return l;
    }

    void RubberBandServer::run()
    {
	uint32_t read_l, read_r, nget;
	uint32_t write_l, write_r, nput;
	uint32_t tmp;
	const unsigned BUFSIZE = 512;
	float* bufs[2];
	float left[BUFSIZE], right[BUFSIZE];
	float time_ratio, pitch_scale;
	bool reset;
	bool proc_output;

	bufs[0] = left;
	bufs[1] = right;

	QMutexLocker lock(&_param_mutex);
	time_ratio = _time_ratio_param;
	pitch_scale = _pitch_scale_param;
	lock.unlock();

	while(_running) {
	    read_l = _inputs[0]->read_space();
	    read_r = _inputs[1]->read_space();
	    nget = (read_l < read_r) ? read_l : read_r;
	    if(nget > BUFSIZE) nget = BUFSIZE;
	    if(nget > _stretcher->getSamplesRequired()) nget = _stretcher->getSamplesRequired();
	    if(_stretcher->available() > 0) nget = 0;
	    if(nget) {
		tmp = _inputs[0]->read(left, nget);
		assert( tmp == nget );
		tmp = _inputs[1]->read(right, nget);
		assert( tmp == nget );
		lock.relock();
		time_ratio = _time_ratio_param;
		pitch_scale = _pitch_scale_param;
		reset = _reset_param;
		_reset_param = false;
		lock.unlock();
		if(reset) {
		    #warning "Hmmmm... this isn't thread safe.  How can we make it so?"
		    // std::cout << "reset()" << std::endl;
		    _stretcher->reset();
		    _inputs[0]->reset();
		    _inputs[1]->reset();
		    _outputs[0]->reset();
		    _outputs[1]->reset();
		}
		_stretcher->setTimeRatio(time_ratio);
		_stretcher->setPitchScale(pitch_scale);
		_stretcher->process(bufs, nget, false);
	    }
	    proc_output = false;
	    nput = 1;
	    while(_stretcher->available() > 0 && nput) {
		write_l = _outputs[0]->write_space();
		write_r = _outputs[1]->write_space();
		nput = (write_l < write_r) ? write_l : write_r;
		if(nput) {
		    proc_output = true;
		    if(nput > BUFSIZE) nput = BUFSIZE;
		    tmp = _stretcher->retrieve(bufs, nput);
		    _outputs[0]->write(left, tmp);
		    _outputs[1]->write(right, tmp);
		}
	    }
	    if( (nget == 0) && (! proc_output) ) {
		usleep(100);
	    }
	}
    }

} // namespace StretchPlayer
