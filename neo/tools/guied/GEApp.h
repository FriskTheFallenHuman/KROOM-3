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

#ifndef __GEAPP_H__
#define __GEAPP_H__

#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/wxprec.h>
#include <wx/glcanvas.h>
#include <wx/notebook.h>
#include <wx/aui/aui.h>
#include <wx/artprov.h>
#include <wx/statbmp.h>
#include <wx/tglbtn.h>
#include <wx/propdlg.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/manager.h>
#include <wx/clrpicker.h>
#include <wx/spinctrl.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include <wx/stc/stc.h>
#include <wx/popupwin.h>
#include <wx/filename.h>
#include <wx/filefn.h>
#include <wx/thread.h>
#include <wx/hashmap.h>
#include <wx/scrolbar.h>
#include <wx/statline.h>

#include "../../ui/Rectangle.h"
#include "../../ui/DeviceContext.h"
#include "../../ui/Window.h"
#include "../../ui/SimpleWindow.h"
#include "../../ui/EditWindow.h"
#include "../../ui/ListWindow.h"
#include "../../ui/BindWindow.h"
#include "../../ui/RenderWindow.h"
#include "../../ui/ChoiceWindow.h"
#include "../../ui/UserInterfaceLocal.h"

#include "../../renderer/RenderCommon.h"
#include "../../renderer/RenderContext.h"
#include "../../sys/sdl/DeviceSDL.h"
#include "../../framework/Common_local.h"

#include "../common/AutoHideStrip.h"
#include "../common/AboutDialog.h"
#include "../common/RegistryOptions.h"
#include "../common/ToolBarStrip.h"
#include "../common/ImageUtils.h"

class rvGEApp;
class rvGEWorkspace;
class rvGEWindowWrapper;

#include "GEWindowWrapper.h"
#include "GESelectionMgr.h"
#include "GEModifier.h"
#include "GEModifierStack.h"
#include "GEModifierGroup.h"
#include "GEDeleteModifier.h"
#include "GEHideModifier.h"
#include "GEInsertModifier.h"
#include "GEKeyValueModifier.h"
#include "GEMoveModifier.h"
#include "GESizeModifier.h"
#include "GEStateModifier.h"
#include "GEZOrderModifier.h"
#include "GEOptions.h"
#include "GEWorkspace.h"
#include "GENavigator.h"
#include "GEProperties.h"
#include "GETransformer.h"
#include "GEItemPropsDlg.h"
#include "GEItemScriptsDlg.h"
#include "GEOptionsDlg.h"
#include "GEViewer.h"

enum GUIEdIDs
{
	IDD_GUIED_ABOUT = wxID_HIGHEST + 1,
	IDD_GUIED_OPTIONS_GRID,
	IDD_GUIED_OPTIONS_GENERAL,
	IDD_GUIED_ITEMPROPS_GENERAL,
	IDD_GUIED_ITEMPROPS_ADVANCED,
	IDD_GUIED_ITEMKEY,
	IDD_GUIED_ALPHA,
	IDD_GUIED_SCRIPTS,
	IDD_GUIED_ITEMPROPS_KEYS,
	IDD_GUIED_ITEM_VARIABLES,
	IDD_GUIED_ITEMPROPS_TEXT,
	IDD_GUIED_ITEMPROPS_IMAGE,
	IDD_GUIED_TRANSFORMER,
	IDD_GUIED_CHECKIN,
	IDR_GUIED_MAIN,
	IDR_GUIED_ACCELERATORS,
	IDR_GUIED_ITEM_POPUP,
	IDR_GUIED_VIEWERTOOLBAR,
	IDI_GUIED,
	IDI_GUIED_NAV_VISIBLE,
	IDI_GUIED_NAV_SCRIPTS,
	IDI_GUIED_NAV_VISIBLEDISABLED,
	IDI_GUIED_NAV_SCRIPTSHI,
	IDI_GUIED_NAV_COLLAPSE,
	IDI_GUIED_NAV_EXPAND,
	IDC_GUIED_OPTIONSTAB,
	IDC_GUIED_GRIDCOLOR,
	IDC_GUIED_SPACINGWIDTH,
	IDC_GUIED_SPACINGHEIGHT,
	IDC_GUIED_GRIDVISIBLE,
	IDC_GUIED_GRIDSNAP,
	IDC_GUIED_WINDOWTREE,
	IDC_GUIED_ITEMNAME,
	IDC_GUIED_ITEMBACKGROUND,
	IDC_GUIED_TYPE,
	IDC_GUIED_USEMATERIAL,
	IDC_GUIED_ITEMBORDERMATERIAL,
	IDC_GUIED_ITEMBACKCOLOR,
	IDC_GUIED_ITEMMATCOLOR,
	IDC_GUIED_ITEMFORECOLOR,
	IDC_GUIED_ITEMBORDERCOLOR,
	IDC_GUIED_ITEMBACKCOLORALPHA,
	IDC_GUIED_ITEMTEXT,
	IDC_GUIED_ITEMBORDERCOLORALPHA,
	IDC_GUIED_ITEMTEXTSCALE,
	IDC_GUIED_ITEMBORDERSIZE,
	IDC_GUIED_ITEMMATCOLORALPHA,
	IDC_GUIED_ITEMTEXTALIGNX,
	IDC_GUIED_ITEMTEXTALIGNY,
	IDC_GUIED_ITEMMATSCALEX,
	IDC_GUIED_ITEMMATSCALEY,
	IDC_GUIED_USEBACKCOLOR,
	IDC_GUIED_ITEMMATCOLORSTATIC,
	IDC_GUIED_ITEMKEYS,
	IDC_GUIED_ADDKEY,
	IDC_GUIED_EDITKEY,
	IDC_GUIED_DELETEKEY,
	IDC_GUIED_ITEMKEY,
	IDC_GUIED_ITEMVALUE,
	IDC_GUIED_ITEMVISIBLE,
	IDC_GUIED_SELECTIONCOLOR,
	IDC_GUIED_ALPHA,
	IDC_GUIED_ALPHASLIDER,
	IDC_GUIED_SCRIPT,
	IDC_GUIED_ADDVARIABLE,
	IDC_GUIED_EDITVARIABLE,
	IDC_GUIED_DELETEVARIABLE,
	IDC_GUIED_ITEMVARIABLES,
	IDC_GUIED_USETEXT,
	IDC_GUIED_ITEMTEXTALIGN,
	IDC_GUIED_STATIC_ALIGNMENT,
	IDC_GUIED_STATIC_X,
	IDC_GUIED_STATIC_Y,
	IDC_GUIED_ITEMFORECOLORALPHA,
	IDC_GUIED_STATIC_SCALE,
	IDC_GUIED_STATIC_COLOR,
	IDC_GUIED_USEBORDERCOLOR,
	IDC_GUIED_STATIC_FONT,
	IDC_GUIED_USEBORDERMATERIAL,
	IDC_GUIED_ITEMTEXTFONT,
	IDC_GUIED_STATIC_FONTSIZE,
	IDC_GUIED_ITEMVARIABLEBACKGROUND,
	IDC_GUIED_ITEMTEXTFONTSIZE,
	IDC_GUIED_XSCALE_STATIC,
	IDC_GUIED_YSCALE_STATIC,
	IDC_GUIED_ITEMNOEVENTS,
	IDC_GUIED_ITEMNOCLIP,
	IDC_GUIED_ITEMNOCLIP2,
	IDC_GUIED_ITEMNOCURSOR,
	IDC_GUIED_ITEMNOTIME,
	IDC_GUIED_ITEMRECTX,
	IDC_GUIED_ITEMRECTW,
	IDC_GUIED_IGNOREDESKTOP,
	IDC_GUIED_ITEMRECTH,
	IDC_GUIED_ITEMTEXTNOWRAP,
	IDC_GUIED_ITEMRECTY,
	IDC_GUIED_FILENAME,
	IDC_GUIED_COMMENT,
	IDC_GUIED_ITEMTEXTSCALE_SPIN,
	IDC_GUIED_HAND,
	ID_GUIED_NEW_HTMLDEF,
	ID_GUIED_ITEM_NEWSLIDERDEF,
	ID_GUIED_FILE_EXIT,
	ID_GUIED_HELP_ABOUT,
	ID_GUIED_WINDOW_CLOSEALL,
	ID_GUIED_FILE_NEW,
	ID_GUIED_FILE_OPEN,
	ID_GUIED_FILE_CLOSE,
	ID_GUIED_VIEW_ZOOMIN,
	ID_GUIED_VIEW_ZOOMOUT,
	ID_GUIED_VIEW_ZOOMFIT,
	ID_GUIED_FILE_SAVE,
	ID_GUIED_FILE_SAVEAS,
	ID_GUIED_VIEW_SHOWGRID,
	ID_GUIED_VIEW_SNAPTOGRID,
	ID_GUIED_VIEW_GRIDSETTINGS,
	ID_GUIED_EDIT_CUT,
	ID_GUIED_EDIT_COPY,
	ID_GUIED_EDIT_PASTE,
	ID_GUIED_VIEW_OPTIONS,
	ID_GUIED_EDIT_REDO,
	ID_GUIED_VIEW_HIDESELECTED,
	ID_GUIED_VIEW_SHOWHIDDEN,
	ID_GUIED_EDIT_UNDO,
	ID_GUIED_EDIT_DELETE,
	ID_GUIED_WINDOW_TILE,
	ID_GUIED_WINDOW_SHOWNAVIGATOR,
	ID_GUIED_FILE_MRU,
	ID_GUIED_ITEM_NEWEDITDEF,
	ID_GUIED_ITEM_NEWSCRIPTDEF,
	ID_GUIED_ITEM_PROPERTIES,
	ID_GUIED_ITEM_ARRANGEBRINGFORWARD,
	ID_GUIED_ITEM_ARRANGESENDTOBACK,
	ID_GUIED_ITEM_ARRANGESENDBACKWARD,
	ID_GUIED_ITEM_ARRANGEBRINGTOFRONT,
	ID_GUIED_ITEM_ALIGNLEFTS,
	ID_GUIED_ITEM_ALIGNCENTERS,
	ID_GUIED_ITEM_ALIGNRIGHTS,
	ID_GUIED_ITEM_ALIGNTOPS,
	ID_GUIED_ITEM_ALIGNMIDDLES,
	ID_GUIED_ITEM_ALIGNBOTTOMS,
	ID_GUIED_ITEM_MAKESAMESIZEWIDTH,
	ID_GUIED_ITEM_MAKESAMESIZEHEIGHT,
	ID_GUIED_ITEM_MAKESAMESIZEBOTH,
	ID_GUIED_ITEM_SCRIPTS,
	ID_GUIED_ITEM_ARRANGEMAKECHILD,
	ID_GUIED_TOOLS_VIEWER,
	ID_GUIED_WINDOW_CASCADE,
	ID_GUIED_ITEM_NEWWINDOWDEF,
	ID_GUIED_VIEW_UNHIDESELECTED,
	ID_GUIED_WINDOW_SHOWTRANSFORMER,
	ID_GUIED_TOOLS_RELOADMATERIALS,
	ID_GUIED_ITEM_NEWHTMLDEF,
	ID_GUIED_VIEW_STATUSBAR,
	ID_GUIED_ITEM_NEWCHOICEDEF,
	ID_GUIED_ITEM_NEWBINDDEF,
	ID_GUIED_ITEM_NEWLISTDEF,
	ID_GUIED_WINDOW_SHOWPROPERTIES,
	ID_GUIED_ITEM_NEWRENDERDEF,
	ID_GUIED_VIEWER_PAUSE,
	ID_GUIED_VIEWER_PLAY,
	ID_GUIED_VIEWER_START
};

const char* StringFromVec4( idVec4& vec );
bool		IsExpression( const char* s );

class rvGEApp : public wxFrame
{
public:
	rvGEApp();
	~rvGEApp() override;

	bool                Initialize();
	void                Shutdown();
	void                RunFrame();

	rvGEWorkspace*      GetActiveWorkspace();
	rvGEWorkspace*      NewWorkspace();
	bool                OpenFile( const char* filename );

	void                ActivateWorkspace( rvGEWorkspace* ws );

	rvGENavigator&		GetNavigator();
	rvGEProperties&		GetProperties();
	rvGETransformer&	GetTransformer();
	rvGEOptions&		GetOptions();

	wxAuiManager&       GetAui();
	wxAuiNotebook*      GetNotebook();
	rvGEViewer*         GetViewer();

	void                CloseViewer();

	bool                IsClosing() const
	{
		return m_closing;
	}

	int                 MessageBox( const wxString& text, long style = wxOK );

private:
	void OnFileNew( wxCommandEvent& e );
	void OnFileOpen( wxCommandEvent& e );
	void OnFileSave( wxCommandEvent& e );
	void OnFileSaveAs( wxCommandEvent& e );
	void OnFileClose( wxCommandEvent& e );
	void OnFileExit( wxCommandEvent& e );
	void OnFileRecent( wxCommandEvent& e );
	void OnEditUndo( wxCommandEvent& e );
	void OnEditRedo( wxCommandEvent& e );
	void OnEditCut( wxCommandEvent& e );
	void OnEditCopy( wxCommandEvent& e );
	void OnEditPaste( wxCommandEvent& e );
	void OnEditDelete( wxCommandEvent& e );
	void OnHelpAbout( wxCommandEvent& e );
	void OnViewOptions( wxCommandEvent& e );
	void OnViewToggleGrid( wxCommandEvent& e );
	void OnViewToggleSnap( wxCommandEvent& e );
	void OnViewZoomFit( wxCommandEvent& e );
	void OnViewStatusBar( wxCommandEvent& e );
	void OnToolsViewer( wxCommandEvent& e );
	void OnToolsReloadMaterials( wxCommandEvent& e );
	void OnUpdateUI( wxUpdateUIEvent& e );
	void OnClose( wxCloseEvent& e );

	void OnNotebookPageChanged( wxAuiNotebookEvent& e );
	void OnNotebookPageClose( wxAuiNotebookEvent& e );
	void OnNotebookPageClosed( wxAuiNotebookEvent& e );
	void OnNotebookButton( wxAuiNotebookEvent& e );

	void OnAuiPaneClose( wxAuiManagerEvent& e );
	void OnAuiPaneButton( wxAuiManagerEvent& e );

	void OnAutoHideTabClick( wxWindow* paneWindow );

private:
	void BuildMenuBar();
	void BuildAuiLayout();
	void BuildAutoHideStrips();
	void UpdateRecentFiles();

	void RebuildAutoHideStrips();

	AutoHideStrip* GetStripForPane( wxAuiPaneInfo& pane ) const;

private:
	rvGEOptions m_options;

	rvGENavigator m_navigator;
	rvGEProperties m_properties;
	rvGETransformer m_transformer;

	wxAuiManager m_aui;
	wxAuiNotebook* m_notebook;
	rvGEViewer* m_viewer;
	bool m_closing;

	AutoHideStrip* m_leftStrip;
	AutoHideStrip* m_rightStrip;
	AutoHideStrip* m_bottomStrip;

	wxMenu* m_recentMenu;

	wxDECLARE_EVENT_TABLE();
};

ID_INLINE rvGENavigator&   rvGEApp::GetNavigator()
{
	return m_navigator;
}
ID_INLINE rvGEProperties&  rvGEApp::GetProperties()
{
	return m_properties;
}
ID_INLINE rvGETransformer& rvGEApp::GetTransformer()
{
	return m_transformer;
}
ID_INLINE rvGEOptions& rvGEApp::GetOptions()
{
	return m_options;
}
ID_INLINE wxAuiManager& rvGEApp::GetAui()
{
	return m_aui;
}
ID_INLINE wxAuiNotebook* rvGEApp::GetNotebook()
{
	return m_notebook;
}
ID_INLINE rvGEViewer* rvGEApp::GetViewer()
{
	return m_viewer;
}

extern rvGEApp* gApp;

#endif /* !__GEAPP_H__ */