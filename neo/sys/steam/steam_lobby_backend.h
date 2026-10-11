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
#ifndef __STEAM_LOBBY_BACKEND_H__
#define __STEAM_LOBBY_BACKEND_H__

#include "../sys_lobby_backend.h"
#include "steam_local.h"

#define STEAM_LOBBY_KEY_NAME "name"		// name of the host,
#define STEAM_LOBBY_KEY_HOST "host"		// id of the engine host
#define STEAM_LOBBY_KEY_NETVER "netver"	// NetGetVersionChecksum, a lobby of another build is never returned by a search
#define STEAM_LOBBY_KEY_BROWSE "browse"	// 1 while the party lobby is listed in the game browser
#define STEAM_LOBBY_KEY_BROWSE_MODE "bmode"
#define STEAM_LOBBY_KEY_BROWSE_MAP "bmap"
#define STEAM_LOBBY_KEY_BROWSE_PLAYERS "bplayers"
#define STEAM_LOBBY_KEY_BROWSE_MAX "bmax"
#define STEAM_LOBBY_KEY_BROWSE_JOINABLE	"bjoin"

/*
================================================

idLobbyBackendSteam

================================================
*/
class idLobbyBackendSteam final : public idLobbyBackend
{
public:
	idLobbyBackendSteam();
	~idLobbyBackendSteam() override;

	idLobbyBackendSteam( const idLobbyBackendSteam& ) = delete;
	idLobbyBackendSteam& operator=( const idLobbyBackendSteam& ) = delete;

	// idLobbyBackend public interface
	void				StartHosting( const idMatchParameters& p, float skillLevel, lobbyBackendType_t type ) override;
	void				StartFinding( const idMatchParameters& p, int numPartyUsers, float skillLevel ) override;
	void				JoinFromConnectInfo( const lobbyConnectInfo_t& connectInfo ) override;
	void				GetSearchResults( idList< lobbyConnectInfo_t >& searchResults ) override;
	lobbyConnectInfo_t	GetConnectInfo() override;
	void				FillMsgWithPostConnectInfo( idBitMsg& msg ) override {}		// peers address the host by Steam id, nothing to send
	void				PostConnectFromMsg( idBitMsg& msg ) override {}
	bool				IsOwnerOfConnectInfo( const lobbyConnectInfo_t& connectInfo ) const override;
	void				Shutdown() override;
	void				GetOwnerAddress( lobbyAddress_t& outAddr ) override;
	void				SetIsJoinable( bool joinable ) override;
	void				Pump() override;
	void				UpdateMatchParms( const idMatchParameters& p ) override;
	void				UpdateLobbySkill( float lobbySkill ) override;
	void				SetInGame( bool value ) override;
	lobbyBackendState_t	GetState() override
	{
		return state;
	}

	void				StartSession() override;
	void				EndSession() override;
	bool				IsSessionStarted() override
	{
		return sessionStarted;
	}
	void				FlushStats() override;

	void				BecomeHost( int numInvites ) override;
	void				FinishBecomeHost() override {}

	// Steam specific
	uint64				GetSteamLobbyId() const
	{
		return lobbyId.ConvertToUint64();
	}
	bool				HasMember( const CSteamID& id ) const;

	// makes the lobby public and searchable.
	void				SetBrowseListing( const steamBrowseListing_t& listing );

private:
	void				OnLobbyCreated( LobbyCreated_t* param, bool ioFailure );
	void				OnLobbyEntered( LobbyEnter_t* param, bool ioFailure );
	void				OnLobbyListReceived( LobbyMatchList_t* param, bool ioFailure );

	void				CreateSteamLobby();
	ELobbyType			GetSteamLobbyType() const;
	void				PublishLobbyData();
	void				ApplyBrowseListing();
	bool				IsLobbyOwner() const;
	void				ResolveOwnerAddress();
	void				LeaveSteamLobby();

	CCallResult< idLobbyBackendSteam, LobbyCreated_t >		createResult;
	CCallResult< idLobbyBackendSteam, LobbyEnter_t >		enterResult;
	CCallResult< idLobbyBackendSteam, LobbyMatchList_t >	listResult;

	lobbyBackendState_t		state;
	CSteamID				lobbyId;				// the lobby we host or are joining/in, invalid if none (set when the join starts, see IsOwnerOfConnectInfo)
	CSteamID				previousLobbyId;		// the lobby we leave when the one created by BecomeHost is ready
	idList< CSteamID >		searchResults;
	lobbyAddress_t*			ownerAddressOut;		// where GetOwnerAddress wants the owner, filled when the lobby is entered
	float					skill;
	bool					joinable;
	bool					inGame;
	bool					sessionStarted;
	bool					entered;				// the join completed, the owner can be resolved
	steamBrowseListing_t	browse;					// what the host wants listed, applied to every lobby it creates
	bool					browseApplied;			// the lobby we own is public and listed right now
};

#endif // __STEAM_LOBBY_BACKEND_H__
