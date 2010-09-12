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

    static const char optstring[] = "JAd:r:p:n:xcCqh";

    static const struct option longopts[] = {
	// const char *name, int has_arg, int *flag, int val
	{"jack", 0, 0, 'J'},
	{"alsa", 0, 0, 'A'},
	{"device", 1, 0, 'd'},
	{"sample-rate", 1, 0, 'r'},
	{"period-size", 1, 0, 'p'},
	{"periods", 1, 0, 'n'},
	{"no-autoconnect", 0, 0, 'x'},
	{"compositing", 0, 0, 'c'},
	{"no-compositing", 0, 0, 'C'},
	{"quiet", 0, 0, 'q'},
	{"help", 0, 0, 'h'},
	{0, 0, 0, 0}
    };

    static const char* opt_defaults[] = {
	"on", // --jack
	"off", // --alsa
	DEFAULT_ALSA_DEVICE, // --device
	DEFAULT_SAMPLE_RATE, // --sample-rate
	DEFAULT_PERIOD_SIZE, // --period-size
	DEFAULT_PERIODS_PER_BUFFER, // --periods
	"off", // --no-autoconnect
	"on", // --compositing
	"off", // --no-compositing
	"off", // --quiet
	"off", // --help
	0
    };

    static const char* opt_doc[] = {
	"use JACK for audio", // --jack
	"use ALSA for audio", // --alsa
	"device to use for ALSA", // --device
	"sample rate to use for ALSA", // --sample-rate
	"period size to use for ALSA", // --period-size
	"periods per buffer for ALSA", // --periods
	"disable auto-connection ot ouputs", // --no-autoconnect
	"enable desktop compositing (if supported by Qt/X11)", // --compositing
	"disable desktop compositing", // --no-compositing
	"suppress most output to console", // --quiet
	"show help/usage and exit", // --help
	0
    };

    static const char usage_line[] = 
	"usage: stretchplayer [options] [audio_file_name]";

    static const char copyright_blurb[] =
	"StretchPlayer version " STRETCHPLAYER_VERSION ", Copyright 2010 Gabriel M. Beddingfield\n"
	"StretchPlayer comes with ABSOLUTELY NO WARRANTY;\n"
	"This is free software, and you are welcome to redistribute it\n"
	"under terms of the GNU Public License (ver. 2 or later)\n";

    static const char version[] = STRETCHPLAYER_VERSION;

    static void check_options_validity()
    {
	const char *str = optstring;
	const option *opts = longopts;
	const char **def = opt_defaults;
	const char **doc = opt_doc;

	assert(str);
	assert(opts);
	assert(def);
	assert(doc);
	int pos = 0;
	while( opts->name != 0 ) {
	    assert( str[pos] );
	    assert( opts->val == str[pos] );
	    assert( *def );
	    assert( *doc );

	    ++pos;
	    if( (str[pos] != 0) && (str[pos] == ':') ) {
		assert(opts->has_arg != 0);
		++pos;
	    } else {
		assert(opts->has_arg == 0);
	    }
	    ++opts;
	    ++def;
	    ++doc;
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

	const struct option *opt = longopts;
	const char **def = opt_defaults;
	const char **doc = opt_doc;
	int align = 14, size;
	while(opt->name != 0) {
	    cout << "  -" << ((char)opt->val)
		 << " --" << opt->name;
	    size = strlen(opt->name);
	    if(opt->has_arg) {
		cout << "=X";
		size += 2;
	    }
	    while(size < align) {
		cout << " ";
		++size;
	    }
	    cout << " " << (*doc)
		 << " (default: " << (*def) << ")"
		 << endl;

	    ++opt;
	    ++def;
	    ++doc;
	}
	cout << endl;
    }

    void Configuration::init(int argc, char* argv[])
    {
	driver = JackDriver;
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
