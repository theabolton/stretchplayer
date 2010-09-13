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

#include "Configuration.hpp"
#include "config.h"
#include <getopt.h>
#include <iostream>
#include <cstdlib>

using namespace std;

#define DEFAULT_SAMPLE_RATE "44100"
#define DEFAULT_PERIOD_SIZE "1024"
#define DEFAULT_PERIODS_PER_BUFFER "2"
#define DEFAULT_ALSA_DEVICE "hw:0"

namespace StretchPlayer
{
    typedef struct _stretchplayer_options_t
    {
	const char *optstring;
	struct option longopts; // {const char *name, int has_arg, int *flag, int val}
	const char *defaults;
	const char *doc;
    } stretchplayer_options_t;

    /* CAREFUL: Because defaults and doc are adjacent "const char*" fields,
     * a missing comma between them will concatenate the strings... and
     * everything in the structure after it will be misaligned (corrupt).
     */

    static const stretchplayer_options_t sp_opts[] = {
#ifdef AUDIO_SUPPORT_JACK
	{ "J",
	  {"jack", 0, 0, 'J'},
	  "on",
	  "use JACK for audio" },
#endif
#ifdef AUDIO_SUPPORT_ALSA
	{ "A",
	  {"alsa", 0, 0, 'A'},
#ifdef AUDIO_SUPPORT_JACK
	  "off",
#else
	  "on",
#endif
	  "use ALSA for audio" },

	{ "d:",
	  {"device", 1, 0, 'd'},
	  DEFAULT_ALSA_DEVICE,
	  "device to use for ALSA" },

	{ "r:",
	  {"sample-rate", 1, 0, 'r'},
	  DEFAULT_SAMPLE_RATE,
	  "sample rate to use for ALSA" },

	{ "p:",
	  {"period-size", 1, 0, 'p'},
	  DEFAULT_PERIOD_SIZE,
	  "period size to use for ALSA" },

	{ "n:",
	  {"periods", 1, 0, 'n'},
	  DEFAULT_PERIODS_PER_BUFFER,
	  "periods per buffer for ALSA" },
#endif

	{ "x",
	  {"no-autoconnect", 0, 0, 'x'},
	  "off",
	  "disable auto-connection ot ouputs" },

	{ "c",
	  {"compositing", 0, 0, 'c'},
	  "on",
	  "enable desktop compositing (if supported by Qt/X11)" },

	{ "C",
	  {"no-compositing", 0, 0, 'C'},
	  "off",
	  "disable desktop compositing" },

	{ "q",
	  {"quiet", 0, 0, 'q'},
	  "off",
	  "suppress most output to console" },

	{ "h",
	  {"help", 0, 0, 'h'},
	  "off",
	  "show help/usage and exit" }, // --help

	{ 0,
	  {0, 0, 0, 0},
	  0,
	  0 }
    };

    static char optstring[256];
    static struct option longopts[128];

    static const char usage_line[] = 
	"usage: stretchplayer [options] [audio_file_name]";

    static const char copyright_blurb[] =
	"StretchPlayer version " STRETCHPLAYER_VERSION ", Copyright 2010 Gabriel M. Beddingfield\n"
	"StretchPlayer comes with ABSOLUTELY NO WARRANTY;\n"
	"This is free software, and you are welcome to redistribute it\n"
	"under terms of the GNU Public License (ver. 2 or later)\n";

    static const char version[] = STRETCHPLAYER_VERSION;

    static void setup_options()
    {
	int os_pos = 0;
	int lo_pos = 0;
	const stretchplayer_options_t *it;

	memset(optstring, 0, sizeof(optstring));
	memset(longopts, 0, sizeof(longopts));

	for( it=sp_opts ; it->optstring != 0 ; ++it ) {
	    assert(os_pos < 256);
	    assert(lo_pos < 128);

	    assert( strnlen(it->optstring, 16) < 16 );
	    strncpy( &optstring[os_pos], it->optstring, strnlen(it->optstring, 16) );
	    os_pos += strnlen(it->optstring, 16);

	    memcpy( &longopts[lo_pos], &(it->longopts), sizeof(struct option) );
	    ++lo_pos;
	}
    }

    static void check_options_validity()
    {
	const char *str = optstring;
	const option *opts = longopts;

	assert(str);
	assert(opts);
	int pos = 0;
	while( opts->name != 0 ) {
	    assert( str[pos] );
	    assert( opts->val == str[pos] );

	    ++pos;
	    if( (str[pos] != 0) && (str[pos] == ':') ) {
		assert(opts->has_arg != 0);
		++pos;
	    } else {
		assert(opts->has_arg == 0);
	    }
	    ++opts;
	}
    }

    Configuration::Configuration(int argc, char* argv[]) :
	version(this, STRETCHPLAYER_VERSION),
	ok(this, false),
	driver(JackDriver), // actually set in init()
	sample_rate(0),
	period_size(0),
	periods_per_buffer(0),
	startup_file()
    {
	setup_options();
	check_options_validity();
	init(argc, argv);
    }

    Configuration::~Configuration()
    {
    }

    void Configuration::copyright()
    {
	cout << copyright_blurb << endl;
	cout << endl;
    }

    void Configuration::usage()
    {
	copyright();
	cout << usage_line << endl;

	const stretchplayer_options_t *it;
	const option *opts;

	it = sp_opts;

	int align = 14, size;
	for( it=sp_opts ; it->optstring != 0 ; ++it ) {
	    opts = &(it->longopts);
	    cout << "  -" << ((char)opts->val)
		 << " --" << opts->name;
	    size = strnlen(opts->name, 32);
	    if(opts->has_arg) {
		cout << "=X";
		size += 2;
	    }
	    while(size < align) {
		cout << " ";
		++size;
	    }
	    cout << " " << (it->doc)
		 << " (default: " << (it->defaults) << ")"
		 << endl;
	}
	cout << endl;
    }

    void Configuration::init(int argc, char* argv[])
    {
#if defined( AUDIO_SUPPORT_JACK )
	driver = JackDriver;
#elif defined( AUDIO_SUPPORT_ALSA )
	driver = AlsaDriver;
#else
#error "Must have support for at least ONE audio API"
#endif
	audio_device( DEFAULT_ALSA_DEVICE );
	sample_rate( atoi(DEFAULT_SAMPLE_RATE) );
	period_size( atoi(DEFAULT_PERIOD_SIZE) );
	periods_per_buffer( atoi(DEFAULT_PERIODS_PER_BUFFER) );
	startup_file( QString() );
	autoconnect(true);
	compositing(true);
	quiet(false);
	help(false);

	bool bad = false;
	int c;

	if(argc && argv) {
	    while(1) {
		c = getopt_long(argc, argv, optstring, longopts, 0);

		if(c == -1)
		    break;

		switch(c)
		{
		case 'J':
		    driver(JackDriver);
		    break;
		case 'A':
		    driver(AlsaDriver);
		    break;
		case 'd':
		    audio_device(optarg);
		    break;
		case 'r':
		    sample_rate( atoi(optarg) );
		    break;
		case 'p':
		    period_size( atoi(optarg) );
		    break;
		case 'n':
		    periods_per_buffer( atoi(optarg) );
		    break;
		case 'x':
		    autoconnect(false);
		    break;
		case 'c':
		    compositing(true);
		    break;
		case 'C':
		    compositing(false);
		    break;
		case 'q':
		    quiet(true);
		    break;
		case 'h':
		    help(true);
		    break;
		default:
		    bad = true;
		}
	    }
	}

	int o = optind;
	for( o=optind ; o < argc; ++o ) {
	    startup_file( argv[o] );
	}

	// Check if setup is sane.
	if( driver() == AlsaDriver ) {
	    if( sample_rate() == 0 ) bad = true;
	    if( audio_device() == "" ) bad = true;
	    if( period_size() == 0 ) bad = true;
	    if( periods_per_buffer() == 0 ) bad = true;
	}

	if( !bad ) ok.set(this, true);
    }



} // namespace StretchPlayer
