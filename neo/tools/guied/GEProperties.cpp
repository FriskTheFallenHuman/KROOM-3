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

/*
================
rvGEProperties::rvGEProperties
================
*/
rvGEProperties::rvGEProperties( wxWindow* parent )
	: wxPanel( parent, wxID_ANY )
	, m_grid( NULL )
	, m_wrapper( NULL )
	, m_workspace( NULL )
	, m_updating( false )
{
	m_grid = new wxPropertyGridManager( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPGMAN_DEFAULT_STYLE | wxPG_SPLITTER_AUTO_CENTER );
	m_grid->SetColumnProportion( 0, 45 );
	m_grid->SetColumnProportion( 1, 55 );
	m_grid->SetMinSize( wxSize( 200, 200 ) );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_grid, 1, wxEXPAND );
	SetSizer( sizer );

	m_grid->Bind( wxEVT_PG_CHANGED, &rvGEProperties::OnPropertyChanged, this );
}

/*
================
rvGEProperties::~rvGEProperties
================
*/
rvGEProperties::~rvGEProperties()
{
}

/*
================
rvGEProperties::EmbedTransformer

Docks a transformer panel at the bottom of the properties pane.
================
*/
void rvGEProperties::EmbedTransformer( wxWindow* transformerPanel )
{
	if( !transformerPanel || transformerPanel->GetParent() != this )
	{
		return;
	}

	wxSizer* sizer = GetSizer();
	if( !sizer )
	{
		return;
	}

	sizer->Add( new wxStaticLine( this ), 0, wxEXPAND | wxTOP | wxBOTTOM, 4 );
	sizer->Add( transformerPanel, 0, wxEXPAND );
	Layout();
}

/*
================
rvGEProperties::SetWorkspace
================
*/
void rvGEProperties::SetWorkspace( rvGEWorkspace* workspace )
{
	m_workspace = workspace;
	Update();
}

/*
================
rvGEProperties::Update

Rebuilds the property grid from the currently selected wrapper.
================
*/
void rvGEProperties::Update()
{
	m_updating = true;

	if( m_grid->GetGrid() )
	{
		m_grid->GetGrid()->Clear();
	}
	m_grid->Clear();

	m_wrapper = NULL;

	if( m_workspace && m_workspace->GetSelectionMgr().Num() == 1 )
	{
		m_wrapper = rvGEWindowWrapper::GetWrapper( m_workspace->GetSelectionMgr()[0] );
	}

	if( !m_wrapper )
	{
		m_updating = false;
		return;
	}

	const idDict& dict = m_wrapper->GetStateDict();
	for( int i = 0; i < dict.GetNumKeyVals(); ++i )
	{
		const idKeyValue* kv = dict.GetKeyVal( i );
		if( !kv )
		{
			continue;
		}
		if( !kv->GetKey().Icmp( "name" ) )
		{
			continue;
		}

		idStr value = kv->GetValue();
		value.StripQuotes();

		// Give a name to our properties.
		const wxString propName = wxString::Format( "p_%d", i );

		wxPGProperty* prop = m_grid->Append(
								 new wxStringProperty( kv->GetKey().c_str(),
										 propName,
										 wxString::FromUTF8( value.c_str() ) ) );

		if( prop )
		{
			prop->SetClientData( const_cast< idKeyValue* >( kv ) );
		}
	}

	m_updating = false;
}

/*
================
rvGEProperties::OnPropertyChanged
================
*/
void rvGEProperties::OnPropertyChanged( wxPropertyGridEvent& e )
{
	if( m_updating || !m_wrapper || !m_workspace )
	{
		return;
	}

	wxPGProperty* prop = e.GetProperty();
	if( !prop )
	{
		return;
	}

	const idKeyValue* kv =	static_cast< const idKeyValue* >( prop->GetClientData() );
	if( !kv )
	{
		return;
	}

	const wxString key   = kv->GetKey().c_str();
	const wxString value = prop->GetValueAsString();

	AddModifier( key.utf8_str(), value.utf8_str() );
}

/*
================
rvGEProperties::AddModifier

Validates the new value against the wrapper, then pushes a modifier.
================
*/
bool rvGEProperties::AddModifier( const char* name, const char* value )
{
	idStr tempValue = value;

	if( !m_wrapper->VerfiyStateKey( name, tempValue ) )
	{
		tempValue = "\"";
		tempValue += value;
		tempValue += "\"";
		if( !m_wrapper->VerfiyStateKey( name, tempValue ) )
		{
			gApp->MessageBox( wxString::Format( "Invalid property value '%s' for property '%s'", value, name ), wxOK | wxICON_ERROR );
			return false;
		}
	}

	idDict tempState = m_wrapper->GetStateDict();
	tempState.Set( name, tempValue );

	m_workspace->GetModifierStack().Append( new rvGEStateModifier( "Property Change", m_wrapper->GetWindow(), tempState ) );

	m_workspace->SetModified( true );
	gApp->GetNavigator().Update();

	return true;
}