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

#include "precompiled.h"
#pragma hdrstop

#include "Game_local.h"

extern idCVar popupDialog_debug;
extern idCVar dialog_saveClearLevel;

/*
========================
idGameDialogsLocal::InitImp
========================
*/
void idGameDialogsLocal::InitImp()
{
	dialog = NULL;
	suppressMouseRelease = false;
}

/*
========================
idGameDialogsLocal::Init
========================
*/
void idGameDialogsLocal::Init()
{
	idLib::PrintfIf( popupDialog_debug.GetBool(), "[%s]\n", __FUNCTION__ );

	Shutdown();

	dialog = uiManager->FindGui( "guis/msg.gui", true, false, true );
	if( !dialog )
	{
		idLib::Warning( "Failed to load dialog GUI 'guis/msg.gui'" );
		return;
	}

	dialog->Activate( false, 0 );
	dialog->SetStateString( "visible_msgbox", "0" );
	dialog->SetStateString( "visible_waitbox", "0" );
	dialog->SetStateString( "visible_hasxp", /*fileSystem->HasD3XP() ? "1" :*/ "0" );
}

/*
========================
idGameDialogsLocal::Shutdown
========================
*/
void idGameDialogsLocal::Shutdown()
{
	idLib::PrintfIf( popupDialog_debug.GetBool(), "[%s]\n", __FUNCTION__ );

	if( dialog != NULL )
	{
		ActivateRenderer( false );
	}

	ClearDialogs( true );
	dialog = NULL;

	suppressMouseRelease = false;
}

/*
========================
idGameDialogsLocal::IsRendererLoaded
========================
*/
bool idGameDialogsLocal::IsRendererLoaded() const
{
	return ( dialog != NULL );
}

/*
========================
idGameDialogsLocal::IsRendererActive
========================
*/
bool idGameDialogsLocal::IsRendererActive() const
{
	return ( dialog != NULL && dialog->IsActive() );
}

/*
========================
idGameDialogsLocal::ActivateRenderer
========================
*/
void idGameDialogsLocal::ActivateRenderer( bool active )
{
	if( dialog == NULL )
	{
		return;
	}

	if( active )
	{
		dialog->Activate( true, Sys_Milliseconds() );
	}
	else
	{
		dialog->Activate( false, Sys_Milliseconds() );
	}
}

/*
========================
idGameDialogsLocal::IsSaveIndicatorActive
========================
*/
bool idGameDialogsLocal::IsSaveIndicatorActive() const
{
	return false;
}

/*
========================
idGameDialogsLocal::RenderDialog
========================
*/
void idGameDialogsLocal::RenderDialog( int timeMicroseconds )
{
	if( dialog != NULL && dialog->IsActive() )
	{
		dialog->Redraw( Sys_Milliseconds(), false );
	}
}

/*
========================
idGameDialogsLocal::RenderSaveIndicator
========================
*/
void idGameDialogsLocal::RenderSaveIndicator( int timeMicroseconds )
{
}

/*
========================
idGameDialogsLocal::SetRendererGlobalInt
========================
*/
void idGameDialogsLocal::SetRendererGlobalInt( const char* name, int val )
{
	if( dialog != NULL )
	{
		dialog->SetStateInt( name, val );
	}
}

/*
========================
idGameDialogsLocal::SetRendererGlobalString
========================
*/
void idGameDialogsLocal::SetRendererGlobalString( const char* name, const char* val )
{
	if( dialog != NULL )
	{
		dialog->SetStateString( name, val );
	}
}

/*
========================
idGameDialogsLocal::HandleDialogCommand
========================
*/
void idGameDialogsLocal::HandleDialogCommand( const char* cmd )
{
	idLib::PrintfIf( popupDialog_debug.GetBool(), "[%s] cmd: %s\n", __FUNCTION__, cmd ? cmd : "NULL" );

	if( messageList.Num() == 0 )
	{
		return;
	}

	idDialogInfo& info = messageList[0];
	if( info.clear )
	{
		return;
	}

	bool shouldClear = false;

	if( !idStr::Icmp( cmd, "mid" ) )
	{
		if( info.acceptCB )
		{
			InvokeCallback( info.acceptCB );
		}
		shouldClear = true;
	}
	else if( !idStr::Icmp( cmd, "left" ) )
	{
		if( info.cancelCB )
		{
			InvokeCallback( info.cancelCB );
		}
		else if( info.altCBOne )
		{
			InvokeCallback( info.altCBOne );
		}
		shouldClear = true;
	}
	else if( !idStr::Icmp( cmd, "right" ) )
	{
		if( info.altCBOne )
		{
			InvokeCallback( info.altCBOne );
		}
		else if( info.cancelCB )
		{
			InvokeCallback( info.cancelCB );
		}
		shouldClear = true;
	}
	else if( !idStr::Icmp( cmd, "stop" ) )
	{
		shouldClear = true;
	}

	if( shouldClear )
	{
		info.clear = true;
		ActivateRenderer( false );
	}
}

/*
========================
idGameDialogsLocal::BindDialogToRenderer
========================
*/
void idGameDialogsLocal::BindDialogToRenderer( const idDialogInfo& info )
{
	if( dialog == NULL )
	{
		return;
	}

	idStr message, title;
	GetDialogMsg( info.msg, message, title );

	dialog->SetStateString( "title", idLocalization::GetString( title ) );
	if( info.overrideMsg.IsEmpty() )
	{
		dialog->SetStateString( "message", idLocalization::GetString( message ) );
	}
	else
	{
		dialog->SetStateString( "message", info.overrideMsg );
	}

	bool visibleMsgBox = true;
	bool visibleWaitBox = false;
	bool visibleMid = false;
	bool visibleLeft = false;
	bool visibleRight = false;

	idStr midText, leftText, rightText;

	switch( info.type )
	{
		case DIALOG_ACCEPT:
		case DIALOG_CONTINUE:
		case DIALOG_CANCEL:
			visibleMid = true;
			midText = ( info.type == DIALOG_CONTINUE ) ? "#str_swf_continue" :
					  ( info.type == DIALOG_CANCEL ) ? "#str_swf_cancel" :
					  "#str_swf_accept";
			break;

		case DIALOG_ACCEPT_CANCEL:
			visibleLeft = true;
			visibleRight = true;
			leftText = "#str_swf_accept";
			rightText = "#str_swf_cancel";
			break;

		case DIALOG_YES_NO:
			visibleLeft = true;
			visibleRight = true;
			leftText = "#str_swf_yes";
			rightText = "#str_swf_no";
			break;

		case DIALOG_WAIT:
		case DIALOG_WAIT_BLACKOUT:
		case DIALOG_WAIT_CANCEL:
			visibleMsgBox = false;
			visibleWaitBox = true;
			break;

		case DIALOG_DYNAMIC:
			// Up to 3 options (accept, cancel, alt)
			if( !info.txt1.IsEmpty() )
			{
				visibleMid = true;
				midText = info.txt1.GetLocalizedString();
			}
			if( !info.txt2.IsEmpty() )
			{
				visibleLeft = true;
				leftText = info.txt2.GetLocalizedString();
			}
			if( !info.txt3.IsEmpty() )
			{
				visibleRight = true;
				rightText = info.txt3.GetLocalizedString();
			}
			// Note: we ignore txt4 (would need a fourth button)
			break;

		case DIALOG_QUICK_SAVE:
		case DIALOG_CRAWL_SAVE:
		case DIALOG_TIMER_ACCEPT_REVERT:
		case DIALOG_BENCHMARK:
			// For these, we can treat as wait or accept, but we'll keep them as generic
			visibleMid = true;
			midText = "#str_swf_accept";
			break;

		default:
			visibleMid = true;
			midText = "#str_swf_accept";
			break;
	}

	// Set visibility flags
	dialog->SetStateBool( "visible_msgbox", visibleMsgBox );
	dialog->SetStateBool( "visible_waitbox", visibleWaitBox );
	dialog->SetStateBool( "visible_mid", visibleMid );
	dialog->SetStateBool( "visible_left", visibleLeft );
	dialog->SetStateBool( "visible_right", visibleRight );

	// Set button labels
	dialog->SetStateString( "mid", idLocalization::GetString( midText ) );
	dialog->SetStateString( "left", idLocalization::GetString( leftText ) );
	dialog->SetStateString( "right", idLocalization::GetString( rightText ) );

	// The GUI does not need explicit callbacks; commands are handled by HandleDialogCommand.
	// However, we store the callbacks in the dialog info for later invocation.

	// For timer-based dialogs (DIALOG_TIMER_ACCEPT_REVERT), we can handle the countdown
	// in the Render loop by checking the remaining time.
	if( info.type == DIALOG_TIMER_ACCEPT_REVERT )
	{
		// We'll use the GUI variable "countdownInfo" to display time remaining
		// This will be updated in Render().
	}
}

/*
========================
idGameDialogsLocal::IsDialogActive
========================
*/
bool idGameDialogsLocal::IsDialogActive() const
{
	return ( dialog && dialog->IsActive() );
}

/*
================================================
idGameDialogsLocal::HandleDialogEvent
================================================
*/
bool idGameDialogsLocal::HandleDialogEvent( const sysEvent_t* sev )
{
	if( dialog != NULL && dialog->IsActive() )
	{
		if( sev->evType == SE_KEY && sev->evValue == K_MOUSE1 && !sev->evValue2 && suppressMouseRelease )
		{
			suppressMouseRelease = false;
			return true;
		}

		if( sev->evType == SE_MOUSE_ABSOLUTE )
		{
			const float width = renderSystem->GetWidth();
			const float height = renderSystem->GetHeight();
			if( width > 0.0f && height > 0.0f )
			{
				dialog->SetCursor( ( float )sev->evValue * SCREEN_WIDTH / width,
								   ( float )sev->evValue2 * SCREEN_HEIGHT / height );
			}
		}

		// Let the GUI process the event; it will return a command string if any.
		const char* cmd = dialog->HandleEvent( sev, Sys_Milliseconds() );
		if( cmd && cmd[0] )
		{
			HandleDialogCommand( cmd );
			idKeyInput::ClearStates();
			Sys_ClearEvents();
		}
		return true;
	}

	return false;
}

/*
================================================
idGameDialogsLocal::ShowSaveIndicator
================================================
*/
void idGameDialogsLocal::ShowSaveIndicator( bool show )
{
	idLib::PrintfIf( popupDialog_debug.GetBool(), "[%s] show=%d\n", __FUNCTION__, show );

	if( show )
	{
		AddDialog( GDM_SAVING, DIALOG_WAIT, NULL, NULL, true, "", 0, false, true, true );
	}
	else
	{
		ClearDialog( GDM_SAVING );
	}
}

CONSOLE_COMMAND( testShowDynamicDialog, "show a dynamic dialog", 0 )
{
	class idDialogContinueCallback : public idDialogCallback
	{
	public:
		void Call() override
		{
			dialogs->ClearDialog( GDM_INSUFFICENT_STORAGE_SPACE );
		}
	};

	idStaticList< idDialogCallback*, 4 > callbacks;
	idStaticList< idStrId, 4 > optionText;
	callbacks.Append( new idDialogContinueCallback() );
	optionText.Append( idStrId( "#str_swf_continue" ) );

	// build custom space required string
	// #str_dlg_space_required ~= "There is insufficient storage available.  Please free %s and try again."
	idStr format = idStrId( "#str_dlg_space_required" ).GetLocalizedString();
	idStr size;
	int requiredSpaceInBytes = 150000;
	if( requiredSpaceInBytes > ( 1024 * 1024 ) )
	{
		size = va( "%.1f MB", ( float ) requiredSpaceInBytes / ( 1024.0f * 1024.0f ) );
	}
	else
	{
		size = va( "%.0f KB", ( float ) requiredSpaceInBytes / 1024.0f );
	}
	idStr msg = va( format.c_str(), size.c_str() );

	ADD_DYNAMIC_DIALOG( GDM_INSUFFICENT_STORAGE_SPACE, callbacks, optionText, true, msg );
}

CONSOLE_COMMAND( testShowDialogBug, "show a dynamic dialog", 0 )
{
	dialogs->ShowSaveIndicator( true );
	dialogs->ShowSaveIndicator( false );

	// This locks the game because it thinks it's paused because we're passing in pause = true but the
	// dialog isn't ever added because of the abuse of dialog->isActive when the save indicator is shown.
	int dialogId = atoi( args.Argv( 1 ) );
	ADD_DIALOG( ( gameDialogMessages_t )dialogId, DIALOG_ACCEPT, NULL, NULL, true );
}