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

#include "MaterialPreviewPropView.h"
#include "MaterialPreviewView.h"

#include <wx/filedlg.h>

/*
================
MaterialPreviewPropView::MaterialPreviewPropView
================
*/
MaterialPreviewPropView::MaterialPreviewPropView( wxWindow* parent )
	: wxPanel( parent )
	, m_grid( NULL )
	, m_addLightBtn( NULL )
	, m_removeLightBtn( NULL )
	, m_browseModelBtn( NULL )
	, m_preview( NULL )
	, m_numLights( 0 )
	, m_columnWidth( 120 )
{
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	// Toolbars
	{
		wxBoxSizer* bar = new wxBoxSizer( wxHORIZONTAL );
		m_addLightBtn = new wxButton( this, wxID_ANY, "Add Light" );
		m_removeLightBtn = new wxButton( this, wxID_ANY, "Remove Light" );
		m_browseModelBtn = new wxButton( this, wxID_ANY, "Browse Model..." );

		bar->Add( m_addLightBtn, 0, wxALL, 2 );
		bar->Add( m_removeLightBtn, 0, wxALL, 2 );
		bar->Add( m_browseModelBtn, 0, wxALL, 2 );

		sizer->Add( bar, 0, wxEXPAND );
	}

	// Property grid
	m_grid = new wxPropertyGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_DEFAULT_STYLE | wxPG_SPLITTER_AUTO_CENTER );
	m_grid->SetExtraStyle( wxPG_EX_HELP_AS_TOOLTIPS );

	m_grid->Bind( wxEVT_PG_CHANGED, &MaterialPreviewPropView::OnPropertyGridChanged, this );
	m_grid->Bind( wxEVT_PG_RIGHT_CLICK, &MaterialPreviewPropView::OnPropertyGridRightClick, this );

	sizer->Add( m_grid, 1, wxEXPAND );

	SetSizer( sizer );

	m_addLightBtn->Bind( wxEVT_BUTTON, &MaterialPreviewPropView::OnAddLight, this );
	m_removeLightBtn->Bind( wxEVT_BUTTON, &MaterialPreviewPropView::OnRemoveLight, this );
	m_browseModelBtn->Bind( wxEVT_BUTTON, &MaterialPreviewPropView::OnBrowseModel, this );
}

/*
================
MaterialPreviewPropView::~MaterialPreviewPropView
================
*/
MaterialPreviewPropView::~MaterialPreviewPropView()
{
}

/*
================
MaterialPreviewPropView::RegisterPreviewView
================
*/
void MaterialPreviewPropView::RegisterPreviewView( MaterialPreviewView* view )
{
	m_preview = view;
}

/*
================
MaterialPreviewPropView::SetColumn
================
*/
void MaterialPreviewPropView::SetColumn( int width )
{
	if( width > 0 )
	{
		m_columnWidth = width;
		if( m_grid )
		{
			m_grid->SetSplitterPosition( width );
		}
	}
}

/*
================
MaterialPreviewPropView::GetColumn
================
*/
int MaterialPreviewPropView::GetColumn() const
{
	if( m_grid )
	{
		return m_grid->GetSplitterPosition();
	}
	return m_columnWidth;
}

/*
================
MaterialPreviewPropView::BuildShaderList
================
*/
void MaterialPreviewPropView::BuildShaderList()
{
	m_lightShaderChoices.Clear();
	m_lightShaderNames.clear();

	const int count = declManager->GetNumDecls( DECL_MATERIAL );
	for( int i = 0; i < count; i++ )
	{
		const idMaterial* mat = declManager->MaterialByIndex( i, false );
		idStr name = mat->GetName();
		name.ToLower();

		if( name.Left( 7 ) == "lights/" || name.Left( 5 ) == "fogs/" )
		{
			m_lightShaderChoices.Add( wxString( name.c_str() ) );
			m_lightShaderNames.push_back( name );
		}
	}
}

/*
================
MaterialPreviewPropView::AddLightCategory
================
*/
void MaterialPreviewPropView::AddLightCategory( int lightId, const idStr& shaderName, const idVec3& color, float radius, bool allowMove )
{
	wxString name  = wxString::Format( "light_%d", lightId );
	wxString label = wxString::Format( "Light #%d", lightId + 1 );

	wxPropertyCategory* cat = new wxPropertyCategory( label, name );
	m_grid->Append( cat );

	// Shader enum
	int shaderIndex = 0;
	{
		for( size_t i = 0; i < m_lightShaderNames.size(); i++ )
		{
			if( m_lightShaderNames[i].Icmp( shaderName ) == 0 )
			{
				shaderIndex = ( int )i;
				break;
			}
		}
	}

	wxArrayInt shaderValues;
	for( size_t i = 0; i < m_lightShaderChoices.GetCount(); i++ )
	{
		shaderValues.Add( ( int )i );
	}

	wxEnumProperty* shaderProp = new wxEnumProperty( "Shader", wxString::Format( "light_%d_shader", lightId ), m_lightShaderChoices,
			shaderValues,
			shaderIndex );
	shaderProp->SetHelpString( "Set the light shader." );
	m_grid->AppendIn( cat, shaderProp );

	// Colour
	wxColourProperty* colorProp = new wxColourProperty( "Color", wxString::Format( "light_%d_color", lightId ),
			wxColour( ( unsigned char )( color.x * 255.f ),
					  ( unsigned char )( color.y * 255.f ),
					  ( unsigned char )( color.z * 255.f ) ) );
	colorProp->SetHelpString( "Color of the light." );
	m_grid->AppendIn( cat, colorProp );

	// Radius
	wxFloatProperty* radiusProp = new wxFloatProperty( "Radius", wxString::Format( "light_%d_radius", lightId ), radius );
	radiusProp->SetHelpString( "Radius of the light." );
	m_grid->AppendIn( cat, radiusProp );

	// Move Light
	wxBoolProperty* moveProp = new wxBoolProperty( "Move light", wxString::Format( "light_%d_move", lightId ), allowMove );
	moveProp->SetAttribute( wxPG_BOOL_USE_CHECKBOX, true );
	moveProp->SetHelpString( "When checked, allow the light to be moved with RMB drag." );
	m_grid->AppendIn( cat, moveProp );

	cat->SetExpanded( true );
}

/*
================
MaterialPreviewPropView::AddLightCategoryDefault
================
*/
void MaterialPreviewPropView::AddLightCategoryDefault( int lightId )
{
	AddLightCategory( lightId, "lights/defaultpointlight", idVec3( 1.f, 1.f, 1.f ), 300.f, true );
}

/*
================
MaterialPreviewPropView::RebuildLightCategories
================
*/
void MaterialPreviewPropView::RebuildLightCategories()
{
	if( !m_preview || !m_grid )
	{
		return;
	}

	const int liveCount = m_preview->GetLightCount();

	std::vector<LightValues> values;
	values.reserve( liveCount );
	for( int i = 0; i < liveCount; i++ )
	{
		LightValues v;
		if( !m_preview->GetLightInfo( i, v.shaderName, v.color, v.radius, v.allowMove ) )
		{
			// Should not happen
			v.shaderName = "lights/defaultpointlight";
			v.color.Set( 1.f, 1.f, 1.f );
			v.radius = 300.f;
			v.allowMove = true;
		}
		values.push_back( v );
	}

	// Delete every light category currently in the grid, regardless of what
	// m_numLights says.
	std::vector<wxPGProperty*> orphans;
	wxPGProperty* root = m_grid->GetRoot();
	const unsigned int childCount = root->GetChildCount();
	for( unsigned int i = 0; i < childCount; ++i )
	{
		wxPGProperty* child = root->Item( i );
		if( child && child->GetName().StartsWith( wxT( "light_" ) ) )
		{
			orphans.push_back( child );
		}
	}
	for( size_t i = 0; i < orphans.size(); ++i )
	{
		m_grid->DeleteProperty( orphans[i] );
	}

	// Rebuild from scratch with indices that match the drawable.
	m_numLights = liveCount;
	for( int i = 0; i < m_numLights; ++i )
	{
		AddLightCategory( i, values[i].shaderName, values[i].color, values[i].radius, values[i].allowMove );
	}
}

/*
================
MaterialPreviewPropView::InitializePropTree
================
*/
void MaterialPreviewPropView::InitializePropTree()
{
	m_grid->Clear();
	m_numLights = 0;

	BuildShaderList();

	// Preview Properties
	wxPropertyCategory* previewCat = new wxPropertyCategory( "Preview Properties", "previewRoot" );
	m_grid->Append( previewCat );

	// Model Type
	{
		wxArrayString modelChoices;
		modelChoices.Add( "Cube" );
		modelChoices.Add( "Box - 2:1" );
		modelChoices.Add( "Box - 4:1" );
		modelChoices.Add( "Box - 1:2" );
		modelChoices.Add( "Box - 1:4" );
		modelChoices.Add( "Cylinder - V" );
		modelChoices.Add( "Cylinder - H" );
		modelChoices.Add( "Sphere" );

		wxArrayInt modelValues;
		for( size_t i = 0; i < modelChoices.GetCount(); i++ )
		{
			modelValues.Add( ( int )i );
		}

		wxEnumProperty* modelProp = new wxEnumProperty( "Model Type", "modelType", modelChoices, modelValues, 0 );
		modelProp->SetHelpString( "Select the type of model on which to preview the material." );
		m_grid->AppendIn( previewCat, modelProp );
	}

	// Custom Model
	{
		wxStringProperty* customProp = new wxStringProperty( "Custom Model", "customModel", wxEmptyString );
		customProp->SetHelpString( "Specify any model to display the current material." );
		m_grid->AppendIn( previewCat, customProp );
	}

	// Show Lights
	{
		wxBoolProperty* showProp = new wxBoolProperty( "Show Lights", "showLights", true );
		showProp->SetAttribute( wxPG_BOOL_USE_CHECKBOX, true );
		showProp->SetHelpString( "Show the light origin sphere and number in the preview." );
		m_grid->AppendIn( previewCat, showProp );
	}

	previewCat->SetExpanded( true );

	// Local Parms
	{
		wxPropertyCategory* parmCat = new wxPropertyCategory( "Local Parms", "localRoot" );
		m_grid->Append( parmCat );

		for( int i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ )
		{
			wxFloatProperty* p = new wxFloatProperty( wxString::Format( "parm%d", i ), wxString::Format( "parm%d", i ), i < 4 ? 1.f : 0.f );
			p->SetHelpString( "Set the local shaderparm for the model." );
			m_grid->AppendIn( parmCat, p );
		}

		parmCat->SetExpanded( false );
	}

	// Global Parms
	{
		wxPropertyCategory* parmCat = new wxPropertyCategory( "Global Parms", "globalRoot" );
		m_grid->Append( parmCat );

		for( int i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ )
		{
			wxFloatProperty* p = new wxFloatProperty( wxString::Format( "global%d", i ), wxString::Format( "global%d", i ), i < 4 ? 1.f : 0.f );
			p->SetHelpString( "Set the global shaderparm for the renderworld." );
			m_grid->AppendIn( parmCat, p );
		}

		parmCat->SetExpanded( false );
	}

	// Initial light
	if( m_preview )
	{
		m_preview->OnAddLight();
	}

	RebuildLightCategories();
}

/*
================
MaterialPreviewPropView::LightIdForProperty
================
*/
int MaterialPreviewPropView::LightIdForProperty( wxPGProperty* prop ) const
{
	while( prop )
	{
		wxString name = prop->GetName();
		if( name.StartsWith( "light_" ) )
		{
			// Strip leading "light_" and read the integer up to the next '_'.
			wxString rest = name.Mid( 6 );
			int underscore = rest.Find( '_' );
			wxString idStr = ( underscore == wxNOT_FOUND ) ? rest : rest.Left( underscore );
			long id = 0;
			if( idStr.ToLong( &id ) )
			{
				return ( int )id;
			}
			return -1;
		}
		prop = prop->GetParent();
	}
	return -1;
}

/*
================
MaterialPreviewPropView::OnPropertyGridChanged
================
*/
void MaterialPreviewPropView::OnPropertyGridChanged( wxPropertyGridEvent& event )
{
	if( !m_preview )
	{
		return;
	}

	wxPGProperty* prop = event.GetProperty();
	if( !prop )
	{
		return;
	}

	const wxString name = prop->GetName();

	// Top-level preview
	if( name == "modelType" )
	{
		m_preview->OnModelChange( ( int )event.GetPropertyValue().GetLong() );
		return;
	}

	if( name == "showLights" )
	{
		m_preview->OnShowLightsChange( event.GetPropertyValue().GetBool() );
		return;
	}

	if( name == "customModel" )
	{
		idStr val( event.GetPropertyValue().GetString().c_str() );
		m_preview->OnCustomModelChange( val );
		return;
	}

	// Shader parms
	if( name.StartsWith( "parm" ) )
	{
		long idx = 0;
		if( name.Mid( 4 ).ToLong( &idx ) )
		{
			m_preview->OnLocalParmChange( ( int )idx, ( float )event.GetPropertyValue().GetDouble() );
		}
		return;
	}

	if( name.StartsWith( "global" ) )
	{
		long idx = 0;
		if( name.Mid( 6 ).ToLong( &idx ) )
		{
			m_preview->OnGlobalParmChange( ( int )idx, ( float )event.GetPropertyValue().GetDouble() );
		}
		return;
	}

	// Per-light
	if( name.StartsWith( "light_" ) )
	{
		const int lightId = LightIdForProperty( prop );
		if( lightId < 0 )
		{
			return;
		}

		if( name.EndsWith( "_shader" ) )
		{
			wxString shaderName = event.GetPropertyValue().GetString();
			m_preview->OnLightShaderChange( lightId, idStr( shaderName.c_str() ) );
		}
		else if( name.EndsWith( "_color" ) )
		{
			wxColour c;
			c << event.GetPropertyValue();

			idVec3 color( c.Red() / 255.f, c.Green() / 255.f, c.Blue() / 255.f );
			m_preview->OnLightColorChange( lightId, color );
		}
		else if( name.EndsWith( "_radius" ) )
		{
			m_preview->OnLightRadiusChange( lightId, ( float )event.GetPropertyValue().GetDouble() );
		}
		else if( name.EndsWith( "_move" ) )
		{
			m_preview->OnLightAllowMoveChange( lightId, event.GetPropertyValue().GetBool() );
		}
	}
}

/*
================
MaterialPreviewPropView::OnPropertyGridRightClick
================
*/
void MaterialPreviewPropView::OnPropertyGridRightClick( wxPropertyGridEvent& event )
{
	if( !m_preview )
	{
		return;
	}

	wxPGProperty* prop = event.GetProperty();
	if( !prop )
	{
		return;
	}

	const int lightId = LightIdForProperty( prop );
	if( lightId < 0 )
	{
		return;  // not a light category
	}

	wxPoint pt = ::wxGetMousePosition();
	pt = m_grid->ScreenToClient( pt );

	wxMenu menu;

	if( lightId == 0 )
	{
		wxMenuItem* item = menu.Append( wxID_ANY, "Remove Light #1 (default - cannot remove)" );
		item->Enable( false );
		GetPopupMenuSelectionFromUser( menu, pt );
		return;
	}

	wxMenuItem* item = menu.Append( wxID_ANY, wxString::Format( "Remove Light #%d", lightId + 1 ) );

	const int selected = GetPopupMenuSelectionFromUser( menu, pt );
	if( selected == item->GetId() )
	{
		OnRemoveLightById( lightId );
	}
}

/*
================
MaterialPreviewPropView::OnAddLight
================
*/
void MaterialPreviewPropView::OnAddLight( wxCommandEvent& /*event*/ )
{
	if( !m_preview )
	{
		return;
	}

	m_preview->OnAddLight();
	RebuildLightCategories();
}

/*
================
MaterialPreviewPropView::OnRemoveLight
================
*/
void MaterialPreviewPropView::OnRemoveLightById( int lightId )
{
	if( !m_preview || lightId <= 0 )
	{
		return;   // light 0 is the default and never removable
	}

	if( lightId >= m_preview->GetLightCount() )
	{
		return;
	}

	m_preview->OnDeleteLight( lightId );
	RebuildLightCategories();
}

/*
================
MaterialPreviewPropView::OnRemoveLight
================
*/
void MaterialPreviewPropView::OnRemoveLight( wxCommandEvent& /*event*/ )
{
	if( !m_preview || m_numLights <= 0 )
	{
		return;
	}

	// Walk up from the currently selected property to find which light the
	// user has in focus.  If nothing is selected, default to the last light.
	int lightId = -1;

	wxPGProperty* sel = m_grid->GetSelectedProperty();
	if( sel )
	{
		lightId = LightIdForProperty( sel );
	}

	if( lightId < 0 )
	{
		lightId = m_numLights - 1;
	}

	if( lightId < 0 )
	{
		return;
	}

	m_preview->OnDeleteLight( lightId );

	m_numLights--;
	RebuildLightCategories();
}

/*
================
MaterialPreviewPropView::OnBrowseModel
================
*/
void MaterialPreviewPropView::OnBrowseModel( wxCommandEvent& /*event*/ )
{
	wxFileDialog dlg( this, "Select a model", "", "", "Models (*.ase;*.lwo;*.md5mesh)|*.ase;*.lwo;*.md5mesh|All files|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST );
	if( dlg.ShowModal() != wxID_OK )
	{
		return;
	}

	idStr relative = fileSystem->OSPathToRelativePath( dlg.GetPath().ToUTF8().data() );
	relative.BackSlashesToSlashes();
	relative.ToLower();

	wxPGProperty* prop = m_grid->GetProperty( "customModel" );
	if( prop )
	{
		m_grid->SetPropertyValue( prop, wxString( relative.c_str() ) );
	}
}