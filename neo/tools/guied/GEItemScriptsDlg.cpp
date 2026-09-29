/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 1999-2011 Raven Software

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

#include "GEApp.h"

rvGEItemScriptsDlg::rvGEItemScriptsDlg( wxWindow* parent, idWindow* window )
	: wxDialog( parent, wxID_ANY, "Item Scripts", wxDefaultPosition, wxSize( 640, 480 ), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
	, m_window( window )
{
	m_edit = new wxStyledTextCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE );

	wxFont mono( 9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );
	m_edit->StyleSetFont( wxSTC_STYLE_DEFAULT, mono );
	m_edit->StyleSetForeground( wxSTC_STYLE_DEFAULT, *wxBLACK );
	m_edit->StyleSetBackground( wxSTC_STYLE_DEFAULT, *wxWHITE );
	m_edit->StyleClearAll();
	m_edit->SetTabWidth( 4 );
	m_edit->SetUseTabs( true );
	m_edit->SetMarginWidth( 0, 32 );   // line numbers

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_edit, 1, wxEXPAND | wxALL, 6 );
	sizer->Add( CreateButtonSizer( wxOK | wxCANCEL ), 0, wxEXPAND | wxALL, 6 );
	SetSizer( sizer );

	Bind( wxEVT_CLOSE_WINDOW, &rvGEItemScriptsDlg::OnClose, this );
	Bind( wxEVT_CHAR_HOOK, &rvGEItemScriptsDlg::OnCharHook, this );

	Init();
}

rvGEItemScriptsDlg::~rvGEItemScriptsDlg() = default;

bool rvGEItemScriptsDlg::Init()
{
	rvGEWindowWrapper* wrapper = rvGEWindowWrapper::GetWrapper( m_window );
	if( !wrapper )
	{
		return false;
	}

	wxString text;

	// Variables first, then scripts.
	const idDict& vars = wrapper->GetVariableDict();
	for( int i = 0; i < vars.GetNumKeyVals(); ++i )
	{
		const idKeyValue* kv = vars.GetKeyVal( i );
		if( !kv )
		{
			continue;
		}
		text += wxString::FromUTF8( kv->GetKey().c_str() );
		text += "\t";
		text += wxString::FromUTF8( kv->GetValue().c_str() );
		text += "\r\n";
	}

	if( vars.GetNumKeyVals() )
	{
		text += "\r\n";
	}

	const idDict& scripts = wrapper->GetScriptDict();
	for( int i = 0; i < scripts.GetNumKeyVals(); ++i )
	{
		const idKeyValue* kv = scripts.GetKeyVal( i );
		if( !kv )
		{
			continue;
		}
		text += wxString::FromUTF8( kv->GetKey().c_str() );
		text += "\r\n";
		text += wxString::FromUTF8( kv->GetValue().c_str() );
		text += "\r\n\r\n";
	}

	m_edit->SetText( text );
	m_edit->GotoPos( 0 );
	m_edit->SetFocus();

	return true;
}

bool rvGEItemScriptsDlg::Apply()
{
	rvGEWindowWrapper* wrapper = rvGEWindowWrapper::GetWrapper( m_window );
	if( !wrapper )
	{
		return false;
	}

	wxString text = m_edit->GetText();
	idStr parse = text.utf8_str();

	try
	{
		idParser src( parse, parse.Length(), "", LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
		idToken token;

		wrapper->GetVariableDict().Clear();
		wrapper->GetScriptDict().Clear();

		while( src.ReadToken( &token ) )
		{
			idStr scriptName;
			idStr out;

			if( !token.Icmp( "definevec4" ) )
			{
				idToken token2;
				if( !src.ReadToken( &token2 ) )
				{
					src.Error( "expected define name" );
					return false;
				}

				idWinVec4 var;
				idUserInterfaceLocal ui;
				idWindow tempwin( &ui );

				src.SetMarker();
				for( int i = 0; i < 3; ++i )
				{
					tempwin.ParseExpression( &src, &var );
					src.ExpectTokenString( "," );
				}
				tempwin.ParseExpression( &src, &var );
				src.GetStringFromMarker( out, true );

				wrapper->GetVariableDict().Set( token + "\t\"" + token2 + "\"", out );
				continue;
			}
			else if( !token.Icmp( "definefloat" ) || !token.Icmp( "float" ) )
			{
				idToken token2;
				if( !src.ReadToken( &token2 ) )
				{
					src.Error( "expected define name" );
					return false;
				}

				idWinFloat var;
				idUserInterfaceLocal ui;
				idWindow tempwin( &ui );

				src.SetMarker();
				tempwin.ParseExpression( &src, &var );
				src.GetStringFromMarker( out, true );

				wrapper->GetVariableDict().Set( token + "\t\"" + token2 + "\"", out );
				continue;
			}
			else if( !token.Icmp( "scriptdef" ) )
			{
				scriptName = "scriptDef";
			}
			else if( !token.Icmp( "ontime" ) )
			{
				if( !src.ReadToken( &token ) )
				{
					src.Error( "expected time" );
					return false;
				}
				scriptName = "onTime ";
				scriptName.Append( token );
			}
			else if( !token.Icmp( "onevent" ) )
			{
				if( !src.ReadToken( &token ) )
				{
					src.Error( "expected event" );
					return false;
				}
				scriptName = "onEvent ";
				scriptName.Append( token );
			}
			else
			{
				int i = 0;
				for( ; i < idWindow::SCRIPT_COUNT; ++i )
				{
					if( idStr::Icmp( idWindow::ScriptNames[i], token ) == 0 )
					{
						scriptName = idWindow::ScriptNames[i];
						break;
					}
				}
				if( i >= idWindow::SCRIPT_COUNT )
				{
					src.Error( "expected script name" );
					return false;
				}
			}

			if( !src.ParseBracedSectionExact( out, 1 ) )
			{
				return false;
			}

			wrapper->GetScriptDict().Set( scriptName, out );
		}
	}
	catch( idException& e )
	{
		wxMessageBox( e.GetError(), "Script Error",
					  wxOK | wxICON_ERROR, this );
		return false;
	}

	return true;
}

void rvGEItemScriptsDlg::OnClose( wxCloseEvent& e )
{
	if( e.CanVeto() && !Apply() )
	{
		e.Veto( true );
		return;
	}
	m_applied = true;
	e.Skip();
}

void rvGEItemScriptsDlg::OnCharHook( wxKeyEvent& e )
{
	if( e.GetKeyCode() == WXK_ESCAPE )
	{
		Close( true );
		return;
	}
	e.Skip();
}