/*
===========================================================================
Copyright (C) 2003 - 2008, OJP contributors

This file is part of the OJP source code.

OJP is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

//header protection
#ifndef _HFILE_H_
#define _HFILE_H_

class hfile
{
public:
	//constructor (opens file) by char *
	hfile(char *);

	//sets the file to read mode for this version of the navigation file data.
	bool open_read( int version, int checkSum );

	//loads a chunk of data from the file.
	void load(void *, size_t);

	//closes the file
	void close();

	//sets the file to write mode for this version of the navigation file data.
	bool open_write( int version, int checkSum );

	//saves data to file.
	void save( void* data, int dataSize);

};

#endif