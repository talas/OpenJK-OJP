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

#ifndef _VECTOR_VS_H_
#define _VECTOR_VS_H_

#include <vector>
#include <algorithm>

namespace ratl
{
	template <class dataType, int MaxSize>
	class vector_vs : public std::vector<dataType>
	{
	public:
		//constructor
		vector_vs()
		{//set the max size of this vector.
			reserve(MaxSize);
		}
		
		//functions
		//true if the vector's size is 0. (std::vector)
		//bool empty() const;

		//Erases all of the elements. (std::vector)
		//void clear()

		//checks to see if the vector is full.
		bool full()
		{
			return (size() == capacity());
		}

		//resizes the max possible size of the vector (std::vector)
		//void resize( int i );

		//removes last element in array (std::vector)
		//void pop_back();

		//adds the given piece of data to the back of the vector (std::vector)
		//void push_back(dataType input);

		//returns the current size of the vector (std::vector)
		//int size();

		//bah?
		void erase_swap(int i)
		{//"moves" the i-th element to the back of the vector and then deletes it
			at(i) = back();  //copy last element's data to the current element.
			pop_back(); //delete the last element.
		}

		//Sort Them By Distance (mNearestNavSort) smallest to largest distance
		void sort()
		{//sort array in terms of smallest values to largest value
			std::sort(begin(),end());
		}

		//operators (std::vector)
		//dataType& operator[](int index) const; //subscript 
	};
}

#endif