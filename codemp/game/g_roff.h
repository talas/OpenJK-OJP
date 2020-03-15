/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
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

//[ROFF]
#ifndef __G_ROFF_H__
#define __G_ROFF_H__


#include "qcommon/q_shared.h"


// ROFF Defines
//-------------------
#define ROFF_VERSION		1	// ver # for the (R)otation (O)bject (F)ile (F)ormat
#define ROFF_VERSION2		2	// ver # for the (R)otation (O)bject (F)ile (F)ormat
#define MAX_ROFFS			32	// hard coded number of max roffs per level, sigh..
#define ROFF_SAMPLE_RATE	20	// 10hz

#define ROFF_INFO_SIZE	30000		//max size for ROFF file.
#define MAXNOTETRACKS	8			//max number of note tracks
#define NOTETRACKSSIZE	100			//size of note tracks
#define MAXNUMDATA		500			//max number of data positions for ROFF.


// ROFF Header file definition
//-------------------------------
typedef struct roff_hdr_s
{
	char	mHeader[4];		// should be "ROFF" (Rotation, Origin File Format)
	int32_t	mVersion;
	int32_t	mCount;
		//						
		//		Move - Rotate data follows....vec3_t delta_origin, vec3_t delta_rotation
		//
} roff_hdr_t;


// ROFF move rotate data element
//--------------------------------
typedef struct move_rotate_s
{
	vec3_t	origin_delta;
	vec3_t	rotate_delta;

} move_rotate_t;

typedef struct roff_hdr2_s
//-------------------------------
{
	char	mHeader[4];				// should match roff_string defined above
	int32_t	mVersion;				// version num, supported version defined above
	int32_t		mCount;					// I think this is a float because of a limitation of the roff exporter
	int32_t		mFrameRate;				// Frame rate the roff should be played at
	int32_t		mNumNotes;				// number of notes (null terminated strings) after the roff data

} roff_hdr2_t;


typedef struct move_rotate2_s
//-------------------------------
{
	vec3_t	origin_delta;
	vec3_t	rotate_delta;
	int32_t		mStartNote, mNumNotes;		// note track info

} move_rotate2_t;


// a precached ROFF list
//-------------------------
typedef struct roff_list_s
{
	int32_t				type;			// roff type number, 1-old, 2-new
	char			*fileName;		// roff filename
	int32_t				frames;			// number of roff entries
	move_rotate2_t	data[MAXNUMDATA];	// delta move and rotate vector list
	int32_t				NumData;			//number of data positions we are currently using.
	int32_t				mFrameTime;		// frame rate
	int32_t				mLerp;			// Lerp rate (FPS)
	int32_t				mNumNoteTracks;
	char			mNoteTrackIndexes[MAXNOTETRACKS][NOTETRACKSSIZE];

} roff_list_t;



extern roff_list_t roffs[];
extern int num_roffs;


// Function prototypes
//-------------------------
int		G_LoadRoff( const char *fileName );
#ifdef _GAME
void	G_Roff( gentity_t *ent );
void	G_SaveCachedRoffs();
void	G_LoadCachedRoffs();
#endif

#endif
//[/ROFF]
