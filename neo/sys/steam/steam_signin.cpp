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

#include "../../framework/PlayerProfile.h"
#include "../sys_session_local.h"

#include "steam_local.h"
#include "steam_signin.h"

extern idCVar ui_name;

/*
========================
BuildGamertag

Cuts a UTF-8 name to what a gamertag can hold, on character boundaries.
========================
*/
static void BuildGamertag( const char* source, idStr& out )
{
	out = source;

	const int length = out.Length();
	if( idStr::IsValidUTF8( source, length ) )
	{
		int index = 0;
		int numChars = 0;
		out.Empty();
		while( index < length && numChars++ < idLocalUserSteam::MAX_GAMERTAG_CHARS )
		{
			const int start = index;
			const uint32 c = idStr::UTF8Char( source, index );
			if( out.Length() + ( index - start ) >= idLocalUserSteam::MAX_GAMERTAG )
			{
				break;
			}
			out.AppendUTF8Char( c );
		}
	}
	else if( out.Length() >= idLocalUserSteam::MAX_GAMERTAG )
	{
		out.CapLength( idLocalUserSteam::MAX_GAMERTAG - 1 );
	}
}

/*
================================================================================================

idLocalUserSteam

================================================================================================
*/

/*
========================
idLocalUserSteam::Init
========================
*/
void idLocalUserSteam::Init( int inputDevice_, const char* name, int numLocalUsers )
{
	isMaster = ( numLocalUsers == 1 );		// numLocalUsers is incremented before we get in here

	if( isMaster )
	{
		gamertag = name;
	}
	else
	{
		// split screen users get "name (2)" for the second player, always based on the number of users on the system
		gamertag.Format( "%s (%i)", name, numLocalUsers );
	}

	inputDevice = inputDevice_;
}

/*
========================
idLocalUserSteam::IsOnline
========================
*/
bool idLocalUserSteam::IsOnline() const
{
	return Steam_IsRunning() && SteamUser() != NULL && SteamUser()->BLoggedOn();
}

/*
========================
idLocalUserSteam::GetOnlineCaps
========================
*/
uint32 idLocalUserSteam::GetOnlineCaps() const
{
	return ( isMaster && IsOnline() ) ? ( CAP_IS_ONLINE | CAP_CAN_PLAY_ONLINE ) : 0;
}

/*
========================
idLocalUserSteam::GetPlatformId
========================
*/
uint64 idLocalUserSteam::GetPlatformId() const
{
	return isMaster ? Steam_GetLocalSteamId() : 0;
}

/*
================================================================================================

idSignInManagerSteam

================================================================================================
*/

/*
========================
idSignInManagerSteam::~idSignInManagerSteam
========================
*/
idSignInManagerSteam::~idSignInManagerSteam()
{
	for( int i = 0; i < localUsers.Num(); i++ )
	{
		delete localUsers[i];
	}
	localUsers.Clear();
}

/*
========================
idSignInManagerSteam::Shutdown
========================
*/
void idSignInManagerSteam::Shutdown()
{
}

/*
========================
idSignInManagerSteam::Pump
========================
*/
void idSignInManagerSteam::Pump()
{
	// If we have more users than we need, then set to the lower amount
	if( localUsers.Num() > 1 && localUsers.Num() > maxDesiredLocalUsers )
	{
		while( localUsers.Num() > maxDesiredLocalUsers )
		{
			delete localUsers[ localUsers.Num() - 1 ];
			localUsers.RemoveIndex( localUsers.Num() - 1 );
		}
	}

#ifndef ID_RETAIL
	// If we don't have enough, then make sure we do
	while( localUsers.Num() < minDesiredLocalUsers )
	{
		RegisterLocalUser( localUsers.Num() );
	}
#endif

	// See if we need to save settings on any of the profiles
	for( int i = 0; i < localUsers.Num(); i++ )
	{
		localUsers[i]->Pump();
	}
}

/*
========================
idSignInManagerSteam::RemoveLocalUserByIndex
========================
*/
void idSignInManagerSteam::RemoveLocalUserByIndex( int index )
{
	session->OnLocalUserSignout( localUsers[index] );
	delete localUsers[index];
	localUsers.RemoveIndex( index );
}

/*
========================
idSignInManagerSteam::RegisterLocalUser
========================
*/
void idSignInManagerSteam::RegisterLocalUser( int inputDevice )
{
	if( GetLocalUserByInputDevice( inputDevice ) != NULL )
	{
		return;
	}

	// First try ui_name
	const char* nameSource = ui_name.GetString();
	if( idStr::Length( nameSource ) == 0 )
	{
		nameSource = Steam_GetPersonaName();
	}
	if( idStr::Length( nameSource ) == 0 )
	{
		nameSource = "Player";
	}

	idStr name;
	BuildGamertag( nameSource, name );

	idLib::Printf( "Added local user: %s\n", name.c_str() );

	idLocalUserSteam* localUser = new( TAG_SYSTEM ) idLocalUserSteam;
	localUsers.Append( localUser );

	localUser->Init( inputDevice, name.c_str(), localUsers.Num() );
	localUser->SetLocalUserHandle( GetUniqueLocalUserHandle( localUser->GetGamerTag() ) );

	session->OnLocalUserSignin( localUser );
}
