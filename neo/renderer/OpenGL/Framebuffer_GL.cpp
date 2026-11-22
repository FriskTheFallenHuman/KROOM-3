/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2014-2020 Robert Beckebans

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

#include "../RenderCommon.h"
#include "Framebuffer_GL.h"

static GLuint g_editorDefaultFBO = 0;
static GLuint g_editorFBO = 0;
static GLuint g_editorColorTex = 0;
static GLuint g_editorDepthStencil = 0;
static int g_editorWidth = 0;
static int g_editorHeight = 0;

static void R_DestroyEditorTarget()
{
	if( g_editorFBO )
	{
		glDeleteFramebuffers( 1, &g_editorFBO );
		g_editorFBO = 0;
	}
	if( g_editorColorTex )
	{
		glDeleteTextures( 1, &g_editorColorTex );
		g_editorColorTex = 0;
	}
	if( g_editorDepthStencil )
	{
		glDeleteRenderbuffers( 1, &g_editorDepthStencil );
		g_editorDepthStencil = 0;
	}
	g_editorWidth = 0;
	g_editorHeight = 0;
	g_editorDefaultFBO = 0;
}

static void R_ListFramebuffers_f( const idCmdArgs& args )
{
	if( !glConfig.framebufferObjectAvailable )
	{
		common->Printf( "GL_EXT_framebuffer_object is not available.\n" );
		return;
	}
}

/*
================
FramebufferGL::FramebufferGL
================
*/
FramebufferGL::FramebufferGL( const char* name, int w, int h ) : Framebuffer( name, w, h )
{
	glGenFramebuffers( 1, &frameBuffer );
}

/*
================
FramebufferGL::~FramebufferGL
================
*/
FramebufferGL::~FramebufferGL()
{
	glDeleteFramebuffers( 1, &frameBuffer );
}

/*
================
FramebufferGL::Create
================
*/
Framebuffer* Framebuffer::Create( const char* name, int width, int height )
{
	return new FramebufferGL( name, width, height );
}

/*
================
Framebuffer::Init
================
*/
void Framebuffer::Init()
{
	cmdSystem->AddCommand( "listFramebuffers", R_ListFramebuffers_f, CMD_FL_RENDERER, "lists framebuffers" );

	backEnd.currentFramebuffer = NULL;

	// SHADOWMAPS
	int width, height;
	width = height = r_shadowMapImageSize.GetInteger();
	for( int i = 0; i < MAX_SHADOWMAP_RESOLUTIONS; i++ )
	{
		width = height = shadowMapResolutions[i];

		globalFramebuffers.shadowFBO[i] = Create( va( "_shadowMap%i", i ), width, height );
		globalFramebuffers.shadowFBO[i]->Bind();
		glDrawBuffers( 0, NULL );
	}

	// HDR
	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();

	globalFramebuffers.hdrFBO = Create( "_hdr", screenWidth, screenHeight );
	globalFramebuffers.hdrFBO->Bind();

#if defined(USE_HDR_MSAA)
	if( glConfig.multisamples )
	{
		globalFramebuffers.hdrFBO->AttachImage2D( GL_TEXTURE_2D_MULTISAMPLE, globalImages->currentRenderHDRImage, 0 );
		globalFramebuffers.hdrFBO->AttachImageDepth( GL_TEXTURE_2D_MULTISAMPLE, globalImages->currentDepthImage );
	}
	else
#endif
	{
		globalFramebuffers.hdrFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->currentRenderHDRImage, 0 );
		globalFramebuffers.hdrFBO->AttachImageDepth( GL_TEXTURE_2D, globalImages->currentDepthImage );
	}
	globalFramebuffers.hdrFBO->Check();

	// HDR no MSAA
#if defined(USE_HDR_MSAA)
	globalFramebuffers.hdrNonMSAAFBO = Create( "_hdrNoMSAA", screenWidth, screenHeight );
	globalFramebuffers.hdrNonMSAAFBO->Bind();
	globalFramebuffers.hdrNonMSAAFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->currentRenderHDRImageNoMSAA, 0 );
	globalFramebuffers.hdrNonMSAAFBO->Check();
#endif

	// HDR CUBEMAP CAPTURE
	globalFramebuffers.envprobeFBO = Create( "_envprobeRender", ENVPROBE_CAPTURE_SIZE, ENVPROBE_CAPTURE_SIZE );
	globalFramebuffers.envprobeFBO->Bind();
	globalFramebuffers.envprobeFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->envprobeHDRImage, 0 );
	globalFramebuffers.envprobeFBO->AttachImageDepth( GL_TEXTURE_2D, globalImages->envprobeDepthImage );
	globalFramebuffers.envprobeFBO->Check();

	// HDR DOWNSCALE
	globalFramebuffers.hdr64FBO = Create( "_hdr64", 64, 64 );
	globalFramebuffers.hdr64FBO->Bind();
	globalFramebuffers.hdr64FBO->AttachImage2D( GL_TEXTURE_2D, globalImages->currentRenderHDRImage64, 0 );
	globalFramebuffers.hdr64FBO->Check();

	// BLOOM
	for( int i = 0; i < MAX_BLOOM_BUFFERS; i++ )
	{
		globalFramebuffers.bloomRenderFBO[i] = Create( va( "_bloomRender%i", i ), screenWidth, screenHeight );
		globalFramebuffers.bloomRenderFBO[i]->Bind();
		globalFramebuffers.bloomRenderFBO[i]->AttachImage2D( GL_TEXTURE_2D, globalImages->bloomRenderImage[i], 0 );
		globalFramebuffers.bloomRenderFBO[i]->Check();
	}

	if( r_ssaoFiltering.GetBool() || r_ssgiFiltering.GetBool() )
	{
		// AMBIENT OCCLUSION
		for( int i = 0; i < MAX_SSAO_BUFFERS; i++ )
		{
			globalFramebuffers.ambientOcclusionFBO[i] = Create( va( "_aoRender%i", i ), screenWidth, screenHeight );
			globalFramebuffers.ambientOcclusionFBO[i]->Bind();
			globalFramebuffers.ambientOcclusionFBO[i]->AttachImage2D( GL_TEXTURE_2D, globalImages->ambientOcclusionImage[i], 0 );
			globalFramebuffers.ambientOcclusionFBO[i]->Check();
		}
	}

	// HIERARCHICAL Z BUFFER
	for( int i = 0; i < MAX_HIERARCHICAL_ZBUFFERS; i++ )
	{
		globalFramebuffers.csDepthFBO[i] = Create( va( "_csz%i", i ), screenWidth / ( 1 << i ), screenHeight / ( 1 << i ) );
		globalFramebuffers.csDepthFBO[i]->Bind();
		globalFramebuffers.csDepthFBO[i]->AttachImage2D( GL_TEXTURE_2D, globalImages->hierarchicalZbufferImage, 0, i );
		globalFramebuffers.csDepthFBO[i]->Check();
	}

	// GEOMETRY BUFFER
	globalFramebuffers.geometryBufferFBO = Create( "_gbuffer", screenWidth, screenHeight );
	globalFramebuffers.geometryBufferFBO->Bind();
	// it is ideal to share the depth buffer between the HDR main context and the geometry render target
	globalFramebuffers.geometryBufferFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->currentNormalsImage, 0 );
	globalFramebuffers.geometryBufferFBO->AttachImageDepth( GL_TEXTURE_2D, globalImages->currentDepthImage );
	globalFramebuffers.geometryBufferFBO->Check();

	// SMAA
	globalFramebuffers.smaaEdgesFBO = Create( "_smaaEdges", screenWidth, screenHeight );
	globalFramebuffers.smaaEdgesFBO->Bind();
	globalFramebuffers.smaaEdgesFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->smaaEdgesImage, 0 );
	globalFramebuffers.smaaEdgesFBO->Check();

	globalFramebuffers.smaaBlendFBO = Create( "_smaaBlend", screenWidth, screenHeight );
	globalFramebuffers.smaaBlendFBO->Bind();
	globalFramebuffers.smaaBlendFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->smaaBlendImage, 0 );
	globalFramebuffers.smaaBlendFBO->Check();

	Unbind();
}

/*
================
Framebuffer::CheckFramebuffers
================
*/
void Framebuffer::CheckFramebuffers()
{
	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();

	if( globalFramebuffers.hdrFBO->GetWidth() != screenWidth || globalFramebuffers.hdrFBO->GetHeight() != screenHeight )
	{
		Unbind();

		// HDR
		globalImages->currentRenderHDRImage->Resize( screenWidth, screenHeight );
		globalImages->currentDepthImage->Resize( screenWidth, screenHeight );

#if defined(USE_HDR_MSAA)
		if( glConfig.multisamples )
		{
			globalImages->currentRenderHDRImageNoMSAA->Resize( screenWidth, screenHeight );

			globalFramebuffers.hdrNonMSAAFBO->Bind();
			globalFramebuffers.hdrNonMSAAFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->currentRenderHDRImageNoMSAA, 0 );
			globalFramebuffers.hdrNonMSAAFBO->Check();
			globalFramebuffers.hdrNonMSAAFBO->Resize( screenWidth, screenHeight );

			globalFramebuffers.hdrFBO->Bind();
			globalFramebuffers.hdrFBO->AttachImage2D( GL_TEXTURE_2D_MULTISAMPLE, globalImages->currentRenderHDRImage, 0 );
			globalFramebuffers.hdrFBO->AttachImageDepth( GL_TEXTURE_2D_MULTISAMPLE, globalImages->currentDepthImage );
			globalFramebuffers.hdrFBO->Check();
		}
		else
#endif
		{
			globalFramebuffers.hdrFBO->Bind();
			globalFramebuffers.hdrFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->currentRenderHDRImage, 0 );
			globalFramebuffers.hdrFBO->AttachImageDepth( GL_TEXTURE_2D, globalImages->currentDepthImage );
			globalFramebuffers.hdrFBO->Check();
		}

		globalFramebuffers.hdrFBO->Resize( screenWidth, screenHeight );

		// HDR quarter
		/*
		globalImages->currentRenderHDRImageQuarter->Resize( screenWidth / 4, screenHeight / 4 );
		globalFramebuffers.hdrQuarterFBO->Bind();
		globalFramebuffers.hdrQuarterFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->currentRenderHDRImageQuarter, 0 );
		globalFramebuffers.hdrQuarterFBO->Check();
		*/

		// BLOOM
		for( int i = 0; i < MAX_BLOOM_BUFFERS; i++ )
		{
			globalImages->bloomRenderImage[i]->Resize( screenWidth / 4, screenHeight / 4 );

			globalFramebuffers.bloomRenderFBO[i]->Resize( screenWidth / 4, screenHeight / 4 );
			globalFramebuffers.bloomRenderFBO[i]->Bind();
			globalFramebuffers.bloomRenderFBO[i]->AttachImage2D( GL_TEXTURE_2D, globalImages->bloomRenderImage[i], 0 );
			globalFramebuffers.bloomRenderFBO[i]->Check();
		}

		// AMBIENT OCCLUSION
		if( r_ssaoFiltering.GetBool() || r_ssgiFiltering.GetBool() )
		{
			// AMBIENT OCCLUSION
			for( int i = 0; i < MAX_SSAO_BUFFERS; i++ )
			{
				globalImages->ambientOcclusionImage[i]->Resize( screenWidth, screenHeight );

				globalFramebuffers.ambientOcclusionFBO[i]->Resize( screenWidth, screenHeight );
				globalFramebuffers.ambientOcclusionFBO[i]->Bind();
				globalFramebuffers.ambientOcclusionFBO[i]->AttachImage2D( GL_TEXTURE_2D, globalImages->ambientOcclusionImage[i], 0 );
				globalFramebuffers.ambientOcclusionFBO[i]->Check();
			}
		}

		// HIERARCHICAL Z BUFFER
		globalImages->hierarchicalZbufferImage->Resize( screenWidth, screenHeight );
		for( int i = 0; i < MAX_HIERARCHICAL_ZBUFFERS; i++ )
		{
			globalFramebuffers.csDepthFBO[i]->Resize( screenWidth / ( 1 << i ), screenHeight / ( 1 << i ) );
			globalFramebuffers.csDepthFBO[i]->Bind();
			globalFramebuffers.csDepthFBO[i]->AttachImage2D( GL_TEXTURE_2D, globalImages->hierarchicalZbufferImage, 0, i );
			globalFramebuffers.csDepthFBO[i]->Check();
		}

		// GEOMETRY BUFFER
		globalImages->currentNormalsImage->Resize( screenWidth, screenHeight );

		globalFramebuffers.geometryBufferFBO->Resize( screenWidth, screenHeight );
		globalFramebuffers.geometryBufferFBO->Bind();
		globalFramebuffers.geometryBufferFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->currentNormalsImage, 0 );
		globalFramebuffers.geometryBufferFBO->AttachImageDepth( GL_TEXTURE_2D, globalImages->currentDepthImage );
		globalFramebuffers.geometryBufferFBO->Check();

		// SMAA
		globalImages->smaaEdgesImage->Resize( screenWidth, screenHeight );

		globalFramebuffers.smaaEdgesFBO->Resize( screenWidth, screenHeight );
		globalFramebuffers.smaaEdgesFBO->Bind();
		globalFramebuffers.smaaEdgesFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->smaaEdgesImage, 0 );
		globalFramebuffers.smaaEdgesFBO->Check();

		globalImages->smaaBlendImage->Resize( screenWidth, screenHeight );

		globalFramebuffers.smaaBlendFBO->Resize( screenWidth, screenHeight );
		globalFramebuffers.smaaBlendFBO->Bind();
		globalFramebuffers.smaaBlendFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->smaaBlendImage, 0 );
		globalFramebuffers.smaaBlendFBO->Check();

		Unbind();
	}
}

void Framebuffer::Shutdown()
{
	R_DestroyEditorTarget();
	framebuffers.DeleteContents( true );
}

void FramebufferGL::Bind()
{
	RENDERLOG_PRINTF( "Framebuffer::Bind( %s )\n", fboName.c_str() );

	if( backEnd.currentFramebuffer != this )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, frameBuffer );
		backEnd.currentFramebuffer = this;
	}
}

void Framebuffer::SetEditorDefaultFramebuffer( uint32_t fbo )
{
	g_editorDefaultFBO = fbo;
}

/*
========================
Framebuffer::EditorTargetBegin
========================
*/
bool Framebuffer::EditorTargetBegin( int w, int h, const float* clearColor )
{
	if( w <= 0 || h <= 0 )
	{
		return false;
	}

	if( g_editorFBO == 0 || w != g_editorWidth || h != g_editorHeight )
	{
		if( g_editorFBO == 0 )
		{
			glGenFramebuffers( 1, &g_editorFBO );
			glGenTextures( 1, &g_editorColorTex );
			glGenRenderbuffers( 1, &g_editorDepthStencil );
		}

		glBindTexture( GL_TEXTURE_2D, g_editorColorTex );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		glBindRenderbuffer( GL_RENDERBUFFER, g_editorDepthStencil );
		glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h );

		glBindFramebuffer( GL_FRAMEBUFFER, g_editorFBO );
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_editorColorTex, 0 );
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, g_editorDepthStencil );

		const GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		glBindRenderbuffer( GL_RENDERBUFFER, 0 );

		if( status != GL_FRAMEBUFFER_COMPLETE )
		{
			common->Warning( "Framebuffer::EditorTargetBegin: FBO incomplete: 0x%x", status );
			R_DestroyEditorTarget();
			return false;
		}

		g_editorWidth = w;
		g_editorHeight = h;
	}

	glBindFramebuffer( GL_FRAMEBUFFER, g_editorFBO );
	glViewport( 0, 0, w, h );
	glScissor( 0, 0, w, h );
	glClearColor( clearColor ? clearColor[0] : 0.0f, clearColor ? clearColor[1] : 0.0f, clearColor ? clearColor[2] : 0.0f, clearColor ? clearColor[3] : 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	g_editorDefaultFBO = g_editorFBO;
	return true;
}

/*
========================
Framebuffer::EditorTargetEnd
========================
*/
bool Framebuffer::EditorTargetEnd( byte* rgba )
{
	g_editorDefaultFBO = 0;

	if( g_editorFBO == 0 || rgba == NULL )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		return false;
	}

	glBindFramebuffer( GL_FRAMEBUFFER, g_editorFBO );
	glReadBuffer( GL_COLOR_ATTACHMENT0 );
	glPixelStorei( GL_PACK_ALIGNMENT, 1 );
	glReadPixels( 0, 0, g_editorWidth, g_editorHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgba );
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	// GL reads bottom to top, the editors expect the top row first
	const size_t pitch = ( size_t )g_editorWidth * 4;
	idTempArray<byte> tmp( ( unsigned int )pitch );
	for( int y = 0; y < g_editorHeight / 2; y++ )
	{
		byte* a = rgba + ( size_t )y * pitch;
		byte* b = rgba + ( size_t )( g_editorHeight - 1 - y ) * pitch;
		memcpy( tmp.Ptr(), a, pitch );
		memcpy( a, b, pitch );
		memcpy( b, tmp.Ptr(), pitch );
	}

	return true;
}

void Framebuffer::Unbind()
{
	RENDERLOG_PRINTF( "Framebuffer::Unbind()\n" );

	glBindFramebuffer( GL_FRAMEBUFFER, g_editorDefaultFBO );
	glBindRenderbuffer( GL_RENDERBUFFER, 0 );

	backEnd.currentFramebuffer = NULL;
}

void FramebufferGL::AttachImage2D( int target, const idImage* image, int index, int mipmapLod )
{
	if( ( target != GL_TEXTURE_2D ) && ( target != GL_TEXTURE_2D_MULTISAMPLE ) && ( target < GL_TEXTURE_CUBE_MAP_POSITIVE_X || target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z ) )
	{
		common->Warning( "Framebuffer::AttachImage2D( %s ): invalid target", fboName.c_str() );
		return;
	}

	if( index < 0 || index >= glConfig.maxColorAttachments )
	{
		common->Warning( "Framebuffer::AttachImage2D( %s ): bad index = %i", fboName.c_str(), index );
		return;
	}

	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, target, image->texnum, mipmapLod );
}

/*
================
FramebufferGL::AttachImageDepth
================
*/
void FramebufferGL::AttachImageDepth( int target, const idImage* image )
{
	if( ( target != GL_TEXTURE_2D ) && ( target != GL_TEXTURE_2D_MULTISAMPLE ) )
	{
		common->Warning( "Framebuffer::AttachImageDepth( %s ): invalid target", fboName.c_str() );
		return;
	}

	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, image->texnum, 0 );
}

/*
================
FramebufferGL::AttachImageDepthLayer
================
*/
void FramebufferGL::AttachImageDepthLayer( const idImage* image, int layer )
{
	glFramebufferTextureLayer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, image->texnum, 0, layer );
}

/*
================
FramebufferGL::Check
================
*/
void FramebufferGL::Check()
{
	int prev;
	glGetIntegerv( GL_FRAMEBUFFER_BINDING, &prev );
	glBindFramebuffer( GL_FRAMEBUFFER, frameBuffer );

	int status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
	if( status == GL_FRAMEBUFFER_COMPLETE )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, prev );
		return;
	}

	// something went wrong
	switch( status )
	{
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, incomplete attachment", fboName.c_str() );
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing attachment", fboName.c_str() );
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing draw buffer", fboName.c_str() );
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing read buffer", fboName.c_str() );
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing layer targets", fboName.c_str() );
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing multisample", fboName.c_str() );
			break;

		case GL_FRAMEBUFFER_UNSUPPORTED:
			common->Error( "Framebuffer::Check( %s ): Unsupported framebuffer format", fboName.c_str() );
			break;

		default:
			common->Error( "Framebuffer::Check( %s ): Unknown error 0x%X", fboName.c_str(), status );
			break;
	};

	glBindFramebuffer( GL_FRAMEBUFFER, prev );
}

/*
=============
FramebufferGL::ReadbackColorRGB16F
=============
*/
bool FramebufferGL::ReadbackColorRGB16F( int w, int h, byte* rgb16f )
{
	glFinish();

	glReadBuffer( GL_BACK );

	Bind();

	glPixelStorei( GL_PACK_ROW_LENGTH, w );
	glReadPixels( 0, 0, w, h, GL_RGB, GL_HALF_FLOAT, rgb16f );

	// OpenGL reads bottom to top
	R_VerticalFlipRGB16F( rgb16f, w, h );

	Framebuffer::Unbind();

	return true;
}
