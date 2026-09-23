/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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

/*
	this source file includes the implementation of stb_vorbis
	having it in a separate source file allows optimizing it in debug builds (for faster load times)
	without hurting the debugability of the source files it's used in

	(I'm doing this instead of renaming stb_vorbis.h to stb_vorbis.c so the configuration
	like STB_VORBIS_BIG_ENDIAN etc can be done here in code)
*/

//#include "SDL_endian.h"
//#if SDL_BYTEORDER == SDL_BIG_ENDIAN
//  #define STB_VORBIS_BIG_ENDIAN
//#endif
#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_PUSHDATA_API // we're using the pulldata API
#pragma warning(push, 0)
#include "stb/stb_vorbis.h"
#pragma warning(pop)

#define DR_MP3_NO_STDIO
#define DR_MP3_IMPLEMENTATION
#pragma warning(push, 0)
#include "drlibs/dr_mp3.h"
#pragma warning(pop)

#define DR_FLAC_NO_STDIO
#define DR_FLAC_NO_OGG // We already handle this
#define DR_FLAC_IMPLEMENTATION
#pragma warning(push, 0)
#include "drlibs/dr_flac.h"
#pragma warning(pop)