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

#include "MaterialEditView.h"
#include "StageView.h"
#include "MaterialPropTreeView.h"
#include "MaterialDocManager.h"

/*
================
MaterialEditView::MaterialEditView
================
*/
MaterialEditView::MaterialEditView( wxWindow* parent )
	: wxPanel( parent )
	, m_nameEdit( NULL )
	, m_tabs( NULL )
	, m_propertiesPage( NULL )
	, m_textPage( NULL )
	, m_editSplitter( NULL )
	, m_stageView( NULL )
	, m_materialPropertyView( NULL )
	, m_textView( NULL )
	, m_sourceInit( false )
	, m_sourceChanged( false )
{
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	// Name row
	m_nameEdit = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxBORDER_SIMPLE );
	sizer->Add( m_nameEdit, 0, wxEXPAND | wxALL, 1 );

	m_tabs = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_BOTTOM );

	// Properties pageS
	m_propertiesPage = new wxPanel( m_tabs );
	m_editSplitter = new wxSplitterWindow( m_propertiesPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3DSASH | wxSP_BORDER );
	m_editSplitter->SetMinimumPaneSize( 80 );

	m_stageView = new StageView( m_editSplitter );
	m_materialPropertyView = new MaterialPropTreeView( m_editSplitter );
	m_editSplitter->SplitVertically( m_stageView, m_materialPropertyView );

	wxBoxSizer* propSizer = new wxBoxSizer( wxVERTICAL );
	propSizer->Add( m_editSplitter, 1, wxEXPAND );
	m_propertiesPage->SetSizer( propSizer );

	// Text page
	m_textPage = new wxPanel( m_tabs );
	m_textView = new wxStyledTextCtrl( m_textPage, wxID_ANY );

	SetupEditorStyles();
	LoadKeywordsFromFile( "editors/defs/material.def" );

	m_textView->SetReadOnly( true );

	wxBoxSizer* textSizer = new wxBoxSizer( wxVERTICAL );
	textSizer->Add( m_textView, 1, wxEXPAND );
	m_textPage->SetSizer( textSizer );

	m_tabs->AddPage( m_propertiesPage, "Properties", true );
	m_tabs->AddPage( m_textPage, "Text", false );

	sizer->Add( m_tabs, 1, wxEXPAND );

	SetSizer( sizer );

	// Events.
	m_tabs->Bind( wxEVT_NOTEBOOK_PAGE_CHANGED, &MaterialEditView::OnTabChanged, this );
	m_textView->Bind( wxEVT_STC_CHANGE, &MaterialEditView::OnTextChanged, this );
}

/*
================
MaterialEditView::~MaterialEditView
================
*/
MaterialEditView::~MaterialEditView()
{
}

/*
================
MaterialEditView::SetupEditorStyles
================
*/
void MaterialEditView::SetupEditorStyles()
{
	m_textView->SetLexer( wxSTC_LEX_CPP );
	m_textView->SetCodePage( wxSTC_CP_UTF8 );
	m_textView->SetMarginType( 0, wxSTC_MARGIN_NUMBER );
	m_textView->SetMarginWidth( 0, 40 );

	wxFont font( 10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );
	m_textView->StyleSetFont( wxSTC_STYLE_DEFAULT, font );
	m_textView->StyleClearAll();

	m_textView->StyleSetForeground( wxSTC_C_COMMENT, wxColour( 0x00, 0x80, 0x00 ) );
	m_textView->StyleSetForeground( wxSTC_C_COMMENTLINE, wxColour( 0x00, 0x80, 0x00 ) );
	m_textView->StyleSetForeground( wxSTC_C_STRING, wxColour( 0xC0, 0x00, 0x00 ) );
	m_textView->StyleSetForeground( wxSTC_C_CHARACTER, wxColour( 0xC0, 0x00, 0x00 ) );
	m_textView->StyleSetForeground( wxSTC_C_WORD, wxColour( 0x00, 0x00, 0xC0 ) );
	m_textView->StyleSetBold( wxSTC_C_WORD, true );
	m_textView->StyleSetForeground( wxSTC_C_WORD2, wxColour( 0x80, 0x00, 0x80 ) );
	m_textView->StyleSetForeground( wxSTC_C_NUMBER, wxColour( 0x80, 0x40, 0x00 ) );
	m_textView->StyleSetForeground( wxSTC_C_OPERATOR, wxColour( 0x00, 0x00, 0x00 ) );

	m_textView->SetTabWidth( 4 );
	m_textView->SetUseTabs( false );
}

/*
================
MaterialEditView::LoadKeywordsFromFile
================
*/
void MaterialEditView::LoadKeywordsFromFile( const char* filename )
{
	m_keywords.LoadFromFile( filename );
	m_textView->SetKeyWords( 0, m_keywords.GetConcatenated() );

	m_textView->Bind( wxEVT_STC_DWELLSTART, &MaterialEditView::OnKeywordDwellStart, this );
	m_textView->Bind( wxEVT_STC_DWELLEND, &MaterialEditView::OnKeywordDwellEnd, this );
}

/*
================
MaterialEditView::OnKeywordDwellStart
================
*/
void MaterialEditView::OnKeywordDwellStart( wxStyledTextEvent& event )
{
	const int pos = event.GetPosition();
	if( pos < 0 )
	{
		return;
	}

	const int start = m_textView->WordStartPosition( pos, true );
	const int end = m_textView->WordEndPosition( pos, true );
	if( start == end )
	{
		return;
	}

	const wxString word = m_textView->GetTextRange( start, end );
	const std::string wordStr = word.ToStdString();

	const SyntaxKeywords::Entry* kw = m_keywords.Find( wordStr.c_str() );
	if( kw && kw->description.Length() > 0 )
	{
		m_textView->CallTipShow( start, wxString::FromUTF8( kw->description.c_str() ) );
	}
}

/*
================
MaterialEditView::OnKeywordDwellEnd
================
*/
void MaterialEditView::OnKeywordDwellEnd( wxStyledTextEvent& /*event*/ )
{
	m_textView->CallTipCancel();
}

/*
================
MaterialEditView::GetSourceText
================
*/
idStr MaterialEditView::GetSourceText()
{
	wxString wxText = m_textView->GetText();

	idStr text = wxText.ToUTF8().data();
	text.Replace( "\r\n", "\n" );
	text.StripTrailing( '\n' );

	return text;
}

/*
================
MaterialEditView::GetMaterialSource
================
*/
void MaterialEditView::GetMaterialSource()
{
	m_sourceInit = true;
	m_textView->SetReadOnly( false );
	m_textView->SetText( wxEmptyString );
	m_sourceInit = false;

	if( !materialDocManager )
	{
		return;
	}

	MaterialDoc* material = materialDocManager->GetCurrentMaterialDoc();
	if( !material )
	{
		return;
	}

	idStr text = material->GetEditSourceText();
	text.Replace( "\r\n", "\n" );
	text.Replace( "\r", "\n" );

	m_sourceInit = true;
	m_textView->SetText( wxString::FromUTF8( text.c_str() ) );
	m_sourceInit = false;
}

/*
================
MaterialEditView::ApplyMaterialSource
================
*/
void MaterialEditView::ApplyMaterialSource()
{
	if( !m_sourceChanged )
	{
		return;
	}

	if( !materialDocManager )
	{
		return;
	}

	MaterialDoc* material = materialDocManager->CreateMaterialDoc( m_currentMaterialName );
	if( material )
	{
		idStr text = GetSourceText();
		material->ApplySourceModify( text );
	}

	m_sourceChanged = false;
}

/*
================
MaterialEditView::OnTabChanged
================
*/
void MaterialEditView::OnTabChanged( wxBookCtrlEvent& event )
{
	const int sel = m_tabs->GetSelection();

	if( sel == 0 )
	{
		ApplyMaterialSource();

		if( m_stageView )
		{
			m_stageView->RefreshStageList();
		}
	}
	else if( sel == 1 )
	{
		// Switching to Text: reload the source and enable editing.
		GetMaterialSource();
		m_textView->SetReadOnly( false );
	}

	event.Skip();
}

/*
================
MaterialEditView::OnTextChanged
================
*/
void MaterialEditView::OnTextChanged( wxStyledTextEvent& /*event*/ )
{
	if( m_sourceInit )
	{
		return;
	}

	if( !materialDocManager )
	{
		return;
	}

	MaterialDoc* material = materialDocManager->GetCurrentMaterialDoc();
	if( material && !material->IsSourceModified() )
	{
		m_sourceChanged = true;
		material->SourceModify( this );
	}
}

/*
================
MaterialEditView::MV_OnMaterialSelectionChange
================
*/
void MaterialEditView::MV_OnMaterialSelectionChange( MaterialDoc* pMaterial )
{
	// Apply any pending edits before changing material.
	ApplyMaterialSource();

	if( pMaterial )
	{
		m_nameEdit->ChangeValue( pMaterial->name.c_str() );

		if( m_tabs->GetSelection() == 1 )
		{
			GetMaterialSource();
		}

		m_textView->SetReadOnly( m_tabs->GetSelection() != 1 );

		m_currentMaterialName = pMaterial->name;
	}
	else
	{
		m_nameEdit->ChangeValue( "" );

		GetMaterialSource();
		m_textView->SetReadOnly( true );

		m_currentMaterialName = "";
	}
}

/*
================
MaterialEditView::MV_OnMaterialNameChanged
================
*/
void MaterialEditView::MV_OnMaterialNameChanged( MaterialDoc* pMaterial, const char* oldName )
{
	if( m_currentMaterialName.Icmp( oldName ) == 0 && pMaterial )
	{
		m_currentMaterialName = pMaterial->name;
		m_nameEdit->ChangeValue( pMaterial->name.c_str() );
	}
}