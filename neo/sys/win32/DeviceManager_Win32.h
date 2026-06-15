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

#ifndef __DEVICEMANAGER_WIN32_H__
#define __DEVICEMANAGER_WIN32_H__

#include "../DeviceManager.h"

/*
=================================
idDeviceManagerWin32
=================================
*/
class idDeviceManagerWin32 : public idDeviceManager
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

private:
	static void			TestSwapBuffers( const idCmdArgs& args );

	void				SaveGamma();
	void				RestoreGamma();

	bool				ChangeDislaySettingsIfNeeded( vidParms_t parms );
	void				PrintCDSError( int value );
	bool				CreateWindows( vidParms_t parms );
	bool				GetWindowDimensions( const vidParms_t parms, int& x, int& y, int& w, int& h );
	void				PrintDevMode( DEVMODE& devmode );
	const char*			DMDFO( int dmDisplayFixedOutput );
	bool				GetDisplayCoordinates( const int deviceNum, int& x, int& y, int& width, int& height, int& displayHz, int* bitsPerPixel = NULL );
	idStr				GetDeviceName( const int deviceNum );
	idStr				GetDisplayName( const int deviceNum );
	void				CreateWindowClasses();
#if !defined(USE_VULKAN)
	bool				InitDriver( vidParms_t parms );
	int					ChoosePixelFormat( const HDC hdc, const int multisamples );
	HGLRC				CreateOpenGLContextOnDC( const HDC hdc, const bool debugContext );
	void				GetWGLExtensionsWithFakeWindow();
	void				CheckWGLExtensions( HDC hDC );
#endif

private:
#if !defined(USE_VULKAN)
	HGLRC					hGLRC;						// handle to GL rendering context
#endif

	int						desktopBitsPixel;
	int						desktopWidth, desktopHeight;

	int						pixelformat;
	PIXELFORMATDESCRIPTOR	pfd;

	unsigned short			oldHardwareGamma[3][256];
	bool					windowClassRegistered;
};

#endif /* !__DEVICEMANAGER_WIN32_H__ */
