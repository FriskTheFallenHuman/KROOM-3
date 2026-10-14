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

#include "Game_local.h"

bool idAchievementManager::cheatingDialogShown = false;

const achievementInfo_t achievementInfo[ACHIEVEMENTS_NUM] =
{
	{ 50,	true,	"ach_earn_all_50_trophies"				}, // ACHIEVEMENT_EARN_ALL_50_TROPHIES
	{ 1,	true,	"ach_completed_difficulty_0"			}, // ACHIEVEMENT_COMPLETED_DIFFICULTY_0
	{ 1,	true,	"ach_completed_difficulty_1"			}, // ACHIEVEMENT_COMPLETED_DIFFICULTY_1
	{ 1,	true,	"ach_completed_difficulty_2"			}, // ACHIEVEMENT_COMPLETED_DIFFICULTY_2
	{ 1,	true,	"ach_completed_difficulty_3"			}, // ACHIEVEMENT_COMPLETED_DIFFICULTY_3
	{ 64,	false,	"ach_pdas_base"							}, // ACHIEVEMENT_PDAS_BASE
	{ 14,	false,	"ach_watch_all_videos"					}, // ACHIEVEMENT_WATCH_ALL_VIDEOS
	{ 1,	false,	"ach_kill_monster_with_1_health_left"	}, // ACHIEVEMENT_KILL_MONSTER_WITH_1_HEALTH_LEFT
	{ 35,	false,	"ach_open_all_lockers"					}, // ACHIEVEMENT_OPEN_ALL_LOCKERS
	{ 20,	true,	"ach_kill_20_enemy_fists_hands"			}, // ACHIEVEMENT_KILL_20_ENEMY_FISTS_HANDS
	{ 1,	true,	"ach_kill_sci_next_to_rcr"				}, // ACHIEVEMENT_KILL_SCI_NEXT_TO_RCR
	{ 1,	true,	"ach_kill_two_imps_one_shotgun"			}, // ACHIEVEMENT_KILL_TWO_IMPS_ONE_SHOTGUN
	{ 1,	true,	"ach_score_25000_turkey_puncher"		}, // ACHIEVEMENT_SCORE_25000_TURKEY_PUNCHER
	{ 50,	true,	"ach_destroy_barrels"					}, // ACHIEVEMENT_DESTROY_BARRELS
	{ 1,	true,	"ach_get_bfg_from_security_office"		}, // ACHIEVEMENT_GET_BFG_FROM_SECURITY_OFFICE
	{ 1,	true,	"ach_complete_level_without_taking_dmg" }, // ACHIEVEMENT_COMPLETE_LEVEL_WITHOUT_TAKING_DMG
	{ 1,	true,	"ach_find_rage_logo"					}, // ACHIEVEMENT_FIND_RAGE_LOGO
	{ 1,	true,	"ach_speed_run"							}, // ACHIEVEMENT_SPEED_RUN
	{ 1,	true,	"ach_defeat_vagary_boss"				}, // ACHIEVEMENT_DEFEAT_VAGARY_BOSS
	{ 1,	true,	"ach_defeat_guardian_boss"				}, // ACHIEVEMENT_DEFEAT_GUARDIAN_BOSS
	{ 1,	true,	"ach_defeat_sabaoth_boss"				}, // ACHIEVEMENT_DEFEAT_SABAOTH_BOSS
	{ 1,	true,	"ach_defeat_cyberdemon_boss"			}, // ACHIEVEMENT_DEFEAT_CYBERDEMON_BOSS
	{ 1,	true,	"ach_sentry_bot_alive_to_dest"			}, // ACHIEVEMENT_SENTRY_BOT_ALIVE_TO_DEST
	{ 20,	true,	"ach_kill_20_enemy_with_chainsaw"		}, // ACHIEVEMENT_KILL_20_ENEMY_WITH_CHAINSAW
	{ 1,	true,	"ach_id_logo_secret_room"				}, // ACHIEVEMENT_ID_LOGO_SECRET_ROOM
	{ 1,	true,	"ach_bloody_handwork_of_betruger"		}, // ACHIEVEMENT_BLOODY_HANDWORK_OF_BETRUGER
	{ 1,	true,	"ach_two_demons_fight_each_other"		}, // ACHIEVEMENT_TWO_DEMONS_FIGHT_EACH_OTHER
	{ 20,	true,	"ach_use_soul_cube_to_defeat_20_enemy"	}, // ACHIEVEMENT_USE_SOUL_CUBE_TO_DEFEAT_20_ENEMY
	{ 1,	true,	"ach_roe_completed_difficulty_0"		}, // ACHIEVEMENT_ROE_COMPLETED_DIFFICULTY_0
	{ 1,	true,	"ach_roe_completed_difficulty_1"		}, // ACHIEVEMENT_ROE_COMPLETED_DIFFICULTY_1
	{ 1,	true,	"ach_roe_completed_difficulty_2"		}, // ACHIEVEMENT_ROE_COMPLETED_DIFFICULTY_2
	{ 1,	true,	"ach_roe_completed_difficulty_3"		}, // ACHIEVEMENT_ROE_COMPLETED_DIFFICULTY_3
	{ 22,	false,	"ach_pdas_roe"							}, // ACHIEVEMENT_PDAS_ROE
	{ 1,	true,	"ach_kill_5_enemy_hell_time"			}, // ACHIEVEMENT_KILL_5_ENEMY_HELL_TIME
	{ 1,	true,	"ach_defeat_helltime_hunter"			}, // ACHIEVEMENT_DEFEAT_HELLTIME_HUNTER
	{ 1,	true,	"ach_defeat_berserk_hunter"				}, // ACHIEVEMENT_DEFEAT_BERSERK_HUNTER
	{ 1,	true,	"ach_defeat_invulnerability_hunter"		}, // ACHIEVEMENT_DEFEAT_INVULNERABILITY_HUNTER
	{ 1,	true,	"ach_defeat_maledict_boss"				}, // ACHIEVEMENT_DEFEAT_MALEDICT_BOSS
	{ 20,	true,	"ach_grabber_kill_20_enemy"				}, // ACHIEVEMENT_GRABBER_KILL_20_ENEMY
	{ 20,	true,	"ach_artifact_with_berserk_punch_20"	}, // ACHIEVEMENT_ARTIFACT_WITH_BERSERK_PUNCH_20
	{ 1,	true,	"ach_le_completed_difficulty_0"			}, // ACHIEVEMENT_LE_COMPLETED_DIFFICULTY_0
	{ 1,	true,	"ach_le_completed_difficulty_1"			}, // ACHIEVEMENT_LE_COMPLETED_DIFFICULTY_1
	{ 1,	true,	"ach_le_completed_difficulty_2"			}, // ACHIEVEMENT_LE_COMPLETED_DIFFICULTY_2
	{ 1,	true,	"ach_le_completed_difficulty_3"			}, // ACHIEVEMENT_LE_COMPLETED_DIFFICULTY_3
	{ 10,	false,	"ach_pdas_le"							}, // ACHIEVEMENT_PDAS_LE
	{ 1,	true,	"ach_mp_kill_player_via_teleport"		}, // ACHIEVEMENT_MP_KILL_PLAYER_VIA_TELEPORT
	{ 1,	true,	"ach_mp_catch_enemy_in_rofc"			}, // ACHIEVEMENT_MP_CATCH_ENEMY_IN_ROFC
	{ 5,	true,	"ach_mp_kill_5_players_using_invis"		}, // ACHIEVEMENT_MP_KILL_5_PLAYERS_USING_INVIS
	{ 1,	true,	"ach_mp_complete_match_without_dying"	}, // ACHIEVEMENT_MP_COMPLETE_MATCH_WITHOUT_DYING
	{ 1,	true,	"ach_mp_use_berserk_to_kill_player"		}, // ACHIEVEMENT_MP_USE_BERSERK_TO_KILL_PLAYER
	{ 1,	true,	"ach_mp_kill_2_guys_in_room_with_bfg"	}  // ACHIEVEMENT_MP_KILL_2_GUYS_IN_ROOM_WITH_BFG
};

compile_time_assert( sizeof( achievementInfo ) / sizeof( achievementInfo[0] ) == ACHIEVEMENTS_NUM );

/*
========================
idAchievementManager::idAchievementManager
========================
*/
idAchievementManager::idAchievementManager() :
	lastPlayerKilledTime( 0 ),
	lastImpKilledTime( 0 ),
	playerTookDamage( false )
{
	counts.Zero();
	ResetHellTimeKills();
}

/*
========================
idAchievementManager::Init
========================
*/
void idAchievementManager::Init( idPlayer* player )
{
	owner = player;
	SyncAchievments();
}

/*
========================
idAchievementManager::SyncAchievments
========================
*/
void idAchievementManager::SyncAchievments()
{
	idLocalUser* user = GetLocalUser();
	if( user == NULL || user->GetProfile() == NULL )
	{
		return;
	}

	// Set achievement counts
	for( int i = 0; i < counts.Num(); i++ )
	{
		if( user->GetProfile()->GetAchievement( i ) )
		{
			counts[i] = achievementInfo[i].required;
		}
		else if( achievementInfo[i].lifetime )
		{
			counts[i] = user->GetStatInt( i );
		}
	}
}

/*
========================
idAchievementManager::GetLocalUser
========================
*/
idLocalUser* idAchievementManager::GetLocalUser()
{
	if( !verify( owner != NULL ) )
	{
		return NULL;
	}
	return session->GetGameLobbyBase().GetLocalUserFromLobbyUser( gameLocal.lobbyUserIDs[ owner->GetEntityNumber() ] );
}

/*
========================
idAchievementManager::Save
========================
*/
void idAchievementManager::Save( idSaveGame* savefile ) const
{
	owner.Save( savefile );

	for( int i = 0; i < ACHIEVEMENTS_NUM; i++ )
	{
		savefile->WriteInt( counts[i] );
	}

	savefile->WriteInt( lastImpKilledTime );
	savefile->WriteInt( lastPlayerKilledTime );
	savefile->WriteBool( playerTookDamage );
	savefile->WriteInt( currentHellTimeKills );
}

/*
========================
idAchievementManager::Restore
========================
*/
void idAchievementManager::Restore( idRestoreGame* savefile )
{
	owner.Restore( savefile );

	for( int i = 0; i < ACHIEVEMENTS_NUM; i++ )
	{
		savefile->ReadInt( counts[i] );
	}

	savefile->ReadInt( lastImpKilledTime );
	savefile->ReadInt( lastPlayerKilledTime );
	savefile->ReadBool( playerTookDamage );
	savefile->ReadInt( currentHellTimeKills );

	SyncAchievments();
}

/*
========================
idAchievementManager::EventCompletesAchievement
========================
*/
void idAchievementManager::EventCompletesAchievement( const achievement_t eventId )
{
	idLocalUser* localUser = GetLocalUser();
	if( localUser == NULL || localUser->GetProfile() == NULL )
	{

		// Send a Reliable Message to the User that needs to unlock this.
		if( owner != NULL )
		{
			int playerId = owner->entityNumber;
			const int bufferSize = sizeof( playerId ) + sizeof( eventId );
			byte buffer[ bufferSize ];
			idBitMsg msg;
			msg.InitWrite( buffer, bufferSize );

			msg.WriteByte( playerId );
			msg.WriteByte( eventId );

			msg.WriteByteAlign();
			idLib::Printf( "Host Sending Achievement\n" );
			session->GetActingGameStateLobbyBase().SendReliableToLobbyUser( gameLocal.lobbyUserIDs[ owner->entityNumber ], GAME_RELIABLE_MESSAGE_ACHIEVEMENT_UNLOCK, msg );
		}

		return; // Remote user or build game
	}

	// Check to see if we've already given the achievement.
	// If so, don't do again because we don't want to autosave every time a trigger is hit
	if( localUser->GetProfile()->GetAchievement( eventId ) )
	{
		return;
	}

#ifdef ID_RETAIL
	if( common->GetConsoleUsed() )
	{
		if( !cheatingDialogShown )
		{
			dialogs->AddDialog( GDM_ACHIEVEMENTS_DISABLED_DUE_TO_CHEATING, DIALOG_ACCEPT, NULL, NULL, true );
			cheatingDialogShown = true;
		}
		return;
	}
#endif

	counts[eventId]++;

	if( counts[eventId] >= achievementInfo[eventId].required )
	{
		session->GetAchievementSystem().AchievementUnlock( localUser, eventId );
	}
	else
	{
		if( achievementInfo[eventId].lifetime )
		{
			localUser->SetStatInt( eventId, counts[eventId] );
		}
	}
}

/*
========================
idAchievementManager::IncrementHellTimeKills
========================
*/
void idAchievementManager::IncrementHellTimeKills()
{
	currentHellTimeKills++;
	if( currentHellTimeKills >= 5 )
	{
		EventCompletesAchievement( ACHIEVEMENT_KILL_5_ENEMY_HELL_TIME );
	}
}

/*
========================
idAchievementManager::SavePersistentData
========================
*/
void idAchievementManager::SavePersistentData( idDict& playerInfo )
{
	for( int i = 0; i < ACHIEVEMENTS_NUM; ++i )
	{
		playerInfo.SetInt( va( "ach_%d", i ), counts[i] );
	}
}

/*
========================
idAchievementManager::RestorePersistentData
========================
*/
void idAchievementManager::RestorePersistentData( const idDict& spawnArgs )
{
	for( int i = 0; i < ACHIEVEMENTS_NUM; ++i )
	{
		counts[i] = spawnArgs.GetInt( va( "ach_%d", i ), "0" );
	}
}


/*
========================
idAchievementManager::LocalUser_CompleteAchievement
========================
*/
void idAchievementManager::LocalUser_CompleteAchievement( achievement_t id )
{
	idLocalUser* localUser = session->GetSignInManager().GetMasterLocalUser();

	// Check to see if we've already given the achievement.
	// If so, don't do again because we don't want to autosave every time a trigger is hit
	if( localUser == NULL || localUser->GetProfile()->GetAchievement( id ) )
	{
		return;
	}

#ifdef ID_RETAIL
	if( common->GetConsoleUsed() )
	{
		if( !cheatingDialogShown )
		{
			dialogs->AddDialog( GDM_ACHIEVEMENTS_DISABLED_DUE_TO_CHEATING, DIALOG_ACCEPT, NULL, NULL, true );
			cheatingDialogShown = true;
		}
		return;
	}
#endif

	session->GetAchievementSystem().AchievementUnlock( localUser, id );
}

/*
=================
AchievementsReset
=================
*/
CONSOLE_COMMAND( AchievementsReset, "Lock an achievement", NULL )
{
	idLocalUser* user = session->GetSignInManager().GetMasterLocalUser();
	if( user == NULL )
	{
		idLib::Printf( "Must be signed in\n" );
		return;
	}
	if( args.Argc() == 1 )
	{
		for( int i = 0; i < ACHIEVEMENTS_NUM; i++ )
		{
			user->SetStatInt( i, 0 );
			session->GetAchievementSystem().AchievementLock( user, i );
		}
	}
	else
	{
		int i = atoi( args.Argv( 1 ) );
		user->SetStatInt( i, 0 );
		session->GetAchievementSystem().AchievementLock( user, i );
	}
	user->SaveProfileSettings();
}

/*
=================
AchievementsUnlock
=================
*/
CONSOLE_COMMAND( AchievementsUnlock, "Unlock an achievement", NULL )
{
	idLocalUser* user = session->GetSignInManager().GetMasterLocalUser();
	if( user == NULL )
	{
		idLib::Printf( "Must be signed in\n" );
		return;
	}
	if( args.Argc() == 1 )
	{
		for( int i = 0; i < ACHIEVEMENTS_NUM; i++ )
		{
			user->SetStatInt( i, achievementInfo[i].required );
			session->GetAchievementSystem().AchievementUnlock( user, i );
		}
	}
	else
	{
		int i = atoi( args.Argv( 1 ) );
		user->SetStatInt( i, achievementInfo[i].required );
		session->GetAchievementSystem().AchievementUnlock( user, i );
	}
	user->SaveProfileSettings();
}

/*
=================
AchievementsList
=================
*/
CONSOLE_COMMAND( AchievementsList, "Lists achievements and status", NULL )
{
	idPlayer* player = gameLocal.GetLocalPlayer();
	idLocalUser* user = ( player == NULL ) ? session->GetSignInManager().GetMasterLocalUser() : session->GetGameLobbyBase().GetLocalUserFromLobbyUser( gameLocal.lobbyUserIDs[ player->GetEntityNumber() ] );
	if( user == NULL )
	{
		idLib::Printf( "Must be signed in\n" );
		return;
	}
	idPlayerProfile* profile = user->GetProfile();

	idArray<bool, 128> achievementState;
	bool achievementStateValid = session->GetAchievementSystem().GetAchievementState( user, achievementState );

	for( int i = 0; i < ACHIEVEMENTS_NUM; i++ )
	{
		const char* pInfo = "";
		if( profile == NULL )
		{
			pInfo = S_COLOR_RED  "unknown" S_COLOR_DEFAULT;
		}
		else if( !profile->GetAchievement( i ) )
		{
			pInfo = S_COLOR_YELLOW "locked" S_COLOR_DEFAULT;
		}
		else
		{
			pInfo = S_COLOR_GREEN "unlocked" S_COLOR_DEFAULT;
		}
		const char* sInfo = "";
		if( !achievementStateValid )
		{
			sInfo = S_COLOR_RED  "unknown" S_COLOR_DEFAULT;
		}
		else if( !achievementState[i] )
		{
			sInfo = S_COLOR_YELLOW "locked" S_COLOR_DEFAULT;
		}
		else
		{
			sInfo = S_COLOR_GREEN "unlocked" S_COLOR_DEFAULT;
		}
		int count = 0;
		if( achievementInfo[i].lifetime )
		{
			count = user->GetStatInt( i );
		}
		else if( player != NULL )
		{
			count = player->GetAchievementManager().GetCount( ( achievement_t ) i );
		}
		else
		{
			count = 0;
		}

		achievementDescription_t data;
		bool descriptionValid = session->GetAchievementSystem().GetAchievementDescription( user, i, data );

		idLib::Printf( "%02d: %2d/%2d | %12.12s | %12.12s | %s%s\n", i, count, achievementInfo[i].required, pInfo, sInfo, descriptionValid ? data.hidden ? "(hidden) " : "" : "(unknown) ", descriptionValid ? data.name : "" );
	}
}
