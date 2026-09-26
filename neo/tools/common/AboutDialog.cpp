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

#include "../../renderer/RenderCommon.h"

#include "AboutDialog.h"


static const int kBodyWidth = 480;   // width of the details text controls
static const int kIconSize = 32;    // size requested from wxArtProvider
static const int kOuterPadding = 12;    // padding around panels and rows

static const wxColour kTopBg( 0xFF, 0xFF, 0xFF );
static const wxColour kTitleCol( 0x00, 0x00, 0xC0 );     // standard alert blue

/*
================
AboutDialog::AboutDialog
================
*/
AboutDialog::AboutDialog( wxWindow* parent, const wxString& productName, const wxString& buildInfo, const wxString& copyright, bool showRenderInfo )
	: wxDialog( parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize )
	, m_outerSizer( NULL )
	, m_topPanel( NULL )
	, m_bottomPanel( NULL )
	, m_iconCtrl( NULL )
	, m_bodySizer( NULL )
	, m_detailsPanel( NULL )
	, m_detailsSizer( NULL )
	, m_detailsToggle( NULL )
{
	// Window title.
	wxString titleSource = productName;
	if( titleSource.IsEmpty() && parent )
	{
		titleSource = parent->GetLabel();
	}
	if( titleSource.IsEmpty() )
	{
		titleSource = "Application";
	}

	SetTitle( wxString::Format( "About %s", titleSource ) );

	// The dialog itself starts white; the bottom panel overrides this below.
	SetBackgroundColour( kTopBg );

	m_outerSizer = new wxBoxSizer( wxVERTICAL );

	m_topPanel = new wxPanel( this );
	m_topPanel->SetBackgroundColour( kTopBg );

	wxBoxSizer* topRow = new wxBoxSizer( wxHORIZONTAL );

	// Default information icon.
	wxBitmap defaultIcon = wxArtProvider::GetBitmap( wxART_INFORMATION, wxART_MESSAGE_BOX, wxSize( kIconSize, kIconSize ) );
	if( defaultIcon.IsOk() )
	{
		m_iconCtrl = new wxStaticBitmap( m_topPanel, wxID_ANY, defaultIcon );
		topRow->Add( m_iconCtrl, 0, wxALIGN_TOP | wxALL, kOuterPadding );
	}

	m_bodySizer = new wxBoxSizer( wxVERTICAL );

	if( !productName.IsEmpty() )
	{
		wxStaticText* titleText = new wxStaticText( m_topPanel, wxID_ANY, productName );

		wxFont titleFont = titleText->GetFont();
		titleFont.SetPointSize( titleFont.GetPointSize() + 2 );
		titleFont.SetWeight( wxFONTWEIGHT_BOLD );
		titleText->SetFont( titleFont );
		titleText->SetForegroundColour( kTitleCol );

		m_bodySizer->Add( titleText, 0, wxBOTTOM, 8 );
	}

	if( !buildInfo.IsEmpty() )
	{
		wxStaticText* buildText = new wxStaticText( m_topPanel, wxID_ANY, buildInfo );
		m_bodySizer->Add( buildText, 0, wxBOTTOM, 4 );
	}

	if( !copyright.IsEmpty() )
	{
		wxStaticText* copyrightText = new wxStaticText( m_topPanel, wxID_ANY, copyright );
		m_bodySizer->Add( copyrightText, 0, wxBOTTOM, 4 );
	}

	m_detailsPanel = new wxPanel( m_topPanel );
	m_detailsPanel->SetBackgroundColour( kTopBg );
	m_detailsSizer = new wxBoxSizer( wxVERTICAL );
	m_detailsPanel->SetSizer( m_detailsSizer );
	m_detailsPanel->Hide();

	m_bodySizer->Add( m_detailsPanel, 0, wxEXPAND | wxTOP, 8 );

	topRow->Add( m_bodySizer, 1, wxEXPAND | wxRIGHT | wxTOP | wxBOTTOM, kOuterPadding );

	m_topPanel->SetSizer( topRow );
	m_outerSizer->Add( m_topPanel, 1, wxEXPAND );

	m_bottomPanel = new wxPanel( this );
	m_bottomPanel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

	wxBoxSizer* bottomSizer = new wxBoxSizer( wxHORIZONTAL );

	m_detailsToggle = new wxToggleButton( m_bottomPanel, wxID_ANY, "More details" );
	m_detailsToggle->SetBitmap( wxArtProvider::GetBitmap( wxART_GO_DOWN, wxART_BUTTON, wxSize( 16, 16 ) ) );
	m_detailsToggle->SetBitmapMargins( 6, 0 );
	m_detailsToggle->Bind( wxEVT_TOGGLEBUTTON, &AboutDialog::OnToggleDetails, this );
	m_detailsToggle->Hide();

	bottomSizer->Add( m_detailsToggle, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxTOP | wxBOTTOM, 8 );

	bottomSizer->AddStretchSpacer( 1 );

	wxButton* okBtn = new wxButton( m_bottomPanel, wxID_OK, "OK" );
	bottomSizer->Add( okBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxTOP | wxBOTTOM, 8 );

	m_bottomPanel->SetSizer( bottomSizer );
	m_outerSizer->Add( m_bottomPanel, 0, wxEXPAND );

	if( showRenderInfo )
	{
		AddRenderInfoSection();
	}

	SetSizerAndFit( m_outerSizer );
	CentreOnParent();
}

/*
================
AboutDialog::~AboutDialog
================
*/
AboutDialog::~AboutDialog()
{
}

/*
================
AboutDialog::SetIcon
================
*/
void AboutDialog::SetIcon( const wxBitmap& bmp )
{
	wxBitmap use = bmp;
	if( !use.IsOk() )
	{
		use = wxArtProvider::GetBitmap( wxART_INFORMATION, wxART_MESSAGE_BOX, wxSize( kIconSize, kIconSize ) );
	}

	if( !use.IsOk() )
	{
		return;
	}

	if( m_iconCtrl )
	{
		m_iconCtrl->SetBitmap( use );
	}
	else
	{
		m_iconCtrl = new wxStaticBitmap( m_topPanel, wxID_ANY, use );
		wxSizer* topRow = m_topPanel->GetSizer();
		if( topRow )
		{
			topRow->Insert( 0, m_iconCtrl, 0, wxALIGN_TOP | wxALL, kOuterPadding );
		}
	}

	m_topPanel->Layout();
	FitToContent();
}

/*
================
AboutDialog::SetIcon
================
*/
void AboutDialog::SetIcon( const wxIcon& icon )
{
	if( icon.IsOk() )
	{
		SetIcon( wxBitmap( icon ) );
	}
	else
	{
		SetIcon( wxNullBitmap );
	}
}

/*
================
AboutDialog::AddInfoSection
================
*/
void AboutDialog::AddInfoSection( const wxString& sectionTitle, const wxString& text )
{
	if( text.IsEmpty() )
	{
		return;
	}

	// Section header.
	wxStaticText* sectionLabel = new wxStaticText( m_detailsPanel, wxID_ANY, sectionTitle );

	wxFont labelFont = sectionLabel->GetFont();
	labelFont.SetWeight( wxFONTWEIGHT_BOLD );
	sectionLabel->SetFont( labelFont );

	wxTextCtrl* info = new wxTextCtrl( m_detailsPanel, wxID_ANY, text, wxDefaultPosition, wxSize( kBodyWidth, 200 ), wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP | wxBORDER_SIMPLE );

	wxFont mono = info->GetFont();
	mono.SetFamily( wxFONTFAMILY_TELETYPE );
	info->SetFont( mono );

	m_detailsSizer->Add( sectionLabel, 0, wxTOP, 8 );
	m_detailsSizer->Add( info, 0, wxEXPAND | wxTOP, 4 );

	if( m_detailsToggle && !m_detailsToggle->IsShown() )
	{
		m_detailsToggle->Show( true );
	}

	FitToContent();
}

/*
================
AboutDialog::AddRenderInfoSection
================
*/
void AboutDialog::AddRenderInfoSection()
{
	AddInfoSection( "Render Information", BuildRenderInfoText() );
}

/*
================
AboutDialog::BuildRenderInfoText
================
*/
wxString AboutDialog::BuildRenderInfoText()
{
	wxString info;

	auto row = []( const char* label, const wxString & value ) -> wxString
	{
		return wxString::Format( "%-20s %s\n", label, value.c_str() );
	};

	const float glVersion   = ( float )atof( glConfig.version_string );
	const float glslVersion = ( float )atof( glConfig.shading_language_string );

	info << row( "OpenGL Version:",  wxString::Format( "%.1f", glVersion ) );
	info << row( "OpenGL Vendor:",   glConfig.vendor_string );
	info << row( "OpenGL Renderer:", glConfig.renderer_string );
	info << row( "OpenGL GLSL:",     wxString::Format( "%.1f", glslVersion ) );
	info << "\n";

	GLint numExtensions = 0;
	glGetIntegerv( GL_NUM_EXTENSIONS, &numExtensions );

	info << wxString::Format( "OpenGL Extensions (%d):\n", ( int )numExtensions );

	for( GLint i = 0; i < numExtensions; i++ )
	{
		info << "  " << ( const char* )glGetStringi( GL_EXTENSIONS, i );
		if( i < numExtensions - 1 )
		{
			info << "\n";
		}
	}

	return info;
}

/*
================
AboutDialog::OnToggleDetails
================
*/
void AboutDialog::OnToggleDetails( wxCommandEvent& event )
{
	if( event.IsChecked() )
	{
		m_detailsToggle->SetLabel( "Fewer details" );
		m_detailsToggle->SetBitmap( wxArtProvider::GetBitmap( wxART_GO_UP, wxART_BUTTON, wxSize( 16, 16 ) ) );
		m_detailsPanel->Show( true );
	}
	else
	{
		m_detailsToggle->SetLabel( "More details" );
		m_detailsToggle->SetBitmap( wxArtProvider::GetBitmap( wxART_GO_DOWN, wxART_BUTTON, wxSize( 16, 16 ) ) );
		m_detailsPanel->Show( false );
	}

	FitToContent();
}

/*
================
AboutDialog::FitToContent
================
*/
void AboutDialog::FitToContent()
{
	const wxPoint pos = GetPosition();

	Layout();
	SetSizerAndFit( m_outerSizer );

	SetPosition( pos );
}