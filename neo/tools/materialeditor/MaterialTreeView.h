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

#ifndef __MATERIALTREEVIEW_H__
#define __MATERIALTREEVIEW_H__

#include <wx/wx.h>
#include <wx/treectrl.h>
#include <wx/imaglist.h>
#include <wx/menu.h>

#include <map>
#include <string>
#include <vector>

#include "MaterialEditor.h"
#include "MaterialView.h"
#include "MaterialDocManager.h"

/**
 * Structure used associate a material name with a tree item.
 */
struct MaterialTreeItem_t
{
	idStr           materialName;
	wxTreeItemId    treeItem;
};

/**
* A tree view of all the materials that have been defined.
*/
class MaterialTreeView : public wxPanel, public MaterialView
{
public:
	explicit MaterialTreeView( wxWindow* parent );
	virtual			~MaterialTreeView();

public: // MaterialTreeView interface

	void        InitializeMaterialList( bool includeFile = true, const char* filename = nullptr );
	void        BuildMaterialList( bool includeFile = true, const char* filename = nullptr );

	bool        CanCopy();
	bool        CanPaste();
	bool        CanCut();
	bool        CanDelete();
	bool        CanRename();
	bool        CanSaveFile();
	idStr       GetSaveFilename();

	bool        FindNextMaterial( MaterialSearchData_t* searchData );

	void        OnCut();
	void        OnCopy();
	void        OnPaste();
	void        OnDeleteMaterial();
	void        OnRenameMaterial();
	void        OnAddMaterial();
	void        OnAddFolder();
	void        OnReloadFile();
	void        OnApplyMaterial();
	void        OnApplyFile();
	void        OnApplyAll();
	void        OnSaveMaterial();
	void        OnSaveFile();
	void        OnSaveAll();

	wxTreeItemId AddFolder( const char* name, wxTreeItemId parent );
	void         RenameFolder( wxTreeItemId item, const char* name );
	void         DeleteFolder( wxTreeItemId item, bool addUndo = true );

public: // MaterialView interface
	virtual void    MV_OnMaterialChange( MaterialDoc* pMaterial ) override;
	virtual void    MV_OnMaterialApply( MaterialDoc* pMaterial ) override;
	virtual void    MV_OnMaterialSaved( MaterialDoc* pMaterial ) override;
	virtual void    MV_OnMaterialAdd( MaterialDoc* pMaterial ) override;
	virtual void    MV_OnMaterialDelete( MaterialDoc* pMaterial ) override;
	virtual void    MV_OnMaterialNameChanged( MaterialDoc* pMaterial, const char* oldName ) override;
	virtual void    MV_OnFileReload( const char* filename ) override;

protected:
	enum ItemType
	{
		TYPE_ROOT = 0,
		TYPE_FOLDER,            // folder that is part of the material FILE's path
		TYPE_FILE,              // a .mtr file node
		TYPE_MATERIAL_FOLDER,   // folder declared inside a material file
		TYPE_MATERIAL           // an actual material leaf
	};

	enum
	{
		IMAGE_FOLDER = 0,
		IMAGE_FILE,
		IMAGE_MATERIAL,
		IMAGE_MATERIAL_FOLDER,
		IMAGE_FILE_MOD,
		IMAGE_MATERIAL_MOD,
		IMAGE_MATERIAL_MOD_APPLY
	};

	void        OnSelChanged( wxTreeEvent& event );
	void        OnBeginLabelEdit( wxTreeEvent& event );
	void        OnEndLabelEdit( wxTreeEvent& event );
	void        OnItemRightClick( wxTreeEvent& event );
	void        OnItemActivated( wxTreeEvent& event );
	void        OnKeyDown( wxKeyEvent& event );

	void        RenameMaterial( wxTreeItemId item, const char* originalName );
	bool        GetFileName( wxTreeItemId item, idStr& out ) const;
	idStr       GetMediaPath( wxTreeItemId item, int type ) const;
	void        GetMaterialPaths( wxTreeItemId item, std::vector<MaterialTreeItem_t>& out ) const;
	void        AddStrList( idStrList* list, bool includeFile );
	void        SetItemImage( wxTreeItemId item, bool mod, bool apply, bool children );
	int         GetItemType( wxTreeItemId item ) const;
	int         GetImageForType( int type ) const;
	wxString    GetItemTextSafe( wxTreeItemId item ) const;
	void        ShowContextMenu( const wxPoint& pt );

	void        CleanLookupTrees( wxTreeItemId item );
	void        BuildLookupTrees( wxTreeItemId item );
	idStr       GetQuicktreePath( wxTreeItemId item ) const;

protected:
	wxTreeCtrl*     m_tree;
	wxImageList*    m_imageList;

	bool    treeWithFile;
	bool    internalChange;

	std::map<std::string, wxTreeItemId> quickTree;
	std::map<std::string, wxTreeItemId> materialToTree;
	std::map<std::string, wxTreeItemId> fileToTree;
	wxTreeItemId renamedFolder;
	std::vector<MaterialTreeItem_t> affectedMaterials;
};

#endif /* !__MATERIALTREEVIEW_H__ */