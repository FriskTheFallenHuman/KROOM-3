/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2021 George Kalmpokis

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

#include "precompiled.h"
#pragma hdrstop

#include "AL_EAX.h"

LPALGENEFFECTS alGenEffectsRef;
LPALEFFECTI alEffectiRef;
LPALEFFECTF alEffectfRef;
LPALEFFECTFV alEffectfvRef;
LPALISEFFECT alIsEffectRef;
LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlotRef;
LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlotsRef;
LPALDELETEEFFECTS alDeleteEffectsRef;
LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSlotiRef;
LPALGENAUXILIARYEFFECTSLOTS	alGenAuxiliaryEffectSlotsRef;
LPALGENFILTERS alGenFiltersRef;
LPALFILTERF alFilterfRef;
LPALFILTERFV alFilterfvRef;
LPALFILTERI alFilteriRef;
LPALDELETEFILTERS alDeleteFiltersRef;
LPALISFILTER alIsFilterRef;

void RegisterEFXFuncs()
{
	alGenEffectsRef = ( LPALGENEFFECTS )alGetProcAddress( "alGenEffects" );
	alEffectiRef = ( LPALEFFECTI )alGetProcAddress( "alEffecti" );
	alEffectfRef = ( LPALEFFECTF )alGetProcAddress( "alEffectf" );
	alEffectfvRef = ( LPALEFFECTFV )alGetProcAddress( "alEffectfv" );
	alIsEffectRef = ( LPALISEFFECT )alGetProcAddress( "alIsEffect" );
	alIsAuxiliaryEffectSlotRef = ( LPALISAUXILIARYEFFECTSLOT )alGetProcAddress( "alIsAuxiliaryEffectSlot" );
	alDeleteAuxiliaryEffectSlotsRef = ( LPALDELETEAUXILIARYEFFECTSLOTS )alGetProcAddress( "alDeleteAuxiliaryEffectSlots" );
	alDeleteEffectsRef = ( LPALDELETEEFFECTS )alGetProcAddress( "alDeleteEffects" );
	alAuxiliaryEffectSlotiRef = ( LPALAUXILIARYEFFECTSLOTI )alGetProcAddress( "alAuxiliaryEffectSloti" );
	alGenAuxiliaryEffectSlotsRef = ( LPALGENAUXILIARYEFFECTSLOTS )alGetProcAddress( "alGenAuxiliaryEffectSlots" );
	alGenFiltersRef = ( LPALGENFILTERS )alGetProcAddress( "alGenFilters" );
	alFilterfRef = ( LPALFILTERF )alGetProcAddress( "alFilterf" );
	alFilterfvRef = ( LPALFILTERFV )alGetProcAddress( "alFilterfv" );
	alFilteriRef = ( LPALFILTERI )alGetProcAddress( "alFilteri" );
	alDeleteFiltersRef = ( LPALDELETEFILTERS )alGetProcAddress( "alDeleteFilters" );
	alIsFilterRef = ( LPALISFILTER )alGetProcAddress( "alIsFilter" );
}