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

rvGETransformer::rvGETransformer( wxWindow* parent )
	: wxPanel( parent, wxID_ANY )
{
	wxFlexGridSizer* sizer = new wxFlexGridSizer( 2, 4, 4, 8 );
	sizer->AddGrowableCol( 1, 1 );
	sizer->AddGrowableCol( 3, 1 );

	auto makeSpin = [this]( const wxString & label ) -> wxSpinCtrl*
	{
		new wxStaticText( this, wxID_ANY, label );
		return nullptr;
	};

	sizer->Add( new wxStaticText( this, wxID_ANY, "X:" ), 0, wxALIGN_CENTER_VERTICAL );
	m_x = new wxSpinCtrl( this, IDC_GUIED_ITEMRECTX, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxSP_WRAP, -32000, 32000, 0 );
	sizer->Add( m_x, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Y:" ), 0, wxALIGN_CENTER_VERTICAL );
	m_y = new wxSpinCtrl( this, IDC_GUIED_ITEMRECTY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxSP_WRAP, -32000, 32000, 0 );
	sizer->Add( m_y, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "W:" ), 0, wxALIGN_CENTER_VERTICAL );
	m_w = new wxSpinCtrl( this, IDC_GUIED_ITEMRECTW, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxSP_WRAP, -32000, 32000, 0 );
	sizer->Add( m_w, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "H:" ), 0, wxALIGN_CENTER_VERTICAL );
	m_h = new wxSpinCtrl( this, IDC_GUIED_ITEMRECTH, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxSP_WRAP, -32000, 32000, 0 );
	sizer->Add( m_h, 1, wxEXPAND );

	SetSizer( sizer );

	m_x->Bind( wxEVT_SPINCTRL, &rvGETransformer::OnSpin, this );
	m_y->Bind( wxEVT_SPINCTRL, &rvGETransformer::OnSpin, this );
	m_w->Bind( wxEVT_SPINCTRL, &rvGETransformer::OnSpin, this );
	m_h->Bind( wxEVT_SPINCTRL, &rvGETransformer::OnSpin, this );

	m_x->Bind( wxEVT_TEXT_ENTER, &rvGETransformer::OnText, this );
	m_y->Bind( wxEVT_TEXT_ENTER, &rvGETransformer::OnText, this );
	m_w->Bind( wxEVT_TEXT_ENTER, &rvGETransformer::OnText, this );
	m_h->Bind( wxEVT_TEXT_ENTER, &rvGETransformer::OnText, this );
}

rvGETransformer::~rvGETransformer() = default;

void rvGETransformer::SetWorkspace( rvGEWorkspace* workspace )
{
	m_workspace = workspace;
	Update();
}

void rvGETransformer::Update()
{
	m_updating = true;

	const bool hasSelection =
		m_workspace && m_workspace->GetSelectionMgr().Num() > 0;

	if( hasSelection )
	{
		m_relative = m_workspace->GetSelectionMgr().GetBottomMost();
		if( m_relative )
		{
			m_relative = m_relative->GetParent();
		}

		idRectangle rect = m_workspace->GetSelectionMgr().GetRect();
		m_workspace->WindowToWorkspace( rect );

		if( m_relative )
		{
			idRectangle& screen =
				rvGEWindowWrapper::GetWrapper( m_relative )->GetScreenRect();
			rect.x -= screen.x;
			rect.y -= screen.y;
		}

		m_x->SetValue( ( int )rect.x );
		m_y->SetValue( ( int )rect.y );
		m_w->SetValue( ( int )rect.w );
		m_h->SetValue( ( int )rect.h );
	}
	else
	{
		m_x->SetValue( 0 );
		m_y->SetValue( 0 );
		m_w->SetValue( 0 );
		m_h->SetValue( 0 );
	}

	m_x->Enable( hasSelection );
	m_y->Enable( hasSelection );
	m_w->Enable( hasSelection );
	m_h->Enable( hasSelection );

	m_updating = false;
}

void rvGETransformer::OnSpin( wxSpinEvent& e )
{
	ApplyField( e.GetId(), e.GetPosition() );
}

void rvGETransformer::OnText( wxCommandEvent& e )
{
	wxSpinCtrl* ctrl = dynamic_cast<wxSpinCtrl*>( e.GetEventObject() );
	if( !ctrl )
	{
		return;
	}
	ApplyField( ctrl->GetId(), ctrl->GetValue() );
}

void rvGETransformer::ApplyField( int id, int newValue )
{
	if( m_updating || !m_workspace )
	{
		return;
	}
	if( m_workspace->GetSelectionMgr().Num() == 0 )
	{
		return;
	}

	idRectangle rect = m_workspace->GetSelectionMgr().GetRect();
	m_workspace->WindowToWorkspace( rect );

	if( m_relative )
	{
		idRectangle& screen =
			rvGEWindowWrapper::GetWrapper( m_relative )->GetScreenRect();
		rect.x -= screen.x;
		rect.y -= screen.y;
	}

	switch( id )
	{
		case IDC_GUIED_ITEMRECTX:
			if( newValue != ( int )rect.x )
			{
				m_workspace->AddModifierMove( "Transform Move", newValue - rect.x, 0, false );
			}
			break;
		case IDC_GUIED_ITEMRECTY:
			if( newValue != ( int )rect.y )
			{
				m_workspace->AddModifierMove( "Transform Move", 0, newValue - rect.y, false );
			}
			break;
		case IDC_GUIED_ITEMRECTW:
			if( newValue != ( int )rect.w )
			{
				m_workspace->AddModifierSize( "Transform Size", 0, 0, newValue - rect.w, 0, false );
			}
			break;
		case IDC_GUIED_ITEMRECTH:
			if( newValue != ( int )rect.h )
			{
				m_workspace->AddModifierSize( "Transform Size", 0, 0, 0, newValue - rect.h, false );
			}
			break;
	}
}