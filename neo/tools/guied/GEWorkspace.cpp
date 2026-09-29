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

idList<rvGEClipboardItem*> rvGEWorkspace::m_clipboard;

static float g_zoomScales[rvGEWorkspace::ZOOM_MAX] =
{
	0.0f, 0.25f, 0.33f, 0.50f, 0.66f, 1.0f, 1.5f, 2.0f, 3.0f
};

/*
================
rvGEWorkspace::rvGEWorkspace

Builds the drawing surface panel and wires all mouse/keyboard events.
================
*/
rvGEWorkspace::rvGEWorkspace( rvGEApp* app, wxWindow* parent )
	: wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS )
	, m_app( app )
	, m_interface( NULL )
	, m_renderWorld( NULL )
	, m_zoom( ZOOM_100 )
	, m_zoomIsFit( false )
	, m_windowWidth( 640 )
	, m_windowHeight( 480 )
	, m_scrollHorz( false )
	, m_scrollVert( false )
	, m_filename( "guis/Untitled.gui" )
	, m_dragType( rvGESelectionMgr::HT_NONE )
	, m_dragTime( 0 )
	, m_dragX( true )
	, m_dragY( true )
	, m_dragScroll( false )
	, m_modified( false )
	, m_new( false )
	, m_suppressSelectionUpdate( false )
	, m_fbo( 0 )
	, m_colorTex( 0 )
	, m_depthStencil( 0 )
	, m_fboWidth( 0 )
	, m_fboHeight( 0 )
{
	SetBackgroundStyle( wxBG_STYLE_PAINT );

	m_selections.SetWorkspace( this );
	m_handCursor = wxCursor( wxCURSOR_HAND );

	Bind( wxEVT_PAINT, &rvGEWorkspace::OnPaint, this );
	Bind( wxEVT_SIZE, &rvGEWorkspace::OnSize, this );
	Bind( wxEVT_ERASE_BACKGROUND, &rvGEWorkspace::OnErase, this );
	Bind( wxEVT_LEFT_DOWN, &rvGEWorkspace::OnMouse, this );
	Bind( wxEVT_LEFT_UP, &rvGEWorkspace::OnMouse, this );
	Bind( wxEVT_LEFT_DCLICK, &rvGEWorkspace::OnMouse, this );
	Bind( wxEVT_RIGHT_DOWN, &rvGEWorkspace::OnMouse, this );
	Bind( wxEVT_MIDDLE_DOWN, &rvGEWorkspace::OnMouse, this );
	Bind( wxEVT_MIDDLE_UP, &rvGEWorkspace::OnMouse, this );
	Bind( wxEVT_MOTION, &rvGEWorkspace::OnMouseMove, this );
	Bind( wxEVT_MOUSEWHEEL, &rvGEWorkspace::OnWheel, this );
	Bind( wxEVT_KEY_DOWN, &rvGEWorkspace::OnKey, this );
	Bind( wxEVT_MOUSE_CAPTURE_LOST, &rvGEWorkspace::OnCaptureLost, this );
}

/*
================
rvGEWorkspace::~rvGEWorkspace

Deletes wrappers under the desktop and releases the FBO.
================
*/
rvGEWorkspace::~rvGEWorkspace()
{
	if( m_interface && m_interface->GetDesktop() )
	{
		rvGEWindowWrapper* desktop =
			rvGEWindowWrapper::GetWrapper( m_interface->GetDesktop() );
		if( desktop )
		{
			desktop->EnumChildren( CleanupEnumProc, NULL );
		}
	}

	delete m_interface;
	m_interface = NULL;
	m_renderWorld = NULL;

	if( m_fbo || m_colorTex || m_depthStencil )
	{
		rRenderContext.MakeCurrent();
		DestroyPreviewFBO();
	}
}

/*
================
rvGEWorkspace::NewFile

Creates a fresh Untitled.gui on disk and loads it.
================
*/
bool rvGEWorkspace::NewFile()
{
	wxBusyCursor wait;

	idStr ospath = fileSystem->RelativePathToOSPath( "guis/Untitled.guiednew", "fs_savepath" );
	const wxString ospathW = wxString::FromUTF8( ospath.c_str() );
	wxRemoveFile( ospathW );

	idFile* file = fileSystem->OpenFileWrite( "guis/Untitled.guiednew" );
	if( !file )
	{
		return false;
	}

	const char* empty = "windowDef Desktop { rect 0,0,640,480 }";
	file->Write( empty, strlen( empty ) );
	fileSystem->CloseFile( file );

	if( !LoadFile( ospath, NULL ) )
	{
		wxRemoveFile( ospathW );
		return false;
	}

	m_new = true;
	wxRemoveFile( ospathW );

	// Reset the filename to a .gui.
	idStr name = ospath;
	name.StripFileExtension();
	name.Append( ".gui" );
	m_filename = name;

	return true;
}

/*
================
rvGEWorkspace::LoadFile

Copies the source .gui into temp.guied and parses it.
================
*/
bool rvGEWorkspace::LoadFile( const char* filename, wxString* error )
{
	if( !filename || !*filename )
	{
		if( error )
		{
			*error = "Empty filename";
		}
		return false;
	}

	delete m_interface;
	m_interface = NULL;
	m_renderWorld = NULL;

	idStr ospath = fileSystem->RelativePathToOSPath( "guis/temp.guied", "fs_savepath" );
	idStr createDir = ospath;
	createDir.StripFilename();
	fileSystem->CreateOSPath( createDir );

	const wxString src = wxString::FromUTF8( filename );
	const wxString dst = wxString::FromUTF8( ospath.c_str() );
	if( !wxCopyFile( src, dst, true ) )
	{
		if( error )
		{
			*error = wxString::Format( "File not found: %s", filename );
		}
		return false;
	}

	m_filename = filename;

	try
	{
		m_interface = reinterpret_cast<idUserInterfaceLocal*>( uiManager->FindGui( "guis/temp.guied", true, true ) );
	}
	catch( idException& e )
	{
		if( error )
		{
			*error = e.GetError();
		}
		return false;
	}

	if( !m_interface )
	{
		if( error )
		{
			*error = "Failed to parse file";
		}
		return false;
	}

	// Expand the desktop so its children show up in the navigator.
	rvGEWindowWrapper* desktop = rvGEWindowWrapper::GetWrapper( m_interface->GetDesktop() );
	if( desktop )
	{
		desktop->Expand();
	}

	m_interface->Activate( true, 0 );

	UpdateScrollbars();

	ZoomFit();

	UpdateTitle();

	return true;
}

/*
================
rvGEWorkspace::SaveFile

Serialises the current interface back to disk.
================
*/
bool rvGEWorkspace::SaveFile( const char* filename )
{
	if( !filename || !*filename )
	{
		return false;
	}

	wxBusyCursor wait;

	idStr tempfile = "guis/temp.guied";
	idStr ospath = fileSystem->RelativePathToOSPath( tempfile, "fs_savepath" );

	const wxString ospathW = wxString::FromUTF8( ospath.c_str() );
	wxRemoveFile( ospathW );

	idFile* file = fileSystem->OpenFileWrite( tempfile );
	if( !file )
	{
		return false;
	}

	WriteWindow( file, 1, m_interface->GetDesktop() );
	fileSystem->CloseFile( file );

	const wxString dst = wxString::FromUTF8( filename );
	if( !wxCopyFile( ospathW, dst, true ) )
	{
		wxRemoveFile( ospathW );
		return false;
	}

	wxRemoveFile( ospathW );

	m_filename = filename;
	m_modified = false;
	m_new = false;
	UpdateTitle();
	return true;
}

/*
================
rvGEWorkspace::WriteTabs

Emits depth tab characters to the output file.
================
*/
void rvGEWorkspace::WriteTabs( idFile* file, int depth )
{
	for( int i = 0; i < depth; ++i )
	{
		file->Write( "\t", 1 );
	}
}

/*
================
rvGEWorkspace::WriteWindow

Recursively serialises a window and its children.
================
*/
bool rvGEWorkspace::WriteWindow( idFile* file, int depth, idWindow* window )
{
	rvGEWindowWrapper* wrapper = rvGEWindowWrapper::GetWrapper( window );
	if( !wrapper || wrapper->IsDeleted() )
	{
		return true;
	}

	idStr out;

	WriteTabs( file, depth - 1 );
	out = wrapper->WindowTypeToString( wrapper->GetWindowType() );
	out.Append( " " );
	out.Append( window->GetName() );
	out.Append( "\r\n" );
	file->Write( out, out.Length() );

	WriteTabs( file, depth - 1 );
	file->Write( "{\r\n", 3 );

	for( int i = 0; i < wrapper->GetStateDict().GetNumKeyVals(); ++i )
	{
		const idKeyValue* key = wrapper->GetStateDict().GetKeyVal( i );
		if( !key )
		{
			continue;
		}
		if( !key->GetKey().Icmp( "name" ) )
		{
			continue;
		}

		WriteTabs( file, depth );
		out = key->GetKey();
		out.Append( "\t" );
		file->Write( out, out.Length() );

		for( const char* p = key->GetValue().c_str(); *p; ++p )
		{
			if( *p == '\n' )
			{
				file->Write( "\\n", 2 );
			}
			else
			{
				file->Write( p, 1 );
			}
		}
		file->Write( "\r\n", 2 );
	}

	for( int i = 0; i < wrapper->GetVariableDict().GetNumKeyVals(); ++i )
	{
		const idKeyValue* key = wrapper->GetVariableDict().GetKeyVal( i );
		if( !key )
		{
			continue;
		}

		WriteTabs( file, depth );
		out = key->GetKey();
		out.Append( "\t" );
		out.Append( key->GetValue() );
		out.Append( "\r\n" );
		file->Write( out, out.Length() );
	}

	if( wrapper->GetScriptDict().GetNumKeyVals() )
	{
		file->Write( "\r\n", 2 );
	}

	for( int i = 0; i < wrapper->GetScriptDict().GetNumKeyVals(); ++i )
	{
		const idKeyValue* key = wrapper->GetScriptDict().GetKeyVal( i );
		if( !key )
		{
			continue;
		}

		WriteTabs( file, depth );
		file->Write( key->GetKey(), key->GetKey().Length() );
		file->Write( " ", 1 );

		idLexer src( key->GetValue(), key->GetValue().Length(), "",
					 LEXFL_ALLOWMULTICHARLITERALS |
					 LEXFL_NOSTRINGCONCAT |
					 LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
		src.ParseBracedSectionExact( out, depth + 1 );

		file->Write( out, out.Length() );
		file->Write( "\r\n\r\n", 4 );
	}

	for( int i = 0; i < wrapper->GetChildCount(); ++i )
	{
		WriteWindow( file, depth + 1, wrapper->GetChild( i ) );
	}

	WriteTabs( file, depth - 1 );
	file->Write( "}\r\n", 3 );

	return true;
}

/*
================
rvGEWorkspace::GetZoomScale
================
*/
float rvGEWorkspace::GetZoomScale() const
{
	return g_zoomScales[m_zoom];
}

/*
================
rvGEWorkspace::OnSize
================
*/
void rvGEWorkspace::OnSize( wxSizeEvent& e )
{
	UpdateScrollbars();
	Refresh( false );
	e.Skip();
}

/*
================
rvGEWorkspace::RunFrame
================
*/
void rvGEWorkspace::RunFrame()
{
	Refresh( false );
}

/*
================
rvGEWorkspace::Render
================
*/
void rvGEWorkspace::Render()
{
	Refresh( false );
}

/*
================
rvGEWorkspace::RenderGuiIntoCurrentFBO

Draws the GUI into the currently bound FBO. The crop rect is seeded with
the workspace region so glViewport maps the interface to the zoomed area.
tr.frameShaderTime is frozen so the workspace does not animate.
================
*/
void rvGEWorkspace::RenderGuiIntoCurrentFBO( int w, int h )
{
	if( !m_interface )
	{
		return;
	}

	int rx = ( int )( m_rect.x + 0.5f );
	int ry = ( int )( m_rect.y + 0.5f );
	int rw = ( int )( m_rect.w + 0.5f );
	int rh = ( int )( m_rect.h + 0.5f );

	if( rw <= 0 || rh <= 0 )
	{
		return;
	}

	if( rx < 0 )
	{
		rw += rx;
		rx = 0;
	}
	if( ry < 0 )
	{
		rh += ry;
		ry = 0;
	}
	if( rx + rw > w )
	{
		rw = w - rx;
	}
	if( ry + rh > h )
	{
		rh = h - ry;
	}
	if( rw <= 0 || rh <= 0 )
	{
		return;
	}

	const float savedShaderTime = tr.frameShaderTime;
	tr.frameShaderTime = 0.0f;

	tr.guiModel->Clear();

	tr.renderCrops[tr.currentRenderCrop].x1 = ( short )rx;
	tr.renderCrops[tr.currentRenderCrop].y1 = ( short )ry;
	tr.renderCrops[tr.currentRenderCrop].x2 = ( short )( rx + rw - 1 );
	tr.renderCrops[tr.currentRenderCrop].y2 = ( short )( ry + rh - 1 );

	renderSystem->CropRenderSize( rw, rh );

	if( idWindow* d = m_interface->GetDesktop() )
	{
		d->SetFlag( WIN_NOCURSOR );
	}

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
rvGEWorkspace::EnsurePreviewFBO

Lazily creates or resizes the offscreen framebuffer.
================
*/
void rvGEWorkspace::EnsurePreviewFBO( int w, int h )
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
		common->Warning( "rvGEWorkspace: FBO incomplete: 0x%x", status );
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
rvGEWorkspace::DestroyPreviewFBO
================
*/
void rvGEWorkspace::DestroyPreviewFBO()
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

	m_fboWidth  = 0;
	m_fboHeight = 0;
}

/*
================
rvGEWorkspace::OnPaint

Renders the GUI offscreen and blits the result plus the overlay.
================
*/
void rvGEWorkspace::OnPaint( wxPaintEvent& )
{
	static bool s_inRender = false;
	if( s_inRender )
	{
		return;
	}

	wxAutoBufferedPaintDC dc( this );

	const wxSize size = GetClientSize();
	const int w = wxMax( 1, size.x );
	const int h = wxMax( 1, size.y );

	if( !m_interface )
	{
		dc.SetBackground( wxBrush( wxColour( 32, 32, 32 ) ) );
		dc.Clear();
		return;
	}

	s_inRender = true;

	rRenderContext.MakeCurrent();
	EnsurePreviewFBO( w, h );

	if( w != m_fboWidth || h != m_fboHeight )
	{
		// FBO not ready for this size yet; skip this frame.
		s_inRender = false;
		return;
	}

	glBindFramebuffer( GL_FRAMEBUFFER, m_fbo );
	glViewport( 0, 0, w, h );
	glScissor( 0, 0, w, h );

	const idVec4& bg = m_app->GetOptions().GetWorkspaceColor();
	glClearColor( bg.x, bg.y, bg.z, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	Framebuffer::SetEditorDefaultFramebuffer( m_fbo );

	RenderGuiIntoCurrentFBO( w, h );

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
		unsigned char* dstRow       = dst + size_t( y ) * size_t( w ) * 3u;
		for( int x = 0; x < w; ++x )
		{
			dstRow[x * 3 + 0] = srcRow[x * 4 + 0];
			dstRow[x * 3 + 1] = srcRow[x * 4 + 1];
			dstRow[x * 3 + 2] = srcRow[x * 4 + 2];
		}
	}

	dc.DrawBitmap( wxBitmap( img ), 0, 0, false );

	DrawOverlayDC( dc );

	s_inRender = false;
}

/*
================
rvGEWorkspace::DrawOverlayDC

Draws the grid and the selection gizmos over the rendered GUI.
================
*/
void rvGEWorkspace::DrawOverlayDC( wxDC& dc )
{
	// Grid
	if( m_app->GetOptions().GetGridVisible() )
	{
		const idVec4& gc = m_app->GetOptions().GetGridColor();
		const wxColour gridCol( ( unsigned char )( gc.x * 255 ),
								( unsigned char )( gc.y * 255 ),
								( unsigned char )( gc.z * 255 ) );

		const float step = wxMax( 4.0f, m_app->GetOptions().GetGridWidth() * g_zoomScales[m_zoom] );

		dc.SetPen( wxPen( gridCol, 1 ) );
		for( float x = m_rect.x; x < m_rect.x + m_rect.w; x += step )
		{
			dc.DrawLine( ( int )x, ( int )m_rect.y, ( int )x, ( int )( m_rect.y + m_rect.h ) );
		}
		for( float y = m_rect.y; y < m_rect.y + m_rect.h; y += step )
		{
			dc.DrawLine( ( int )m_rect.x, ( int )y, ( int )( m_rect.x + m_rect.w ), ( int )y );
		}
	}

	// Selection
	if( m_selections.Num() == 0 )
	{
		return;
	}

	const idVec4& color = m_app->GetOptions().GetSelectionColor();
	const wxColour selCol( ( unsigned char )( color.x * 255 ),
						   ( unsigned char )( color.y * 255 ),
						   ( unsigned char )( color.z * 255 ) );

	const idRectangle& selRect = m_selections.GetRect();

	dc.SetBrush( *wxTRANSPARENT_BRUSH );
	dc.SetPen( wxPen( selCol, 1 ) );
	dc.DrawRectangle( ( int )selRect.x, ( int )selRect.y, ( int )selRect.w, ( int )selRect.h );

	const int grab = 7;
	const int center = 5;

	for( int i = 0; i < m_selections.Num(); i++ )
	{
		rvGEWindowWrapper* wrapper = rvGEWindowWrapper::GetWrapper( m_selections[i] );
		if( !wrapper )
		{
			continue;
		}

		idRectangle rect = wrapper->GetScreenRect();
		WorkspaceToWindow( rect );

		dc.DrawRectangle( ( int )rect.x, ( int )rect.y, ( int )rect.w, ( int )rect.h );

		if( i == 0 )
		{
			dc.SetBrush( wxBrush( selCol ) );
			dc.DrawRectangle( ( int )rect.x, ( int )rect.y, grab, grab );
			dc.SetBrush( *wxTRANSPARENT_BRUSH );
		}

		dc.SetBrush( wxBrush( selCol ) );
		dc.DrawRectangle( ( int )( rect.x + ( rect.w - center ) / 2 ),
						  ( int )( rect.y + ( rect.h - center ) / 2 ),
						  center, center );
		dc.SetBrush( *wxTRANSPARENT_BRUSH );
	}

	if( m_selections.IsExpression() )
	{
		return;
	}

	dc.SetBrush( wxBrush( selCol ) );
	dc.SetPen( wxPen( selCol, 1 ) );

	dc.DrawRectangle( ( int )( selRect.x - grab ), ( int )( selRect.y - grab ), grab, grab );
	dc.DrawRectangle( ( int )( selRect.x - grab ), ( int )( selRect.y + selRect.h / 2 - grab / 2 ), grab, grab );
	dc.DrawRectangle( ( int )( selRect.x - grab ), ( int )( selRect.y + selRect.h ), grab, grab );

	dc.DrawRectangle( ( int )( selRect.x + selRect.w / 2 - grab / 2 ), ( int )( selRect.y - grab ), grab, grab );
	dc.DrawRectangle( ( int )( selRect.x + selRect.w / 2 - grab / 2 ), ( int )( selRect.y + selRect.h ), grab, grab );

	dc.DrawRectangle( ( int )( selRect.x + selRect.w ), ( int )( selRect.y - grab ), grab, grab );
	dc.DrawRectangle( ( int )( selRect.x + selRect.w ), ( int )( selRect.y + selRect.h / 2 - grab / 2 ), grab, grab );
	dc.DrawRectangle( ( int )( selRect.x + selRect.w ), ( int )( selRect.y + selRect.h ), grab, grab );
}

/*
================
rvGEWorkspace::UpdateTitle

Refreshes the notebook tab caption and status bar zoom text.
================
*/
void rvGEWorkspace::UpdateTitle()
{
	if( !m_app || !m_app->GetNotebook() )
	{
		return;
	}

	const int idx = m_app->GetNotebook()->FindPage( this );
	if( idx == wxNOT_FOUND )
	{
		return;
	}

	const wxString zoomPart = m_zoomIsFit ? wxString( "(Fit)" ) : wxString::Format( "(%d%%)", ( int )( g_zoomScales[m_zoom] * 100 ) );

	wxString title = wxString::Format( "%s%s %s", wxFileName( m_filename.c_str() ).GetFullName(), m_modified ? "*" : "", zoomPart );

	m_app->GetNotebook()->SetPageText( idx, title );

	m_app->SetStatusText( wxString::Format( "Zoom: %d%%", ( int )( g_zoomScales[m_zoom] * 100 ) ), 2 );
}

/*
================
rvGEWorkspace::SetModified
================
*/
void rvGEWorkspace::SetModified( bool mod )
{
	if( m_modified == mod )
	{
		return;
	}

	m_modified = mod;
	UpdateTitle();
}

/*
================
rvGEWorkspace::ZoomIn
================
*/
rvGEWorkspace::EZoomLevel rvGEWorkspace::ZoomIn()
{
	if( m_zoom < ZOOM_MAX - 1 )
	{
		++m_zoom;
		m_zoomIsFit = false;
		UpdateScrollbars();
		UpdateTitle();
		Refresh( false );
	}
	return ( EZoomLevel )m_zoom;
}

/*
================
rvGEWorkspace::ZoomOut
================
*/
rvGEWorkspace::EZoomLevel rvGEWorkspace::ZoomOut()
{
	if( m_zoom > ZOOM_MIN + 1 )
	{
		--m_zoom;
		m_zoomIsFit = false;
		UpdateScrollbars();
		UpdateTitle();
		Refresh( false );
	}
	return ( EZoomLevel )m_zoom;
}

/*
================
rvGEWorkspace::ZoomFit
================
*/
rvGEWorkspace::EZoomLevel rvGEWorkspace::ZoomFit()
{
	const wxSize sz = GetClientSize();
	if( sz.x <= 0 || sz.y <= 0 )
	{
		return ( EZoomLevel )m_zoom;
	}

	const float scaleX = ( float )sz.x / ( float )SCREEN_WIDTH;
	const float scaleY = ( float )sz.y / ( float )SCREEN_HEIGHT;
	const float scale  = wxMin( scaleX, scaleY );

	const float tolerance = 1.10f;
	const float maxScale  = scale * tolerance;

	EZoomLevel best = ZOOM_25;
	for( int i = ZOOM_25; i < ZOOM_MAX; ++i )
	{
		if( g_zoomScales[i] <= maxScale )
		{
			best = ( EZoomLevel )i;
		}
	}

	m_zoom = best;
	m_zoomIsFit = true;

	SetScrollPos( wxHORIZONTAL, 0 );
	SetScrollPos( wxVERTICAL, 0 );

	UpdateScrollbars();
	UpdateTitle();
	Refresh( false );

	return ( EZoomLevel )m_zoom;
}

/*
================
rvGEWorkspace::WindowToWorkspace
================
*/
idVec2& rvGEWorkspace::WindowToWorkspace( idVec2& point )
{
	point.x = ( point.x - m_rect.x ) / m_rect.w * SCREEN_WIDTH;
	point.y = ( point.y - m_rect.y ) / m_rect.h * SCREEN_HEIGHT;
	return point;
}

/*
================
rvGEWorkspace::WorkspaceToWindow
================
*/
idVec2& rvGEWorkspace::WorkspaceToWindow( idVec2& point )
{
	point.x = m_rect.x + ( point.x / SCREEN_WIDTH  * m_rect.w );
	point.y = m_rect.y + ( point.y / SCREEN_HEIGHT * m_rect.h );
	return point;
}

/*
================
rvGEWorkspace::WindowToWorkspace
================
*/
idRectangle& rvGEWorkspace::WindowToWorkspace( idRectangle& rect )
{
	rect.x = ( rect.x - m_rect.x ) / m_rect.w * SCREEN_WIDTH;
	rect.y = ( rect.y - m_rect.y ) / m_rect.h * SCREEN_HEIGHT;
	rect.w = rect.w / m_rect.w * SCREEN_WIDTH;
	rect.h = rect.h / m_rect.h * SCREEN_HEIGHT;
	return rect;
}

/*
================
rvGEWorkspace::WorkspaceToWindow
================
*/
idRectangle& rvGEWorkspace::WorkspaceToWindow( idRectangle& rect )
{
	rect.x = m_rect.x + ( rect.x / SCREEN_WIDTH  * m_rect.w );
	rect.y = m_rect.y + ( rect.y / SCREEN_HEIGHT * m_rect.h );
	rect.w = rect.w / SCREEN_WIDTH  * m_rect.w;
	rect.h = rect.h / SCREEN_HEIGHT * m_rect.h;
	return rect;
}

/*
================
rvGEWorkspace::UpdateRectangle

Recomputes m_rect, optionally applying the current scroll offsets.
================
*/
void rvGEWorkspace::UpdateRectangle( bool useScroll )
{
	const wxSize sz = GetClientSize();
	m_windowWidth  = sz.x;
	m_windowHeight = sz.y;

	const float scale = g_zoomScales[m_zoom];

	m_rect.x = m_windowWidth  * 0.5f - ( SCREEN_WIDTH  * scale ) * 0.5f;
	m_rect.y = m_windowHeight * 0.5f - ( SCREEN_HEIGHT * scale ) * 0.5f;
	m_rect.w = SCREEN_WIDTH  * scale;
	m_rect.h = SCREEN_HEIGHT * scale;

	if( useScroll )
	{
		m_rect.y -= ( float )GetScrollPos( wxVERTICAL )   / 1000.0f;
		m_rect.x -= ( float )GetScrollPos( wxHORIZONTAL ) / 1000.0f;
	}
}

/*
================
rvGEWorkspace::UpdateScrollbars
================
*/
void rvGEWorkspace::UpdateScrollbars()
{
	UpdateRectangle( false );

	const int vRangeMax = ( int )( ( m_rect.h - m_windowHeight + 10 ) * 1000 / 2 );
	if( vRangeMax > 0 )
	{
		m_scrollVert = true;
		SetScrollbar( ( int )wxVERTICAL, GetScrollPos( ( int )wxVERTICAL ), wxMax( 1, vRangeMax ), 0, vRangeMax );
	}
	else if( m_scrollVert )
	{
		m_scrollVert = false;
		SetScrollPos( ( int )wxVERTICAL, 0 );
	}

	const int hRangeMax = ( int )( ( m_rect.w - m_windowWidth + 10 ) * 1000 / 2 );
	if( hRangeMax > 0 )
	{
		m_scrollHorz = true;
		SetScrollbar( ( int )wxHORIZONTAL, GetScrollPos( ( int )wxHORIZONTAL ), wxMax( 1, hRangeMax ), 0, hRangeMax );
	}
	else if( m_scrollHorz )
	{
		m_scrollHorz = false;
		SetScrollPos( ( int )wxHORIZONTAL, 0 );
	}

	UpdateRectangle( true );
}

/*
================
rvGEWorkspace::Scroll
================
*/
void rvGEWorkspace::Scroll( int scrollbar, int offset )
{
	if( ( scrollbar == wxHORIZONTAL && !m_scrollHorz ) || ( scrollbar == wxVERTICAL   && !m_scrollVert ) )
	{
		return;
	}

	const int pos = GetScrollPos( scrollbar );
	int       newPos = pos + offset * 1000;

	const int range = GetScrollRange( scrollbar );
	if( newPos < 0 )
	{
		newPos = 0;
	}
	if( newPos > range )
	{
		newPos = range;
	}

	SetScrollPos( scrollbar, newPos );
	UpdateRectangle();
	Refresh( false );
}

/*
================
rvGEWorkspace::OnMouse

Routes the wx mouse events to the specific button handlers.
================
*/
void rvGEWorkspace::OnMouse( wxMouseEvent& e )
{
	if( e.LeftDown() )
	{
		HandleLButtonDown( e );
	}
	else if( e.LeftUp() )
	{
		HandleLButtonUp( e );
	}
	else if( e.LeftDClick() )
	{
		HandleLButtonDblClk( e );
	}
	else if( e.RightDown() )
	{
		HandleRButtonDown( e );
	}
	else if( e.MiddleDown() )
	{
		HandleMButtonDown( e );
	}
	else if( e.MiddleUp() )
	{
		HandleMButtonUp( e );
	}
}

/*
================
rvGEWorkspace::OnMouseMove
================
*/
void rvGEWorkspace::OnMouseMove( wxMouseEvent& e )
{
	HandleMouseMove( e );
}

/*
================
rvGEWorkspace::OnWheel

Wheel up/down drives ZoomIn/ZoomOut.
================
*/
void rvGEWorkspace::OnWheel( wxMouseEvent& e )
{
	if( e.GetWheelRotation() > 0 )
	{
		ZoomIn();
	}
	else
	{
		ZoomOut();
	}
}

/*
================
rvGEWorkspace::OnKey
================
*/
void rvGEWorkspace::OnKey( wxKeyEvent& e )
{
	HandleKeyDown( e );
}

/*
================
rvGEWorkspace::OnCaptureLost
================
*/
void rvGEWorkspace::OnCaptureLost( wxMouseCaptureLostEvent& )
{
	m_dragType   = rvGESelectionMgr::HT_NONE;
	m_dragScroll = false;
}

/*
================
rvGEWorkspace::HandleLButtonDown
================
*/
int rvGEWorkspace::HandleLButtonDown( wxMouseEvent& e )
{
	if( m_dragScroll )
	{
		return 0;
	}

	idVec2 point( e.GetX(), e.GetY() );
	WindowToWorkspace( point );

	m_modifiers.BlockNextMerge();

	m_dragPoint = idVec2( e.GetX(), e.GetY() );
	m_dragTime  = Sys_Milliseconds();
	m_dragX     = true;
	m_dragY     = true;

	if( m_selections.Num() )
	{
		m_dragType = m_selections.HitTest( m_dragPoint.x, m_dragPoint.y );
	}

	rvGEWindowWrapper* desktop = rvGEWindowWrapper::GetWrapper( m_interface->GetDesktop() );
	idWindow* window = desktop ? desktop->WindowFromPoint( point.x, point.y ) : NULL;

	if( m_app->GetOptions().GetIgnoreDesktopSelect() && window == m_interface->GetDesktop() )
	{
		window = NULL;
	}

	if( m_dragType == rvGESelectionMgr::HT_MOVE ||
			m_dragType == rvGESelectionMgr::HT_NONE )
	{
		if( window )
		{
			const bool selected = m_selections.IsSelected( window );

			if( wxGetKeyState( WXK_SHIFT ) )
			{
				if( !selected )
				{
					m_selections.Add( window );
					m_dragType = rvGESelectionMgr::HT_MOVE;
				}
				else
				{
					m_selections.Remove( window );
				}
			}
			else if( !selected && m_dragType == rvGESelectionMgr::HT_NONE )
			{
				m_selections.Set( window );
				m_dragType = rvGESelectionMgr::HT_MOVE;
			}
		}
		else
		{
			m_selections.Clear();
		}
	}

	if( m_selections.IsExpression() )
	{
		m_dragType = rvGESelectionMgr::HT_SELECT;
	}
	else if( m_dragType != rvGESelectionMgr::HT_NONE )
	{
		if( !HasCapture() )
		{
			CaptureMouse();
		}
	}

	WindowToWorkspace( m_dragPoint );
	return 0;
}

/*
================
rvGEWorkspace::HandleLButtonUp
================
*/
int rvGEWorkspace::HandleLButtonUp( wxMouseEvent& )
{
	if( m_dragType != rvGESelectionMgr::HT_NONE )
	{
		if( HasCapture() )
		{
			ReleaseMouse();
		}

		m_modifiers.BlockNextMerge();
		m_app->GetTransformer().Update();
	}

	m_dragType = rvGESelectionMgr::HT_NONE;
	return 0;
}

/*
================
rvGEWorkspace::HandleLButtonDblClk
================
*/
int rvGEWorkspace::HandleLButtonDblClk( wxMouseEvent& )
{
	EditSelectedProperties();
	return 0;
}

/*
================
rvGEWorkspace::HandleMButtonDown

Starts a hand-scroll pan.
================
*/
int rvGEWorkspace::HandleMButtonDown( wxMouseEvent& e )
{
	if( m_dragType != rvGESelectionMgr::HT_NONE )
	{
		return 0;
	}

	m_dragPoint.Set( e.GetX(), e.GetY() );
	m_dragScroll = true;

	SetCursor( m_handCursor );
	if( !HasCapture() )
	{
		CaptureMouse();
	}

	WindowToWorkspace( m_dragPoint );
	return 0;
}

/*
================
rvGEWorkspace::HandleMButtonUp
================
*/
int rvGEWorkspace::HandleMButtonUp( wxMouseEvent& )
{
	if( m_dragScroll )
	{
		m_dragScroll = false;
		if( HasCapture() )
		{
			ReleaseMouse();
		}
	}
	return 0;
}

/*
================
rvGEWorkspace::HandleRButtonDown

Builds and pops the workspace context menu.
================
*/
int rvGEWorkspace::HandleRButtonDown( wxMouseEvent& e )
{
	const wxPoint point = e.GetPosition();

	m_selectMenu.Clear();

	m_selectMenuPos[0] = point.x;
	m_selectMenuPos[1] = point.y;
	WindowToWorkspace( m_selectMenuPos );

	rvGEWindowWrapper::GetWrapper( m_interface->GetDesktop() )->EnumChildren( BuildSelectMenuEnumProc, this );
	m_selectMenu.Append( m_interface->GetDesktop() );

	wxMenu menu;

	wxMenu* newMenu = new wxMenu;
	newMenu->Append( ID_GUIED_ITEM_NEWWINDOWDEF, "&windowDef" );
	newMenu->Append( ID_GUIED_ITEM_NEWEDITDEF, "&editDef" );
	newMenu->Append( ID_GUIED_ITEM_NEWHTMLDEF, "&htmlDef" );
	newMenu->Append( ID_GUIED_ITEM_NEWCHOICEDEF, "&choiceDef" );
	newMenu->Append( ID_GUIED_ITEM_NEWSLIDERDEF, "&sliderDef" );
	newMenu->Append( ID_GUIED_ITEM_NEWBINDDEF, "&bindDef" );
	newMenu->Append( ID_GUIED_ITEM_NEWLISTDEF, "&listDef" );
	newMenu->Append( ID_GUIED_ITEM_NEWRENDERDEF, "&renderDef" );
	menu.AppendSubMenu( newMenu, "&New" );

	menu.AppendSeparator();

	wxMenu* selectMenu = new wxMenu;
	for( int i = 0; i < m_selectMenu.Num(); ++i )
	{
		rvGEWindowWrapper* wrapper = rvGEWindowWrapper::GetWrapper( m_selectMenu[i] );
		selectMenu->AppendCheckItem( 9800 + i, m_selectMenu[i]->GetName() );
		selectMenu->Check( 9800 + i, wrapper && wrapper->IsSelected() );
	}
	menu.AppendSubMenu( selectMenu, "&Select" );

	menu.AppendSeparator();
	menu.Append( ID_GUIED_ITEM_PROPERTIES, "&Properties..." );
	menu.Append( ID_GUIED_ITEM_SCRIPTS, "&Scripts..." );

	menu.Bind( wxEVT_MENU, &rvGEWorkspace::OnSelectMenu, this, 9800, 9899 );
	menu.Bind( wxEVT_MENU, &rvGEWorkspace::OnNewWindowDef, this, ID_GUIED_ITEM_NEWWINDOWDEF );
	menu.Bind( wxEVT_MENU, &rvGEWorkspace::OnNewEditDef, this, ID_GUIED_ITEM_NEWEDITDEF );
	menu.Bind( wxEVT_MENU, &rvGEWorkspace::OnEditProps, this, ID_GUIED_ITEM_PROPERTIES );
	menu.Bind( wxEVT_MENU, &rvGEWorkspace::OnEditScripts, this, ID_GUIED_ITEM_SCRIPTS );

	PopupMenu( &menu, point );
	return 0;
}

/*
================
rvGEWorkspace::OnSelectMenu
================
*/
void rvGEWorkspace::OnSelectMenu( wxCommandEvent& e )
{
	const int i = e.GetId() - 9800;
	if( i < 0 || i >= m_selectMenu.Num() )
	{
		return;
	}

	idWindow* window = m_selectMenu[i];
	rvGEWindowWrapper* wrapper = rvGEWindowWrapper::GetWrapper( window );
	if( !wrapper )
	{
		return;
	}

	if( wxGetKeyState( WXK_SHIFT ) )
	{
		if( wrapper->IsSelected() )
		{
			m_selections.Remove( window );
		}
		else
		{
			m_selections.Add( window );
		}
	}
	else
	{
		m_selections.Set( window );
	}
}

/*
================
rvGEWorkspace::OnNewWindowDef
================
*/
void rvGEWorkspace::OnNewWindowDef( wxCommandEvent& )
{
	AddWindow( rvGEWindowWrapper::WT_NORMAL );
}

/*
================
rvGEWorkspace::OnNewEditDef
================
*/
void rvGEWorkspace::OnNewEditDef( wxCommandEvent& )
{
	AddWindow( rvGEWindowWrapper::WT_EDIT );
}

/*
================
rvGEWorkspace::OnEditProps
================
*/
void rvGEWorkspace::OnEditProps( wxCommandEvent& )
{
	EditSelectedProperties();
}

/*
================
rvGEWorkspace::OnEditScripts
================
*/
void rvGEWorkspace::OnEditScripts( wxCommandEvent& )
{
	EditSelectedScripts();
}

/*
================
rvGEWorkspace::HandleMouseMove
================
*/
int rvGEWorkspace::HandleMouseMove( wxMouseEvent& e )
{
	idVec2 cursor( e.GetX(), e.GetY() );
	WindowToWorkspace( cursor );

	m_app->SetStatusText( wxString::Format( "x: %d  y: %d", ( int )cursor.x, ( int )cursor.y ), 1 );

	if( m_dragScroll )
	{
		HandleScrollPan( cursor );
		return 0;
	}

	if( m_dragType == rvGESelectionMgr::HT_NONE )
	{
		UpdateCursor( cursor.x, cursor.y );
		return 0;
	}

	if( m_dragType == rvGESelectionMgr::HT_MOVE && Sys_Milliseconds() - m_dragTime <= 50 )
	{
		return 0;
	}

	if( m_app->GetOptions().GetGridSnap() )
	{
		const int gw = m_app->GetOptions().GetGridWidth();
		cursor.x = ( float )( ( ( int )cursor.x + gw / 2 ) / gw * gw );
		cursor.y = ( float )( ( ( int )cursor.y + gw / 2 ) / gw * gw );
	}

	if( ( int )cursor.x == ( int )m_dragPoint.x && ( int )cursor.y == ( int )m_dragPoint.y )
	{
		return 0;
	}

	const bool snap = m_app->GetOptions().GetGridSnap();

	switch( m_dragType )
	{
		case rvGESelectionMgr::HT_MOVE:
			AddModifierMove( "Move", cursor.x - m_dragPoint.x, cursor.y - m_dragPoint.y, snap );
			break;

		case rvGESelectionMgr::HT_SIZE_BOTTOM:
			AddModifierSize( "Size", 0, 0, 0, cursor.y - m_dragPoint.y, snap );
			break;
		case rvGESelectionMgr::HT_SIZE_TOP:
			AddModifierSize( "Size", 0, cursor.y - m_dragPoint.y, 0, 0, snap );
			break;
		case rvGESelectionMgr::HT_SIZE_RIGHT:
			AddModifierSize( "Size", 0, 0, cursor.x - m_dragPoint.x, 0, snap );
			break;
		case rvGESelectionMgr::HT_SIZE_LEFT:
			AddModifierSize( "Size", cursor.x - m_dragPoint.x, 0, 0, 0, snap );
			break;
		case rvGESelectionMgr::HT_SIZE_TOPLEFT:
			AddModifierSize( "Size", cursor.x - m_dragPoint.x, cursor.y - m_dragPoint.y, 0, 0, snap );
			break;
		case rvGESelectionMgr::HT_SIZE_TOPRIGHT:
			AddModifierSize( "Size", 0, cursor.y - m_dragPoint.y, cursor.x - m_dragPoint.x, 0, snap );
			break;
		case rvGESelectionMgr::HT_SIZE_BOTTOMLEFT:
			AddModifierSize( "Size", cursor.x - m_dragPoint.x, 0, 0, cursor.y - m_dragPoint.y, snap );
			break;
		case rvGESelectionMgr::HT_SIZE_BOTTOMRIGHT:
			AddModifierSize( "Size", 0, 0, cursor.x - m_dragPoint.x, cursor.y - m_dragPoint.y, snap );
			break;
	}

	UpdateCursor( m_dragType );

	if( ( int )cursor.x != ( int )m_dragPoint.x && m_dragX )
	{
		m_dragPoint.x = cursor.x;
	}
	if( ( int )cursor.y != ( int )m_dragPoint.y && m_dragY )
	{
		m_dragPoint.y = cursor.y;
	}

	Refresh( false );
	return 0;
}

/*
================
rvGEWorkspace::HandleScrollPan
================
*/
void rvGEWorkspace::HandleScrollPan( const idVec2& cursor )
{
	Scroll( wxHORIZONTAL, ( int )( m_dragPoint.x - cursor.x ) );
	Scroll( wxVERTICAL, ( int )( m_dragPoint.y - cursor.y ) );

	SetCursor( m_handCursor );
	m_dragPoint = cursor;
}

/*
================
rvGEWorkspace::HandleKeyDown

Arrow keys nudge selection; shift+arrow resizes; escape deselects.
================
*/
int rvGEWorkspace::HandleKeyDown( wxKeyEvent& e )
{
	const bool shift = e.ShiftDown();

	switch( e.GetKeyCode() )
	{
		case WXK_LEFT:
			if( shift )
			{
				AddModifierSizeNudge( -1,  0, false );
			}
			else
			{
				AddModifierMoveNudge( -1,  0, false );
			}
			break;
		case WXK_RIGHT:
			if( shift )
			{
				AddModifierSizeNudge( 1,  0, false );
			}
			else
			{
				AddModifierMoveNudge( 1,  0, false );
			}
			break;
		case WXK_DOWN:
			if( shift )
			{
				AddModifierSizeNudge( 0,  1, false );
			}
			else
			{
				AddModifierMoveNudge( 0,  1, false );
			}
			break;
		case WXK_UP:
			if( shift )
			{
				AddModifierSizeNudge( 0, -1, false );
			}
			else
			{
				AddModifierMoveNudge( 0, -1, false );
			}
			break;

		case WXK_ESCAPE:
			m_selections.Clear();
			m_app->GetNavigator().Update();
			break;

		default:
			e.Skip();
			return 0;
	}

	return 0;
}

/*
================
rvGEWorkspace::UpdateCursor (current position)
================
*/
void rvGEWorkspace::UpdateCursor()
{
	if( m_dragType == rvGESelectionMgr::HT_NONE )
	{
		const wxPoint pt = wxGetMousePosition() - GetScreenPosition();
		idVec2 cursor( pt.x, pt.y );
		WindowToWorkspace( cursor );
		UpdateCursor( cursor.x, cursor.y );
	}
	else
	{
		UpdateCursor( m_dragType );
	}
}

/*
================
rvGEWorkspace::UpdateCursor
================
*/
void rvGEWorkspace::UpdateCursor( float x, float y )
{
	const idVec2 point = WorkspaceToWindow( idVec2( x, y ) );
	const rvGESelectionMgr::EHitTest type =	m_selections.HitTest( point.x, point.y );

	if( type != rvGESelectionMgr::HT_NONE )
	{
		UpdateCursor( type );
	}
	else
	{
		SetCursor( wxCursor( wxCURSOR_ARROW ) );
	}
}

/*
================
rvGEWorkspace::UpdateCursor
================
*/
void rvGEWorkspace::UpdateCursor( rvGESelectionMgr::EHitTest type )
{
	switch( type )
	{
		case rvGESelectionMgr::HT_SELECT:
			SetCursor( wxCursor( wxCURSOR_ARROW ) );
			break;
		case rvGESelectionMgr::HT_MOVE:
			SetCursor( wxCursor( wxCURSOR_SIZING ) );
			break;
		case rvGESelectionMgr::HT_SIZE_LEFT:
		case rvGESelectionMgr::HT_SIZE_RIGHT:
			SetCursor( wxCursor( wxCURSOR_SIZEWE ) );
			break;
		case rvGESelectionMgr::HT_SIZE_TOP:
		case rvGESelectionMgr::HT_SIZE_BOTTOM:
			SetCursor( wxCursor( wxCURSOR_SIZENS ) );
			break;
		case rvGESelectionMgr::HT_SIZE_TOPRIGHT:
		case rvGESelectionMgr::HT_SIZE_BOTTOMLEFT:
			SetCursor( wxCursor( wxCURSOR_SIZENESW ) );
			break;
		case rvGESelectionMgr::HT_SIZE_BOTTOMRIGHT:
		case rvGESelectionMgr::HT_SIZE_TOPLEFT:
			SetCursor( wxCursor( wxCURSOR_SIZENWSE ) );
			break;
		default:
			SetCursor( wxCursor( wxCURSOR_ARROW ) );
			break;
	}
}

/*
================
rvGEWorkspace::AddModifierMove
================
*/
void rvGEWorkspace::AddModifierMove( const char* modName, float x, float y, bool snap )
{
	idRectangle scaleRect = m_selections.GetRect();
	WindowToWorkspace( scaleRect );

	idRectangle newRect = scaleRect;
	newRect.x += x;
	newRect.y += y;

	if( snap )
	{
		m_app->GetOptions().SnapRectToGrid( newRect, true, true, false, false );
	}

	rvGEModifierGroup* group = new rvGEModifierGroup;

	for( int i = 0; i < m_selections.Num(); ++i )
	{
		idWindow* w = m_selections[i];
		if( !w->GetParent() )
		{
			continue;
		}

		rvGEWindowWrapper* parentWrapper = rvGEWindowWrapper::GetWrapper( w->GetParent() );

		if( parentWrapper && parentWrapper->IsSelected() )
		{
			group->Append( new rvGEMoveModifier( modName, w, 0, 0 ) );
			continue;
		}

		group->Append( new rvGEMoveModifier( modName, w, newRect.x - scaleRect.x, newRect.y - scaleRect.y ) );
	}

	m_modifiers.Append( group );
	SetModified( true );
}

/*
================
rvGEWorkspace::AddModifierSize
================
*/
void rvGEWorkspace::AddModifierSize( const char* modName, float l, float t, float r, float b, bool snap )
{
	idRectangle scaleRect = m_selections.GetRect();
	WindowToWorkspace( scaleRect );

	idRectangle newRect = scaleRect;
	newRect.x += l;
	newRect.y += t;
	newRect.w += ( r - l );
	newRect.h += ( b - t );

	if( newRect.w <= 1 )
	{
		newRect.x -= ( l ? ( 1 - newRect.w ) : 0 );
		m_dragPoint.x = newRect.x;
		newRect.w = 1;
		m_dragX = false;
	}
	else
	{
		m_dragX = true;
	}

	if( newRect.h <= 1 )
	{
		newRect.y -= ( t ? ( 1 - newRect.h ) : 0 );
		m_dragPoint.y = newRect.y;
		newRect.h = 1;
		m_dragY = false;
	}
	else
	{
		m_dragY = true;
	}

	if( snap )
	{
		m_app->GetOptions().SnapRectToGrid( newRect, l != 0.0f, t != 0.0f, r != 0.0f, b != 0.0f );
	}

	rvGEModifierGroup* group = new rvGEModifierGroup;

	for( int i = 0; i < m_selections.Num(); ++i )
	{
		idWindow* w = m_selections[i];

		idRectangle sizeRect = rvGEWindowWrapper::GetWrapper( w )->GetScreenRect();

		l = ( newRect.x + ( ( sizeRect.x - scaleRect.x ) / scaleRect.w ) * newRect.w ) - sizeRect.x;
		t = ( newRect.y + ( ( sizeRect.y - scaleRect.y ) / scaleRect.h ) * newRect.h ) - sizeRect.y;
		r = ( sizeRect.w / scaleRect.w * newRect.w ) - sizeRect.w + l;
		b = ( sizeRect.h / scaleRect.h * newRect.h ) - sizeRect.h + t;

		idWindow* parent = w;
		while( ( parent = parent->GetParent() ) != NULL )
		{
			rvGEWindowWrapper* pwrapper = rvGEWindowWrapper::GetWrapper( parent );
			if( !pwrapper || !pwrapper->IsSelected() )
			{
				continue;
			}

			sizeRect = pwrapper->GetScreenRect();

			float offset = ( newRect.x + ( ( sizeRect.x - scaleRect.x ) / scaleRect.w ) * newRect.w ) - sizeRect.x;
			l -= offset;
			r -= offset;

			offset = ( newRect.y + ( ( sizeRect.y - scaleRect.y ) / scaleRect.h ) * newRect.h ) - sizeRect.y;
			t -= offset;
			b -= offset;

			break;
		}

		group->Append( new rvGESizeModifier( modName, w, l, t, r, b ) );
	}

	m_modifiers.Append( group );
	SetModified( true );
}

/*
================
rvGEWorkspace::AddModifiers
================
*/
void rvGEWorkspace::AddModifiers( EModifierType type, ... )
{
	if( !m_selections.Num() )
	{
		return;
	}

	if( m_selections.Num() > 1 )
	{
		rvGEModifierGroup* group = new rvGEModifierGroup;

		for( int i = 0; i < m_selections.Num(); ++i )
		{
			va_list args;
			va_start( args, type );
			group->Append( CreateModifier( type, m_selections[i], args ) );
			va_end( args );
		}

		m_modifiers.Append( group );
	}
	else
	{
		va_list args;
		va_start( args, type );
		m_modifiers.Append( CreateModifier( type, m_selections[0], args ) );
		va_end( args );
	}

	SetModified( true );
}

/*
================
rvGEWorkspace::AddModifiers
================
*/
void rvGEWorkspace::AddModifiers( idWindow* window, EModifierType type, ... )
{
	va_list args;
	va_start( args, type );
	m_modifiers.Append( CreateModifier( type, window, args ) );
	va_end( args );

	SetModified( true );
}

/*
================
rvGEWorkspace::CreateModifier
================
*/
rvGEModifier* rvGEWorkspace::CreateModifier( EModifierType type,
		idWindow* window, va_list )
{
	switch( type )
	{
		case MOD_DELETE:
			return new rvGEDeleteModifier( "Delete", window );
		case MOD_HIDE:
			return new rvGEHideModifier( "Hide", window, true );
		case MOD_UNHIDE:
			return new rvGEHideModifier( "Hide", window, false );
		case MOD_SEND_BACKWARD:
			return new rvGEZOrderModifier( "Send Backward", window, rvGEZOrderModifier::ZO_BACKWARD );
		case MOD_SEND_BACK:
			return new rvGEZOrderModifier( "Send to Back", window, rvGEZOrderModifier::ZO_BACK );
		case MOD_BRING_FORWARD:
			return new rvGEZOrderModifier( "Bring Forward", window, rvGEZOrderModifier::ZO_FORWARD );
		case MOD_BRING_FRONT:
			return new rvGEZOrderModifier( "Bring to Front", window, rvGEZOrderModifier::ZO_FRONT );
		default:
			return NULL;
	}
}

/*
================
rvGEWorkspace::AddModifierShowAll
================
*/
void rvGEWorkspace::AddModifierShowAll()
{
	rvGEModifierGroup* group = new rvGEModifierGroup;
	rvGEWindowWrapper::GetWrapper( m_interface->GetDesktop() )->EnumChildren( ShowAllEnumProc, group );

	if( !group->GetCount() )
	{
		delete group;
	}
	else
	{
		m_modifiers.Append( group );
	}

	m_app->GetNavigator().Update();
}

/*
================
rvGEWorkspace::DeleteSelected
================
*/
void rvGEWorkspace::DeleteSelected()
{
	AddModifiers( MOD_DELETE );
	m_selections.Clear();
	m_app->GetNavigator().Update();
}

/*
================
rvGEWorkspace::HideSelected
================
*/
void rvGEWorkspace::HideSelected()
{
	AddModifiers( MOD_HIDE );
	m_selections.Clear();
	m_app->GetNavigator().Update();
	Refresh( false );
}

/*
================
rvGEWorkspace::UnhideSelected
================
*/
void rvGEWorkspace::UnhideSelected()
{
	AddModifiers( MOD_UNHIDE );
	m_app->GetNavigator().Update();
	Refresh( false );
}

/*
================
rvGEWorkspace::HideWindow
================
*/
void rvGEWorkspace::HideWindow( idWindow* window )
{
	AddModifiers( window, MOD_HIDE );
	m_app->GetNavigator().Update();
	Refresh( false );
}

/*
================
rvGEWorkspace::UnhideWindow
================
*/
void rvGEWorkspace::UnhideWindow( idWindow* window )
{
	AddModifiers( window, MOD_UNHIDE );
	m_app->GetNavigator().Update();
	Refresh( false );
}

/*
================
rvGEWorkspace::ShowHidden
================
*/
void rvGEWorkspace::ShowHidden()
{
	AddModifierShowAll();
	m_app->GetNavigator().Update();
	Refresh( false );
}

/*
================
rvGEWorkspace::BringSelectedForward
================
*/
void rvGEWorkspace::BringSelectedForward()
{
	AddModifiers( MOD_BRING_FORWARD );
	m_app->GetNavigator().Update();
}

/*
================
rvGEWorkspace::BringSelectedToFront
================
*/
void rvGEWorkspace::BringSelectedToFront()
{
	AddModifiers( MOD_BRING_FRONT );
	m_app->GetNavigator().Update();
}

/*
================
rvGEWorkspace::SendSelectedBackward
================
*/
void rvGEWorkspace::SendSelectedBackward()
{
	AddModifiers( MOD_SEND_BACKWARD );
	m_app->GetNavigator().Update();
}

/*
================
rvGEWorkspace::SendSelectedToBack
================
*/
void rvGEWorkspace::SendSelectedToBack()
{
	AddModifiers( MOD_SEND_BACK );
	m_app->GetNavigator().Update();
}

/*
================
rvGEWorkspace::MakeSelectedSameSize
================
*/
void rvGEWorkspace::MakeSelectedSameSize( bool changeWidth, bool changeHeight )
{
	rvGEModifierGroup* group = new rvGEModifierGroup;

	idRectangle rectTo = rvGEWindowWrapper::GetWrapper( m_selections[0] )->GetClientRect();

	for( int i = 1; i < m_selections.Num(); ++i )
	{
		idRectangle rectFrom = rvGEWindowWrapper::GetWrapper( m_selections[i] )->GetClientRect();

		float width  = changeWidth  ? ( rectTo.w - rectFrom.w ) : 0.0f;
		float height = changeHeight ? ( rectTo.h - rectFrom.h ) : 0.0f;

		group->Append( new rvGESizeModifier( "Make Same Size", m_selections[i], 0, 0, width, height ) );
	}

	m_modifiers.Append( group );
	m_modifiers.BlockNextMerge();

	SetModified( true );
}

/*
================
rvGEWorkspace::AlignSelected
================
*/
void rvGEWorkspace::AlignSelected( EItemAlign align )
{
	static const char* alignNames[] =
	{ "Lefts", "Centers", "Rights", "Tops", "Middles", "Bottoms" };

	assert( m_selections.Num() > 1 );

	const idStr modName = idStr( "Align " ) + alignNames[align];

	rvGEModifierGroup* group = new rvGEModifierGroup;

	const idRectangle rectTo = rvGEWindowWrapper::GetWrapper( m_selections[0] )->GetScreenRect();

	for( int i = 1; i < m_selections.Num(); ++i )
	{
		const idRectangle rectFrom = rvGEWindowWrapper::GetWrapper( m_selections[i] )->GetScreenRect();

		float x = 0.0f;
		float y = 0.0f;

		switch( align )
		{
			case ALIGN_LEFTS:
				x = rectTo.x - rectFrom.x;
				break;
			case ALIGN_RIGHTS:
				x = ( rectTo.x + rectTo.w ) - ( rectFrom.x + rectFrom.w );
				break;
			case ALIGN_CENTERS:
				x = ( rectTo.x + rectTo.w * 0.5f ) - ( rectFrom.x + rectFrom.w * 0.5f );
				break;
			case ALIGN_TOPS:
				y = rectTo.y - rectFrom.y;
				break;
			case ALIGN_BOTTOMS:
				y = ( rectTo.y + rectTo.h ) - ( rectFrom.y + rectFrom.h );
				break;
			case ALIGN_MIDDLES:
				y = ( rectTo.y + rectTo.h * 0.5f ) - ( rectFrom.y + rectFrom.h * 0.5f );
				break;
			default:
				assert( false );
				break;
		}

		group->Append( new rvGEMoveModifier( modName, m_selections[i], x, y ) );
	}

	m_modifiers.Append( group );
	m_modifiers.BlockNextMerge();

	SetModified( true );
}

/*
================
rvGEWorkspace::MakeSelectedAChild
================
*/
void rvGEWorkspace::MakeSelectedAChild()
{
	if( !rvGEWindowWrapper::GetWrapper( m_selections[0] )->CanHaveChildren() )
	{
		m_app->MessageBox( "Cannot add children to an htmlDef item", wxOK | wxICON_ERROR );
		return;
	}

	rvGEModifierGroup* group = new rvGEModifierGroup;

	for( int i = 1; i < m_selections.Num(); ++i )
	{
		if( m_selections[i]->GetParent() == m_selections[0] )
		{
			continue;
		}
		if( !m_selections[i]->GetParent() )
		{
			continue;
		}

		group->Append( new rvGEInsertModifier( "Make Child", m_selections[i], m_selections[0], NULL ) );
	}

	m_modifiers.Append( group );

	m_app->GetNavigator().Update();
	SetModified( true );
}

/*
================
rvGEWorkspace::Copy
================
*/
void rvGEWorkspace::Copy()
{
	for( int i = 0; i < m_clipboard.Num(); ++i )
	{
		delete m_clipboard[i];
	}
	m_clipboard.Clear();

	for( int i = 0; i < m_selections.Num(); ++i )
	{
		rvGEWindowWrapper* wrapper = rvGEWindowWrapper::GetWrapper( m_selections[i] );
		if( !wrapper )
		{
			continue;
		}

		rvGEClipboardItem* item = new rvGEClipboardItem;
		item->mStateDict  = wrapper->GetStateDict();
		item->mScriptDict = wrapper->GetScriptDict();
		item->mVarDict    = wrapper->GetVariableDict();

		item->mStateDict.Set( "windowType", rvGEWindowWrapper::WindowTypeToString( wrapper->GetWindowType() ) );

		m_clipboard.Append( item );
	}
}

/*
================
rvGEWorkspace::Paste
================
*/
void rvGEWorkspace::Paste()
{
	rvGEModifierGroup* group = new rvGEModifierGroup;

	m_selections.Clear();

	for( int i = 0; i < m_clipboard.Num(); ++i )
	{
		idDict state;
		state.Copy( m_clipboard[i]->mStateDict );

		const rvGEWindowWrapper::EWindowType type = rvGEWindowWrapper::StringToWindowType( state.GetString( "windowType", "windowDef" ) );
		state.Delete( "windowType" );

		idWindow* window = NewWindow( &state, type );
		if( !window )
		{
			continue;
		}

		group->Append( new rvGEInsertModifier( "Paste", window, m_interface->GetDesktop(), NULL ) );
		m_selections.Add( window );

		rvGEWindowWrapper::GetWrapper( window )->GetScriptDict() = m_clipboard[i]->mScriptDict;
		rvGEWindowWrapper::GetWrapper( window )->GetVariableDict() = m_clipboard[i]->mVarDict;
	}

	m_modifiers.Append( group );
	m_app->GetNavigator().Update();
	SetModified( true );
}

/*
================
rvGEWorkspace::NewWindow
================
*/
idWindow* rvGEWorkspace::NewWindow( idDict* state, rvGEWindowWrapper::EWindowType type )
{
	idWindow* window = NULL;

	switch( type )
	{
		case rvGEWindowWrapper::WT_NORMAL:
			window = new idWindow( m_interface );
			break;
		case rvGEWindowWrapper::WT_BIND:
			window = new idBindWindow( m_interface );
			break;
		case rvGEWindowWrapper::WT_RENDER:
			window = new idRenderWindow( m_interface );
			break;
		case rvGEWindowWrapper::WT_CHOICE:
			window = new idChoiceWindow( m_interface );
			break;
		case rvGEWindowWrapper::WT_EDIT:
			window = new idEditWindow( m_interface );
			break;
		default:
			assert( false );
			return NULL;
	}

	idStr baseName = state ? state->GetString( "name", "unnamed" ) : "unnamed";
	baseName.StripQuotes();

	int count = 0;
	if( m_interface->GetDesktop()->FindChildByName( baseName ) )
	{
		count = 1;
		while( true )
		{
			drawWin_t* dw = m_interface->GetDesktop()->FindChildByName( va( "%s%d", baseName.c_str(), count ) );
			if( !dw )
			{
				break;
			}

			rvGEWindowWrapper* wrapper = rvGEWindowWrapper::GetWrapper( dw->win );
			if( wrapper && wrapper->IsDeleted() )
			{
				break;
			}

			++count;
		}
	}

	idStr winName = count ? va( "%s%d", baseName.c_str(), count ) : baseName;
	idStr winTemplate = winName + " { }";

	idParser src( winTemplate, winTemplate.Length(), "",
				  LEXFL_ALLOWMULTICHARLITERALS |
				  LEXFL_NOSTRINGCONCAT |
				  LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
	window->Parse( &src );

	rvGEWindowWrapper* wrapper = rvGEWindowWrapper::GetWrapper( window );

	if( state )
	{
		wrapper->SetState( *state );
	}

	wrapper->SetStateKey( "name", winName );
	wrapper->Finish();

	SetModified( true );
	return window;
}

/*
================
rvGEWorkspace::AddWindow
================
*/
idWindow* rvGEWorkspace::AddWindow( rvGEWindowWrapper::EWindowType type )
{
	idDict state;
	state.Set( "rect", "0,0,100,100" );
	state.Set( "visible", "1" );

	idWindow* window = NewWindow( &state, type );
	if( !window )
	{
		return NULL;
	}

	m_modifiers.Append( new rvGEInsertModifier( "New", window, m_interface->GetDesktop(), NULL ) );

	m_selections.Set( window );

	m_app->GetNavigator().Update();
	m_app->GetTransformer().Update();
	m_app->GetProperties().Update();

	return window;
}

/*
================
rvGEWorkspace::EditSelectedProperties
================
*/
bool rvGEWorkspace::EditSelectedProperties()
{
	if( m_selections.Num() != 1 )
	{
		return false;
	}

	idDict dict;
	if( GEItemPropsDlg_DoModal( this, m_selections[0], dict ) )
	{
		m_modifiers.Append( new rvGEStateModifier( "Item Properties", m_selections[0], dict ) );
		SetModified( true );
	}

	m_app->GetNavigator().Update();
	m_app->GetTransformer().Update();
	m_app->GetProperties().Update();

	return true;
}

/*
================
rvGEWorkspace::EditSelectedScripts
================
*/
bool rvGEWorkspace::EditSelectedScripts()
{
	if( m_selections.Num() != 1 )
	{
		return false;
	}

	rvGEItemScriptsDlg dlg( this, m_selections[0] );
	if( dlg.ShowModal() == wxID_OK )
	{
		m_app->GetNavigator().Refresh();
		SetModified( true );
	}

	return true;
}

/*
================
rvGEWorkspace::CleanupEnumProc
================
*/
bool rvGEWorkspace::CleanupEnumProc( rvGEWindowWrapper* wrapper, void* data )
{
	if( !wrapper )
	{
		return true;
	}
	wrapper->EnumChildren( CleanupEnumProc, data );
	delete wrapper;
	return true;
}

/*
================
rvGEWorkspace::ShowAllEnumProc
================
*/
bool rvGEWorkspace::ShowAllEnumProc( rvGEWindowWrapper* wrapper, void* data )
{
	rvGEModifierGroup* group = ( rvGEModifierGroup* )data;

	wrapper->EnumChildren( ShowAllEnumProc, data );

	if( wrapper->IsHidden() )
	{
		group->Append( new rvGEHideModifier( "Show Hidden", wrapper->GetWindow(), false ) );
	}

	return true;
}

/*
================
rvGEWorkspace::BuildSelectMenuEnumProc
================
*/
bool rvGEWorkspace::BuildSelectMenuEnumProc( rvGEWindowWrapper* wrapper, void* data )
{
	rvGEWorkspace* workspace = ( rvGEWorkspace* )data;
	assert( workspace );

	if( !wrapper )
	{
		return true;
	}

	wrapper->EnumChildren( BuildSelectMenuEnumProc, data );

	if( wrapper->IsDeleted() || wrapper->IsHidden() )
	{
		return true;
	}

	if( wrapper->GetScreenRect().Contains( workspace->m_selectMenuPos[0], workspace->m_selectMenuPos[1] ) )
	{
		workspace->m_selectMenu.Append( wrapper->GetWindow() );
	}

	return true;
}