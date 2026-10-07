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
#ifndef __SYS_SIGNIN_LOCAL_H__
#define __SYS_SIGNIN_LOCAL_H__

#include "localuser.h"

/*
================================================
idSignInManagerLocal
================================================
*/
class idSignInManagerLocal final : public idSignInManager
{
public:
	idSignInManagerLocal() = default;
	~idSignInManagerLocal() override;

	idSignInManagerLocal( const idSignInManagerLocal& ) = delete;
	idSignInManagerLocal& operator=( const idSignInManagerLocal& ) = delete;

	//==========================================================================================
	// idSignInManager interface
	//==========================================================================================
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
	void					RegisterLocalUser( int inputDevice ) override;		// Register a local user to the passed in controller

private:
	idStaticList< idLocalUserLocal*, MAX_INPUT_DEVICES >	localUsers;		// heap allocated so a user keeps its identity when others are removed
};

#endif // __SYS_SIGNIN_LOCAL_H__
