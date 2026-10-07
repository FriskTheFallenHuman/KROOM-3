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

idCVar com_requireNonProductionSignIn( "com_requireNonProductionSignIn", "1", CVAR_BOOL | CVAR_ARCHIVE, "If true, will require sign in, even on non production builds." );
extern idCVar fs_savepath;

/*
========================
idSignInManager::ProcessInputEvent
========================
*/
bool idSignInManager::ProcessInputEvent( const sysEvent_t* ev )
{
	// Users are registered explicitly, never from raw inputs
	return false;
}

/*
========================
idSignInManager::GetDefaultProfile
========================
*/
idPlayerProfile* idSignInManager::GetDefaultProfile()
{
	if( defaultProfile == NULL )
	{
		// Create a new profile
		defaultProfile = idPlayerProfile::CreatePlayerProfile( 0 );
	}
	return defaultProfile;
}

/*
========================
idSignInManager::GetLocalUserByInputDevice
========================
*/
idLocalUser* idSignInManager::GetLocalUserByInputDevice( int index )
{
	for( int i = 0; i < GetNumLocalUsers(); i++ )
	{
		if( GetLocalUserByIndex( i )->GetInputDevice() == index )
		{
			return GetLocalUserByIndex( i );	// Found it
		}
	}

	return NULL;		// Not found
}

/*
========================
idSignInManager::GetLocalUserByHandle
========================
*/
idLocalUser* idSignInManager::GetLocalUserByHandle( localUserHandle_t handle )
{
	for( int i = 0; i < GetNumLocalUsers(); i++ )
	{
		if( GetLocalUserByIndex( i )->GetLocalUserHandle() == handle )
		{
			return GetLocalUserByIndex( i );	// Found it
		}
	}

	return NULL;		// Not found
}

/*
========================
idSignInManager::GetPlayerProfileByInputDevice
========================
*/
idPlayerProfile* idSignInManager::GetPlayerProfileByInputDevice( int index )
{
	idLocalUser* user = session->GetSignInManager().GetLocalUserByInputDevice( index );
	idPlayerProfile* profile = NULL;
	if( user != NULL )
	{
		profile = user->GetProfile();
	}
	return profile;
}

/*
========================
idSignInManager::RemoveLocalUserByInputDevice
========================
*/
bool idSignInManager::RemoveLocalUserByInputDevice( int index )
{
	for( int i = 0; i < GetNumLocalUsers(); i++ )
	{
		if( GetLocalUserByIndex( i )->GetInputDevice() == index )
		{
			RemoveLocalUserByIndex( i );
			return true;
		}
	}

	return false;		// Not found
}

/*
========================
idSignInManager::RemoveLocalUserByHandle
========================
*/
bool idSignInManager::RemoveLocalUserByHandle( localUserHandle_t handle )
{
	for( int i = 0; i < GetNumLocalUsers(); i++ )
	{
		if( GetLocalUserByIndex( i )->GetLocalUserHandle() == handle )
		{
			RemoveLocalUserByIndex( i );
			return true;
		}
	}

	return false;		// Not found
}

/*
========================
idSignInManager::SaveUserProfiles
========================
*/
void idSignInManager::SaveUserProfiles()
{
	for( int i = 0; i < GetNumLocalUsers(); i++ )
	{
		idLocalUser* localUser = GetLocalUserByIndex( i );
		if( localUser != NULL )
		{
			idPlayerProfile* profile = localUser->GetProfile();
			if( profile != NULL )
			{
				profile->SaveSettings( false );
			}
		}
	}
}

/*
========================
idSignInManager::RemoveAllLocalUsers
========================
*/
void idSignInManager::RemoveAllLocalUsers()
{
	while( GetNumLocalUsers() > 0 )
	{
		RemoveLocalUserByIndex( 0 );
	}
}

/*
========================
idSignInManager::ValidateLocalUsers
========================
*/
void idSignInManager::ValidateLocalUsers( bool requireOnline )
{
	if( !RequirePersistentMaster() )
	{
		return;
	}
	for( int i = GetNumLocalUsers() - 1; i >= 0; i-- )
	{
		idLocalUser* localUser = GetLocalUserByIndex( i );

		// If this user does not have a profile, remove them.
		// If this user is not online-capable and we require online, remove them.
		if( !localUser->IsProfileReady() ||
				( requireOnline && ( !localUser->IsOnline() || !localUser->CanPlayOnline() ) ) )
		{
			RemoveLocalUserByIndex( i );
		}
	}
}

/*
========================
idSignInManager::RequirePersistentMaster
========================
*/
bool idSignInManager::RequirePersistentMaster()
{
#ifdef ID_RETAIL
	return true;		// ALWAYS require persistent master on retail builds
#else
	// Non retail production builds require a persistent profile unless si_splitscreen is set
	extern idCVar si_splitscreen;

	// If we are forcing splitscreen, then we won't require a profile (this is for development)
	if( si_splitscreen.GetInteger() != 0 )
	{
		return false;
	}

	return com_requireNonProductionSignIn.GetBool();
#endif
}

/*
========================
idSignInManager::GetUniqueLocalUserHandle
Uniquely generate a handle based on name and time
========================
*/
localUserHandle_t idSignInManager::GetUniqueLocalUserHandle( const char* name )
{
	MD5_CTX			ctx;
	unsigned char	digest[16];
	int64			clockTicks = Sys_GetClockTicks();

	MD5_Init( &ctx );
	MD5_Update( &ctx, ( const unsigned char* )name, idStr::Length( name ) );
	MD5_Update( &ctx, ( const unsigned char* )&clockTicks, sizeof( clockTicks ) );
	MD5_Final( &ctx, ( unsigned char* )digest );

	// Quantize the 128 bit hash down to the number of bits needed for a localUserHandle_t
	const int STRIDE_BYTES	= sizeof( localUserHandle_t::userHandleType_t );
	const int NUM_LOOPS		= 16 / STRIDE_BYTES;

	localUserHandle_t::userHandleType_t handle = 0;

	for( int i = 0; i < NUM_LOOPS; i++ )
	{
		localUserHandle_t::userHandleType_t tempHandle = 0;

		for( int j = 0; j < STRIDE_BYTES; j++ )
		{
			tempHandle |= ( ( localUserHandle_t::userHandleType_t )digest[( i * STRIDE_BYTES ) + j ] ) << ( j * 8 );
		}

		handle ^= tempHandle;
	}

	return localUserHandle_t( handle );
}
