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
#ifndef __FIELDWINDOW_H
#define __FIELDWINDOW_H

#include "Window.h"


class idFieldWindow : public idWindow
{
public:
	idFieldWindow( idUserInterfaceLocal* gui );
	virtual ~idFieldWindow();

	virtual void Draw( int time, float x, float y );

private:
	virtual bool ParseInternalVar( const char* name, idTokenParser* src );
	void CommonInit();
	void CalcPaintOffset( int len );
	int cursorPos;
	int lastTextLength;
	int lastCursorPos;
	int paintOffset;
	bool showCursor;
	idStr cursorVar;
};

#endif // __FIELDWINDOW_H
