/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
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

//Rocket Trooper AI code
#include "g_local.h"

void RT_FireDecide( void )
{
}

//=====================================================================================
//FLYING behavior 
//=====================================================================================
qboolean RT_Flying( gentity_t *self )
{
	return ((qboolean)(self->client->ps.eFlags2 & EF2_FLYING));
}

