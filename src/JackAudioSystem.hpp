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
#ifndef JACKAUDIOSYSTEM_HPP
#define JACKAUDIOSYSTEM_HPP

#include <AudioSystem.hpp>
#include <jack/jack.h>

namespace StretchPlayer
{
    class Configuration;

    /**
     * \brief Pure virtual interface to an audio driver API.
     *
     * This AudioSystem assumes a very simple system with two audio
     * outputs.  It maintains those ports and buffers and the
     * connection of them.
     */
    class JackAudioSystem : public AudioSystem
    {
    public:
	JackAudioSystem();
	virtual ~JackAudioSystem();

	/* Implementing all of AudioSystem's interface:
	 */
	virtual int init(QString * app_name, Configuration *config, QString *err_msg = 0);
	virtual void cleanup();
	virtual int set_process_callback(process_callback_t cb, void* arg, QString* err_msg = 0);
	virtual int set_segment_size_callback(segment_size_callback_t cb, void* arg, QString* err_msg = 0);
	virtual int activate(QString *err_msg = 0);
	virtual int deactivate(QString *err_msg = 0);
	virtual sample_t* output_buffer(int index);
	virtual uint32_t output_buffer_size(int index);
	virtual uint32_t sample_rate();
	virtual float dsp_load();
	virtual uint32_t time_stamp();
	virtual uint32_t segment_start_time_stamp();
	virtual uint32_t current_segment_size();

    private:
	jack_client_t *_client;
	jack_port_t* _port[2];
    };

} // namespace StretchPlayer

#endif // AUDIOSYSTEM_HPP
