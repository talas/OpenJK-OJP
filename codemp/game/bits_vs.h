/*
===========================================================================
Copyright (C) 2003 - 2008, OJP contributors

This file is part of the OpenJK-OJP source code.

OpenJK-OJP is free software; you can redistribute it and/or modify it
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

//header file protector
#ifndef _BITS_VS_H_
#define _BITS_VS_H_

#include <bitset>

namespace ratl
{
	template <int bitSize>
	class bits_vs : public std::bitset<bitSize>
	{
	public:
		bool get_bit(int index) const
		{//gives the current setting of a given array
			return operator[](index);
		}

		void clear_bit(int index)
		{//clears an individual bit on the bit array.
			reset(index);
		}

		void set_bit(int index)
		{//sets the inputted bit to be true.
			set(index);
		}

		void clear()
		{//clears out the entire bit array.
			reset();
		}
	};
}

#endif