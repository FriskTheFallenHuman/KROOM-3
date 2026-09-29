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

#ifndef __GEVIEWER_H___
#define __GEVIEWER_H___

class idUserInterfaceLocal;

/*
================================================================================================

rvGEViewer

================================================================================================
*/
class rvGEViewer : public wxFrame
{
public:
	rvGEViewer( wxWindow* parent );
	~rvGEViewer() override;

	bool OpenFile( const char* filename );
	void RunFrame();

private:
	void OnPaint( wxPaintEvent& e );
	void OnSize( wxSizeEvent& e );
	void OnErase( wxEraseEvent& e ) {}
	void OnMouse( wxMouseEvent& e );
	void OnCaptureLost( wxMouseCaptureLostEvent& e );
	void OnKey( wxKeyEvent& e );
	void OnChar( wxKeyEvent& e );
	void OnClose( wxCloseEvent& e );
	void OnPlay( wxCommandEvent& e );
	void OnPause( wxCommandEvent& e );
	void OnStart( wxCommandEvent& e );
	void OnToolbarRightClick( wxMouseEvent& e );

	void Render();
	void SetupRenderView( renderView_t& rv );

	void RenderGuiIntoFBO( int w, int h );

	void EnsurePreviewFBO( int w, int h );
	void DestroyPreviewFBO();

	void Play();
	void Pause();

	wxPanel* m_canvas;
	wxToolBar* m_toolbar;
	rvToolbarImageStrip m_toolbarIcons;

	idUserInterfaceLocal* m_interface;

	bool m_paused;
	int m_lastTime;
	int m_time;

	unsigned int m_fbo;
	unsigned int m_colorTex;
	unsigned int m_depthStencil;
	int m_fboWidth;
	int m_fboHeight;

	std::vector<unsigned char> m_pixels;
};

#endif /* !__GEVIEWER_H__ */