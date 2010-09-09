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
#ifndef ALSAAUDIOSYSTEMPRIVATE_HPP
#define ALSAAUDIOSYSTEMPRIVATE_HPP

#include <QtCore/QThread>

namespace StretchPlayer
{
    /**
     * \brief Thread for AlsaAudioSystem's main loop.
     *
     */
    class AlsaAudioSystemPrivate : public QThread
    {
    public:
	typedef void (*callback_t)(AlsaAudioSystem*);

	AlsaAudioSystemPrivate() :
	    _run_callback(0),
	    _parent(0)
	    {}

	virtual ~AlsaAudioSystemPrivate()
	    {}

	void parent(AlsaAudioSystem *parent) {
	    _parent = parent;
	}
	AlsaAudioSystem* parent() {
	    return _parent;
	}

	void run_callback( callback_t run_callback ) {
	    _run_callback = run_callback;
	}
	callback_t run_callback() {
	    return _run_callback;
	}

    private:
	virtual void run() {
	    (*_run_callback)(_parent);
	}

	callback_t _run_callback;
	AlsaAudioSystem *_parent;
    };

} // namespace StretchPlayer

#endif // AUDIOSYSTEM_HPP
