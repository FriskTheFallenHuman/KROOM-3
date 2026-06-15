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

#ifndef __DEVICEMANAGER_H__
#define __DEVICEMANAGER_H__

struct vidMode_t
{
	int width;
	int height;
	int displayHz;

	vidMode_t()
	{
		width = 640;
		height = 480;
		displayHz = 60;
	}

	vidMode_t( int width, int height, int displayHz ) :
		width( width ), height( height ), displayHz( displayHz ) {}

	bool operator==( const vidMode_t& a )
	{
		return a.width == width && a.height == height && a.displayHz == displayHz;
	}
};

struct vidParms_t
{
	int			x;				// ignored in fullscreen
	int			y;				// ignored in fullscreen
	int			width;
	int			height;
	int			fullScreen;		// 0 = windowed, otherwise 1 based monitor number to go full screen on
	int			displayHz;
	int			multiSamples;
};

/*
=================================
idDeviceManager

Replacement for Glimp/Vkimp
=================================
*/
class idDeviceManager
{
public:
	virtual				~idDeviceManager() {};

	// GetModeListForDisplay is called before Init(), but SDL needs SDL_Init() first.
	// So add PreInit for platforms that need it, others can just stub it.
	virtual void		PreInit() = 0;

	// If the desired mode can't be set satisfactorily, false will be returned.
	// If succesful, sets glConfig.nativeScreenWidth, glConfig.nativeScreenHeight, and glConfig.pixelAspect
	// The renderer will then reset the glimpParms to "safe mode" of 640x480
	// fullscreen and try again.  If that also fails, the error will be fatal.
	virtual bool		Init( vidParms_t parms ) = 0;

	// Destroys the rendering context, closes the window, resets the resolution,
	// and resets the gamma ramps.
	virtual void		Shutdown() = 0;

	// will set up gl up with the new parms
	virtual bool		SetScreenParms( vidParms_t parms ) = 0;

	// the number of displays can be found by itterating this until it returns false
	// displayNum is the 0 based value passed to EnumDisplayDevices(), you must add
	// 1 to this to get an r_fullScreen value.
	virtual bool		GetModeListForDisplay( const int displayNum, idList<vidMode_t>& modeList, const int minHeight ) = 0;
	virtual bool		GetDefaultDisplayMode( int& defaultDisplayNum, vidMode_t& defaultMode ) = 0;

	// Sets the hardware gamma ramps for gamma and brightness adjustment.
	// These are now taken as 16 bit values, so we can take full advantage
	// of dacs with >8 bits of precision
	virtual void		SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] ) = 0;

	virtual void		SwapBuffers() = 0;
	virtual void		DumpAllDisplayDevices() = 0;

public:
	static idDeviceManager* GetInstance();
	static idDeviceManager* s_instance;
};

#endif /* !__DEVICEMANAGER_H__ */
