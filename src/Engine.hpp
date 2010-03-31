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
#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <jack/jack.h>
#include <QString>
#include <QMutex>
#include <vector>

namespace StretchPlayer
{

class Engine
{
public:
    Engine();
    ~Engine();

    void load_song(const QString& filename);
    void play();
    void stop();
    void set_stretch(float factor);  // [0.5, 2.0] :: 1.0 == no stretch
    float get_position(); // in seconds
    float get_length();   // in seconds

private:
    static int static_jack_callback(jack_nframes_t nframes, void* arg) {
	Engine *e = static_cast<Engine*>(arg);
	return e->jack_callback(nframes);
    }

    int jack_callback(jack_nframes_t nframes);

    void _zero_buffers(jack_nframes_t nframes);
    void _process_playing(jack_nframes_t nframes);

    jack_client_t* _jack_client;
    jack_port_t *_port_left, *_port_right;

    bool _playing;
    QMutex _audio_lock;
    std::vector<float> _left;
    std::vector<float> _right;
    unsigned long _position;
    float _sample_rate;

}; // Engine

} // namespace StretchPlayer

#endif // ENGINE_HPP
