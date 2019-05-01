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

#ifndef _GRAPH_REGION_H
#define	_GRAPH_REGION_H

#include "user_vs.h"

namespace ragl
{
	class graph_vs;

	template <class Node, int nodeNum, class Edge, int edgeNum, int edgesPerNode, int regionNum, int regionNum2>
	class graph_region
	{
	public:
		//constructor using graph_vs as input
		graph_region(graph_vs <Node, nodeNum, Edge, edgeNum, edgesPerNode> a);

		//I assume this blanks out the class object.
		void clear(void);

		//I think this inits/reserves a region index and then returns the index value.
		int reserve(void);

		//assigns the given node to a given region
		void assign_region(int nodeIndex, int regionIndex);

		bool find_region_edges();

		bool find_regions(user_vs userDude);

		//determine if there's a valid edge path between these two points for this user.  
		//user is set to be blank if you want it to check all edges in region
		bool has_valid_edge(int nodeA, int nodeB, user_vs userDude);

		//returns the size (probably in nodes) of the region class.
		int size();

		//spew data contents maybe?
		void ProfileSpew();
	};
}

#endif