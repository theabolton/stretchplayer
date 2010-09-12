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
#ifndef STRETCHPLAYER_CONFIGURATION_HPP
#define STRETCHPLAYER_CONFIGURATION_HPP

#include <QtCore/QString>
#include <cassert>

namespace StretchPlayer
{

template <typename T>
class Property {
public:
    Property() {}
    Property(const T& t) : _d(t) {}
    ~Property() {}

    const T& operator() () { return _d; }
    void operator() (const T& t) { _d = t; }
private:
    T _d;
};

template <typename T>
class ReadOnlyProperty {
public:
    ReadOnlyProperty(void *parent) : _p(parent) {}
    ReadOnlyProperty(void *parent, const T& t) : 
	_p(parent),
	_d(t)
	{}
    ~ReadOnlyProperty() {}

    const T& operator() () { return _d; }
    void set(void *parent, const T& value) {
	if(parent == _p) {
	    _d = value;
	} else {
	    assert(parent == _p);
	}
    }

private:
    void operator() (const T& t) { _d = t; }

    void *_p;
    T _d;
};


/**
 * Application configuration manager.
 *
 * Note that this isn't a fully flexable configuration object.
 * It is intended that the data remain constant after being
 * initialized.
 */
class Configuration
{
public:
    typedef enum { JackDriver = 1, AlsaDriver = 2 } driver_t;

    Configuration(int argc, char* argv[]);
    ~Configuration();

    void copyright();
    void usage();

    ReadOnlyProperty<QString> version;
    ReadOnlyProperty<bool> ok;
    Property<driver_t> driver;
    Property<QString>  audio_device;
    Property<unsigned> sample_rate;
    Property<unsigned> period_size;
    Property<unsigned> periods_per_buffer;
    Property<QString>  startup_file;
    Property<bool>     autoconnect; // Automatically connect to first 2 outputs
    Property<bool>     compositing;
    Property<bool>     quiet;
    Property<bool>     help;

private:
    void init(int argc, char* argv[]);

}; // class Configuration

} // namespace StretchPlayer

#endif // STRETCHPLAYER_CONFIGURATION_HPP
