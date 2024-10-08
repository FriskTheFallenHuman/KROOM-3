/*
===========================================================================

KROOM 3 GPL Source Code

This file is part of the KROOM 3 Source Code, originally based
on the Doom 3 with bits and pieces from Doom 3 BFG edition GPL Source Codes both published in 2011 and 2012.

KROOM 3 Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Extra attributions can be found on the CREDITS.txt file

===========================================================================
*/

#ifndef __EDITWINDOW_H__
#define __EDITWINDOW_H__

#include "Window.h"

const int MAX_EDITFIELD = 4096;

class idUserInterfaceLocal;
class idSliderWindow;

class idEditWindow : public idWindow
{
public:
	idEditWindow( idUserInterfaceLocal* gui );
	virtual 			~idEditWindow();

	virtual void		Draw( int time, float x, float y );
	virtual const char* HandleEvent( const sysEvent_t* event, bool* updateVisuals );
	virtual void		PostParse();
	virtual void		GainFocus();
	virtual size_t		Allocated()
	{
		return idWindow::Allocated();
	};

	virtual idWinVar* 	GetWinVarByName( const char* _name, bool winLookup = false, drawWin_t** owner = NULL );

	virtual void 		HandleBuddyUpdate( idWindow* buddy );
	virtual void		Activate( bool activate, idStr& act );

	void				RunNamedEvent( const char* eventName );

private:

	virtual bool		ParseInternalVar( const char* name, idTokenParser* src );

	void				InitCvar();
	// true: read the updated cvar from cvar system
	// false: write to the cvar system
	// force == true overrides liveUpdate 0
	void				UpdateCvar( bool read, bool force = false );

	void				CommonInit();
	void				EnsureCursorVisible();
	void				InitScroller( bool horizontal );

	int					maxChars;
	int					paintOffset;
	int					cursorPos;
	int					cursorLine;
	int					cvarMax;
	bool				wrap;
	bool				readonly;
	bool				numeric;
	idStr				sourceFile;
	idSliderWindow* 	scroller;
	idList<int>			breaks;
	float				sizeBias;
	int					textIndex;
	int					lastTextLength;
	bool				forceScroll;
	idWinBool			password;

	idWinStr			cvarStr;
	idCVar* 			cvar;

	idWinBool			liveUpdate;
	idWinStr			cvarGroup;
};

#endif /* !__EDITWINDOW_H__ */
