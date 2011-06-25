/*
    Copyright (C) 2000 Paul Davis & Benno Senoner

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/
/*  This file came from the Ardour sources, SVN Rev 3435 2008-06-02.
    Changed file name and added Tritium namespaces and include guards.
    - Gabriel Beddingfield 2009-04-17, 2009-11-25
*/

#ifndef TRITIUM_RINGBUFFER_HPP
#define TRITIUM_RINGBUFFER_HPP

#include <cstring>
#include <QAtomicInt>

namespace Tritium
{

template<class T>
class RingBuffer 
{
  public:
	RingBuffer (unsigned sz) {
//	size = ffs(sz); /* find first [bit] set is a single inlined assembly instruction. But it looks like the API rounds up so... */
	unsigned power_of_two;
	for (power_of_two = 1; 1U<<power_of_two < sz; power_of_two++);
		size = 1<<power_of_two;
		size_mask = size;
		size_mask -= 1;
		buf = new T[size];
		reset ();

	};
	
	virtual ~RingBuffer() {
		delete [] buf;
	}

	void reset () {
		/* !!! NOT THREAD SAFE !!! */
		write_idx.fetchAndStoreOrdered(0);
		read_idx.fetchAndStoreOrdered(0);
	}

	void set (unsigned r, unsigned w) {
		/* !!! NOT THREAD SAFE !!! */
		write_idx.fetchAndStoreOrdered(w);
		read_idx.fetchAndStoreOrdered(r);
	}
	
	unsigned read  (T *dest, unsigned cnt);
	unsigned  write (T *src, unsigned cnt);

	struct rw_vector {
	    T *buf[2];
	    unsigned len[2];
	};

	void get_read_vector (rw_vector *);
	void get_write_vector (rw_vector *);
	
	void decrement_read_idx (unsigned cnt) {
		read_idx.fetchAndStoreOrdered(
			read_idx.fetchAndAddOrdered( - (int)cnt ) & size_mask
			);
	}                

	void increment_read_idx (unsigned cnt) {
		read_idx.fetchAndStoreOrdered(
			read_idx.fetchAndAddOrdered( (int)cnt ) & size_mask
			);
	}                

	void increment_write_idx (unsigned cnt) {
		write_idx.fetchAndStoreOrdered(
			write_idx.fetchAndAddOrdered( (int)cnt ) & size_mask
			);
	}                

	unsigned write_space () {
		unsigned w, r;
		
		w = (int) write_idx;
		r = (int) read_idx;
		
		if (w > r) {
			return ((r - w + size) & size_mask) - 1;
		} else if (w < r) {
			return (r - w) - 1;
		} else {
			return size - 1;
		}
	}
	
	unsigned read_space () {
		unsigned w, r;
		
		w = (int) write_idx;
		r = (int) read_idx;
		
		if (w > r) {
			return w - r;
		} else {
			return (w - r + size) & size_mask;
		}
	}

	T *buffer () { return buf; }
	unsigned get_write_idx () const { return (int)write_idx; }
	unsigned get_read_idx () const { return (int)read_idx; }
	unsigned bufsize () const { return size; }

  protected:
	T *buf;
	unsigned size;
	mutable QAtomicInt write_idx;
	mutable QAtomicInt read_idx;
	unsigned size_mask;
};

template<class T> unsigned 
RingBuffer<T>::read (T *dest, unsigned cnt)
{
        unsigned free_cnt;
        unsigned cnt2;
        unsigned to_read;
        unsigned n1, n2;
        unsigned priv_read_idx;

        priv_read_idx = (int) read_idx;

        if ((free_cnt = read_space ()) == 0) {
                return 0;
        }

        to_read = cnt > free_cnt ? free_cnt : cnt;
        
        cnt2 = priv_read_idx + to_read;

        if (cnt2 > size) {
                n1 = size - priv_read_idx;
                n2 = cnt2 & size_mask;
        } else {
                n1 = to_read;
                n2 = 0;
        }
        
        memcpy (dest, &buf[priv_read_idx], n1 * sizeof (T));
        priv_read_idx = (priv_read_idx + n1) & size_mask;

        if (n2) {
                memcpy (dest+n1, buf, n2 * sizeof (T));
                priv_read_idx = n2;
        }

        read_idx.fetchAndStoreOrdered(priv_read_idx);
        return to_read;
}

template<class T> unsigned
RingBuffer<T>::write (T *src, unsigned cnt)

{
        unsigned free_cnt;
        unsigned cnt2;
        unsigned to_write;
        unsigned n1, n2;
        unsigned priv_write_idx;

        priv_write_idx = (int) write_idx;

        if ((free_cnt = write_space ()) == 0) {
                return 0;
        }

        to_write = cnt > free_cnt ? free_cnt : cnt;
        
        cnt2 = priv_write_idx + to_write;

        if (cnt2 > size) {
                n1 = size - priv_write_idx;
                n2 = cnt2 & size_mask;
        } else {
                n1 = to_write;
                n2 = 0;
        }

        memcpy (&buf[priv_write_idx], src, n1 * sizeof (T));
        priv_write_idx = (priv_write_idx + n1) & size_mask;

        if (n2) {
                memcpy (buf, src+n1, n2 * sizeof (T));
                priv_write_idx = n2;
        }

	write_idx.fetchAndStoreOrdered( priv_write_idx );
        return to_write;
}

template<class T> void
RingBuffer<T>::get_read_vector (RingBuffer<T>::rw_vector *vec)

{
	unsigned free_cnt;
	unsigned cnt2;
	unsigned w, r;
	
	w = (int) write_idx;
	r = (int) read_idx;
	
	if (w > r) {
		free_cnt = w - r;
	} else {
		free_cnt = (w - r + size) & size_mask;
	}

	cnt2 = r + free_cnt;

	if (cnt2 > size) {
		/* Two part vector: the rest of the buffer after the
		   current write ptr, plus some from the start of 
		   the buffer.
		*/

		vec->buf[0] = &buf[r];
		vec->len[0] = size - r;
		vec->buf[1] = buf;
		vec->len[1] = cnt2 & size_mask;

	} else {
		
		/* Single part vector: just the rest of the buffer */
		
		vec->buf[0] = &buf[r];
		vec->len[0] = free_cnt;
		vec->len[1] = 0;
	}
}

template<class T> void
RingBuffer<T>::get_write_vector (RingBuffer<T>::rw_vector *vec)

{
	unsigned free_cnt;
	unsigned cnt2;
	unsigned w, r;
	
	w = (int) write_idx;
	r = (int) read_idx;
	
	if (w > r) {
		free_cnt = ((r - w + size) & size_mask) - 1;
	} else if (w < r) {
		free_cnt = (r - w) - 1;
	} else {
		free_cnt = size - 1;
	}
	
	cnt2 = w + free_cnt;

	if (cnt2 > size) {
		
		/* Two part vector: the rest of the buffer after the
		   current write ptr, plus some from the start of 
		   the buffer.
		*/

		vec->buf[0] = &buf[w];
		vec->len[0] = size - w;
		vec->buf[1] = buf;
		vec->len[1] = cnt2 & size_mask;
	} else {
		vec->buf[0] = &buf[w];
		vec->len[0] = free_cnt;
		vec->len[1] = 0;
	}
}

} // namespace Tritium

#endif // TRITIUM_RINGBUFFER_HPP
