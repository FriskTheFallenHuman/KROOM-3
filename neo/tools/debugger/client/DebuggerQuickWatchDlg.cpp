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

#include "precompiled.h"
#pragma hdrstop

#include "DebuggerQuickWatchDlg.h"
#include "DebuggerApp.h"
#include "DebuggerClient.h"

wxBEGIN_EVENT_TABLE( rvDebuggerQuickWatchDlg, wxDialog )
	EVT_BUTTON( ID_QUICKWATCH_ADDWATCH, rvDebuggerQuickWatchDlg::OnAddWatch )
	EVT_BUTTON( ID_QUICKWATCH_RECALC, rvDebuggerQuickWatchDlg::OnRecalc )
	EVT_BUTTON( ID_QUICKWATCH_CLOSE, rvDebuggerQuickWatchDlg::OnClose )
	EVT_TEXT( wxID_ANY, rvDebuggerQuickWatchDlg::OnVariableChange )
wxEND_EVENT_TABLE()

/*
================
rvDebuggerQuickWatchDlg::rvDebuggerQuickWatchDlg
================
*/
rvDebuggerQuickWatchDlg::rvDebuggerQuickWatchDlg( wxWindow* parent,
		rvDebuggerWindow* debuggerWin,
		int callstackDepth,
		const char* variable )
	: wxDialog( parent, wxID_ANY, "Quick Watch", wxDefaultPosition, wxSize( 400, 300 ) ),
	  mDebuggerWindow( debuggerWin ),
	  mCallstackDepth( callstackDepth ),
	  mVariable( variable ? variable : "" )
{
	wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* topSizer = new wxBoxSizer( wxHORIZONTAL );
	topSizer->Add( new wxStaticText( this, wxID_ANY, "Variable:" ), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5 );
	mVarText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	topSizer->Add( mVarText, 1, wxALL | wxEXPAND, 5 );
	mainSizer->Add( topSizer, 0, wxEXPAND );

	wxBoxSizer* btnSizer = new wxBoxSizer( wxHORIZONTAL );
	mAddWatchBtn = new wxButton( this, ID_QUICKWATCH_ADDWATCH, "Add Watch" );
	btnSizer->Add( mAddWatchBtn, 0, wxALL, 5 );
	mRecalcBtn = new wxButton( this, ID_QUICKWATCH_RECALC, "Recalculate" );
	btnSizer->Add( mRecalcBtn, 0, wxALL, 5 );
	mCloseBtn = new wxButton( this, ID_QUICKWATCH_CLOSE, "Close" );
	btnSizer->Add( mCloseBtn, 0, wxALL, 5 );
	mainSizer->Add( btnSizer, 0, wxALIGN_RIGHT );

	mValueLabel = new wxStaticText( this, wxID_ANY, "Current value:" );
	mainSizer->Add( mValueLabel, 0, wxALL, 5 );

	mValueList = new wxListView( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT );
	mValueList->AppendColumn( "Name", wxLIST_FORMAT_LEFT, 100 );
	mValueList->AppendColumn( "Value", wxLIST_FORMAT_LEFT, 200 );
	mainSizer->Add( mValueList, 1, wxEXPAND | wxALL, 5 );

	SetSizer( mainSizer );
	Centre();

	// Disable controls until a variable is entered
	mAddWatchBtn->Enable( false );
	mRecalcBtn->Enable( false );
	mValueList->Enable( false );
	mValueLabel->Enable( false );

	if( !mVariable.IsEmpty() )
	{
		mVarText->SetValue( mVariable.c_str() );
		SetVariable( mVariable, true );
	}
}

/*
================
rvDebuggerQuickWatchDlg::~rvDebuggerQuickWatchDlg
================
*/
rvDebuggerQuickWatchDlg::~rvDebuggerQuickWatchDlg()
{

}

/*
================
rvDebuggerQuickWatchDlg::DoModal
================
*/
bool rvDebuggerQuickWatchDlg::DoModal()
{
	return ShowModal() == wxID_OK;
}

/*
================
rvDebuggerQuickWatchDlg::OnVariableChange
================
*/
void rvDebuggerQuickWatchDlg::OnVariableChange( wxCommandEvent& WXUNUSED( event ) )
{
	bool enable = !mVarText->GetValue().IsEmpty();
	mAddWatchBtn->Enable( enable );
	mRecalcBtn->Enable( enable );
}

/*
================
rvDebuggerQuickWatchDlg::OnAddWatch
================
*/
void rvDebuggerQuickWatchDlg::OnAddWatch( wxCommandEvent& WXUNUSED( event ) )
{
	wxString var = mVarText->GetValue();
	if( !var.IsEmpty() )
	{
		mDebuggerWindow->AddWatch( var.c_str() );
	}
}

/*
================
rvDebuggerQuickWatchDlg::OnRecalc
================
*/
void rvDebuggerQuickWatchDlg::OnRecalc( wxCommandEvent& WXUNUSED( event ) )
{
	wxString var = mVarText->GetValue();
	if( !var.IsEmpty() )
	{
		SetVariable( var.c_str() );
	}
}

/*
================
rvDebuggerQuickWatchDlg::OnClose
================
*/
void rvDebuggerQuickWatchDlg::OnClose( wxCommandEvent& WXUNUSED( event ) )
{
	EndModal( wxID_OK );
}

/*
================
rvDebuggerQuickWatchDlg::SetVariable
================
*/
void rvDebuggerQuickWatchDlg::SetVariable( const char* varname, bool force )
{
	if( !force && mVariable.Icmp( varname ) == 0 )
	{
		return;
	}

	mValueList->DeleteAllItems();

	// Request the variable value
	gDebuggerApp.GetClient().InspectVariable( varname, mCallstackDepth );

	// Wait for response (max 2.5 seconds)
	if( !gDebuggerApp.GetClient().WaitFor( DBMSG_INSPECTVARIABLE, 2500 ) )
	{
		return;
	}

	const char* value = gDebuggerApp.GetClient().GetVariableValue( varname, mCallstackDepth );
	if( value[0] == '\0' )
	{
		return;
	}

	mVariable = varname;

	// Enable display
	mValueList->Enable( true );
	mValueLabel->Enable( true );

	long idx = mValueList->InsertItem( 0, varname );
	mValueList->SetItem( idx, 1, value );
}