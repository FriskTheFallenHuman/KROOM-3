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
#ifndef __SYS_LOCALUSER_LOCAL_H__
#define __SYS_LOCALUSER_LOCAL_H__

/*
================================================
idLocalUserLocal
================================================
*/
class idLocalUserLocal final : public idLocalUser
{
public:
	static const int MAX_GAMERTAG = 64;			// max number of bytes for a gamertag
	static const int MAX_GAMERTAG_CHARS = 16;	// max number of UTF-8 characters to show

	idLocalUserLocal() : inputDevice( 0 ) {}

	//==========================================================================================
	// idLocalUser interface
	//==========================================================================================
	bool			IsProfileReady() const override;
	bool			IsOnline() const override;
	bool			IsInParty() const override;
	int				GetPartyCount() const override;
	uint32			GetOnlineCaps() const override
	{
		return ( IsPersistent() && IsOnline() ) ? ( CAP_IS_ONLINE | CAP_CAN_PLAY_ONLINE ) : 0;
	}
	int				GetInputDevice() const override
	{
		return inputDevice;
	}
	const char* 	GetGamerTag() const override
	{
		return gamertag.c_str();
	}

	void			Init( int inputDevice_, const char* gamertag_, int numLocalUsers );

private:
	idStrStatic< MAX_GAMERTAG >	gamertag;
	int							inputDevice;
};

#endif // __SYS_LOCALUSER_LOCAL_H__
