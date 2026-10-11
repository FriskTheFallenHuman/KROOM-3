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
#ifndef __SYS_ACHIEVEMENTS_H__
#define __SYS_ACHIEVEMENTS_H__

class idLocalUser;

// data structure for online achievement entry descriptions
// this is used for testing purposes to make sure that the consoles
// achievement settings match the game's decls
struct achievementDescription_t
{
	void	Clear()
	{
		name[0] = '\0';
		description[0] = '\0';
		hidden = false;
	};
	char	name[500];
	char	description[1000];
	bool	hidden;
};

#define ACHIEVEMENT_PREFIX		"ach_"

/*
================================================
idAchievementSystem
================================================
*/
class idAchievementSystem
{
public:
	static const int MAX_ACHIEVEMENTS = 128;		// This matches the max number of achievements bits in the profile

	virtual			~idAchievementSystem() {}

	// Initialize for the system, not for a particular user
	virtual void	Init() = 0;

	// Do any necessary cleanup
	virtual void	Shutdown() = 0;

	// Is the achievement system ready for requests
	virtual bool	IsInitialized() const = 0;

	// Should be done every frame
	virtual void	Pump() = 0;

	// Add / remove a local user to the system
	virtual void	RegisterLocalUser( idLocalUser* user ) = 0;
	virtual void	RemoveLocalUser( idLocalUser* user ) = 0;

	// Unlocks the achievement, silently fails if the achievement has already been unlocked
	virtual void	AchievementUnlock( idLocalUser* user, const int achievementID ) = 0;

	// Puts the achievement(s) back to their original state, implementation may not allow this
	virtual void	AchievementLock( idLocalUser* user, const int achievementID ) = 0;
	virtual void	AchievementLockAll( idLocalUser* user, const int maxId ) = 0;

	// Retrieves textual information about a given achievement
	// returns false if there was an error
	virtual bool	GetAchievementDescription( idLocalUser* user, const int id, achievementDescription_t& data ) const = 0;

	// Retrieves state about of all achievements cached locally (may not be online yet)
	// returns false if there was an error
	virtual bool	GetAchievementState( idLocalUser* user, idArray< bool, idAchievementSystem::MAX_ACHIEVEMENTS >& achievements ) const = 0;

	// Sets state of all the achievements within list (for debug purposes only)
	// returns false if there was an error
	virtual bool	SetAchievementState( idLocalUser* user, idArray< bool, idAchievementSystem::MAX_ACHIEVEMENTS >& achievements ) = 0;

	// Helper built on top of GetAchievementState(): copies the system's cached achievement status into the
	// user's profile because the profile may not have been saved with the achievement bits after an unlock.
	void			SyncAchievementBits( idLocalUser* user );
};

#endif // __SYS_ACHIEVEMENTS_H__
