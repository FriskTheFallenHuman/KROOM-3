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

/*
================================================================================================

	Leaderboards on top of ISteamUserStats.

================================================================================================
*/

static const int NAME_WAIT_TIMEOUT_MS = 3000;		// how long a download waits for the persona names of players that are not friends

/*
================================================
Boards that are already looked up
================================================
*/
struct boardHandle_t
{
	int					id;
	SteamLeaderboard_t	handle;
};
static boardHandle_t	boardHandles[ MAX_LEADERBOARDS ];
static int				numBoardHandles = 0;

static SteamLeaderboard_t FindBoardHandle( int id )
{
	for( int i = 0; i < numBoardHandles; i++ )
	{
		if( boardHandles[i].id == id )
		{
			return boardHandles[i].handle;
		}
	}
	return 0;
}

static void CacheBoardHandle( int id, SteamLeaderboard_t handle )
{
	if( FindBoardHandle( id ) == 0 && numBoardHandles < MAX_LEADERBOARDS )
	{
		boardHandles[ numBoardHandles ].id = id;
		boardHandles[ numBoardHandles ].handle = handle;
		numBoardHandles++;
	}
}

/*
========================
ClampToInt32
========================
*/
static int32 ClampToInt32( int64 value )
{
	if( value > 0x7FFFFFFF )
	{
		return 0x7FFFFFFF;
	}
	if( value < -( int64 )0x7FFFFFFF - 1 )
	{
		return -0x7FFFFFFF - 1;
	}
	return ( int32 )value;
}

/*
================================================
idSteamLeaderboardOp
================================================
*/
class idSteamLeaderboardOp
{
public:
	enum opType_t
	{
		OP_UPLOAD,
		OP_DOWNLOAD,
	};

	idSteamLeaderboardOp( opType_t type, const leaderboardDefinition_t* def );
	~idSteamLeaderboardOp();

	void	SetUpload( const column_t* stats );
	void	SetDownload( int startingRank, int numRows, const idLeaderboardCallback& callback );

	void	Start();
	void	Update();
	void	Cancel();
	bool	IsDone() const
	{
		return state == STATE_DONE;
	}

private:
	enum state_t
	{
		STATE_FINDING,
		STATE_FETCHING_CURRENT,		// upload of a summed column: reading the score that is stored
		STATE_UPLOADING,
		STATE_DOWNLOADING,
		STATE_WAITING_NAMES,
		STATE_DELIVER,				// the result is complete, handed over by the next Update()
		STATE_DONE,
	};

	struct entry_t
	{
		CSteamID	steamId;
		int32		rank;
		int32		score;
		int32		numDetails;
		int32		details[ MAX_LEADERBOARD_COLUMNS ];
	};

	void	OnFound( LeaderboardFindResult_t* result, bool ioFailure );
	void	OnCurrentScore( LeaderboardScoresDownloaded_t* result, bool ioFailure );
	void	OnUploaded( LeaderboardScoreUploaded_t* result, bool ioFailure );
	void	OnDownloaded( LeaderboardScoresDownloaded_t* result, bool ioFailure );

	void	Execute();
	void	SubmitScore( int64 score, ELeaderboardUploadScoreMethod method );
	void	FailWith( leaderboardError_t error );
	bool	NamesPending();
	void	Deliver();

	CCallResult< idSteamLeaderboardOp, LeaderboardFindResult_t >		findResult;
	CCallResult< idSteamLeaderboardOp, LeaderboardScoresDownloaded_t >	downloadResult;
	CCallResult< idSteamLeaderboardOp, LeaderboardScoreUploaded_t >		uploadResult;

	opType_t						type;
	state_t							state;
	const leaderboardDefinition_t* 	def;
	SteamLeaderboard_t				handle;

	// upload
	int64							columns[ MAX_LEADERBOARD_COLUMNS ];

	// download
	int								startingRank;
	int								numRows;
	idLeaderboardCallback* 			callback;			// our own copy of the caller's callback
	leaderboardError_t				error;
	idList< entry_t >				entries;
	int								numRowsInBoard;
	int								startIndex;
	int								waitStart;
};

static idList< idSteamLeaderboardOp* >* leaderboardOps = NULL;

/*
========================
idSteamLeaderboardOp::idSteamLeaderboardOp
========================
*/
idSteamLeaderboardOp::idSteamLeaderboardOp( opType_t type_, const leaderboardDefinition_t* def_ ) :
	type( type_ ),
	state( STATE_FINDING ),
	def( def_ ),
	handle( 0 ),
	startingRank( 0 ),
	numRows( 0 ),
	callback( NULL ),
	error( LEADERBOARD_ERROR_NONE ),
	numRowsInBoard( 0 ),
	startIndex( 0 ),
	waitStart( 0 )
{
	memset( columns, 0, sizeof( columns ) );
}

/*
========================
idSteamLeaderboardOp::~idSteamLeaderboardOp
========================
*/
idSteamLeaderboardOp::~idSteamLeaderboardOp()
{
	delete callback;
}

/*
========================
idSteamLeaderboardOp::SetUpload
========================
*/
void idSteamLeaderboardOp::SetUpload( const column_t* stats )
{
	const int n = Min( def->numColumns, ( int )MAX_LEADERBOARD_COLUMNS );
	for( int i = 0; i < n; i++ )
	{
		columns[i] = stats[i].value;
	}
}

/*
========================
idSteamLeaderboardOp::SetDownload
========================
*/
void idSteamLeaderboardOp::SetDownload( int startingRank_, int numRows_, const idLeaderboardCallback& callback_ )
{
	startingRank = startingRank_;
	numRows = Max( numRows_, 1 );
	callback = callback_.Clone();
}

/*
========================
idSteamLeaderboardOp::Start
========================
*/
void idSteamLeaderboardOp::Start()
{
	ISteamUserStats* stats = SteamUserStats();
	ISteamUser* user = SteamUser();

	if( stats == NULL || user == NULL || !user->BLoggedOn() )
	{
		FailWith( LEADERBOARD_ERROR_NOT_ONLINE );
		return;
	}

	handle = FindBoardHandle( def->id );
	if( handle != 0 )
	{
		Execute();
		return;
	}

	const char* name = va( STEAM_LEADERBOARD_PREFIX "%d", def->id );
	const ELeaderboardSortMethod sort = ( def->rankOrder == RANK_LEAST_FIRST ) ? k_ELeaderboardSortMethodAscending : k_ELeaderboardSortMethodDescending;
	const ELeaderboardDisplayType display = ( def->numColumns > 0 && def->columnDefs[0].displayType == STATS_COLUMN_DISPLAY_TIME_MILLISECONDS ) ? k_ELeaderboardDisplayTypeTimeMilliSeconds : k_ELeaderboardDisplayTypeNumeric;

	const SteamAPICall_t call = stats->FindOrCreateLeaderboard( name, sort, display );
	if( call == k_uAPICallInvalid )
	{
		FailWith( LEADERBOARD_ERROR_FAILED );
		return;
	}

	state = STATE_FINDING;
	findResult.Set( call, this, &idSteamLeaderboardOp::OnFound );
}

/*
========================
idSteamLeaderboardOp::OnFound
========================
*/
void idSteamLeaderboardOp::OnFound( LeaderboardFindResult_t* result, bool ioFailure )
{
	if( ioFailure || !result->m_bLeaderboardFound )
	{
		idLib::Warning( "STEAM: leaderboard %d could not be found or created", def->id );
		FailWith( LEADERBOARD_ERROR_FAILED );
		return;
	}

	handle = result->m_hSteamLeaderboard;
	CacheBoardHandle( def->id, handle );
	Execute();
}

/*
========================
idSteamLeaderboardOp::Execute
========================
*/
void idSteamLeaderboardOp::Execute()
{
	ISteamUserStats* stats = SteamUserStats();

	if( type == OP_UPLOAD )
	{
		if( def->numColumns <= 0 )
		{
			state = STATE_DONE;
			return;
		}

		// Steam has no "add to the stored score": read the stored score of the user and write the sum
		if( def->columnDefs[0].aggregationMethod == AGGREGATE_SUM )
		{
			CSteamID self = SteamUser()->GetSteamID();
			const SteamAPICall_t call = stats->DownloadLeaderboardEntriesForUsers( handle, &self, 1 );
			if( call == k_uAPICallInvalid )
			{
				FailWith( LEADERBOARD_ERROR_FAILED );
				return;
			}
			state = STATE_FETCHING_CURRENT;
			downloadResult.Set( call, this, &idSteamLeaderboardOp::OnCurrentScore );
			return;
		}

		// MAX on a board that ranks the greatest first and MIN on one that ranks the least first is what KeepBest does.
		// LAST, and the combinations that KeepBest can not express, replace the stored score.
		const aggregationMethod_t aggregation = def->columnDefs[0].aggregationMethod;
		const bool keepBest = ( aggregation == AGGREGATE_MAX && def->rankOrder == RANK_GREATEST_FIRST ) || ( aggregation == AGGREGATE_MIN && def->rankOrder == RANK_LEAST_FIRST );
		SubmitScore( columns[0], keepBest ? k_ELeaderboardUploadScoreMethodKeepBest : k_ELeaderboardUploadScoreMethodForceUpdate );
		return;
	}

	// download
	ELeaderboardDataRequest request;
	int rangeStart;
	int rangeEnd;

	if( startingRank == 0 )
	{
		request = k_ELeaderboardDataRequestGlobalAroundUser;
		rangeStart = -( numRows / 2 );
		rangeEnd = rangeStart + numRows - 1;
		startIndex	= 0;		// known when the rows arrive
	}
	else if( startingRank < 0 )
	{
		request = k_ELeaderboardDataRequestFriends;
		rangeStart = 0;
		rangeEnd = 0;
		startIndex = 0;
	}
	else
	{
		request = k_ELeaderboardDataRequestGlobal;
		rangeStart = startingRank;
		rangeEnd = startingRank + numRows - 1;
		startIndex = startingRank - 1;
	}

	const SteamAPICall_t call = stats->DownloadLeaderboardEntries( handle, request, rangeStart, rangeEnd );
	if( call == k_uAPICallInvalid )
	{
		FailWith( LEADERBOARD_ERROR_DOWNLOAD );
		return;
	}

	state = STATE_DOWNLOADING;
	downloadResult.Set( call, this, &idSteamLeaderboardOp::OnDownloaded );
}

/*
========================
idSteamLeaderboardOp::OnCurrentScore
========================
*/
void idSteamLeaderboardOp::OnCurrentScore( LeaderboardScoresDownloaded_t* result, bool ioFailure )
{
	if( ioFailure )
	{
		FailWith( LEADERBOARD_ERROR_FAILED );
		return;
	}

	int64 current = 0;
	if( result->m_cEntryCount > 0 )
	{
		LeaderboardEntry_t entry;
		if( SteamUserStats()->GetDownloadedLeaderboardEntry( result->m_hSteamLeaderboardEntries, 0, &entry, NULL, 0 ) )
		{
			current = entry.m_nScore;
		}
	}

	SubmitScore( current + columns[0], k_ELeaderboardUploadScoreMethodForceUpdate );
}

/*
========================
idSteamLeaderboardOp::SubmitScore
========================
*/
void idSteamLeaderboardOp::SubmitScore( int64 score, ELeaderboardUploadScoreMethod method )
{
	int32 details[ MAX_LEADERBOARD_COLUMNS ];
	int numDetails = 0;
	for( int i = 1; i < def->numColumns && i < MAX_LEADERBOARD_COLUMNS; i++ )
	{
		details[ numDetails++ ] = ClampToInt32( columns[i] );
	}

	const SteamAPICall_t call = SteamUserStats()->UploadLeaderboardScore( handle, method, ClampToInt32( score ), numDetails > 0 ? details : NULL, numDetails );
	if( call == k_uAPICallInvalid )
	{
		FailWith( LEADERBOARD_ERROR_FAILED );
		return;
	}

	state = STATE_UPLOADING;
	uploadResult.Set( call, this, &idSteamLeaderboardOp::OnUploaded );
}

/*
========================
idSteamLeaderboardOp::OnUploaded
========================
*/
void idSteamLeaderboardOp::OnUploaded( LeaderboardScoreUploaded_t* result, bool ioFailure )
{
	if( ioFailure || !result->m_bSuccess )
	{
		idLib::Warning( "STEAM: score upload to leaderboard %d failed", def->id );
	}
	else
	{
		STEAM_VERBOSE_PRINT( "STEAM: leaderboard %d: score %d (%s), rank %d -> %d\n", def->id, result->m_nScore,
							 result->m_bScoreChanged ? "stored" : "not better than the stored one", result->m_nGlobalRankPrevious, result->m_nGlobalRankNew );
	}

	state = STATE_DONE;
}

/*
========================
idSteamLeaderboardOp::OnDownloaded
========================
*/
void idSteamLeaderboardOp::OnDownloaded( LeaderboardScoresDownloaded_t* result, bool ioFailure )
{
	if( ioFailure )
	{
		FailWith( LEADERBOARD_ERROR_DOWNLOAD );
		return;
	}

	ISteamUserStats* stats = SteamUserStats();
	const int maxDetails = Max( Min( def->numColumns - 1, ( int )MAX_LEADERBOARD_COLUMNS ), 0 );

	entries.Clear();
	for( int i = 0; i < result->m_cEntryCount; i++ )
	{
		LeaderboardEntry_t entry;
		entry_t row;
		memset( row.details, 0, sizeof( row.details ) );

		if( !stats->GetDownloadedLeaderboardEntry( result->m_hSteamLeaderboardEntries, i, &entry, row.details, maxDetails ) )
		{
			continue;
		}

		row.steamId = entry.m_steamIDUser;
		row.rank = entry.m_nGlobalRank;
		row.score = entry.m_nScore;
		row.numDetails	= Min( entry.m_cDetails, maxDetails );
		entries.Append( row );
	}

	if( startingRank == 0 && entries.Num() > 0 )
	{
		startIndex = Max( entries[0].rank - 1, 0 );
	}

	// the rows of a friends board are the friends that have a score, not a part of the global board
	numRowsInBoard = ( startingRank < 0 ) ? entries.Num() : stats->GetLeaderboardEntryCount( handle );

	// Names of players that are not friends are not known to the client yet
	if( NamesPending() )
	{
		waitStart = Sys_Milliseconds();
		state = STATE_WAITING_NAMES;
		return;
	}

	Deliver();
}

/*
========================
idSteamLeaderboardOp::NamesPending
========================
*/
bool idSteamLeaderboardOp::NamesPending()
{
	ISteamFriends* friends = SteamFriends();
	bool pending = false;

	for( int i = 0; friends != NULL && i < entries.Num(); i++ )
	{
		if( friends->RequestUserInformation( entries[i].steamId, true ) )
		{
			pending = true;
		}
	}
	return pending;
}

/*
========================
idSteamLeaderboardOp::FailWith
========================
*/
void idSteamLeaderboardOp::FailWith( leaderboardError_t error_ )
{
	error = error_;

	if( type == OP_DOWNLOAD && callback != NULL )
	{
		// Not delivered from here, the caller is most likely still inside LeaderboardDownload()
		state = STATE_DELIVER;
		return;
	}

	state = STATE_DONE;
}

/*
========================
idSteamLeaderboardOp::Update
========================
*/
void idSteamLeaderboardOp::Update()
{
	if( state == STATE_WAITING_NAMES )
	{
		if( !NamesPending() || Sys_Milliseconds() - waitStart > NAME_WAIT_TIMEOUT_MS )
		{
			Deliver();
		}
	}
	else if( state == STATE_DELIVER )
	{
		Deliver();
	}
}

/*
========================
idSteamLeaderboardOp::Deliver
Hands the rows to the callback of the caller.
========================
*/
void idSteamLeaderboardOp::Deliver()
{
	state = STATE_DONE;

	if( callback == NULL )
	{
		return;
	}

	ISteamFriends* friends = SteamFriends();
	const uint64 localId = Steam_GetLocalSteamId();
	int localIndex = -1;

	callback->SetDef( def );
	callback->SetErrorCode( error );
	callback->ResetRows();

	if( error == LEADERBOARD_ERROR_NONE )
	{
		for( int i = 0; i < entries.Num(); i++ )
		{
			const entry_t& entry = entries[i];

			idLeaderboardCallback::row_t row;
			row.hasAttachment = false;
			row.attachmentID = 0;
			row.rank = entry.rank;
			row.user_id = entry.steamId.ConvertToUint64();
			for( int c = 0; c < row.columns.Num(); c++ )
			{
				row.columns[c] = 0;
			}

			row.name = ( friends != NULL ) ? friends->GetFriendPersonaName( entry.steamId ) : "";
			if( row.user_id == localId )
			{
				row.name = Steam_GetPersonaName();
				localIndex = i;
			}

			row.columns[0] = entry.score;
			for( int d = 0; d < entry.numDetails && d + 1 < row.columns.Num(); d++ )
			{
				row.columns[ d + 1 ] = entry.details[d];
			}

			callback->AddRow( row );
		}
	}

	callback->SetStartIndex( startIndex );
	callback->SetLocalIndex( localIndex );
	callback->SetNumRowsInLeaderboard( error == LEADERBOARD_ERROR_NONE ? numRowsInBoard : 0 );

	callback->Call();
}

/*
========================
idSteamLeaderboardOp::Cancel
========================
*/
void idSteamLeaderboardOp::Cancel()
{
	findResult.Cancel();
	downloadResult.Cancel();
	uploadResult.Cancel();
	state = STATE_DONE;
}

/*
========================
StartOp
========================
*/
static void StartOp( idSteamLeaderboardOp* op )
{
	if( leaderboardOps == NULL )
	{
		leaderboardOps = new( TAG_NETWORKING ) idList< idSteamLeaderboardOp* >;
	}
	leaderboardOps->Append( op );
	op->Start();
}

/*
========================
Steam_LeaderboardUpload
========================
*/
void Steam_LeaderboardUpload( const leaderboardDefinition_t* def, const column_t* stats )
{
	if( def == NULL || stats == NULL || !Steam_IsRunning() )
	{
		return;
	}

	idSteamLeaderboardOp* op = new( TAG_NETWORKING ) idSteamLeaderboardOp( idSteamLeaderboardOp::OP_UPLOAD, def );
	op->SetUpload( stats );
	StartOp( op );
}

/*
========================
Steam_LeaderboardDownload
========================
*/
void Steam_LeaderboardDownload( const leaderboardDefinition_t* def, int startingRank, int numRows, const idLeaderboardCallback& callback )
{
	if( def == NULL || !Steam_IsRunning() )
	{
		return;
	}

	idSteamLeaderboardOp* op = new( TAG_NETWORKING ) idSteamLeaderboardOp( idSteamLeaderboardOp::OP_DOWNLOAD, def );
	op->SetDownload( startingRank, numRows, callback );
	StartOp( op );
}

/*
========================
Steam_LeaderboardPump
========================
*/
void Steam_LeaderboardPump()
{
	if( leaderboardOps == NULL )
	{
		return;
	}

	// a callback can start another request, it is appended and handled by the same loop
	for( int i = 0; i < leaderboardOps->Num(); )
	{
		idSteamLeaderboardOp* op = ( *leaderboardOps )[i];
		op->Update();

		if( op->IsDone() )
		{
			delete op;
			leaderboardOps->RemoveIndex( i );
			continue;
		}
		i++;
	}
}

/*
========================
Steam_LeaderboardShutdown
========================
*/
void Steam_LeaderboardShutdown()
{
	if( leaderboardOps != NULL )
	{
		for( int i = 0; i < leaderboardOps->Num(); i++ )
		{
			( *leaderboardOps )[i]->Cancel();
		}
		leaderboardOps->DeleteContents( true );
		delete leaderboardOps;
		leaderboardOps = NULL;
	}
	numBoardHandles = 0;
}
