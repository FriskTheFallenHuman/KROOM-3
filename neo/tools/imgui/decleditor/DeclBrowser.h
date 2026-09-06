/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Daniel Gibson

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

#ifndef __DECLBROWSER_H__
#define __DECBROWSER_H__

#include "../util/PathTreeCtrl.h"

#include "DeclNew.h"
#include "DeclEditor.h"

/*
===============================================================================

	DeclBrowser dialog

===============================================================================
*/


class DeclBrowser : public idImGuiWindow
{
public:
	friend class DeclNew;
	friend class DeclEditor;

private:
	bool				isShown;
	idStr				statusBarText;
	PathTreeCtrl		declTree;
	idStr				findNameStatic;
	idStr				findTextStatic;
	idStr				findNameEdit;
	idStr				findTextEdit;
	bool				findButtonEnabled;
	bool				editButtonEnabled;
	bool				newButtonEnabled;
	bool				reloadButtonEnabled;
	bool				cancelButtonEnabled;


	PathTreeCtrl		baseDeclTree;
	int					numListedDecls;
	idStr				findNameString;
	idStr				findTextString;

	DeclNew				declNewDlg;
	DeclEditor			declEditorDlg;

	void				OnTreeDblclk();
	void				OnBnClickedFind();
	void				OnBnClickedEdit();
	void				OnBnClickedNew();
	void				OnBnClickedNewAccepted();
	void				OnBnClickedReload();

	void				AddDeclTypeToTree( declType_t type, const char* root, PathTreeCtrl& tree );
	void				AddScriptsToTree( PathTreeCtrl& tree );
	void				AddGUIsToTree( PathTreeCtrl& tree );
	void				InitBaseDeclTree();

	void				GetDeclName( TreeNode* item, idStr& typeName, idStr& declName ) const;
	const idDecl* 		GetDeclFromTreeItem( TreeNode* item ) const;
	const idDecl* 		GetSelectedDecl() const;
	void				EditSelected();

	void				Reset();

	DeclBrowser();

public:
	void				ReloadDeclarations();
	bool				CompareDecl( TreeNode* item, const char* name ) const;
	bool				OnToolTipNotify( TreeNode* item, idStr& tooltipText ) const;
	void				OnTreeSelChanged( bool doubleClicked );

public:
	const char* GetWindowName() const override
	{
		return "###DeclBrowser";
	}
	DockRegion GetDockRegion() const override
	{
		return DOCK_REGION_NONE;
	}
	bool IsShown() const override
	{
		return isShown;
	}
	bool IsFreeCameraActive() const override
	{
		return false;
	}

	void Draw() override;

	static DeclBrowser&	Instance();

	ID_INLINE void			ShowIt( bool show )
	{
		isShown = show;
	}
};

#endif /* !__DECLBROWSER_H__ */
