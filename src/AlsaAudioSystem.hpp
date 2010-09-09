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
#ifndef ALSAAUDIOSYSTEM_HPP
#define ALSAAUDIOSYSTEM_HPP

#include <AudioSystem.hpp>
#include <alsa/asoundlib.h>

namespace StretchPlayer
{
    class AlsaAudioSystemPrivate;

    /**
     * \brief ALSA audio driver implementation.
     *
     */
    class AlsaAudioSystem : public AudioSystem
    {
    public:
	AlsaAudioSystem();
	virtual ~AlsaAudioSystem();

	/* Implementing all of AudioSystem's interface:
	 */
	virtual int init(QString * app_name, QString *err_msg = 0);
	virtual void cleanup();
	virtual int set_process_callback(process_callback_t cb, void* arg, QString* err_msg = 0);
	virtual int activate(QString *err_msg = 0);
	virtual int deactivate(QString *err_msg = 0);
	virtual sample_t* output_buffer(int index);
	virtual uint32_t output_buffer_size(int index);
	virtual uint32_t sample_rate();
	virtual float dsp_load();

    private:
	static void run(AlsaAudioSystem *that) {
	    that->_run();
	}
	void _run();
	void _convert_to_output(uint32_t nframes);
	void _convert_to_output_int(uint32_t nframes);
	void _convert_to_output_uint(uint32_t nframes);
	void _convert_to_output_float(uint32_t nframes);

    private:
	// Configuration variables:
	unsigned _channels;
	enum { INT, UINT, FLOAT } _type;
	unsigned _bits;
	bool _little_endian;
	uint32_t _sample_rate;
	uint32_t _period_nframes;

	// ALSA handles
	bool _active;
	snd_pcm_t *_playback_handle;
	float *_left_root, *_right_root;
	float *_left, *_right;
	unsigned short *_buf_root, *_buf;

	process_callback_t _callback;
	void *_callback_arg;

	// Private object
	AlsaAudioSystemPrivate *_d;

    };

} // namespace StretchPlayer

#endif // AUDIOSYSTEM_HPP