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

#ifndef __MATERIALEDITOR_H__
#define __MATERIALEDITOR_H__

#include <wx/wx.h>

enum MaterialEditorIDs
{
	IDD_ME_ABOUTBOX = wxID_HIGHEST + 1,
	IDD_TEST_DIALOG,
	IDD_CONSOLE_FORM,
	IDD_FIND,
	IDD_MATERIALEDIT_FORM,
	IDR_ME_MAINFRAME,
	IDR_ME_MATERIALTREE_POPUP,
	IDR_ME_FILETOOLBAR,
	IDR_ME_STAGELIST_POPUP,
	IDB_ME_TREEBITMAP,
	IDI_ME_ON_ICON,
	IDI_ME_OFF_ICON,
	IDI_ME_DISABLED_ICON,
	IDS_STRING7002,
	ID_EDITMENU_INSERTFILE,
	IDC_MATERIALEDITOR_EDIT_TEXT,
	IDC_TAB1,
	IDC_TAB2,
	IDC_CONSOLE_OUTPUT,
	IDC_EDIT_FINDTEXT,
	IDC_EDIT2,
	IDC_CONSOLE_EDIT,
	IDC_CHECK_MATCH_CASE,
	IDC_CHECK_NAME_ONLY,
	IDC_CHECK_MATCH_WORD,
	IDC_RADIO_SEARCHFILE,
	IDC_RADIO_SEARCHSCOPE,
	IDC_RADIO_SEARCHALL,
	IDC_MATERIAL_NAME,

	ID_ME_FILE_EXIT,
	ID_ME_FILE_OPEN,
	ID_ME_FILE_SAVE,
	ID_ME_FILE_NEW,
	ID_ME_FILE_SAVEAS,
	ID_ME_FILE_SHOW_ALL_MATERIALS,
	ID_VIEW_INCLUDEFILENAME,
	ID_PREVIEW_RELOADSHADERS,
	ID_ME_PREVIEW_APPLYCHANGES,
	ID_ME_PREVIEW_APPLYALL,
	ID_POPUP_APPLYCHANGES,
	ID_ME_FILE_SAVEMATERIAL,
	ID_ME_FILE_SAVEFILE,
	ID_POPUP_SAVEMATERIAL,
	ID_POPUP_SAVEFILE,
	ID_POPUP_SAVEALL,
	ID_POPUP_APPLYFILE,
	ID_POPUP2_SAVEFILE,
	ID_POPUP2_SAVEALL,
	ID_POPUP_APPLYALL,
	ID_POPUP3_SAVEALL,
	ID_POPUP_APPLYMATERIAL,
	ID_ME_PREVIEW_APPLYFILE,
	ID_STAGEPOPUP_ADDSTAGE,
	ID_STAGEPOPUP_ADDBUMPMAP,
	ID_STAGEPOPUP_ADDDIFFUSEMAP,
	ID_STAGEPOPUP_ADDSPECULAR,
	ID_STAGEPOPUP_DELETESTAGE,
	ID_Menu7052,
	ID_STAGEPOPUP_RENAMESTAGE,
	ID_STAGEPOPUP_DELETEALLSTAGES,
	ID_PREVIEW_RELOADIMAGES,
	ID_POPUP_ADDMATERIAL,
	ID_POPUP_DELETEMATERIAL,
	ID_POPUP_RENAMEMATERIAL,
	ID_POPUP_ADDFOLDER,
	ID_ME_EDIT_UNDO,
	ID_ME_EDIT_REDO,
	ID_ME_EDIT_CUT,
	ID_Menu7063,
	ID_ME_EDIT_COPY,
	ID_ME_EDIT_PASTE,
	ID_ME_EDIT_DELETE,
	ID_POPUP_CUT,
	ID_POPUP_COPY,
	ID_POPUP_PASTE,
	ID_STAGEPOPUP_CUT,
	ID_STAGEPOPUP_COPY,
	ID_STAGEPOPUP_PASTE,
	ID_STAGEPOPUP_ADD,
	ID_ME_EDIT_FIND,
	ID_ME_EDIT_RENAME,
	ID_POPUP_RELOADFILE,
	ID_FIND_NEXT,
	ID_BUTTON40001,
	ID_BUTTON40002,
	ID_BUTTON40003,
	ID_BUTTON40004,
	ID_BUTTON40005,
	ID_BUTTON40006,
	ID_BUTTON40007,
	ID_ME_FILE_SAVE_ACEL,
	ID_ME_EDIT_FIND_NEXT
};

/**
* Structure used to store the user defined search parameters.
 */
typedef struct
{
	bool		searched;
	idStr		searchText;
	int			nameOnly;
	int			searchScope;
} MaterialSearchData_t;

class MaterialEditorApp : public wxApp
{
public:
	virtual bool OnInit();
};

extern wxWindow* GetMaterialEditorWindow();

#endif /* !__MATERIALEDITOR_H__ */