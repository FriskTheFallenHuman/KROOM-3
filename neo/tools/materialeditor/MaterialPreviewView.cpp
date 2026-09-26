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

#include "../../renderer/RenderCommon.h"
#include "../../renderer/RenderContext.h"

#include "MaterialPreviewView.h"
#include "MaterialDocManager.h"

const char* const PREVIEW_CAPTURE_IMAGE = "_materialPreviewCapture";

// TODO: I think i should remove these ones
static bool Sys_KeyDown( int key )
{
	return ( ( ::GetAsyncKeyState( key ) & 0x8000 ) != 0 );
}

static float fDiff( float f1, float f2 )
{
	if( f1 > f2 )
	{
		return f1 - f2;
	}
	else
	{
		return f2 - f1;
	}
}

#ifndef MK_LBUTTON
	#define MK_LBUTTON  0x0001
	#define MK_RBUTTON  0x0002
	#define MK_MBUTTON  0x0004
#endif
// END TODO

/*
==============================================================================

idGLDrawableView

==============================================================================
*/

/*
================
idGLDrawableView::idGLDrawableView
================
*/
idGLDrawableView::idGLDrawableView()
{
	material = NULL;
	modelDefHandle = -1;

	objectId = 0;
	showLights = true;

	viewOrigin.Set( 0.f, 0.f, 0.f );
	viewRotation.Set( 0.f, 0.f, 0.f );
	viewDistance = -196.f;

	world = NULL;
	worldModel = NULL;

	button = 0;
	handleMove = false;
	pressX = 0.f;
	pressY = 0.f;

	ResetView();
}

/*
================
idGLDrawableView::~idGLDrawableView
================
*/
idGLDrawableView::~idGLDrawableView()
{
	delete world;
	delete worldModel;
}

/*
================
idGLDrawableView::ResetView
================
*/
void idGLDrawableView::ResetView()
{
	idDict spawnArgs;

	InitWorld();

	memset( &worldEntity, 0, sizeof( worldEntity ) );
	spawnArgs.Clear();
	spawnArgs.Set( "classname", "func_static" );
	spawnArgs.Set( "name", spawnArgs.GetString( "model" ) );
	spawnArgs.Set( "origin", "0 0 0" );

	gameEdit->ParseSpawnArgsToRenderEntity( &spawnArgs, &worldEntity );

	// Load a model and set the current material as its custom shader.
	worldModel = renderModelManager->FindModel( "models/materialeditor/cube128.ase" );
	worldEntity.hModel = worldModel;

	// current material
	worldEntity.customShader = material;

	// current rotation
	worldEntity.axis = mat3_identity;

	// set global shader parms
	memset( globalParms, 0, sizeof( globalParms ) );
	globalParms[0] = globalParms[1] = globalParms[2] = globalParms[3] = 1.f;

	worldEntity.shaderParms[0] = 1.f;
	worldEntity.shaderParms[1] = 1.f;
	worldEntity.shaderParms[2] = 1.f;
	worldEntity.shaderParms[3] = 1.f;

	modelDefHandle = world->AddEntityDef( &worldEntity );
}

/*
================
idGLDrawableView::InitWorld
================
*/
void idGLDrawableView::InitWorld()
{
	if( world == NULL )
	{
		world = renderSystem->AllocRenderWorld();
	}
	if( worldModel == NULL )
	{
		worldModel = renderModelManager->AllocModel();
	}

	world->InitFromMap( NULL );
	worldModel->InitEmpty( "GLWorldModel" );

	viewLights.Clear();
}

/*
================
idGLDrawableView::zoomBy
================
*/
void idGLDrawableView::zoomBy( float delta )
{
	viewDistance += delta;
	viewDistance = idMath::ClampFloat( -1024.f, -32.f, viewDistance );
}

/*
================
idGLDrawableView::panObject
================
*/
void idGLDrawableView::panObject( float dx, float dy )
{
	idVec3 forward, right, up;
	viewRotation.ToVectors( &forward, &right, &up );

	const float scale = -viewDistance / 300.f;

	viewOrigin += right * ( -dx * scale );
	viewOrigin += up    * ( dy * scale );

	UpdateModel();
}

/*
================
idGLDrawableView::buttonDown
================
*/
void idGLDrawableView::buttonDown( int _button, float x, float y )
{
	pressX = x;
	pressY = y;
	button = _button;
	if( button == MK_RBUTTON || button == MK_LBUTTON || button == MK_MBUTTON )
	{
		handleMove = true;
	}
}

/*
================
idGLDrawableView::mouseMove
================
*/
void idGLDrawableView::mouseMove( float x, float y )
{
	const float sensitivity = 0.5f;

	if( !handleMove )
	{
		return;
	}

	// Left mouse button rotates and zooms the view
	if( button == MK_LBUTTON )
	{
		const bool doZoom = Sys_KeyDown( VK_MENU );
		if( doZoom )
		{
			if( y != pressY )
			{
				viewDistance -= ( y - pressY );
				pressY = y;
			}
		}
		else
		{
			float xo = 0.f;
			float yo = 0.f;

			if( x != pressX )
			{
				xo = ( x - pressX );
				pressX = x;
			}
			if( y != pressY )
			{
				yo = ( y - pressY );
				pressY = y;
			}

			viewRotation.yaw += -( xo * sensitivity );
			viewRotation.pitch += ( yo * sensitivity );

			viewRotation.pitch = idMath::ClampFloat( -89.9f, 89.9f, viewRotation.pitch );
		}

		// Right mouse button moves lights in the view plane
	}
	else if( button == MK_RBUTTON )
	{
		float   lightMovement = 0.f;
		idVec3  lightForward, lightRight, lightUp;
		idVec3  lightMove;
		lightMove.Zero();

		viewRotation.ToVectors( &lightForward, &lightRight, &lightUp );

		const bool doZoom = Sys_KeyDown( VK_MENU );
		if( doZoom )
		{
			if( y != pressY )
			{
				lightMovement = -( y - pressY ) * sensitivity;
				pressY = y;

				lightMovement = idMath::ClampFloat( -32.f, 32.f, lightMovement );
				lightMove = lightForward * lightMovement;
			}
		}
		else
		{
			if( x != pressX )
			{
				lightMovement = ( x - pressX ) * sensitivity;
				pressX = x;

				lightMovement = idMath::ClampFloat( -32.f, 32.f, lightMovement );
				lightMove = lightRight * lightMovement;
			}
			if( y != pressY )
			{
				lightMovement = -( y - pressY ) * sensitivity;
				pressY = y;

				lightMovement = idMath::ClampFloat( -32.f, 32.f, lightMovement );
				lightMove += lightUp * lightMovement;
			}
		}

		// Go through the lights and move the ones that are set to allow movement
		for( int i = 0; i < viewLights.Num(); i++ )
		{
			lightInfo_t* vLight = &viewLights[i];
			if( vLight->allowMove )
			{
				vLight->origin += lightMove;
			}
		}

		// Middle mouse button moves object up and down
	}
	else if( button == MK_MBUTTON )
	{
		float xo = 0.f;
		float yo = 0.f;
		if( x != pressX )
		{
			xo = ( x - pressX );
			pressX = x;
		}
		if( y != pressY )
		{
			yo = ( y - pressY );
			pressY = y;
		}

		if( xo != 0.f || yo != 0.f )
		{
			panObject( xo, yo );
		}
	}
}

/*
================
idGLDrawableView::addLight
================
*/
void idGLDrawableView::addLight()
{
	const int lightId = viewLights.Num();

	idStr str;
	idDict spawnArgs;
	spawnArgs.Set( "classname", "light" );
	spawnArgs.Set( "name", va( "light_%d", lightId ) );
	spawnArgs.Set( "origin", va( "-128 0 %d", ( lightId * 16 ) ) );
	spawnArgs.Set( "light", "300" );
	spawnArgs.Set( "texture", "lights/defaultPointLight" );
	sprintf( str, "%f %f %f", 1.f, 1.f, 1.f );
	spawnArgs.Set( "_color", str );

	lightInfo_t viewLight;
	gameEdit->ParseSpawnArgsToRenderLight( &spawnArgs, &viewLight.renderLight );

	viewLight.lightDefHandle = world->AddLightDef( &viewLight.renderLight );
	viewLight.origin = viewLight.renderLight.origin;
	viewLight.shader = declManager->FindMaterial( "lights/defaultPointLight", false );
	viewLight.color.x = viewLight.renderLight.shaderParms[ SHADERPARM_RED ];
	viewLight.color.y = viewLight.renderLight.shaderParms[ SHADERPARM_GREEN ];
	viewLight.color.z = viewLight.renderLight.shaderParms[ SHADERPARM_BLUE ];
	viewLight.radius = 300.f;
	viewLight.allowMove = true;

	// Add light to the list
	viewLights.Append( viewLight );
}

/*
================
idGLDrawableView::deleteLight
================
*/
void idGLDrawableView::deleteLight( const int lightId )
{
	if( lightId >= 0 && lightId < viewLights.Num() )
	{
		world->FreeLightDef( viewLights[lightId].lightDefHandle );
		viewLights.RemoveIndex( lightId );
	}
}

/*
================
idGLDrawableView::UpdateCamera
================
*/
void idGLDrawableView::UpdateCamera( renderView_t* refdef )
{
	// Set the camera origin
	idVec3 pos = viewRotation.ToForward();
	pos *= viewDistance;
	refdef->vieworg = pos;

	// Set the view to point back at the origin
	const idVec3 dir = vec3_origin - pos;
	const idAngles angs = dir.ToAngles();
	refdef->viewaxis = angs.ToMat3();
}

/*
================
idGLDrawableView::UpdateModel
================
*/
void idGLDrawableView::UpdateModel()
{
	switch( objectId )
	{
		case 0:
			worldModel = renderModelManager->FindModel( "models/materialeditor/cube128.ase" );
			break;
		case 1:
			worldModel = renderModelManager->FindModel( "models/materialeditor/box128x64.ase" );
			break;
		case 2:
			worldModel = renderModelManager->FindModel( "models/materialeditor/box128x32.ase" );
			break;
		case 3:
			worldModel = renderModelManager->FindModel( "models/materialeditor/box64x128.ase" );
			break;
		case 4:
			worldModel = renderModelManager->FindModel( "models/materialeditor/box32x128.ase" );
			break;
		case 5:
			worldModel = renderModelManager->FindModel( "models/materialeditor/cylinder_v.ase" );
			break;
		case 6:
			worldModel = renderModelManager->FindModel( "models/materialeditor/cylinder_h.ase" );
			break;
		case 7:
			worldModel = renderModelManager->FindModel( "models/materialeditor/sphere64.ase" );
			break;
		case -1:
			worldModel = renderModelManager->FindModel( customModelName.c_str() );
			break;
		default:
			worldModel = renderModelManager->FindModel( "models/materialeditor/cube128.ase" );
			break;
	}

	worldEntity.hModel = worldModel;

	// current material
	worldEntity.customShader = material;
	// current rotation
	worldEntity.origin = viewOrigin;

	worldEntity.axis = mat3_identity;

	world->UpdateEntityDef( modelDefHandle, &worldEntity );
}

/*
================
idGLDrawableView::UpdateLights
================
*/
void idGLDrawableView::UpdateLights()
{
	for( int i = 0; i < viewLights.Num(); i++ )
	{
		lightInfo_t* vLight = &viewLights[i];

		vLight->renderLight.shader = vLight->shader;

		vLight->renderLight.shaderParms[SHADERPARM_RED]   = vLight->color.x;
		vLight->renderLight.shaderParms[SHADERPARM_GREEN] = vLight->color.y;
		vLight->renderLight.shaderParms[SHADERPARM_BLUE]  = vLight->color.z;

		vLight->renderLight.lightRadius[0] =
			vLight->renderLight.lightRadius[1] =
				vLight->renderLight.lightRadius[2] = vLight->radius;

		vLight->renderLight.origin = vLight->origin;

		world->UpdateLightDef( vLight->lightDefHandle, &vLight->renderLight );
	}
}

/*
================
idGLDrawableView::drawLights
================
*/
void idGLDrawableView::drawLights( renderView_t* refdef )
{
	for( int i = 0; i < viewLights.Num(); i++ )
	{
		lightInfo_t* vLight = &viewLights[i];

		const idVec4 lColor( vLight->color.x, vLight->color.y, vLight->color.z, 1.f );

		const idSphere sphere( vLight->renderLight.origin, 4 );
		world->DebugSphere( lColor, sphere, 0, true );
		world->DrawText( va( "%d", i + 1 ),
						 vLight->renderLight.origin + idVec3( 0, 0, 5 ),
						 0.25f,
						 idVec4( 1, 1, 0, 1 ),
						 refdef->viewaxis, 1, 0, true );
	}
}

/*
================
idGLDrawableView::GetLightInfo
================
*/
bool idGLDrawableView::GetLightInfo( int lightId, idStr& shaderName, idVec3& color, float& radius, bool& allowMove ) const
{
	if( lightId < 0 || lightId >= viewLights.Num() )
	{
		return false;
	}

	const lightInfo_t* vLight = &viewLights[lightId];
	shaderName = vLight->shader ? vLight->shader->GetName() : "";
	color = vLight->color;
	radius = vLight->radius;
	allowMove  = vLight->allowMove;
	return true;
}

/*
================
idGLDrawableView::renderToEngineFramebuffer
================
*/
void idGLDrawableView::renderToEngineFramebuffer( int w, int h )
{
	if( !material || w < 1 || h < 1 )
	{
		return;
	}

	world->DebugClearLines( 0 );

	UpdateLights();

	renderView_t refdef;
	memset( &refdef, 0, sizeof( refdef ) );

	UpdateCamera( &refdef );

	for( int i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ )
	{
		refdef.shaderParms[ i ] = globalParms[ i ];
	}

	refdef.rdflags |= RDF_EDITOR_PREVIEW;

	refdef.fov_x = 90;
	refdef.fov_y = 2 * atan( ( float )h / w ) * idMath::M_RAD2DEG;

	refdef.time[0] = refdef.time[1] = eventLoop->Milliseconds();

	if( showLights )
	{
		drawLights( &refdef );
	}

	renderSystem->RenderEditorView( world, &refdef, w, h );

	world->DebugClearLines( 0 );
}

/*
================
idGLDrawableView::setMedia
================
*/
void idGLDrawableView::setMedia( const char* name )
{
	float ratio = 1.f;

	if( name && *name )
	{
		material = declManager->FindMaterial( name );
	}
	else
	{
		material = NULL;
	}

	if( material && material->GetNumStages() == 0 )
	{
		material = declManager->FindMaterial( "_default" );
	}

	if( material && material->GetStage( 0 ) && material->GetStage( 0 )->texture.image )
	{
		ratio = ( float )material->GetImageWidth() / ( float )material->GetImageHeight();
	}

	if( objectId == -1 )
	{
		// Custom model - don't change.
	}
	else if( ratio == 1.f )
	{
		objectId = 0;
	}
	else if( ratio == 2.f )
	{
		objectId = 1;
	}
	else if( ratio == 4.f )
	{
		objectId = 2;
	}
	else if( ratio == 0.5f )
	{
		objectId = 3;
	}
	else if( ratio == 0.25f )
	{
		objectId = 4;
	}

	UpdateModel();
}

/*
================
idGLDrawableView::setLocalParm
================
*/
void idGLDrawableView::setLocalParm( int parmNum, float value )
{
	if( parmNum < 0 || parmNum >= MAX_ENTITY_SHADER_PARMS )
	{
		return;
	}
	worldEntity.shaderParms[parmNum] = value;
	UpdateModel();
}

/*
================
idGLDrawableView::setGlobalParm
================
*/
void idGLDrawableView::setGlobalParm( int parmNum, float value )
{
	if( parmNum < 0 || parmNum >= MAX_GLOBAL_SHADER_PARMS )
	{
		return;
	}
	globalParms[parmNum] = value;
}

/*
================
idGLDrawableView::setLightShader
================
*/
void idGLDrawableView::setLightShader( const int lightId, const idStr shaderName )
{
	if( lightId >= 0 && lightId < viewLights.Num() )
	{
		viewLights[lightId].shader = declManager->FindMaterial( shaderName, false );
	}
}

/*
================
idGLDrawableView::setLightColor
================
*/
void idGLDrawableView::setLightColor( const int lightId, const idVec3& value )
{
	if( lightId >= 0 && lightId < viewLights.Num() )
	{
		viewLights[lightId].color = value;
	}
}

/*
================
idGLDrawableView::setLightRadius
================
*/
void idGLDrawableView::setLightRadius( const int lightId, const float radius )
{
	if( lightId >= 0 && lightId < viewLights.Num() )
	{
		viewLights[lightId].radius = radius;
	}
}

/*
================
idGLDrawableView::setLightAllowMove
================
*/
void idGLDrawableView::setLightAllowMove( const int lightId, const bool move )
{
	if( lightId >= 0 && lightId < viewLights.Num() )
	{
		viewLights[lightId].allowMove = move;
	}
}

/*
================
idGLDrawableView::setObject
================
*/
void idGLDrawableView::setObject( int id )
{
	objectId = id;
	UpdateModel();
}

/*
================
idGLDrawableView::setCustomModel
================
*/
void idGLDrawableView::setCustomModel( const idStr modelName )
{
	if( modelName.Length() )
	{
		objectId = -1;
	}
	else
	{
		objectId = 0;
	}

	customModelName = modelName;
	UpdateModel();
}

/*
================
idGLDrawableView::setShowLights
================
*/
void idGLDrawableView::setShowLights( bool _showLights )
{
	showLights = _showLights;
}

/*
==============================================================================

MaterialPreviewView

==============================================================================
*/

/*
================
MaterialPreviewView::MaterialPreviewView
================
*/
MaterialPreviewView::MaterialPreviewView( wxWindow* parent )
	: wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS )
	, m_repaintTimer( this )
	, m_needsRepaint( false )
	, m_fbo( 0 )
	, m_colorTex( 0 )
	, m_depthStencil( 0 )
	, m_fboWidth( 0 )
	, m_fboHeight( 0 )
{
	SetBackgroundStyle( wxBG_STYLE_PAINT );

	// Default material.
	m_renderedView.setMedia( "_default" );

	Bind( wxEVT_PAINT, &MaterialPreviewView::OnPaint, this );
	Bind( wxEVT_SIZE, &MaterialPreviewView::OnSize, this );
	Bind( wxEVT_ERASE_BACKGROUND, &MaterialPreviewView::OnEraseBackground, this );
	Bind( wxEVT_LEFT_DOWN, &MaterialPreviewView::OnMouseDown, this );
	Bind( wxEVT_RIGHT_DOWN, &MaterialPreviewView::OnMouseDown, this );
	Bind( wxEVT_MIDDLE_DOWN, &MaterialPreviewView::OnMouseDown, this );
	Bind( wxEVT_LEFT_UP, &MaterialPreviewView::OnMouseUp, this );
	Bind( wxEVT_RIGHT_UP, &MaterialPreviewView::OnMouseUp, this );
	Bind( wxEVT_MIDDLE_UP, &MaterialPreviewView::OnMouseUp, this );
	Bind( wxEVT_MOTION, &MaterialPreviewView::OnMouseMotion, this );
	Bind( wxEVT_MOUSEWHEEL, &MaterialPreviewView::OnMouseWheel, this );
	Bind( wxEVT_LEAVE_WINDOW, &MaterialPreviewView::OnMouseLeave, this );
}

/*
================
MaterialPreviewView::~MaterialPreviewView
================
*/
MaterialPreviewView::~MaterialPreviewView()
{
	if( m_fbo || m_colorTex || m_depthStencil )
	{
		rRenderContext.MakeCurrent();
		DestroyPreviewFBO();
	}
}

/*
================
MaterialPreviewView::EnsurePreviewFBO
================
*/
void MaterialPreviewView::EnsurePreviewFBO( int w, int h )
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
		common->Warning( "MaterialPreviewView: FBO incomplete: 0x%x", status );
	}

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glBindRenderbuffer( GL_RENDERBUFFER, 0 );

	m_fboWidth  = w;
	m_fboHeight = h;

	m_pixels.assign( size_t( w ) * size_t( h ) * 4u, 0 );

	RequestRepaint();
}

/*
================
MaterialPreviewView::DestroyPreviewFBO
================
*/
void MaterialPreviewView::DestroyPreviewFBO()
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
MaterialPreviewView::RequestRepaint
================
*/
void MaterialPreviewView::RequestRepaint()
{
	m_needsRepaint = true;

	Refresh( false );
}

/*
================
MaterialPreviewView::OnPaint
================
*/
void MaterialPreviewView::OnPaint( wxPaintEvent& /*event*/ )
{
	static bool s_inEditorRender = false;
	if( s_inEditorRender )
	{
		return;
	}

	wxAutoBufferedPaintDC dc( this );

	const wxSize size = GetClientSize();
	const int w = size.GetWidth();
	const int h = size.GetHeight();

	if( w <= 0 || h <= 0 )
	{
		return;
	}

	s_inEditorRender = true;

	rRenderContext.MakeCurrent();
	EnsurePreviewFBO( w, h );

	glBindFramebuffer( GL_FRAMEBUFFER, m_fbo );
	glViewport( 0, 0, w, h );
	glScissor( 0, 0, w, h );
	glClearColor( 0.1f, 0.1f, 0.1f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	Framebuffer::SetEditorDefaultFramebuffer( m_fbo );

	m_renderedView.renderToEngineFramebuffer( w, h );

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

	wxBitmap bmp( img );
	dc.DrawBitmap( bmp, 0, 0, false );

	m_needsRepaint = false;
	s_inEditorRender = false;
}

/*
================
MaterialPreviewView::OnSize
================
*/
void MaterialPreviewView::OnSize( wxSizeEvent& event )
{
	RequestRepaint();
	event.Skip();
}

/*
================
MaterialPreviewView::OnEraseBackground
================
*/
void MaterialPreviewView::OnEraseBackground( wxEraseEvent& /*event*/ )
{
	// Handled by the paint handler.
}

/*
================
MaterialPreviewView::OnMouseDown
================
*/
void MaterialPreviewView::OnMouseDown( wxMouseEvent& event )
{
	SetFocus();

	if( !HasCapture() )
	{
		CaptureMouse();
	}

	int button = 0;
	if( event.LeftDown() )
	{
		button = MK_LBUTTON;
	}
	else if( event.RightDown() )
	{
		button = MK_RBUTTON;
	}
	else if( event.MiddleDown() )
	{
		button = MK_MBUTTON;
	}

	m_renderedView.buttonDown( button, ( float )event.GetX(), ( float )event.GetY() );
}

/*
================
MaterialPreviewView::OnMouseUp
================
*/
void MaterialPreviewView::OnMouseUp( wxMouseEvent& /*event*/ )
{
	if( HasCapture() )
	{
		ReleaseMouse();
	}
	m_renderedView.buttonUp( 0 );
	RequestRepaint();
}

/*
================
MaterialPreviewView::OnMouseMotion
================
*/
void MaterialPreviewView::OnMouseMotion( wxMouseEvent& event )
{
	if( HasCapture() )
	{
		m_renderedView.mouseMove( ( float )event.GetX(), ( float )event.GetY() );
		RequestRepaint();
	}
}

/*
================
MaterialPreviewView::OnMouseWheel
================
*/
void MaterialPreviewView::OnMouseWheel( wxMouseEvent& event )
{
	const float notches = event.GetWheelRotation() / 120.f;

	m_renderedView.zoomBy( notches * 32.f );

	RequestRepaint();
}

/*
================
MaterialPreviewView::OnMouseLeave
================
*/
void MaterialPreviewView::OnMouseLeave( wxMouseEvent& /*event*/ )
{
	if( HasCapture() )
	{
		ReleaseMouse();
	}
}

/*
================
MaterialPreviewView::MV_OnMaterialSelectionChange
================
*/
void MaterialPreviewView::MV_OnMaterialSelectionChange( MaterialDoc* pMaterial )
{
	if( pMaterial && pMaterial->renderMaterial )
	{
		m_currentMaterial = pMaterial->renderMaterial->GetName();
		m_renderedView.setMedia( m_currentMaterial );
		RequestRepaint();
	}
}

/*
================
MaterialPreviewView::OnModelChange
================
*/
void MaterialPreviewView::OnModelChange( int modelId )
{
	m_renderedView.setObject( modelId );
	RequestRepaint();
}

/*
================
MaterialPreviewView::OnCustomModelChange
================
*/
void MaterialPreviewView::OnCustomModelChange( idStr modelName )
{
	m_renderedView.setCustomModel( modelName );
	RequestRepaint();
}

/*
================
MaterialPreviewView::OnShowLightsChange
================
*/
void MaterialPreviewView::OnShowLightsChange( bool showLights )
{
	m_renderedView.setShowLights( showLights );
	RequestRepaint();
}

/*
================
MaterialPreviewView::OnLocalParmChange
================
*/
void MaterialPreviewView::OnLocalParmChange( int parmNum, float value )
{
	m_renderedView.setLocalParm( parmNum, value );
	RequestRepaint();
}

/*
================
MaterialPreviewView::OnGlobalParmChange
================
*/
void MaterialPreviewView::OnGlobalParmChange( int parmNum, float value )
{
	m_renderedView.setGlobalParm( parmNum, value );
	RequestRepaint();
}

/*
================
MaterialPreviewView::OnLightShaderChange
================
*/
void MaterialPreviewView::OnLightShaderChange( int lightId, idStr shaderName )
{
	m_renderedView.setLightShader( lightId, shaderName );
	RequestRepaint();
}

/*
================
MaterialPreviewView::OnLightRadiusChange
================
*/
void MaterialPreviewView::OnLightRadiusChange( int lightId, float radius )
{
	m_renderedView.setLightRadius( lightId, radius );
	RequestRepaint();
}

/*
================
MaterialPreviewView::OnLightColorChange
================
*/
void MaterialPreviewView::OnLightColorChange( int lightId, idVec3& color )
{
	m_renderedView.setLightColor( lightId, color );
	RequestRepaint();
}

/*
================
MaterialPreviewView::OnLightAllowMoveChange
================
*/
void MaterialPreviewView::OnLightAllowMoveChange( int lightId, bool move )
{
	m_renderedView.setLightAllowMove( lightId, move );
	RequestRepaint();
}

/*
================
MaterialPreviewView::OnAddLight
================
*/
void MaterialPreviewView::OnAddLight()
{
	m_renderedView.addLight();
	RequestRepaint();
}

/*
================
MaterialPreviewView::OnDeleteLight
================
*/
void MaterialPreviewView::OnDeleteLight( int lightId )
{
	m_renderedView.deleteLight( lightId );
	RequestRepaint();
}