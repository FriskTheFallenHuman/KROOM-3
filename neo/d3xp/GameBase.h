/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __GAMEBASE_H__
#define __GAMEBASE_H__

// default scripts
#define SCRIPT_DEFAULTDEFS			"script/doom_defs.script"
#define SCRIPT_DEFAULT				"script/doom_main.script"
#define SCRIPT_DEFAULTFUNC			"doom_main"

#define TIME_GROUP1		0
#define TIME_GROUP2		1
// the "gameversion" client command will print this plus compile date
#define	GAME_VERSION		"baseDOOM-1"

#define	MAX_CLIENTS				MAX_PLAYERS
#define	MAX_CLIENTS_IN_PVS		(MAX_CLIENTS >> 3)
#define	GENTITYNUM_BITS			12
#define	MAX_GENTITIES			(1 << GENTITYNUM_BITS)
#define	ENTITYNUM_NONE			(MAX_GENTITIES - 1)
#define	ENTITYNUM_WORLD			(MAX_GENTITIES - 2)
#define	ENTITYNUM_MAX_NORMAL	(MAX_GENTITIES - 2)
#define	ENTITYNUM_FIRST_NON_REPLICATED	(ENTITYNUM_MAX_NORMAL - 256)

#define	MIN_FOV		50.0f
#define	MAX_FOV		120.0f

#define	MAX_GAME_MESSAGE_SIZE		8192
#define	MAX_ENTITY_STATE_SIZE		512
#define	ENTITY_PVS_SIZE				( ( MAX_GENTITIES + 31 ) >> 5 )

// content masks
#define	MASK_ALL					(-1)
#define	MASK_SOLID					(CONTENTS_SOLID)
#define	MASK_MONSTERSOLID			(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BODY)
#define	MASK_PLAYERSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define	MASK_DEADSOLID				(CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define	MASK_WATER					(CONTENTS_WATER)
#define	MASK_OPAQUE					(CONTENTS_OPAQUE)
#define	MASK_SHOT_RENDERMODEL		(CONTENTS_SOLID|CONTENTS_RENDERMODEL)
#define	MASK_SHOT_BOUNDINGBOX		(CONTENTS_SOLID|CONTENTS_BODY)

#define DEFAULT_GRAVITY_STRING		"1066"

extern void gameError( const char* fmt, ... );

#endif /* !__GAMEBASE_H__ */
