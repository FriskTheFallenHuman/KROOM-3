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

#ifndef __GEWORKSPACE_H__
#define __GEWORKSPACE_H__

#ifndef __GESELECTIONMGR_H__
	#include "GESelectionMgr.h"
#endif

#ifndef __GEMODIFIERSTACK_H__
	#include "GEModifierStack.h"
#endif

class rvGEApp;
class idUserInterfaceLocal;

class rvGEClipboardItem
{
public:
	idDict mStateDict;
	idDict mScriptDict;
	idDict mVarDict;
};

class rvGEWorkspace : public wxPanel
{
public:
	enum EZoomLevel
	{
		ZOOM_MIN, ZOOM_25, ZOOM_33, ZOOM_50, ZOOM_66,
		ZOOM_100, ZOOM_150, ZOOM_200, ZOOM_300, ZOOM_MAX
	};

	enum EItemAlign
	{
		ALIGN_LEFTS, ALIGN_CENTERS, ALIGN_RIGHTS,
		ALIGN_TOPS, ALIGN_MIDDLES, ALIGN_BOTTOMS
	};

	enum EModifierType
	{
		MOD_UNKNOWN,
		MOD_DELETE,
		MOD_HIDE,
		MOD_UNHIDE,
		MOD_SHOWHIDDEN,
		MOD_SEND_BACKWARD,
		MOD_BRING_FORWARD,
		MOD_SEND_BACK,
		MOD_BRING_FRONT
	};

	rvGEWorkspace( rvGEApp* app, wxWindow* parent );
	~rvGEWorkspace() override;

	bool NewFile();
	bool LoadFile( const char* filename, wxString* error = nullptr );
	bool SaveFile( const char* filename );

	const char* GetFilename() const
	{
		return m_filename.c_str();
	}
	bool IsModified() const
	{
		return m_modified;
	}
	bool IsNew() const
	{
		return m_new;
	}

	void SetModified( bool mod );
	void UpdateTitle();

	void RunFrame();
	void Render();

	void HideSelected();
	void UnhideSelected();
	void DeleteSelected();
	bool EditSelectedProperties();
	bool EditSelectedScripts();
	void BringSelectedForward();
	void BringSelectedToFront();
	void SendSelectedBackward();
	void SendSelectedToBack();
	void AlignSelected( EItemAlign align );
	void MakeSelectedSameSize( bool width, bool height );
	void MakeSelectedAChild();
	void ShowHidden();

	void HideWindow( idWindow* window );
	void UnhideWindow( idWindow* window );

	void Copy();
	void Paste();

	EZoomLevel ZoomIn();
	EZoomLevel ZoomOut();
	EZoomLevel ZoomFit();

	EZoomLevel GetZoom() const
	{
		return ( EZoomLevel )m_zoom;
	}
	float GetZoomScale() const;

	rvGEApp* GetApplication();
	idUserInterfaceLocal* GetInterface();
	rvGEModifierStack& GetModifierStack();
	rvGESelectionMgr& GetSelectionMgr();
	idList<rvGEClipboardItem*> GetClipboard() const;
	wxWindow* GetWindow();

	idVec2& WindowToWorkspace( idVec2& point );
	idVec2& WorkspaceToWindow( idVec2& point );
	idRectangle& WindowToWorkspace( idRectangle& rect );
	idRectangle& WorkspaceToWindow( idRectangle& rect );

	void AddModifierMove( const char* modName, float x, float y, bool snap );
	void AddModifierSize( const char* modName, float l, float t, float r, float b, bool snap );

	idWindow* AddWindow( rvGEWindowWrapper::EWindowType type );

private:
	void OnPaint( wxPaintEvent& e );
	void OnSize( wxSizeEvent& e );
	void OnErase( wxEraseEvent& e ) {}
	void OnMouse( wxMouseEvent& e );
	void OnMouseMove( wxMouseEvent& e );
	void OnWheel( wxMouseEvent& e );
	void OnKey( wxKeyEvent& e );
	void OnCaptureLost( wxMouseCaptureLostEvent& e );

	int HandleLButtonDown( wxMouseEvent& e );
	int HandleLButtonUp( wxMouseEvent& e );
	int HandleLButtonDblClk( wxMouseEvent& e );
	int HandleMButtonDown( wxMouseEvent& e );
	int HandleMButtonUp( wxMouseEvent& e );
	int HandleRButtonDown( wxMouseEvent& e );
	int HandleMouseMove( wxMouseEvent& e );
	int HandleKeyDown( wxKeyEvent& e );

	void OnSelectMenu( wxCommandEvent& e );
	void OnNewWindowDef( wxCommandEvent& e );
	void OnNewEditDef( wxCommandEvent& e );
	void OnEditProps( wxCommandEvent& e );
	void OnEditScripts( wxCommandEvent& e );

	void UpdateCursor();
	void UpdateCursor( float x, float y );
	void UpdateCursor( rvGESelectionMgr::EHitTest type );

	void UpdateScrollbars();
	void UpdateRectangle( bool useScroll = true );
	void Scroll( int scrollbar, int offset );
	void HandleScrollPan( const idVec2& cursor );

	void DrawOverlayDC( wxDC& dc );

	void EnsurePreviewFBO( int w, int h );
	void DestroyPreviewFBO();

	void RenderGuiIntoCurrentFBO( int w, int h );

	void AddModifiers( EModifierType type, ... );
	void AddModifiers( idWindow* window, EModifierType type, ... );
	rvGEModifier* CreateModifier( EModifierType type, idWindow* window, va_list args );
	void AddModifierMoveNudge( float x, float y, bool snap )
	{
		AddModifierMove( "Nudge Move", x, y, snap );
	}
	void AddModifierSizeNudge( float w, float h, bool snap )
	{
		AddModifierSize( "Nudge Size", 0, 0, w, h, snap );
	}
	void AddModifierShowAll();

	idWindow* NewWindow( idDict* state, rvGEWindowWrapper::EWindowType type );

	static bool CleanupEnumProc( rvGEWindowWrapper* wrapper, void* data );
	static bool ShowAllEnumProc( rvGEWindowWrapper* wrapper, void* data );
	static bool BuildSelectMenuEnumProc( rvGEWindowWrapper* wrapper, void* data );

	void WriteTabs( idFile* file, int depth );
	bool WriteWindow( idFile* file, int depth, idWindow* window );

	rvGEApp* m_app;

	idUserInterfaceLocal* m_interface;
	idRenderWorld* m_renderWorld;

	int m_zoom;
	bool m_zoomIsFit;
	idRectangle m_rect;
	int m_windowWidth;
	int m_windowHeight;

	bool m_scrollHorz;
	bool m_scrollVert;

	idStr m_filename;

	rvGEModifierStack m_modifiers;
	rvGESelectionMgr m_selections;

	rvGESelectionMgr::EHitTest m_dragType;
	idVec2 m_dragPoint;
	int m_dragTime;
	bool m_dragX;
	bool m_dragY;
	bool m_dragScroll;

	bool m_modified;
	bool m_new;

	wxCursor m_handCursor;

	static idList<rvGEClipboardItem*> m_clipboard;

	idList<idWindow*> m_selectMenu;
	idVec2 m_selectMenuPos;

	bool m_suppressSelectionUpdate;

	unsigned int m_fbo;
	unsigned int m_colorTex;
	unsigned int m_depthStencil;
	int m_fboWidth;
	int m_fboHeight;

	std::vector<unsigned char> m_pixels;
};

ID_INLINE rvGEApp* rvGEWorkspace::GetApplication()
{
	return m_app;
}

ID_INLINE idUserInterfaceLocal* rvGEWorkspace::GetInterface()
{
	return m_interface;
}

ID_INLINE rvGEModifierStack& rvGEWorkspace::GetModifierStack()
{
	return m_modifiers;
}

ID_INLINE rvGESelectionMgr& rvGEWorkspace::GetSelectionMgr()
{
	return m_selections;
}

ID_INLINE idList<rvGEClipboardItem*> rvGEWorkspace::GetClipboard() const
{
	return m_clipboard;
}

ID_INLINE wxWindow* rvGEWorkspace::GetWindow()
{
	return this;
}

#endif /* !__GEWORKSPACE_H__ */