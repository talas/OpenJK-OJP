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

namespace ratl
{
	template <class T, int I>
	class array_vs	
	{
	public:
		T arrayData[I];  //data array

		const static int CAPACITY = I; //the total size of this array object.

		T& operator[](int i)
		{
			return arrayData[i];
		}

		void fill( int fillValue ) 
		{//inits the entire array with this value.
			memset(arrayData, fillValue, sizeof(arrayData));
		}
	};
}