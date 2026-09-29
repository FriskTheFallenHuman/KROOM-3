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

#include "precompiled.h"
#pragma hdrstop

#include "GEApp.h"

/*
================
rvGEViewer::rvGEViewer

Builds the viewer frame, toolbar and canvas, wires input events.
================
*/
rvGEViewer::rvGEViewer( wxWindow* parent )
	: wxFrame( parent, wxID_ANY, "GUI Viewer", wxDefaultPosition, wxSize( 800, 600 ), wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT )
	, m_canvas( NULL )
	, m_toolbar( NULL )
	, m_interface( NULL )
	, m_paused( true )
	, m_lastTime( 0 )
	, m_time( 0 )
	, m_fbo( 0 )
	, m_colorTex( 0 )
	, m_depthStencil( 0 )
	, m_fboWidth( 0 )
	, m_fboHeight( 0 )
{
	m_canvas = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( 640, 480 ), wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS );
	m_canvas->SetBackgroundStyle( wxBG_STYLE_PAINT );

	const bool loaded = m_toolbarIcons.Load( "editors/gfx/guied_viewer_toolbar.bmp", 16, 16 );
	if( !loaded )
	{
		common->FatalError( "%s: Couldn't load 'editors/gfx/guied_viewer_toolbar.bmp' aborting..." __FUNCTION__ );
		return;
	}

	m_toolbar = CreateToolBar( wxTB_FLAT | wxTB_HORIZONTAL | ( loaded ? 0 : wxTB_TEXT ) );
	m_toolbar->SetToolBitmapSize( wxSize( 16, 16 ) );

	m_toolbar->AddTool( ID_GUIED_VIEWER_PLAY, "Play", m_toolbarIcons.GetIcon( 0 ), "Play" );
	m_toolbar->AddTool( ID_GUIED_VIEWER_PAUSE, "Pause", m_toolbarIcons.GetIcon( 1 ), "Pause" );
	m_toolbar->AddTool( ID_GUIED_VIEWER_START, "Start", m_toolbarIcons.GetIcon( 2 ), "Start" );

	m_toolbar->Realize();

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_canvas, 1, wxEXPAND );
	SetSizer( sizer );

	m_canvas->Bind( wxEVT_PAINT, &rvGEViewer::OnPaint, this );
	m_canvas->Bind( wxEVT_SIZE, &rvGEViewer::OnSize, this );
	m_canvas->Bind( wxEVT_ERASE_BACKGROUND, &rvGEViewer::OnErase, this );
	m_canvas->Bind( wxEVT_LEFT_DOWN, &rvGEViewer::OnMouse, this );
	m_canvas->Bind( wxEVT_LEFT_UP, &rvGEViewer::OnMouse, this );
	m_canvas->Bind( wxEVT_RIGHT_DOWN, &rvGEViewer::OnMouse, this );
	m_canvas->Bind( wxEVT_RIGHT_UP, &rvGEViewer::OnMouse, this );
	m_canvas->Bind( wxEVT_MIDDLE_DOWN, &rvGEViewer::OnMouse, this );
	m_canvas->Bind( wxEVT_MIDDLE_UP, &rvGEViewer::OnMouse, this );
	m_canvas->Bind( wxEVT_MOTION, &rvGEViewer::OnMouse, this );
	m_canvas->Bind( wxEVT_MOUSEWHEEL, &rvGEViewer::OnMouse, this );
	m_canvas->Bind( wxEVT_KEY_DOWN, &rvGEViewer::OnKey, this );
	m_canvas->Bind( wxEVT_KEY_UP, &rvGEViewer::OnKey, this );
	m_canvas->Bind( wxEVT_CHAR, &rvGEViewer::OnChar, this );
	m_canvas->Bind( wxEVT_MOUSE_CAPTURE_LOST, &rvGEViewer::OnCaptureLost, this );

#ifdef __WXMSW__
	wxImage blankCursorImg( 1, 1 );
	blankCursorImg.SetAlpha();
	blankCursorImg.SetAlpha( 0, 0, 0 );
	m_canvas->SetCursor( wxCursor( blankCursorImg ) );
#else
	m_canvas->SetCursor( wxCursor( wxCURSOR_BLANK ) );
#endif

	Bind( wxEVT_CLOSE_WINDOW, &rvGEViewer::OnClose, this );
	Bind( wxEVT_MENU, &rvGEViewer::OnPlay, this, ID_GUIED_VIEWER_PLAY );
	Bind( wxEVT_MENU, &rvGEViewer::OnPause, this, ID_GUIED_VIEWER_PAUSE );
	Bind( wxEVT_MENU, &rvGEViewer::OnStart, this, ID_GUIED_VIEWER_START );

	m_canvas->SetFocus();

	CentreOnParent();

	if( gApp )
	{
		gApp->GetOptions().GetWindowPlacement( "viewer", this );
	}
}

/*
================
rvGEViewer::~rvGEViewer

Releases the interface and destroys the offscreen FBO.
================
*/
rvGEViewer::~rvGEViewer()
{
	delete m_interface;
	m_interface = NULL;

	if( m_fbo || m_colorTex || m_depthStencil )
	{
		rRenderContext.MakeCurrent();
		DestroyPreviewFBO();
	}
}

/*
================
rvGEViewer::OpenFile

Copies the .gui into a temp file, parses it, and starts playback.
================
*/
bool rvGEViewer::OpenFile( const char* filename )
{
	if( !filename || !*filename )
	{
		return false;
	}

	delete m_interface;
	m_interface = NULL;

	idStr tempfile = filename;
	tempfile.StripPath();
	tempfile.StripFileExtension();
	tempfile = va( "guis/temp.guied", tempfile.c_str() );

	idStr ospath = fileSystem->RelativePathToOSPath( tempfile, "fs_savepath" );

	idStr createDir = ospath;
	createDir.StripFilename();
	fileSystem->CreateOSPath( createDir );

	const wxString src = wxString::FromUTF8( filename );
	const wxString dst = wxString::FromUTF8( ospath.c_str() );

	if( !wxCopyFile( src, dst, true ) )
	{
		wxMessageBox( wxString::Format( "Could not copy '%s' into the save path.", filename ), "GUI Viewer", wxOK | wxICON_ERROR, this );
		return false;
	}

	m_interface = reinterpret_cast<idUserInterfaceLocal*>( uiManager->FindGui( tempfile, true, true ) );

	if( !m_interface )
	{
		wxRemoveFile( dst );
		wxMessageBox( wxString::Format( "Failed to parse '%s'.", filename ), "GUI Viewer", wxOK | wxICON_ERROR, this );
		return false;
	}

	m_interface->SetStateString( "guied_item_0", "guied 1" );
	m_interface->SetStateString( "guied_item_1", "guied 2" );
	m_interface->SetStateString( "guied_item_2", "guied 3" );

	if( idWindow* d = m_interface->GetDesktop() )
	{
		if( !( d->GetFlags() & WIN_MENUGUI ) )
		{
			d->SetFlag( WIN_NOCURSOR );
		}
	}

	m_time = 0;
	m_interface->Activate( true, m_time );

	wxRemoveFile( dst );

	Play();
	return true;
}

/*
================
rvGEViewer::RunFrame

Repaints only while playing.
================
*/
void rvGEViewer::RunFrame()
{
	if( m_interface && !m_paused )
	{
		m_canvas->Refresh( false );
	}
}

/*
================
rvGEViewer::Play
================
*/
void rvGEViewer::Play()
{
	if( !m_paused )
	{
		return;
	}

	m_lastTime = eventLoop->Milliseconds();
	m_paused   = false;

	if( m_toolbar )
	{
		m_toolbar->SetToolShortHelp( ID_GUIED_VIEWER_PAUSE, "Pause" );
	}
}

/*
================
rvGEViewer::Pause
================
*/
void rvGEViewer::Pause()
{
	if( m_paused )
	{
		return;
	}

	m_paused = true;

	if( m_toolbar )
	{
		m_toolbar->SetToolShortHelp( ID_GUIED_VIEWER_PLAY, "Play" );
	}
}

/*
================
rvGEViewer::OnPlay
================
*/
void rvGEViewer::OnPlay( wxCommandEvent& )
{
	Play();
}

/*
================
rvGEViewer::OnPause
================
*/
void rvGEViewer::OnPause( wxCommandEvent& )
{
	Pause();
}

/*
================
rvGEViewer::OnStart
================
*/
void rvGEViewer::OnStart( wxCommandEvent& )
{
	Play();
}

/*
================
rvGEViewer::OnSize
================
*/
void rvGEViewer::OnSize( wxSizeEvent& e )
{
	m_canvas->Refresh( false );
	e.Skip();
}

/*
================
rvGEViewer::OnPaint

Renders the GUI into an offscreen FBO and blits it to the canvas.
================
*/
void rvGEViewer::OnPaint( wxPaintEvent& )
{
	static bool s_inRender = false;
	if( s_inRender )
	{
		return;
	}

	wxAutoBufferedPaintDC dc( m_canvas );

	const wxSize size = m_canvas->GetClientSize();
	const int w = wxMax( 1, size.x );
	const int h = wxMax( 1, size.y );

	if( !m_interface )
	{
		dc.SetBackground( wxBrush( wxColour( 0, 0, 0 ) ) );
		dc.Clear();
		return;
	}

	s_inRender = true;

	rRenderContext.MakeCurrent();
	EnsurePreviewFBO( w, h );

	if( w != m_fboWidth || h != m_fboHeight )
	{
		s_inRender = false;
		return;
	}

	glBindFramebuffer( GL_FRAMEBUFFER, m_fbo );
	glViewport( 0, 0, w, h );
	glScissor( 0, 0, w, h );
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	Framebuffer::SetEditorDefaultFramebuffer( m_fbo );

	RenderGuiIntoFBO( w, h );

	Framebuffer::SetEditorDefaultFramebuffer( 0 );
	glBindFramebuffer( GL_FRAMEBUFFER, m_fbo );

	glReadBuffer( GL_COLOR_ATTACHMENT0 );
	glPixelStorei( GL_PACK_ALIGNMENT, 1 );
	glReadPixels( 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels.data() );

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	wxImage img( w, h );
	unsigned char* dst = img.GetData();

	for( int y = 0; y < h; ++y )
	{
		const unsigned char* srcRow = m_pixels.data() + size_t( h - 1 - y ) * size_t( w ) * 4u;
		unsigned char* dstRow = dst + size_t( y ) * size_t( w ) * 3u;

		for( int x = 0; x < w; ++x )
		{
			dstRow[x * 3 + 0] = srcRow[x * 4 + 0];
			dstRow[x * 3 + 1] = srcRow[x * 4 + 1];
			dstRow[x * 3 + 2] = srcRow[x * 4 + 2];
		}
	}

	dc.DrawBitmap( wxBitmap( img ), 0, 0, false );

	s_inRender = false;
}

/*
================
rvGEViewer::SetupRenderView

Zeroes a renderView_t and fills in sane editor-preview defaults.
================
*/
void rvGEViewer::SetupRenderView( renderView_t& rv )
{
	memset( &rv, 0, sizeof( rv ) );
	rv.viewID = 0;
	rv.vieworg = vec3_origin;
	rv.viewaxis = mat3_identity;
	rv.fov_x = 90.0f;
	rv.fov_y = 90.0f;
	rv.time[0] = m_time;
	rv.time[1] = m_time;
	rv.rdflags = RDF_EDITOR_PREVIEW;
}

/*
================
rvGEViewer::RenderGuiIntoFBO

Advances playback time (when unpaused) and redraws the interface into
the currently bound FBO.
================
*/
void rvGEViewer::RenderGuiIntoFBO( int w, int h )
{
	if( !m_interface )
	{
		return;
	}

	if( !m_paused )
	{
		const int now = eventLoop->Milliseconds();
		m_time += now - m_lastTime;
		m_lastTime = now;
	}

	const float savedShaderTime = tr.frameShaderTime;
	tr.frameShaderTime = m_time * 0.001f;

	tr.guiModel->Clear();

	renderSystem->CropRenderSize( w, h );

	m_interface->Redraw( 0, false );

	tr.guiModel->EmitFullScreen();
	tr.guiModel->Clear();

	renderSystem->UnCrop();

	const emptyCommand_t* cmd = renderSystem->SwapCommandBuffers_FinishCommandBuffers();
	if( cmd )
	{
		renderSystem->RenderCommandBuffers( cmd );
	}

	tr.frameShaderTime = savedShaderTime;
}

/*
================
rvGEViewer::Render
================
*/
void rvGEViewer::Render()
{
	if( m_canvas )
	{
		m_canvas->Refresh( false );
	}
}

/*
================
rvGEViewer::EnsurePreviewFBO

Lazily creates or resizes the offscreen framebuffer.
================
*/
void rvGEViewer::EnsurePreviewFBO( int w, int h )
{
	if( m_fbo && w == m_fboWidth && h == m_fboHeight )
	{
		return;
	}

	if( !m_fbo )
	{
		glGenFramebuffers( 1, &m_fbo );
		glGenTextures( 1, &m_colorTex );
		glGenRenderbuffers( 1, &m_depthStencil );
	}

	glBindTexture( GL_TEXTURE_2D, m_colorTex );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	glBindRenderbuffer( GL_RENDERBUFFER, m_depthStencil );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h );

	glBindFramebuffer( GL_FRAMEBUFFER, m_fbo );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0 );
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthStencil );

	const GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
	if( status != GL_FRAMEBUFFER_COMPLETE )
	{
		common->Warning( "rvGEViewer: FBO incomplete: 0x%x", status );
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		glBindRenderbuffer( GL_RENDERBUFFER, 0 );
		return;
	}

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glBindRenderbuffer( GL_RENDERBUFFER, 0 );

	m_fboWidth  = w;
	m_fboHeight = h;

	m_pixels.assign( size_t( w ) * size_t( h ) * 4u, 0 );
}

/*
================
rvGEViewer::DestroyPreviewFBO
================
*/
void rvGEViewer::DestroyPreviewFBO()
{
	if( m_fbo )
	{
		glDeleteFramebuffers( 1, &m_fbo );
		m_fbo = 0;
	}

	if( m_colorTex )
	{
		glDeleteTextures( 1, &m_colorTex );
		m_colorTex = 0;
	}

	if( m_depthStencil )
	{
		glDeleteRenderbuffers( 1, &m_depthStencil );
		m_depthStencil = 0;
	}

	m_fboWidth = 0;
	m_fboHeight = 0;
}

/*
================
rvGEViewer::OnMouse

Translates wx mouse events into sysEvent_t and forwards them to the UI.
================
*/
void rvGEViewer::OnMouse( wxMouseEvent& e )
{
	if( !m_interface )
	{
		return;
	}

	sysEvent_t ev;
	memset( &ev, 0, sizeof( ev ) );
	bool visuals = false;

	if( m_canvas )
	{
		const wxSize cs = m_canvas->GetClientSize();
		const float vx = idMath::ClampFloat( 0.0f, 640.0f, e.GetX() * 640.0f / float( wxMax( 1, cs.x ) ) );
		const float vy = idMath::ClampFloat( 0.0f, 480.0f, e.GetY() * 480.0f / float( wxMax( 1, cs.y ) ) );
		m_interface->SetCursor( vx, vy );
	}

	if( e.LeftDown() || e.RightDown() || e.MiddleDown() )
	{
		if( m_canvas && !m_canvas->HasCapture() )
		{
			m_canvas->CaptureMouse();
		}

		int key = K_MOUSE1;
		if( e.RightDown() )
		{
			key = K_MOUSE2;
		}

		if( e.MiddleDown() )
		{
			key = K_MOUSE3;
		}

		ev.evType = SE_KEY;
		ev.evValue = key;
		ev.evValue2 = 1;
		m_interface->HandleEvent( &ev, m_time, &visuals );
	}
	else if( e.LeftUp() || e.RightUp() || e.MiddleUp() )
	{
		int key = K_MOUSE1;
		if( e.RightUp() )
		{
			key = K_MOUSE2;
		}

		if( e.MiddleUp() )
		{
			key = K_MOUSE3;
		}

		ev.evType = SE_KEY;
		ev.evValue = key;
		ev.evValue2 = 0;
		m_interface->HandleEvent( &ev, m_time, &visuals );

		if( m_canvas && m_canvas->HasCapture() )
		{
			m_canvas->ReleaseMouse();
		}
	}
	else if( e.Moving() || e.Dragging() )
	{
		ev.evType = SE_MOUSE;
		ev.evValue = 0;
		ev.evValue2 = 0;
		m_interface->HandleEvent( &ev, m_time, &visuals );
	}
	else if( e.GetWheelRotation() != 0 )
	{
		ev.evType = SE_KEY;
		ev.evValue = e.GetWheelRotation() > 0 ? K_MWHEELUP : K_MWHEELDOWN;
		ev.evValue2 = 1;
		m_interface->HandleEvent( &ev, m_time, &visuals );
	}

	if( m_canvas )
	{
		m_canvas->Refresh( false );
	}
}

/*
================
rvGEViewer::OnCaptureLost

Releases any pressed mouse buttons on the UI.
================
*/
void rvGEViewer::OnCaptureLost( wxMouseCaptureLostEvent& )
{
	if( m_interface )
	{
		sysEvent_t ev;
		memset( &ev, 0, sizeof( ev ) );

		bool visuals = false;

		for( int i = 0; i < 3; ++i )
		{
			ev.evType = SE_KEY;
			ev.evValue = K_MOUSE1 + i;
			ev.evValue2 = 0;
			m_interface->HandleEvent( &ev, m_time, &visuals );
		}
	}
}

/*
================
wxKeyToDoomKey

Maps a wxWidgets key code to the corresponding Doom K_* constant.
================
*/
static int wxKeyToDoomKey( int wxKey )
{
	switch( wxKey )
	{
		case WXK_ESCAPE:
			return K_ESCAPE;
		case WXK_RETURN:
			return K_ENTER;
		case WXK_BACK:
			return K_BACKSPACE;
		case WXK_TAB:
			return K_TAB;
		case WXK_SPACE:
			return K_SPACE;
		case WXK_LEFT:
			return K_LEFTARROW;
		case WXK_RIGHT:
			return K_RIGHTARROW;
		case WXK_UP:
			return K_UPARROW;
		case WXK_DOWN:
			return K_DOWNARROW;
		case WXK_HOME:
			return K_HOME;
		case WXK_END:
			return K_END;
		case WXK_PAGEUP:
			return K_PGUP;
		case WXK_PAGEDOWN:
			return K_PGDN;
		case WXK_INSERT:
			return K_INS;
		case WXK_DELETE:
			return K_DEL;
		default:
			break;
	}

	if( wxKey >= 'A' && wxKey <= 'Z' )
	{
		return K_A + ( wxKey - 'A' );
	}
	if( wxKey >= '0' && wxKey <= '9' )
	{
		return K_1 + ( wxKey - '1' + ( wxKey == '0' ? 9 : 0 ) );
	}
	return 0;
}

/*
================
rvGEViewer::OnKey
================
*/
void rvGEViewer::OnKey( wxKeyEvent& e )
{
	if( !m_interface )
	{
		return;
	}

	const int key = wxKeyToDoomKey( e.GetKeyCode() );
	if( key == 0 )
	{
		e.Skip();
		return;
	}

	sysEvent_t ev;
	memset( &ev, 0, sizeof( ev ) );
	ev.evType = SE_KEY;
	ev.evValue = key;
	ev.evValue2 = ( e.GetEventType() == wxEVT_KEY_DOWN ) ? 1 : 0;

	bool visuals = false;
	m_interface->HandleEvent( &ev, m_time, &visuals );

	e.Skip();
}

/*
================
rvGEViewer::OnChar

Escape closes the viewer; all other input is forwarded as SE_CHAR.
================
*/
void rvGEViewer::OnChar( wxKeyEvent& e )
{
	if( e.GetKeyCode() == WXK_ESCAPE )
	{
		Close( true );
		return;
	}

	if( !m_interface )
	{
		return;
	}

	sysEvent_t ev;
	memset( &ev, 0, sizeof( ev ) );
	ev.evType = SE_CHAR;
	ev.evValue = e.GetUnicodeKey() ? e.GetUnicodeKey() : e.GetKeyCode();
	ev.evValue2 = 0;

	bool visuals = false;
	m_interface->HandleEvent( &ev, m_time, &visuals );
}

/*
================
rvGEViewer::OnClose

Persists placement and notifies the app so it clears its pointer.
================
*/
void rvGEViewer::OnClose( wxCloseEvent& e )
{
	if( gApp )
	{
		gApp->GetOptions().SetWindowPlacement( "viewer", this );
		gApp->CloseViewer();
	}
	e.Skip();
}

/*
================
rvGEViewer::OnToolbarRightClick
================
*/
void rvGEViewer::OnToolbarRightClick( wxMouseEvent& )
{
	/* reserved */
}