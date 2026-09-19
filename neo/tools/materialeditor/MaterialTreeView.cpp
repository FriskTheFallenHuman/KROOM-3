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

#include "MaterialTreeView.h"
#include "MEMainFrame.h"
#include "../common/ToolBarStrip.h"

#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/filedlg.h>

// Helper that is use to cast wxTreeItemData* to our item type.
class TreeItemData : public wxTreeItemData
{
public:
	explicit TreeItemData( int t ) : m_type( t ) {}
	int GetType() const
	{
		return m_type;
	}
private:
	int m_type;
};

/*
================
SplitPath

Split a string on a single '/' character, appending to `out`.
================
*/
static void SplitPath( const idStr& path, idList<idStr>& out )
{
	out.Clear();
	int start = 0;
	while( start <= path.Length() )
	{
		int slash = path.Find( '/', start );
		if( slash < 0 )
		{
			idStr tail = path.Mid( start, path.Length() - start );
			if( tail.Length() )
			{
				out.Append( tail );
			}
			break;
		}
		idStr piece = path.Mid( start, slash - start );
		if( piece.Length() )
		{
			out.Append( piece );
		}
		start = slash + 1;
	}
}

/*
================
MaterialTreeView::MaterialTreeView
================
*/
MaterialTreeView::MaterialTreeView( wxWindow* parent )
	: wxPanel( parent )
	, m_tree( NULL )
	, m_imageList( NULL )
	, treeWithFile( false )
	, internalChange( false )
{
	m_tree = new wxTreeCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
							 wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT |
							 wxTR_EDIT_LABELS | wxTR_SINGLE | wxTR_HIDE_ROOT );

	// Load our icon strip
	rvToolbarImageStrip strip;
	if( strip.Load( "editors/gfx/matedtree.bmp", 16, 16 ) )
	{
		m_imageList = strip.CreateImageList();
		m_tree->SetImageList( m_imageList );
	}

	m_tree->AddRoot( "root" );

	// Events
	m_tree->Bind( wxEVT_TREE_SEL_CHANGED, &MaterialTreeView::OnSelChanged, this );
	m_tree->Bind( wxEVT_TREE_BEGIN_LABEL_EDIT, &MaterialTreeView::OnBeginLabelEdit, this );
	m_tree->Bind( wxEVT_TREE_END_LABEL_EDIT, &MaterialTreeView::OnEndLabelEdit, this );
	m_tree->Bind( wxEVT_TREE_ITEM_RIGHT_CLICK, &MaterialTreeView::OnItemRightClick, this );
	m_tree->Bind( wxEVT_TREE_ITEM_ACTIVATED, &MaterialTreeView::OnItemActivated, this );
	m_tree->Bind( wxEVT_KEY_DOWN, &MaterialTreeView::OnKeyDown, this );

	// Binds
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnApplyMaterial();
	}, ID_POPUP_APPLYMATERIAL );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnApplyFile();
	}, ID_POPUP_APPLYFILE );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnApplyAll();
	}, ID_POPUP_APPLYALL );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnSaveMaterial();
	}, ID_POPUP_SAVEMATERIAL );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnSaveFile();
	}, ID_POPUP_SAVEFILE );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnSaveAll();
	}, ID_POPUP_SAVEALL );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnCut();
	}, ID_POPUP_CUT );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnCopy();
	}, ID_POPUP_COPY );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnPaste();
	}, ID_POPUP_PASTE );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnDeleteMaterial();
	}, ID_POPUP_DELETEMATERIAL );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnAddMaterial();
	}, ID_POPUP_ADDMATERIAL );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnAddFolder();
	}, ID_POPUP_ADDFOLDER );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnRenameMaterial();
	}, ID_POPUP_RENAMEMATERIAL );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnReloadFile();
	}, ID_POPUP_RELOADFILE );

	auto* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_tree, 1, wxEXPAND );
	SetSizer( sizer );
}

/*
================
MaterialTreeView::~MaterialTreeView
================
*/
MaterialTreeView::~MaterialTreeView()
{
	delete m_imageList;
	m_imageList = NULL;
}

/*
================
MaterialTreeView::GetQuicktreePath
================
*/
idStr MaterialTreeView::GetQuicktreePath( wxTreeItemId item ) const
{
	const wxTreeItemId root = m_tree->GetRootItem();

	idStr qt;
	wxTreeItemId cur = item;
	while( cur.IsOk() && cur != root )
	{
		qt = idStr( "/" ) + GetItemTextSafe( cur ).c_str() + qt;
		cur = m_tree->GetItemParent( cur );
	}
	return qt;
}

/*
================
MaterialTreeView::CleanLookupTrees
================
*/
void MaterialTreeView::CleanLookupTrees( wxTreeItemId item )
{
	idStr qt = GetQuicktreePath( item );
	quickTree.erase( qt.c_str() );

	int type = GetItemType( item );
	if( type == TYPE_FILE )
	{
		idStr file = GetMediaPath( item, TYPE_FILE );
		fileToTree.erase( file.c_str() );
	}
	else if( type == TYPE_MATERIAL )
	{
		idStr name = GetMediaPath( item, TYPE_MATERIAL );
		materialToTree.erase( name.c_str() );
	}

	wxTreeItemIdValue cookie;
	for( wxTreeItemId child = m_tree->GetFirstChild( item, cookie );
			child.IsOk();
			child = m_tree->GetNextChild( item, cookie ) )
	{
		CleanLookupTrees( child );
	}
}

/*
================
MaterialTreeView::BuildLookupTrees
================
*/
void MaterialTreeView::BuildLookupTrees( wxTreeItemId item )
{
	idStr qt = GetQuicktreePath( item );
	quickTree[ qt.c_str() ] = item;

	if( GetItemType( item ) == TYPE_FILE )
	{
		idStr file = GetMediaPath( item, TYPE_FILE );
		fileToTree[ file.c_str() ] = item;
	}

	wxTreeItemIdValue cookie;
	for( wxTreeItemId child = m_tree->GetFirstChild( item, cookie );
			child.IsOk();
			child = m_tree->GetNextChild( item, cookie ) )
	{
		if( GetItemType( child ) == TYPE_MATERIAL_FOLDER || GetItemType( child ) == TYPE_FOLDER )
		{
			BuildLookupTrees( child );
		}
	}
}

/*
================
MaterialTreeView::GetItemType
================
*/
int MaterialTreeView::GetItemType( wxTreeItemId item ) const
{
	if( !item.IsOk() )
	{
		return TYPE_ROOT;
	}
	wxTreeItemData* data = m_tree->GetItemData( item );
	if( !data )
	{
		return TYPE_ROOT;
	}
	return static_cast<TreeItemData*>( data )->GetType();
}

/*
================
MaterialTreeView::GetImageForType
================
*/
int MaterialTreeView::GetImageForType( int type ) const
{
	switch( type )
	{
		case TYPE_FOLDER:
			return IMAGE_FOLDER;
		case TYPE_FILE:
			return IMAGE_FILE;
		case TYPE_MATERIAL_FOLDER:
			return IMAGE_MATERIAL_FOLDER;
		case TYPE_MATERIAL:
			return IMAGE_MATERIAL;
		default:
			return IMAGE_FOLDER;
	}
}

/*
================
MaterialTreeView::GetItemTextSafe
================
*/
wxString MaterialTreeView::GetItemTextSafe( wxTreeItemId item ) const
{
	if( !item.IsOk() || item == m_tree->GetRootItem() )
	{
		return wxString();
	}
	return m_tree->GetItemText( item );
}

/*
================
MaterialTreeView::GetMediaPath
================
*/
idStr MaterialTreeView::GetMediaPath( wxTreeItemId item, int type ) const
{
	int stopType = TYPE_ROOT;
	switch( type )
	{
		case TYPE_MATERIAL:
		case TYPE_MATERIAL_FOLDER:
			stopType = TYPE_FILE;
			break;
		case TYPE_FILE:
		default:
			stopType = TYPE_ROOT;
			break;
	}

	const wxTreeItemId root = m_tree->GetRootItem();

	idStr mediaName = GetItemTextSafe( item ).c_str();
	wxTreeItemId parent = m_tree->GetItemParent( item );
	while( parent.IsOk() && parent != root )
	{
		if( GetItemType( parent ) == stopType )
		{
			break;
		}

		idStr strParent = GetItemTextSafe( parent ).c_str();
		strParent += "/";
		strParent += mediaName;
		mediaName = strParent;

		parent = m_tree->GetItemParent( parent );
	}

	return mediaName;
}

/*
================
MaterialTreeView::GetFileName
================
*/
bool MaterialTreeView::GetFileName( wxTreeItemId item, idStr& out ) const
{
	out = "";

	int type = GetItemType( item );
	if( type != TYPE_MATERIAL && type != TYPE_MATERIAL_FOLDER && type != TYPE_FILE )
	{
		return false;
	}

	if( type == TYPE_FILE )
	{
		out = GetMediaPath( item, TYPE_FILE );
		return true;
	}

	wxTreeItemId parent = m_tree->GetItemParent( item );
	while( parent.IsOk() )
	{
		if( GetItemType( parent ) == TYPE_FILE )
		{
			out = GetMediaPath( parent, TYPE_FILE );
			return true;
		}
		parent = m_tree->GetItemParent( parent );
	}

	return false;
}

/*
================
MaterialTreeView::GetMaterialPaths
================
*/
void MaterialTreeView::GetMaterialPaths( wxTreeItemId item, std::vector<MaterialTreeItem_t>& out ) const
{
	wxTreeItemIdValue cookie;
	for( wxTreeItemId child = m_tree->GetFirstChild( item, cookie );
			child.IsOk();
			child = m_tree->GetNextChild( item, cookie ) )
	{
		int t = GetItemType( child );
		if( t == TYPE_MATERIAL )
		{
			MaterialTreeItem_t mat;
			mat.materialName = GetMediaPath( child, TYPE_MATERIAL );
			mat.treeItem     = child;
			out.push_back( mat );
		}
		else if( t == TYPE_MATERIAL_FOLDER )
		{
			GetMaterialPaths( child, out );
		}
	}
}

/*
================
MaterialTreeView::InitializeMaterialList
================
*/
void MaterialTreeView::InitializeMaterialList( bool includeFile, const char* filename )
{
	treeWithFile = includeFile;

	// Delete children of the hidden root but keep the root itself.
	wxTreeItemId root = m_tree->GetRootItem();
	if( !root.IsOk() )
	{
		root = m_tree->AddRoot( "root" );
	}
	m_tree->DeleteChildren( root );

	quickTree.clear();
	materialToTree.clear();
	fileToTree.clear();

	BuildMaterialList( includeFile, filename );
}

/*
================
MaterialTreeView::BuildMaterialList
================
*/
void MaterialTreeView::BuildMaterialList( bool includeFile, const char* filename )
{
	idStrList list( 1024 );

	const int count = declManager->GetNumDecls( DECL_MATERIAL );
	if( count <= 0 )
	{
		return;
	}

	for( int i = 0; i < count; i++ )
	{
		const idMaterial* mat = declManager->MaterialByIndex( i, false );

		if( filename && strcmp( filename, mat->GetFileName() ) )
		{
			continue;
		}

		idStr file = mat->GetFileName();

		// Skip implicit definitions.
		if( !file.Icmp( "<implicit file>" ) )
		{
			continue;
		}

		idStr temp;
		if( includeFile )
		{
			file.StripPath();
			temp = idStr( mat->GetFileName() ) + "/" + idStr( mat->GetName() ) + "|" + file;
		}
		else
		{
			temp = mat->GetName();
		}

		list.Append( temp );
	}

	if( list.Num() > 0 )
	{
		AddStrList( &list, includeFile );
	}
}

/*
================
MaterialTreeView::AddStrList
================
*/
void MaterialTreeView::AddStrList( idStrList* list, bool includeFile )
{
	wxTreeItemId rootItem = m_tree->GetRootItem();
	if( !rootItem.IsOk() )
	{
		rootItem = m_tree->AddRoot( "root" );
		m_tree->SetItemData( rootItem, new TreeItemData( TYPE_ROOT ) );
	}

	//list->Sort(); //BFG_FIXME
	const int count = list->Num();

	for( int i = 0; i < count; i++ )
	{
		idStr fullName = ( *list )[i];
		idStr filename;

		if( includeFile )
		{
			int bar = fullName.Find( "|" );
			if( bar >= 0 )
			{
				filename = fullName.Right( fullName.Length() - bar - 1 );
				fullName = fullName.Left( bar );
			}
		}

		fullName.BackSlashesToSlashes();
		fullName.Strip( ' ' );

		idList<idStr> parts;
		SplitPath( fullName, parts );

		wxTreeItemId current = rootItem;
		idStr curPath;
		bool passedFile = !includeFile;

		for( int j = 0; j < parts.Num(); j++ )
		{
			const idStr& component = parts[j];
			curPath += component;
			curPath += "/";

			bool isLast = ( j == parts.Num() - 1 );
			bool isFile = includeFile && !passedFile && ( component == filename );

			int type;
			if( isLast )
			{
				type = TYPE_MATERIAL;
			}
			else if( isFile )
			{
				type = TYPE_FILE;
			}
			else if( passedFile )
			{
				type = TYPE_MATERIAL_FOLDER;
			}
			else
			{
				type = TYPE_FOLDER;
			}

			wxTreeItemId existing;
			std::map<std::string, wxTreeItemId>::iterator it = quickTree.find( curPath.c_str() );
			if( it != quickTree.end() )
			{
				existing = it->second;
			}

			if( !existing.IsOk() )
			{
				existing = m_tree->AppendItem( current, component.c_str() );
				m_tree->SetItemData( existing, new TreeItemData( type ) );
				m_tree->SetItemImage( existing, GetImageForType( type ) );
				quickTree[ curPath.c_str() ] = existing;

				if( type == TYPE_FILE )
				{
					idStr file = GetMediaPath( existing, TYPE_FILE );
					fileToTree[ file.c_str() ] = existing;
				}
			}

			current = existing;

			if( isFile )
			{
				passedFile = true;
			}
		}

		if( current.IsOk() && GetItemType( current ) == TYPE_MATERIAL )
		{
			idStr materialName = GetMediaPath( current, TYPE_MATERIAL );
			materialToTree[ materialName.c_str() ] = current;
		}
	}
}

/*
================
MaterialTreeView::SetItemImage
================
*/
void MaterialTreeView::SetItemImage( wxTreeItemId item, bool mod, bool apply, bool children )
{
	int image = IMAGE_FOLDER;
	switch( GetItemType( item ) )
	{
		case TYPE_FILE:
			image = mod ? IMAGE_FILE_MOD : IMAGE_FILE;
			break;
		case TYPE_MATERIAL_FOLDER:
			image = IMAGE_MATERIAL_FOLDER;
			break;
		case TYPE_MATERIAL:
			if( mod && apply )
			{
				image = IMAGE_MATERIAL_MOD_APPLY;
			}
			else if( mod )
			{
				image = IMAGE_MATERIAL_MOD;
			}
			else
			{
				image = IMAGE_MATERIAL;
			}
			break;
		default:
			image = IMAGE_FOLDER;
			break;
	}

	m_tree->SetItemImage( item, image );

	if( children )
	{
		wxTreeItemIdValue cookie;
		for( wxTreeItemId child = m_tree->GetFirstChild( item, cookie );
				child.IsOk();
				child = m_tree->GetNextChild( item, cookie ) )
		{
			SetItemImage( child, mod, apply, children );
		}
	}
}

/*
================
MaterialTreeView::MV_OnMaterialChange
================
*/
void MaterialTreeView::MV_OnMaterialChange( MaterialDoc* pMaterial )
{
	if( !pMaterial )
	{
		return;
	}

	std::map<std::string, wxTreeItemId>::iterator it = materialToTree.find( pMaterial->name.c_str() );
	if( it == materialToTree.end() )
	{
		return;
	}

	m_tree->SetItemImage( it->second, IMAGE_MATERIAL_MOD_APPLY );

	if( treeWithFile )
	{
		idStr file = pMaterial->renderMaterial->GetFileName();
		std::map<std::string, wxTreeItemId>::iterator fit = fileToTree.find( file.c_str() );
		if( fit != fileToTree.end() )
		{
			m_tree->SetItemImage( fit->second, IMAGE_FILE_MOD );
		}
	}
}

/*
================
MaterialTreeView::MV_OnMaterialApply
================
*/
void MaterialTreeView::MV_OnMaterialApply( MaterialDoc* pMaterial )
{
	if( !pMaterial )
	{
		return;
	}
	std::map<std::string, wxTreeItemId>::iterator it = materialToTree.find( pMaterial->name.c_str() );
	if( it == materialToTree.end() )
	{
		return;
	}
	m_tree->SetItemImage( it->second, IMAGE_MATERIAL_MOD );
}

/*
================
MaterialTreeView::MV_OnMaterialSaved
================
*/
void MaterialTreeView::MV_OnMaterialSaved( MaterialDoc* pMaterial )
{
	if( !pMaterial )
	{
		return;
	}

	std::map<std::string, wxTreeItemId>::iterator it = materialToTree.find( pMaterial->name.c_str() );
	if( it != materialToTree.end() )
	{
		m_tree->SetItemImage( it->second, IMAGE_MATERIAL );
	}

	if( treeWithFile )
	{
		if( !materialDocManager->IsFileModified( pMaterial->renderMaterial->GetFileName() ) )
		{
			idStr file = pMaterial->renderMaterial->GetFileName();
			std::map<std::string, wxTreeItemId>::iterator fit = fileToTree.find( file.c_str() );
			if( fit != fileToTree.end() )
			{
				m_tree->SetItemImage( fit->second, IMAGE_FILE );
			}
		}
	}
}

/*
================
MaterialTreeView::MV_OnMaterialAdd
================
*/
void MaterialTreeView::MV_OnMaterialAdd( MaterialDoc* pMaterial )
{
	if( !pMaterial )
	{
		return;
	}

	idStrList list( 4 );
	const idMaterial* mat = pMaterial->renderMaterial;

	idStr temp;
	if( treeWithFile )
	{
		idStr fname = mat->GetFileName();
		fname.StripPath();
		temp = idStr( mat->GetFileName() ) + "/" + idStr( mat->GetName() ) + "|" + fname;
	}
	else
	{
		temp = mat->GetName();
	}

	list.Append( temp );
	AddStrList( &list, treeWithFile );

	// Keep the leaf sorted.
	std::map<std::string, wxTreeItemId>::iterator it = materialToTree.find( pMaterial->name.c_str() );
	if( it != materialToTree.end() )
	{
		wxTreeItemId parent = m_tree->GetItemParent( it->second );
		if( parent.IsOk() )
		{
			m_tree->SortChildren( parent );
		}
	}

	MV_OnMaterialChange( pMaterial );
}

/*
================
MaterialTreeView::MV_OnMaterialDelete
================
*/
void MaterialTreeView::MV_OnMaterialDelete( MaterialDoc* pMaterial )
{
	if( !pMaterial )
	{
		return;
	}

	std::map<std::string, wxTreeItemId>::iterator it = materialToTree.find( pMaterial->name.c_str() );
	if( it == materialToTree.end() )
	{
		return;
	}

	m_tree->Delete( it->second );
	materialToTree.erase( it );
}

/*
================
MaterialTreeView::MV_OnMaterialNameChanged
================
*/
void MaterialTreeView::MV_OnMaterialNameChanged( MaterialDoc* pMaterial, const char* oldName )
{
	if( !pMaterial || internalChange )
	{
		return;
	}

	// Remove the old tree node.
	std::map<std::string, wxTreeItemId>::iterator it = materialToTree.find( oldName );
	if( it != materialToTree.end() )
	{
		wxTreeItemId oldItem = it->second;
		CleanLookupTrees( oldItem );
		m_tree->Delete( oldItem );
	}

	idStrList list( 4 );
	const idMaterial* mat = pMaterial->renderMaterial;

	idStr temp;
	if( treeWithFile )
	{
		idStr fname = mat->GetFileName();
		fname.StripPath();
		temp = idStr( mat->GetFileName() ) + "/" + idStr( mat->GetName() ) + "|" + fname;
	}
	else
	{
		temp = mat->GetName();
	}

	list.Append( temp );
	AddStrList( &list, treeWithFile );

	// Reselect.
	std::map<std::string, wxTreeItemId>::iterator nit = materialToTree.find( pMaterial->name.c_str() );
	if( nit != materialToTree.end() )
	{
		wxTreeItemId parent = m_tree->GetItemParent( nit->second );
		if( parent.IsOk() )
		{
			m_tree->SortChildren( parent );
		}
		m_tree->SelectItem( nit->second );
	}

	MV_OnMaterialChange( pMaterial );
}

/*
================
MaterialTreeView::MV_OnFileReload
================
*/
void MaterialTreeView::MV_OnFileReload( const char* filename )
{
	std::map<std::string, wxTreeItemId>::iterator fit = fileToTree.find( filename );
	if( fit == fileToTree.end() )
	{
		return;
	}

	wxTreeItemId fileItem = fit->second;
	CleanLookupTrees( fileItem );
	m_tree->Delete( fileItem );

	BuildMaterialList( treeWithFile, filename );

	// Restore sort order.
	std::map<std::string, wxTreeItemId>::iterator nit = fileToTree.find( filename );
	if( nit != fileToTree.end() )
	{
		wxTreeItemId parent = m_tree->GetItemParent( nit->second );
		if( parent.IsOk() )
		{
			m_tree->SortChildren( parent );
		}
	}
}

/*
================
IsMaterialSelected
================
*/
static bool IsMaterialSelected( MaterialTreeView* view, wxTreeItemId* outItem )
{
	// Helper used by Can* methods.  Returns true if a material leaf is selected.
	return true;
}

/*
================
MaterialTreeView::CanCopy
================
*/
bool MaterialTreeView::CanCopy()
{
	wxTreeItemId item = m_tree->GetSelection();
	if( !item.IsOk() )
	{
		return false;
	}
	return GetItemType( item ) == TYPE_MATERIAL;
}

/*
================
MaterialTreeView::CanPaste
================
*/
bool MaterialTreeView::CanPaste()
{
	return materialDocManager->IsCopyMaterial();
}

/*
================
MaterialTreeView::CanCut
================
*/
bool MaterialTreeView::CanCut()
{
	return CanCopy();
}

/*
================
MaterialTreeView::CanDelete
================
*/
bool MaterialTreeView::CanDelete()
{
	wxTreeItemId item = m_tree->GetSelection();
	if( !item.IsOk() )
	{
		return false;
	}
	int t = GetItemType( item );
	return t == TYPE_MATERIAL_FOLDER || t == TYPE_MATERIAL;
}

/*
================
MaterialTreeView::CanRename
================
*/
bool MaterialTreeView::CanRename()
{
	wxTreeItemId item = m_tree->GetSelection();
	if( !item.IsOk() )
	{
		return false;
	}
	int t = GetItemType( item );
	return t == TYPE_MATERIAL_FOLDER || t == TYPE_MATERIAL;
}

/*
================
MaterialTreeView::CanSaveFile
================
*/
bool MaterialTreeView::CanSaveFile()
{
	wxTreeItemId item = m_tree->GetSelection();
	if( !item.IsOk() )
	{
		return false;
	}

	idStr filename;
	if( !GetFileName( item, filename ) )
	{
		return false;
	}

	return materialDocManager->IsFileModified( filename.c_str() );
}

/*
================
MaterialTreeView::GetSaveFilename
================
*/
idStr MaterialTreeView::GetSaveFilename()
{
	wxTreeItemId item = m_tree->GetSelection();
	idStr filename;
	if( item.IsOk() && !GetFileName( item, filename ) )
	{
		filename = "";
	}
	return filename;
}

/*
================
MaterialTreeView::FindNextMaterial
================
*/
bool MaterialTreeView::FindNextMaterial( MaterialSearchData_t* searchData )
{
	if( !searchData )
	{
		return false;
	}

	wxTreeItemId start = m_tree->GetSelection();
	if( !start.IsOk() )
	{
		wxTreeItemId root = m_tree->GetRootItem();
		if( !root.IsOk() )
		{
			return false;
		}

		wxTreeItemIdValue cookie;
		start = m_tree->GetFirstChild( root, cookie );
		if( !start.IsOk() )
		{
			return false;
		}
	}

	std::vector<wxTreeItemId> all;
	{
		std::function<void( wxTreeItemId )> collect = [&]( wxTreeItemId node )
		{
			wxTreeItemIdValue cookie;
			for( wxTreeItemId c = m_tree->GetFirstChild( node, cookie );
					c.IsOk();
					c = m_tree->GetNextChild( node, cookie ) )
			{
				all.push_back( c );
				collect( c );
			}
		};
		collect( m_tree->GetRootItem() );
	}

	if( all.empty() )
	{
		return false;
	}

	// Find the index of the currently selected item (or the beginning).
	size_t startIdx = 0;
	for( size_t i = 0; i < all.size(); i++ )
	{
		if( all[i] == start )
		{
			startIdx = ( i + 1 ) % all.size();
			break;
		}
	}

	const size_t n = all.size();
	for( size_t step = 0; step < n; step++ )
	{
		size_t idx = ( startIdx + step ) % n;
		wxTreeItemId item = all[idx];
		int t = GetItemType( item );

		if( t == TYPE_MATERIAL )
		{
			idStr itemName = GetItemTextSafe( item ).c_str();
			int findPos = itemName.Find( searchData->searchText, false );
			if( findPos != -1 )
			{
				m_tree->SelectItem( item );
				m_tree->EnsureVisible( item );
				return true;
			}

			if( !searchData->nameOnly )
			{
				idStr materialName = GetMediaPath( item, TYPE_MATERIAL );
				if( materialDocManager->FindMaterial( materialName, searchData, false ) )
				{
					m_tree->SelectItem( item );
					m_tree->EnsureVisible( item );
					return true;
				}
			}
		}
		else
		{
			idStr itemName = GetItemTextSafe( item ).c_str();
			int findPos = itemName.Find( searchData->searchText, false );
			if( findPos != -1 )
			{
				m_tree->SelectItem( item );
				m_tree->EnsureVisible( item );
				return true;
			}
		}
	}

	return false;
}

/*
================
MaterialTreeView::OnSelChanged
================
*/
void MaterialTreeView::OnSelChanged( wxTreeEvent& event )
{
	wxTreeItemId item = event.GetItem();
	if( !item.IsOk() )
	{
		return;
	}

	if( GetItemType( item ) == TYPE_MATERIAL )
	{
		idStr mediaName = GetMediaPath( item, TYPE_MATERIAL );
		const idMaterial* material = declManager->FindMaterial( mediaName );
		materialDocManager->SetSelectedMaterial( const_cast<idMaterial*>( material ) );
	}
	else
	{
		materialDocManager->SetSelectedMaterial( NULL );
	}
}

/*
================
MaterialTreeView::OnBeginLabelEdit
================
*/
void MaterialTreeView::OnBeginLabelEdit( wxTreeEvent& event )
{
	wxTreeItemId item = event.GetItem();
	if( !item.IsOk() )
	{
		event.Veto();
		return;
	}

	int t = GetItemType( item );
	if( t != TYPE_MATERIAL && t != TYPE_MATERIAL_FOLDER )
	{
		event.Veto();
	}
}

/*
================
MaterialTreeView::OnEndLabelEdit
================
*/
void MaterialTreeView::OnEndLabelEdit( wxTreeEvent& event )
{
	if( !event.IsEditCancelled() )
	{
		wxString newLabel = event.GetLabel();
		idStr canonical = newLabel.c_str();
		canonical.ToLower();

		wxTreeItemId item = event.GetItem();
		int t = GetItemType( item );

		if( t == TYPE_MATERIAL )
		{
			MaterialDoc* pMaterial = materialDocManager->GetCurrentMaterialDoc();
			if( !pMaterial )
			{
				return;
			}

			materialToTree.erase( pMaterial->name.c_str() );

			idStr material;
			wxTreeItemId parent = m_tree->GetItemParent( item );
			if( parent.IsOk() && GetItemType( parent ) == TYPE_MATERIAL_FOLDER )
			{
				material = GetMediaPath( parent, TYPE_MATERIAL_FOLDER );
				material += "/";
			}
			material += canonical;

			if( declManager->FindMaterial( material, false ) )
			{
				wxMessageBox( "Unable to rename material because it conflicts "
							  "with another material.",
							  "Error", wxOK | wxICON_ERROR, this );
				event.Veto();
			}
			else
			{
				materialToTree[ material.c_str() ] = item;

				internalChange = true;
				pMaterial->SetMaterialName( material );
				internalChange = false;

				m_tree->SetItemText( item, canonical.c_str() );

				renamedFolder = item;

				wxTreeItemId parentItem = m_tree->GetItemParent( item );
				if( parentItem.IsOk() )
				{
					m_tree->SortChildren( parentItem );
				}
			}
		}
		else if( t == TYPE_MATERIAL_FOLDER )
		{
			CleanLookupTrees( item );

			renamedFolder = item;
			affectedMaterials.clear();
			GetMaterialPaths( item, affectedMaterials );

			m_tree->SetItemText( item, canonical.c_str() );

			BuildLookupTrees( item );

			for( size_t i = 0; i < affectedMaterials.size(); i++ )
				RenameMaterial( affectedMaterials[i].treeItem,
								affectedMaterials[i].materialName );

			wxTreeItemId parent = m_tree->GetItemParent( item );
			if( parent.IsOk() )
			{
				m_tree->SortChildren( parent );
			}
		}
	}
}

/*
================
MaterialTreeView::RenameMaterial
================
*/
void MaterialTreeView::RenameMaterial( wxTreeItemId item, const char* originalName )
{
	const idMaterial* material = declManager->FindMaterial( originalName );
	if( !material )
	{
		return;
	}

	MaterialDoc* pMaterial = materialDocManager->CreateMaterialDoc(
								 const_cast<idMaterial*>( material ) );

	materialToTree.erase( originalName );

	idStr materialName;
	wxTreeItemId parent = m_tree->GetItemParent( item );
	if( parent.IsOk() && GetItemType( parent ) == TYPE_MATERIAL_FOLDER )
	{
		materialName = GetMediaPath( parent, TYPE_MATERIAL_FOLDER );
		materialName += "/";
	}
	materialName += GetItemTextSafe( item ).c_str();

	materialToTree[ materialName.c_str() ] = item;

	internalChange = true;
	pMaterial->SetMaterialName( materialName, false );
	internalChange = false;
}

/*
================
MaterialTreeView::OnItemRightClick
================
*/
void MaterialTreeView::OnItemRightClick( wxTreeEvent& event )
{
	wxTreeItemId item = event.GetItem();
	if( !item.IsOk() )
	{
		return;
	}

	m_tree->SelectItem( item );
	ShowContextMenu( event.GetPoint() );
}

/*
================
MaterialTreeView::OnItemActivated
================
*/
void MaterialTreeView::OnItemActivated( wxTreeEvent& event )
{
	wxTreeItemId item = event.GetItem();
	if( item.IsOk() && GetItemType( item ) == TYPE_MATERIAL )
	{
		m_tree->SelectItem( item );
	}

	event.Skip();
}

/*
================
MaterialTreeView::PopupMenu
================
*/
void MaterialTreeView::ShowContextMenu( const wxPoint& pt )
{
	wxTreeItemId item = m_tree->GetSelection();
	if( !item.IsOk() )
	{
		return;
	}

	int itemType = GetItemType( item );

	MaterialDoc* pDoc = materialDocManager->GetCurrentMaterialDoc();

	wxMenu menu;

	// Apply
	{
		wxMenuItem* mi = menu.Append( ID_POPUP_APPLYMATERIAL, "Apply Material" );
		mi->Enable( pDoc && pDoc->applyWaiting );
	}
	{
		wxMenuItem* mi = menu.Append( ID_POPUP_APPLYFILE, "Apply File" );
		idStr filename;
		mi->Enable( GetFileName( item, filename ) && materialDocManager->DoesFileNeedApply( filename.c_str() ) );
	}
	{
		wxMenuItem* mi = menu.Append( ID_POPUP_APPLYALL, "Apply All" );
		mi->Enable( materialDocManager->DoesAnyNeedApply() );
	}

	menu.AppendSeparator();

	// Save
	{
		wxMenuItem* mi = menu.Append( ID_POPUP_SAVEMATERIAL, "Save Material" );
		mi->Enable( pDoc && pDoc->modified );
	}
	{
		wxMenuItem* mi = menu.Append( ID_POPUP_SAVEFILE, "Save File" );
		idStr filename;
		mi->Enable( GetFileName( item, filename ) && materialDocManager->IsFileModified( filename.c_str() ) );
	}
	{
		wxMenuItem* mi = menu.Append( ID_POPUP_SAVEALL, "Save All" );
		mi->Enable( materialDocManager->IsAnyModified() );
	}

	menu.AppendSeparator();

	// Cut / Copy / Paste
	{
		wxMenuItem* mi = menu.Append( ID_POPUP_CUT, "Cut" );
		mi->Enable( itemType == TYPE_MATERIAL );
	}
	{
		wxMenuItem* mi = menu.Append( ID_POPUP_COPY, "Copy" );
		mi->Enable( itemType == TYPE_MATERIAL );
	}
	{
		wxMenuItem* mi = menu.Append( ID_POPUP_PASTE, "Paste" );
		mi->Enable( ( itemType == TYPE_MATERIAL || itemType == TYPE_FILE ||
					  itemType == TYPE_MATERIAL_FOLDER ) &&
					materialDocManager->IsCopyMaterial() );
	}
	{
		wxMenuItem* mi = menu.Append( ID_POPUP_DELETEMATERIAL, "Delete" );
		mi->Enable( itemType == TYPE_MATERIAL || itemType == TYPE_MATERIAL_FOLDER );
	}

	menu.AppendSeparator();

	// Add
	{
		wxMenuItem* mi = menu.Append( ID_POPUP_ADDMATERIAL, "Add Material" );
		mi->Enable( itemType == TYPE_FILE || itemType == TYPE_MATERIAL_FOLDER ||
					itemType == TYPE_MATERIAL );
	}
	{
		wxMenuItem* mi = menu.Append( ID_POPUP_ADDFOLDER, "Add Folder" );
		mi->Enable( itemType == TYPE_FILE || itemType == TYPE_MATERIAL_FOLDER ||
					itemType == TYPE_MATERIAL );
	}
	{
		wxMenuItem* mi = menu.Append( ID_POPUP_RENAMEMATERIAL, "Rename" );
		mi->Enable( itemType == TYPE_MATERIAL || itemType == TYPE_MATERIAL_FOLDER );
	}

	menu.AppendSeparator();

	{
		wxMenuItem* mi = menu.Append( ID_POPUP_RELOADFILE, "Reload File" );
		mi->Enable( itemType == TYPE_FILE || itemType == TYPE_MATERIAL_FOLDER ||
					itemType == TYPE_MATERIAL );
	}

	PopupMenu( &menu, pt );
}

/*
================
MaterialTreeView::OnApplyMaterial
================
*/
void MaterialTreeView::OnApplyMaterial()
{
	materialDocManager->ApplyMaterial( materialDocManager->GetCurrentMaterialDoc() );
}

/*
================
MaterialTreeView::OnApplyFile
================
*/
void MaterialTreeView::OnApplyFile()
{
	wxTreeItemId item = m_tree->GetSelection();
	idStr filename;
	if( GetFileName( item, filename ) )
	{
		materialDocManager->ApplyFile( filename.c_str() );
	}
}

/*
================
MaterialTreeView::OnApplyAll
================
*/
void MaterialTreeView::OnApplyAll()
{
	materialDocManager->ApplyAll();
}

/*
================
MaterialTreeView::OnSaveMaterial
================
*/
void MaterialTreeView::OnSaveMaterial()
{
	materialDocManager->SaveMaterial( materialDocManager->GetCurrentMaterialDoc() );
}

/*
================
MaterialTreeView::OnSaveFile
================
*/
void MaterialTreeView::OnSaveFile()
{
	wxTreeItemId item = m_tree->GetSelection();
	idStr filename;
	if( GetFileName( item, filename ) )
	{
		materialDocManager->SaveFile( filename.c_str() );
	}
}

/*
================
MaterialTreeView::OnSaveAll
================
*/
void MaterialTreeView::OnSaveAll()
{
	materialDocManager->SaveAllMaterials();
}

/*
================
MaterialTreeView::OnCut
================
*/
void MaterialTreeView::OnCut()
{
	wxTreeItemId item = m_tree->GetSelection();
	if( !item.IsOk() )
	{
		return;
	}

	if( GetItemType( item ) == TYPE_MATERIAL )
	{
		materialDocManager->CopyMaterial( materialDocManager->GetCurrentMaterialDoc(), true );
	}
}

/*
================
MaterialTreeView::OnCopy
================
*/
void MaterialTreeView::OnCopy()
{
	wxTreeItemId item = m_tree->GetSelection();
	if( !item.IsOk() )
	{
		return;
	}

	if( GetItemType( item ) == TYPE_MATERIAL )
	{
		materialDocManager->CopyMaterial( materialDocManager->GetCurrentMaterialDoc(), false );
	}
}

/*
================
MaterialTreeView::OnPaste
================
*/
void MaterialTreeView::OnPaste()
{
	if( !materialDocManager->IsCopyMaterial() )
	{
		return;
	}

	wxTreeItemId item = m_tree->GetSelection();
	if( !item.IsOk() )
	{
		return;
	}

	int itemType = GetItemType( item );

	// Back up if a leaf was selected.
	if( itemType == TYPE_MATERIAL )
	{
		item = m_tree->GetItemParent( item );
		if( !item.IsOk() )
		{
			return;
		}
		itemType = GetItemType( item );
	}

	idStr materialName;
	if( itemType != TYPE_FILE )
	{
		materialName = GetMediaPath( item, itemType );
		materialName += "/";
	}

	idStr copyName = materialDocManager->GetCopyMaterialName();
	idStr copyMaterialName;
	copyName.ExtractFileName( copyMaterialName );
	materialName += copyMaterialName;

	idStr filename;
	GetFileName( item, filename );

	materialName = materialDocManager->GetUniqueMaterialName( materialName );

	materialDocManager->PasteMaterial( materialName, filename );
}

/*
================
MaterialTreeView::OnDeleteMaterial
================
*/
void MaterialTreeView::OnDeleteMaterial()
{
	wxTreeItemId item = m_tree->GetSelection();
	if( !item.IsOk() )
	{
		return;
	}

	int itemType = GetItemType( item );

	if( itemType == TYPE_MATERIAL_FOLDER )
	{
		int result = wxMessageBox( "Are you sure you want to delete this folder?", "Delete?", wxICON_QUESTION | wxYES_NO, this );
		if( result == wxYES )
		{
			DeleteFolder( item );
		}
	}
	else if( itemType == TYPE_MATERIAL )
	{
		int result = wxMessageBox( "Are you sure you want to delete this material?", "Delete?", wxICON_QUESTION | wxYES_NO, this );
		if( result == wxYES )
		{
			materialDocManager->DeleteMaterial( materialDocManager->GetCurrentMaterialDoc() );
		}
	}
}

/*
================
MaterialTreeView::DeleteFolder
================
*/
void MaterialTreeView::DeleteFolder( wxTreeItemId item, bool addUndo )
{
	if( !item.IsOk() )
	{
		return;
	}

	std::vector<MaterialTreeItem_t> materialsToDelete;
	GetMaterialPaths( item, materialsToDelete );

	// Delete each material via the doc manager.
	for( size_t i = 0; i < materialsToDelete.size(); i++ )
	{
		const idMaterial* material = declManager->FindMaterial( materialsToDelete[i].materialName );
		MaterialDoc* pMaterial = materialDocManager->CreateMaterialDoc( const_cast<idMaterial*>( material ) );
		materialDocManager->DeleteMaterial( pMaterial, false );
	}

	CleanLookupTrees( item );
	m_tree->Delete( item );

	( void )addUndo;
}

/*
================
MaterialTreeView::AddFolder
================
*/
wxTreeItemId MaterialTreeView::AddFolder( const char* name, wxTreeItemId parent )
{
	if( !parent.IsOk() )
	{
		parent = m_tree->GetRootItem();
	}

	wxTreeItemId newItem = m_tree->AppendItem( parent, name );
	m_tree->SetItemData( newItem, new TreeItemData( TYPE_MATERIAL_FOLDER ) );
	m_tree->SetItemImage( newItem, IMAGE_MATERIAL_FOLDER );
	m_tree->Expand( parent );

	// Register in the quick-lookup table.
	idStr qt = GetQuicktreePath( newItem );
	quickTree[ qt.c_str() ] = newItem;

	// Keep siblings sorted.
	wxTreeItemId grandParent = m_tree->GetItemParent( newItem );
	if( grandParent.IsOk() )
	{
		m_tree->SortChildren( grandParent );
	}

	return newItem;
}

/*
================
MaterialTreeView::RenameFolder
================
*/
void MaterialTreeView::RenameFolder( wxTreeItemId item, const char* name )
{
	if( !item.IsOk() )
	{
		return;
	}

	// The quick-lookup path contains the label, so it has to be rebuilt.
	CleanLookupTrees( item );

	m_tree->SetItemText( item, name );

	BuildLookupTrees( item );
}

/*
================
MaterialTreeView::OnRenameMaterial
================
*/
void MaterialTreeView::OnRenameMaterial()
{
	wxTreeItemId item = m_tree->GetSelection();
	if( item.IsOk() )
	{
		m_tree->EditLabel( item );
	}
}

/*
================
MaterialTreeView::OnAddMaterial
================
*/
void MaterialTreeView::OnAddMaterial()
{
	wxTreeItemId item = m_tree->GetSelection();
	if( !item.IsOk() )
	{
		return;
	}

	int itemType = GetItemType( item );

	// Determine the containing file.
	wxTreeItemId fileItem = item;
	while( fileItem.IsOk() && GetItemType( fileItem ) != TYPE_FILE )
	{
		fileItem = m_tree->GetItemParent( fileItem );
	}

	if( !fileItem.IsOk() )
	{
		return;
	}
	idStr filename = GetMediaPath( fileItem, TYPE_FILE );

	// Determine the material folder.
	idStr materialFolder;
	if( itemType == TYPE_MATERIAL )
	{
		wxTreeItemId parentFolderItem = m_tree->GetItemParent( item );
		if( parentFolderItem.IsOk() && GetItemType( parentFolderItem ) == TYPE_MATERIAL_FOLDER )
		{
			materialFolder = GetMediaPath( parentFolderItem, TYPE_MATERIAL_FOLDER );
		}
	}
	else if( itemType == TYPE_MATERIAL_FOLDER )
	{
		materialFolder = GetMediaPath( item, TYPE_MATERIAL_FOLDER );
	}
	// else: TYPE_FILE, no folder

	// Generate a unique name.
	idStr name;
	int num = 1;
	while( true )
	{
		if( materialFolder.Length() > 0 )
		{
			name = va( "%s/newmaterial%d", materialFolder.c_str(), num );
		}
		else
		{
			name = va( "newmaterial%d", num );
		}

		if( !declManager->FindMaterial( name, false ) )
		{
			break;
		}
		num++;
	}

	materialDocManager->AddMaterial( name.c_str(), filename.c_str() );
}

/*
================
MaterialTreeView::OnAddFolder
================
*/
void MaterialTreeView::OnAddFolder()
{
	wxTreeItemId item = m_tree->GetSelection();
	if( !item.IsOk() )
	{
		return;
	}

	// Back up to a folder-capable parent.
	if( GetItemType( item ) == TYPE_MATERIAL )
	{
		item = m_tree->GetItemParent( item );
		if( !item.IsOk() )
		{
			return;
		}
	}

	// Choose a unique name.
	idStr newFolder;
	int num = 1;
	while( true )
	{
		newFolder = va( "newfolder%d", num );

		bool found = false;
		if( m_tree->ItemHasChildren( item ) )
		{
			wxTreeItemIdValue cookie;
			for( wxTreeItemId child = m_tree->GetFirstChild( item, cookie );
					child.IsOk();
					child = m_tree->GetNextChild( item, cookie ) )
			{
				if( !newFolder.Icmp( GetItemTextSafe( child ).c_str() ) )
				{
					found = true;
					break;
				}
			}
		}
		if( !found )
		{
			break;
		}
		num++;
	}

	wxTreeItemId newItem = m_tree->AppendItem( item, newFolder.c_str() );
	m_tree->SetItemData( newItem, new TreeItemData( TYPE_MATERIAL_FOLDER ) );
	m_tree->SetItemImage( newItem, IMAGE_MATERIAL_FOLDER );
	m_tree->Expand( item );

	idStr qt = GetQuicktreePath( newItem );
	quickTree[ qt.c_str() ] = newItem;

	wxTreeItemId parent = m_tree->GetItemParent( newItem );
	if( parent.IsOk() )
	{
		m_tree->SortChildren( parent );
	}
}

/*
================
MaterialTreeView::OnReloadFile
================
*/
void MaterialTreeView::OnReloadFile()
{
	wxTreeItemId item = m_tree->GetSelection();
	if( !item.IsOk() )
	{
		return;
	}

	int itemType = GetItemType( item );
	if( itemType != TYPE_MATERIAL && itemType != TYPE_FILE &&
			itemType != TYPE_MATERIAL_FOLDER )
	{
		return;
	}

	idStr filename;
	GetFileName( item, filename );

	if( materialDocManager->IsFileModified( filename ) )
	{
		int result = wxMessageBox( "This file has been modified. Are you sure you want to reload this file?", "Reload?", wxICON_QUESTION | wxYES_NO, this );
		if( result != wxYES )
		{
			return;
		}
	}

	materialDocManager->ReloadFile( filename );
}

/*
================
MaterialTreeView::OnKeyDown
================
*/
void MaterialTreeView::OnKeyDown( wxKeyEvent& event )
{
	if( event.GetKeyCode() == WXK_DELETE )
	{
		OnDeleteMaterial();
		return;
	}
	event.Skip();
}