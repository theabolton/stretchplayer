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

#include <QApplication>

#include "PlayerWidget.hpp"
#include <iostream>
#include <QPlastiqueStyle>
#include <memory>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    std::auto_ptr<StretchPlayer::PlayerWidget> pw;

    std::cout << "StretchPlayer version 0.500, Copyright 2010 Gabriel M. Beddingfield\n"
	      << "StretchPlayer comes with ABSOLUTELY NO WARRANTY;\n"
	      << "This is free software, and you are welcome to redistribute it\n"
	      << "under terms of the GNU Public License (ver. 2 or later)\n"
	      << std::endl;	

    pw.reset(new StretchPlayer::PlayerWidget);

    app.setStyle( new QPlastiqueStyle );

    pw->show();

    if(argc > 1) {
	QString fn(argv[1]);
	std::cout << "Loading file " << argv[1] << std::endl;
	pw->load_song(fn);
    }

    app.exec();

    return 0;
}
