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
#include "precompiled.h"
#pragma hdrstop

#include "../sys_session_local.h"

#include "steam_local.h"
#include "steam_achievements.h"

extern idCVar achievements_Verbose;

/*
========================
idAchievementSystemSteam::GetAchievementName
========================
*/
const char* idAchievementSystemSteam::GetAchievementName( int id )
{
	if( gameAchievements != NULL )
	{
		const char* name = gameAchievements->GetAchievementName( id );
		if( name != NULL && name[0] != '\0' )
		{
			return name;
		}
	}
	return va( ACHIEVEMENT_PREFIX "%d", id );
}

/*
========================
idAchievementSystemSteam::NumAchievements
========================
*/
int idAchievementSystemSteam::NumAchievements()
{
	return ( gameAchievements != NULL ) ? gameAchievements->GetNumAchievements() : 0;
}

/*
========================
idAchievementSystemSteam::CanUse
========================
*/
bool idAchievementSystemSteam::CanUse( const idLocalUser* user )
{
	return Steam_IsRunning() && SteamUserStats() != NULL && user != NULL && user->GetPlatformId() != 0;
}

/*
========================
idAchievementSystemSteam::Init
========================
*/
void idAchievementSystemSteam::Init()
{
	initialized = Steam_IsRunning() && SteamUserStats() != NULL;
}

/*
========================
idAchievementSystemSteam::Shutdown
========================
*/
void idAchievementSystemSteam::Shutdown()
{
	if( initialized )
	{
		Steam_StoreStats();
	}
	initialized = false;
}

/*
========================
idAchievementSystemSteam::AchievementUnlock
========================
*/
void idAchievementSystemSteam::AchievementUnlock( idLocalUser* user, const int achievementID )
{
	if( !initialized || !CanUse( user ) )
	{
		return;
	}

	const char* name = GetAchievementName( achievementID );

	bool achieved = false;
	if( SteamUserStats()->GetAchievement( name, &achieved ) && achieved )
	{
		return;		// already unlocked
	}

	if( SteamUserStats()->SetAchievement( name ) )
	{
		if( achievements_Verbose.GetBool() )
		{
			idLib::Printf( "STEAM: unlocked achievement %s\n", name );
		}
		SteamUserStats()->StoreStats();
	}
	else
	{
		idLib::Warning( "STEAM: could not unlock achievement '%s', is this correct?", name );
	}
}

/*
========================
idAchievementSystemSteam::AchievementLock
========================
*/
void idAchievementSystemSteam::AchievementLock( idLocalUser* user, const int achievementID )
{
	if( !initialized || !CanUse( user ) )
	{
		return;
	}

	SteamUserStats()->ClearAchievement( GetAchievementName( achievementID ) );
	SteamUserStats()->StoreStats();
}

/*
========================
idAchievementSystemSteam::AchievementLockAll
========================
*/
void idAchievementSystemSteam::AchievementLockAll( idLocalUser* user, const int maxId )
{
	if( !initialized || !CanUse( user ) )
	{
		return;
	}

	for( int i = 0; i < maxId && i < NumAchievements(); i++ )
	{
		SteamUserStats()->ClearAchievement( GetAchievementName( i ) );
	}
	SteamUserStats()->StoreStats();
}

/*
========================
idAchievementSystemSteam::GetAchievementDescription
========================
*/
bool idAchievementSystemSteam::GetAchievementDescription( idLocalUser* user, const int id, achievementDescription_t& data ) const
{
	data.Clear();

	if( !initialized || !CanUse( user ) )
	{
		return false;
	}

	const char* name = GetAchievementName( id );
	const char* displayName = SteamUserStats()->GetAchievementDisplayAttribute( name, "name" );
	if( displayName == NULL || displayName[0] == '\0' )
	{
		return false;		// not defined
	}

	const char* description = SteamUserStats()->GetAchievementDisplayAttribute( name, "desc" );
	const char* hidden = SteamUserStats()->GetAchievementDisplayAttribute( name, "hidden" );

	idStr::Copynz( data.name, displayName, sizeof( data.name ) );
	idStr::Copynz( data.description, ( description != NULL ) ? description : "", sizeof( data.description ) );
	data.hidden = ( hidden != NULL && hidden[0] == '1' );

	return true;
}

/*
========================
idAchievementSystemSteam::GetAchievementState
========================
*/
bool idAchievementSystemSteam::GetAchievementState( idLocalUser* user, idArray< bool, idAchievementSystem::MAX_ACHIEVEMENTS >& achievements ) const
{
	achievements.Zero();

	if( !initialized || !CanUse( user ) )
	{
		return false;
	}

	const int numNamed = NumAchievements();
	for( int i = 0; i < numNamed && i < achievements.Num(); i++ )
	{
		bool achieved = false;
		if( SteamUserStats()->GetAchievement( GetAchievementName( i ), &achieved ) )
		{
			achievements[i] = achieved;
		}
	}

	return true;
}

/*
========================
idAchievementSystemSteam::SetAchievementState
========================
*/
bool idAchievementSystemSteam::SetAchievementState( idLocalUser* user, idArray< bool, idAchievementSystem::MAX_ACHIEVEMENTS >& achievements )
{
	if( !initialized || !CanUse( user ) )
	{
		return false;
	}

	const int numNamed = NumAchievements();
	for( int i = 0; i < numNamed && i < achievements.Num(); i++ )
	{
		if( achievements[i] )
		{
			SteamUserStats()->SetAchievement( GetAchievementName( i ) );
		}
		else
		{
			SteamUserStats()->ClearAchievement( GetAchievementName( i ) );
		}
	}
	SteamUserStats()->StoreStats();

	return true;
}