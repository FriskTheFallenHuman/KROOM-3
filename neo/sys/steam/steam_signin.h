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
#ifndef __STEAM_SIGNIN_H__
#define __STEAM_SIGNIN_H__

/*
================================================

idLocalUserSteam

================================================
*/
class idLocalUserSteam final : public idLocalUser
{
public:
	static const int MAX_GAMERTAG = 64;			// max number of bytes for a gamertag
	static const int MAX_GAMERTAG_CHARS = 16;	// max number of UTF-8 characters to show

	idLocalUserSteam() : inputDevice( 0 ), isMaster( false ) {}

	// idLocalUser local interface
	bool			IsProfileReady() const override
	{
		return true;			// the profile is a local file, it must stay available in Steam offline mode
	}
	bool			IsOnline() const override;
	uint32			GetOnlineCaps() const override;
	int				GetInputDevice() const override
	{
		return inputDevice;
	}
	const char* 	GetGamerTag() const override
	{
		return gamertag.c_str();
	}
	bool			IsInParty() const override
	{
		return false;			// Steam has no party concept
	}
	int				GetPartyCount() const override
	{
		return 0;
	}
	uint64			GetPlatformId() const override;

	void			Init( int inputDevice_, const char* name, int numLocalUsers );

private:
	idStrStatic< MAX_GAMERTAG >	gamertag;
	int							inputDevice;
	bool						isMaster;
};

/*
================================================

idSignInManagerSteam

================================================
*/
class idSignInManagerSteam final : public idSignInManager
{
public:
	idSignInManagerSteam() = default;
	~idSignInManagerSteam() override;

	idSignInManagerSteam( const idSignInManagerSteam& ) = delete;
	idSignInManagerSteam& operator=( const idSignInManagerSteam& ) = delete;

	// idSignInManager public interface
	void					Pump() override;
	void					Shutdown() override;
	int						GetNumLocalUsers() const override
	{
		return localUsers.Num();
	}
	idLocalUser* 			GetLocalUserByIndex( int index ) override
	{
		return localUsers[index];
	}
	const idLocalUser* 		GetLocalUserByIndex( int index ) const override
	{
		return localUsers[index];
	}
	void					RemoveLocalUserByIndex( int index ) override;
	void					RegisterLocalUser( int inputDevice ) override;

private:
	idStaticList< idLocalUserSteam*, MAX_INPUT_DEVICES >	localUsers;		// heap allocated so a user keeps its identity when others are removed
};

#endif // __STEAM_SIGNIN_H__
