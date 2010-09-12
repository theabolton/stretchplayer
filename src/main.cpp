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

#include "config.h"

#include <QApplication>

#include "Configuration.hpp"
#include "PlayerWidget.hpp"
#include <iostream>
#include <QPlastiqueStyle>
#include <memory>
#include <stdexcept>
#include <QtGui/QMessageBox>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    StretchPlayer::Configuration config(argc, argv);

    if(config.help() || ( !config.ok() )) {
	config.usage();
	if( !config.ok() ) return -1;
	return 0;
    }

    if( !config.quiet() ) {
	config.copyright();
    }

    std::auto_ptr<StretchPlayer::PlayerWidget> pw;
    try{
	pw.reset( new StretchPlayer::PlayerWidget(&config) );

	app.setStyle( new QPlastiqueStyle );

	pw->show();

	if(! config.startup_file().isEmpty() ) {
	    std::cout << "Loading file " 
		      << (config.startup_file().toLocal8Bit().data())
		      << std::endl;
	    pw->load_song( config.startup_file() );
	}

	app.exec();
    } catch (std::runtime_error& e) {
	std::cerr << "Exception caught: " << e.what() << std::endl;
	QMessageBox::critical( 0,
			       "Runtime Exception",
			       QString("StretchPlayer Exception: %2").arg(e.what())
	    );
    } catch (...) {
	std::cerr << "Unhandled exception... aborting." << std::endl;
	QMessageBox::critical( 0,
			       "Unhandled Exception",
			       "StretchPlayer Exception: There was an unhandled exception... aborting"
	    );
    }

    return 0;
}
