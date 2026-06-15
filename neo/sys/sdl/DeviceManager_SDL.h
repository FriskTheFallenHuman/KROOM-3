/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 dhewg (dhewm3)

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#ifndef __DEVICEMANAGER_SDL_H__
#define __DEVICEMANAGER_SDL_H__

#include "../DeviceManager.h"

const int GRAB_ENABLE		= BIT( 0 );
const int GRAB_REENABLE		= BIT( 1 );
const int GRAB_HIDECURSOR	= BIT( 2 );
const int GRAB_SETSTATE		= BIT( 3 );

/*
=================================
idDeviceManagerSDL
=================================
*/
class idDeviceManagerSDL : public idDeviceManager
{
public:
	virtual void		PreInit();
	virtual bool		Init( vidParms_t parms );
	virtual void		Shutdown( bool shutdownSDL = false );
	virtual bool		SetScreenParms( vidParms_t parms );
	virtual bool		GetModeListForDisplay( const int displayNum, idList<vidMode_t>& modeList, const int minHeight );
	virtual bool		GetDefaultDisplayMode( int& defaultDisplayNum, vidMode_t& defaultMode );
	virtual void		SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] );
	virtual void		SwapBuffers();
	virtual void		DumpAllDisplayDevices();

public:
	void						GrabInput( int flags );
	std::vector<const char*>	GetRequiredExtensions();

private:
	static void			TestSwapBuffers( const idCmdArgs& args );

	void				SaveGamma();
	void				RestoreGamma();

	void				SetWindowsIcon( void* window );
	bool				SetScreenParmsWindowed( vidParms_t parms );
	int					ScreenParmsHandleDisplayIndex( vidParms_t parms );
	bool				SetScreenParmsFullscreen( vidParms_t parms );

private:
	unsigned short		oldHardwareGamma[3][256];
	bool				gammaSaved = false;
};

#endif /* !__DEVICEMANAGER_SDL_H__ */
