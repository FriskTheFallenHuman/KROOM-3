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

#include "precompiled.h"
#pragma hdrstop

#include "Window.h"
#include "UserInterfaceLocal.h"
#include "BindWindow.h"


void idBindWindow::CommonInit()
{
	bindName = "";
	waitingOnKey = false;
}

idBindWindow::idBindWindow( idUserInterfaceLocal* g ) : idWindow( g )
{
	gui = g;
	CommonInit();
}

idBindWindow::~idBindWindow()
{

}


const char* idBindWindow::HandleEvent( const sysEvent_t* event, bool* updateVisuals )
{
	static char ret[ 256 ];

	if( !( event->evType == SE_KEY && event->evValue2 ) )
	{
		return "";
	}

	int key = event->evValue;

	if( waitingOnKey )
	{
		waitingOnKey = false;
		if( key == K_ESCAPE )
		{
			idStr::snPrintf( ret, sizeof( ret ), "clearbind \"%s\"", bindName.GetName() );
		}
		else
		{
			idStr::snPrintf( ret, sizeof( ret ), "bind %i \"%s\"", key, bindName.GetName() );
		}
		return ret;
	}
	else
	{
		if( key == K_MOUSE1 )
		{
			waitingOnKey = true;
			gui->SetBindHandler( this );
			return "";
		}
	}

	return "";
}

idWinVar* idBindWindow::GetWinVarByName( const char* _name, bool fixup, drawWin_t** owner )
{

	if( idStr::Icmp( _name, "bind" ) == 0 )
	{
		return &bindName;
	}

	return idWindow::GetWinVarByName( _name, fixup, owner );
}

void idBindWindow::PostParse()
{
	idWindow::PostParse();
	bindName.SetGuiInfo( gui->GetStateDict(), bindName );
	bindName.Update();
	//bindName = state.GetString("bind");
	flags |= ( WIN_HOLDCAPTURE | WIN_CANFOCUS );
}

void idBindWindow::Draw( int time, float x, float y )
{
	idVec4 color = foreColor;

	idStr str;
	if( waitingOnKey )
	{
		str = common->GetLanguageDict()->GetString( "#str_07000" );
	}
	else if( bindName.Length() )
	{
		str = bindName.c_str();
	}
	else
	{
		str = common->GetLanguageDict()->GetString( "#str_07001" );
	}

	if( waitingOnKey || ( hover && !noEvents && Contains( gui->CursorX(), gui->CursorY() ) ) )
	{
		color = hoverColor;
	}
	else
	{
		hover = false;
	}

	dc->DrawText( str, textScale, textAlign, color, textRect, false, -1 );
}

void idBindWindow::Activate( bool activate, idStr& act )
{
	idWindow::Activate( activate, act );
	bindName.Update();
}
