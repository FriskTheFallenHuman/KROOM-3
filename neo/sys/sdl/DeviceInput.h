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

#ifndef __DEVICEINPUT_SDL_H__
#define __DEVICEINPUT_SDL_H__

class idJoystickSDL : public idJoystick
{
public:
	idJoystickSDL();
	virtual ~idJoystickSDL();

	// idJoystick interface
	virtual bool Init();
	virtual void Shutdown();
	virtual void SetRumble( int deviceNum, int rumbleLow, int rumbleHigh );
	virtual int  PollInputEvents( int inputDeviceNum );
	virtual int  ReturnInputEvent( const int n, int& action, int& value );
	virtual void EndInputEvents();

public: // Called on the SDL event loop
	void AddPollEvent( int action, int value );

private:
	struct JoystickPollEvent
	{
		int action;
		int value;
		JoystickPollEvent() {}
		JoystickPollEvent( int a, int v ) : action( a ), value( v ) {}
	};

	SDL_Joystick* m_joy;
	int m_hasHat;           // 1 if the joystick has a hat
	idList<JoystickPollEvent> m_pollEvents;
};

#endif /* !__DEVICEINPUT_SDL_H__ */