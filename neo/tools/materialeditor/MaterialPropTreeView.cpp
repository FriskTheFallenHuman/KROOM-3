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

#include "MaterialPropTreeView.h"

/*
================
MaterialPropTreeView::MaterialPropTreeView
================
*/
MaterialPropTreeView::MaterialPropTreeView( wxWindow* parent )
	: wxPanel( parent )
	, m_grid( NULL )
	, currentPropDefs( NULL )
	, currentListType( -1 )
	, currentStage( -1 )
	, internalChange( false )
{
	registry.Init( "materialpropview.cfg", "PropTreeView" );

	m_grid = new wxPropertyGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_DEFAULT_STYLE | wxPG_SPLITTER_AUTO_CENTER | wxPG_BOLD_MODIFIED );
	m_grid->SetExtraStyle( wxPG_EX_HELP_AS_TOOLTIPS );

	m_grid->Bind( wxEVT_PG_CHANGED, &MaterialPropTreeView::OnPropertyGridChanged, this );
	m_grid->Bind( wxEVT_PG_ITEM_COLLAPSED, &MaterialPropTreeView::OnPropertyGridItemCollapsed, this );
	m_grid->Bind( wxEVT_PG_ITEM_EXPANDED, &MaterialPropTreeView::OnPropertyGridItemExpanded, this );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_grid, 1, wxEXPAND );
	SetSizer( sizer );
}

/*
================
MaterialPropTreeView::~MaterialPropTreeView
================
*/
MaterialPropTreeView::~MaterialPropTreeView()
{
}

/*
================
MaterialPropTreeView::LoadSettings
================
*/
void MaterialPropTreeView::LoadSettings()
{
	registry.Load();
}

/*
================
MaterialPropTreeView::SaveSettings
================
*/
void MaterialPropTreeView::SaveSettings()
{
	registry.Save();
}

/*
================
MaterialPropTreeView::SetColumn
================
*/
void MaterialPropTreeView::SetColumn( int width )
{
	if( m_grid && width > 0 )
	{
		m_grid->SetSplitterPosition( width );
	}
}

/*
================
MaterialPropTreeView::GetColumn
================
*/
int MaterialPropTreeView::GetColumn() const
{
	if( m_grid )
	{
		return m_grid->GetSplitterPosition();
	}
	return 0;
}

/*
================
MaterialPropTreeView::SetPropertyListType
================
*/
void MaterialPropTreeView::SetPropertyListType( int listType, int stageNum )
{
	currentListType = listType;
	currentStage    = stageNum;

	// Clear out the old grid.
	m_grid->Clear();
	m_propertyMap.clear();

	MaterialDefList* propList = MaterialDefManager::GetMaterialDefs( listType );
	currentPropDefs = propList;

	if( !propList )
	{
		return;
	}

	wxPGProperty* currentGroup = NULL;

	for( int i = 0; i < propList->Num(); i++ )
	{
		MaterialDef* def = ( *propList )[i];

		wxString label( def->displayName.c_str() );
		wxString name( def->dictName.c_str() );

		wxPGProperty* prop = NULL;

		switch( def->type )
		{
			case MaterialDef::MATERIAL_DEF_TYPE_GROUP:
			{
				wxPropertyCategory* cat = new wxPropertyCategory( label, name );
				cat->SetClientData( def );

				bool expand = registry.GetBool( va( "Expand%d%s", currentListType, def->displayName.c_str() ), true );
				cat->SetExpanded( expand );

				m_grid->Append( cat );
				currentGroup = cat;
				continue;
			}

			case MaterialDef::MATERIAL_DEF_TYPE_BOOL:
			{
				prop = new wxBoolProperty( label, name, false );
				prop->SetAttribute( wxPG_BOOL_USE_CHECKBOX, true );
			}
			break;

			case MaterialDef::MATERIAL_DEF_TYPE_STRING:
			{
				prop = new wxStringProperty( label, name, wxEmptyString );
			}
			break;

			case MaterialDef::MATERIAL_DEF_TYPE_FLOAT:
			{
				prop = new wxFloatProperty( label, name, 0.0f );
			}
			break;

			case MaterialDef::MATERIAL_DEF_TYPE_INT:
			{
				prop = new wxIntProperty( label, name, 0L );
			}
			break;

			default:
				continue;
		}

		if( prop )
		{
			prop->SetHelpString( wxString( def->displayInfo.c_str() ) );
			prop->SetClientData( def );

			if( currentGroup )
			{
				m_grid->AppendIn( currentGroup, prop );
			}
			else
			{
				m_grid->Append( prop );
			}

			m_propertyMap[ def->dictName.c_str() ] = prop;
		}
	}

	RefreshProperties();
}

/*
================
MaterialPropTreeView::RefreshProperties
================
*/
void MaterialPropTreeView::RefreshProperties()
{
	MaterialDefList* propList = MaterialDefManager::GetMaterialDefs( currentListType );
	if( !propList )
	{
		return;
	}

	MaterialDoc* materialDoc = materialDocManager->GetCurrentMaterialDoc();
	if( !materialDoc )
	{
		return;
	}

	internalChange = true;

	for( int i = 0; i < propList->Num(); i++ )
	{
		MaterialDef* def = ( *propList )[i];

		std::map<std::string, wxPGProperty*>::iterator it = m_propertyMap.find( def->dictName.c_str() );
		if( it == m_propertyMap.end() )
		{
			continue;
		}

		wxPGProperty* prop = it->second;

		switch( def->type )
		{
			case MaterialDef::MATERIAL_DEF_TYPE_BOOL:
			{
				bool val = materialDoc->GetAttributeBool( currentStage, def->dictName );
				m_grid->SetPropertyValue( prop, val );
			}
			break;

			case MaterialDef::MATERIAL_DEF_TYPE_STRING:
			{
				idStr val = materialDoc->GetAttribute( currentStage, def->dictName );
				m_grid->SetPropertyValue( prop, wxString( val.c_str() ) );
			}
			break;

			case MaterialDef::MATERIAL_DEF_TYPE_FLOAT:
			{
				float val = materialDoc->GetAttributeFloat( currentStage, def->dictName );
				m_grid->SetPropertyValue( prop, wxVariant( val ) );
			}
			break;

			case MaterialDef::MATERIAL_DEF_TYPE_INT:
			{
				int val = materialDoc->GetAttributeInt( currentStage, def->dictName );
				m_grid->SetPropertyValue( prop, wxVariant( ( long )val ) );
			}
			break;

			default:
				break;
		}
	}

	internalChange = false;
}

/*
================
MaterialPropTreeView::OnPropertyGridChanged
================
*/
void MaterialPropTreeView::OnPropertyGridChanged( wxPropertyGridEvent& event )
{
	if( internalChange )
	{
		return;
	}

	wxPGProperty* prop = event.GetProperty();
	if( !prop )
	{
		return;
	}

	MaterialDef* propItem = static_cast<MaterialDef*>( prop->GetClientData() );
	if( !propItem )
	{
		return;
	}

	MaterialDoc* materialDoc = materialDocManager->GetCurrentMaterialDoc();
	if( !materialDoc )
	{
		return;
	}

	internalChange = true;

	switch( propItem->type )
	{
		case MaterialDef::MATERIAL_DEF_TYPE_BOOL:
		{
			bool val = event.GetPropertyValue().GetBool();
			materialDoc->SetAttributeBool( currentStage, propItem->dictName, val );
		}
		break;

		case MaterialDef::MATERIAL_DEF_TYPE_STRING:
		{
			wxString wxs = event.GetPropertyValue().GetString();
			idStr val( wxs.ToStdString().c_str() );
			materialDoc->SetAttribute( currentStage, propItem->dictName, val );
		}
		break;

		case MaterialDef::MATERIAL_DEF_TYPE_FLOAT:
		{
			float val = ( float )event.GetPropertyValue().GetDouble();
			materialDoc->SetAttributeFloat( currentStage, propItem->dictName, val );
		}
		break;

		case MaterialDef::MATERIAL_DEF_TYPE_INT:
		{
			int val = ( int )event.GetPropertyValue().GetLong();
			materialDoc->SetAttributeInt( currentStage, propItem->dictName, val );
		}
		break;

		default:
			break;
	}

	internalChange = false;
}

/*
================
MaterialPropTreeView::OnPropertyGridItemCollapsed
================
*/
void MaterialPropTreeView::OnPropertyGridItemCollapsed( wxPropertyGridEvent& event )
{
	wxPGProperty* prop = event.GetProperty();
	if( !prop )
	{
		return;
	}

	MaterialDef* def = static_cast<MaterialDef*>( prop->GetClientData() );
	if( !def )
	{
		return;
	}

	registry.SetBool( va( "Expand%d%s", currentListType, def->displayName.c_str() ), false );
	registry.Save();
}

/*
================
MaterialPropTreeView::OnPropertyGridItemExpanded
================
*/
void MaterialPropTreeView::OnPropertyGridItemExpanded( wxPropertyGridEvent& event )
{
	wxPGProperty* prop = event.GetProperty();
	if( !prop )
	{
		return;
	}

	MaterialDef* def = static_cast<MaterialDef*>( prop->GetClientData() );
	if( !def )
	{
		return;
	}

	registry.SetBool( va( "Expand%d%s", currentListType, def->displayName.c_str() ), true );
	registry.Save();
}

/*
================
MaterialPropTreeView::MV_OnMaterialChange
================
*/
void MaterialPropTreeView::MV_OnMaterialChange( MaterialDoc* pMaterial )
{
	if( !pMaterial )
	{
		return;
	}

	MaterialDoc* current = materialDocManager->GetCurrentMaterialDoc();
	if( !internalChange && current && !pMaterial->name.Icmp( current->name ) )
	{
		RefreshProperties();
	}
}