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

#include "JackAudioSystem.hpp"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <jack/jack.h>
#include <QString>

namespace StretchPlayer
{
    JackAudioSystem::JackAudioSystem() :
	_client(0)
    {
	_port[0] = 0;
	_port[1] = 0;
    }

    JackAudioSystem::~JackAudioSystem()
    {
	cleanup();
    }

    int JackAudioSystem::init(QString * app_name, QString *err_msg)
    {
	QString name("StretchPlayer"), err;

	if(app_name) {
	    name = *app_name;
	}

	_client = jack_client_open(name.toAscii(), JackNullOption, 0);
	if(!_client) {
	    err = "Could not set up JACK";
	    goto init_bail;
	}

	_port[0] = jack_port_register( _client,
					"left",
					JACK_DEFAULT_AUDIO_TYPE,
					JackPortIsOutput,
					0 );
	if(!_port[0]) {
	    err = "Could not set up left out";
	    goto init_bail;
	}

	_port[1] = jack_port_register( _client,
					"right",
					JACK_DEFAULT_AUDIO_TYPE,
					JackPortIsOutput,
					0 );
	if(!_port[1]) {
	    err = "Could not set up right out";
	    goto init_bail;
	}

	return 0;

    init_bail:
	if(err_msg) {
	    *err_msg = err;
	}
	return 0xDEADBEEF;
    }

    void JackAudioSystem::cleanup()
    {
	deactivate();
	if( _port[0] ) {
	    assert(_client);
	    jack_port_unregister(_client, _port[0]);
	    _port[0] = 0;
	}
	if( _port[1] ) {
	    assert(_client);
	    jack_port_unregister(_client, _port[1]);
	    _port[1] = 0;
	}
	if(_client) {
	    jack_client_close(_client);
	    _client = 0;
	}
    }

    int JackAudioSystem::set_process_callback(process_callback_t cb, void* arg, QString* err_msg)
    {
	assert(_client);

	int rv = jack_set_process_callback( _client,
					    cb,
					    arg );
	if(rv && err_msg) {
	    *err_msg = "Could not set up jack callback.";
	}
	return rv;
    }

    int JackAudioSystem::activate(QString *err_msg)
    {
	assert(_client);
	assert(_port[0]);
	assert(_port[1]);

	jack_activate(_client);

	// Autoconnection to first two ports we find.
	const char** ports = jack_get_ports( _client,
					     0,
					     JACK_DEFAULT_AUDIO_TYPE,
					     JackPortIsInput
	    );
	int k, rv = 0;
	for( k=0 ; ports && ports[k] != 0 ; ++k ) {
	    if(k==0) {
		rv = jack_connect( _client,
				   jack_port_name(_port[0]),
				   ports[k] );
	    } else if (k==1) {
		rv = jack_connect( _client,
				   jack_port_name(_port[1]),
				   ports[k] );
	    } else {
		break;
	    }
	    if( rv && err_msg ) {
		*err_msg = "Could not connect output ports";
	    }
	}
	if(k==0 && err_msg) {
	    *err_msg = "There were no output ports to connect to.";
	    rv = 1;
	}
	if(ports) {
	    free(ports);
	}
	return rv;
    }

    int JackAudioSystem::deactivate(QString *err_msg)
    {
	int rv = 0;
	if(_client) {
	    rv = jack_deactivate(_client);
	}
	return rv;
    }

    AudioSystem::sample_t* JackAudioSystem::output_buffer(int index)
    {
	jack_nframes_t nframes = output_buffer_size(index);

	if(index == 0) {
	    assert(_port[0]);
	    return static_cast<float*>( jack_port_get_buffer(_port[0], nframes) );
	}else if(index == 1) {
	    assert(_port[1]);
	    return static_cast<float*>( jack_port_get_buffer(_port[1], nframes) );
	}
	return 0;
    }

    uint32_t JackAudioSystem::output_buffer_size(int /*index*/)
    {
	if( !_client ) return 0;
	return jack_get_buffer_size(_client);
    }

    uint32_t JackAudioSystem::sample_rate()
    {
	if( !_client ) return 0;
	return jack_get_sample_rate(_client);
    }

    float JackAudioSystem::dsp_load()
    {
	if( !_client )  return -1;
	return jack_cpu_load(_client)/100.0;
    }

    uint32_t JackAudioSystem::time_stamp()
    {
	if( !_client ) return 0;
	return jack_frame_time(_client);
    }

    uint32_t JackAudioSystem::segment_start_time_stamp()
    {
	if( !_client ) return 0;
	return jack_last_frame_time(_client);
    }

} // namespace StretchPlayer
