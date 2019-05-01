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

//This is technically supposed to be a dynamically allocated array structure but I've implimented it statically.

namespace ratl
{
	template <class dataType, int maxSize>
	class pool_vs	
	{
	public:
		pool_vs()
		{//sets all elements to be not in use.
			for(int i = 0; i < maxSize; i++)
			{
				inuse[i] = false;
			}
		}

		//substript operator
		dataType& operator[](int i)
		{
			return poolData[i];
		}

		int alloc()
		{//allocates a empty slot in the pool and returns the index of it.  This doesn't do a full check.
			for(int i = 0; i < maxSize; i++)
			{
				if(!inuse[i])
				{//empty element, make it be in use and then return the index of it.
					inuse[i] = true;
					return i;
				}
			}

			//default case, this shouldn't happen
			assert(0);
			return 0;
		}

		bool full()
		{//checks to see if the pool is full.
			for(int i = 0; i < maxSize; i++)
			{
				if(!inuse[i])
				{
					return false;
				}
			}

			return true;
		}

		void free(int index)
		{//frees the data at the given pool index
			inuse[index] = false;
			memset( &poolData[index], 0, sizeof(dataType) );  //clear that memory area.
		}

	private:
		dataType poolData[maxSize];  //actual data in pool
		bool inuse[maxSize];  //inuse flags
	};
}