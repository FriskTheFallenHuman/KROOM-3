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

#include "DebuggerApp.h"
#include "DebuggerFindDlg.h"

wxBEGIN_EVENT_TABLE( rvDebuggerFindDlg, wxDialog )
	EVT_BUTTON( wxID_OK, rvDebuggerFindDlg::OnOK )
	EVT_BUTTON( wxID_CANCEL, rvDebuggerFindDlg::OnCancel )
wxEND_EVENT_TABLE()

/*
================
rvDebuggerFindDlg::rvDebuggerFindDlg
================
*/
rvDebuggerFindDlg::rvDebuggerFindDlg( wxWindow* parent )
	: wxDialog( parent, wxID_ANY, "Find", wxDefaultPosition, wxSize( 300, 100 ) )
{
	mFindText = NULL;

	wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* rowSizer = new wxBoxSizer( wxHORIZONTAL );
	rowSizer->Add( new wxStaticText( this, wxID_ANY, "Find what:" ), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5 );
	mTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	rowSizer->Add( mTextCtrl, 1, wxALL | wxEXPAND, 5 );
	mainSizer->Add( rowSizer, 0, wxEXPAND );

	wxBoxSizer* btnSizer = new wxBoxSizer( wxHORIZONTAL );
	btnSizer->Add( new wxButton( this, wxID_OK, "Find" ), 0, wxALL, 5 );
	btnSizer->Add( new wxButton( this, wxID_CANCEL, "Cancel" ), 0, wxALL, 5 );
	mainSizer->Add( btnSizer, 0, wxALIGN_CENTER );

	SetSizer( mainSizer );
	Centre();
}

/*
================
rvDebuggerFindDlg::~rvDebuggerFindDlg
================
*/
rvDebuggerFindDlg::~rvDebuggerFindDlg()
{
}

/*
================
rvDebuggerFindDlg::DoModal

Launch the dialog
================
*/
bool rvDebuggerFindDlg::DoModal()
{
	return ShowModal() == wxID_OK;
}

/*
================
rvDebuggerFindDlg::OnOK
================
*/
void rvDebuggerFindDlg::OnOK( wxCommandEvent& WXUNUSED( event ) )
{
	mFindText = mTextCtrl->GetValue();
	EndModal( wxID_OK );
}

/*
================
rvDebuggerFindDlg::OnCancel
================
*/
void rvDebuggerFindDlg::OnCancel( wxCommandEvent& WXUNUSED( event ) )
{
	EndModal( wxID_CANCEL );
}