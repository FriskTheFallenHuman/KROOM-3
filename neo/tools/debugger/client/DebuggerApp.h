/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 1999-2011 Raven Software
Copyright (C) 2021 Harrie van Ginneken

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

#ifndef __DEBUGGERAPP_H__
#define __DEBUGGERAPP_H__

#include <wx/wx.h>

#include "../../sys/sdl/DeviceSDL.h"
//#include "../../framework/sync/Msg.h"

#ifndef __REGISTRYOPTIONS_H__
	#include "../../common/RegistryOptions.h"
#endif

#ifndef __DEBUGGERWINDOW_H__
	#include "DebuggerWindow.h"
#endif

#ifndef __DEBUGGERMESSAGES_H__
	#include "../server/DebuggerMessages.h"
#endif

#ifndef __DEBUGGERCLIENT_H__
	#include "DebuggerClient.h"
#endif

// These were changed to static by ID so to make it easy we just throw them
// in this header
// we need a lot to be able to list all threads in mars_city1
const int MAX_MSGLEN = 8600;

class rvDebuggerMainApp : public wxApp
{
public:
	virtual bool OnInit();
};

class rvDebuggerApp
{
public:

	rvDebuggerApp();
	~rvDebuggerApp();

	bool				Initialize();
	int					Run();

	rvRegistryOptions&	GetOptions();
	rvDebuggerClient&	GetClient();
	rvDebuggerWindow&	GetWindow();

	void                NotifyWindowDestroyed( rvDebuggerWindow* wnd );
protected:

	rvRegistryOptions	mOptions;
	rvDebuggerWindow*	mDebuggerWindow;
	rvDebuggerClient	mClient;

	bool				mIsInitialized;
	wxTimer*			mNetworkPollTimer;

private:
	void	OnNetworkPollTimer( wxTimerEvent& event );
	bool	ProcessNetMessages();
	bool	ProcessWindowMessages();
};

ID_INLINE rvDebuggerClient& rvDebuggerApp::GetClient()
{
	return mClient;
}

ID_INLINE rvRegistryOptions& rvDebuggerApp::GetOptions()
{
	return mOptions;
}

ID_INLINE rvDebuggerWindow& rvDebuggerApp::GetWindow()
{
	assert( mDebuggerWindow );
	return *mDebuggerWindow;
}

extern rvDebuggerApp gDebuggerApp;

#endif /* !__DEBUGGERAPP_H__ */
