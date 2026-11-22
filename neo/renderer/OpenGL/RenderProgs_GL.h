/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2018 Robert Beckebans
Copyright (C) 2016-2017 Dustin Land

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

#ifndef __RENDERPROGS_GL_H__
#define __RENDERPROGS_GL_H__

#include "../RenderProgs.h"

class idRenderProgManagerGL : public idRenderProgManager
{
public:
	virtual void	StartFrame();
	virtual void	BindProgram( int index );
	virtual void	Unbind();
	virtual void	CommitUniforms( uint64 stateBits );
	virtual void	KillAllShaders();

protected:
	struct shaderGL_t : public shader_t
	{
		shaderGL_t() :
			progId( INVALID_PROGID ),
			uniformArray( -1 ) {}

		uint			progId;
		int				uniformArray;
		idList<int>		uniforms;
	};

	struct renderProgGL_t : public renderProg_t {};

	virtual shader_t*		CreateShader()
	{
		return new shaderGL_t();
	}
	virtual renderProg_t*	CreateRenderProg()
	{
		return new renderProgGL_t();
	}

	virtual void			InitBackend() {}

	virtual void			LoadShader( int index, rpStage_t stage );
	virtual void			LoadShader( shader_t& shader );
	virtual void			LoadGLSLProgram( const int programIndex, const int vertexShaderIndex, const int fragmentShaderIndex );
};

extern idRenderProgManagerGL renderProgManagerGL;

#endif /* !__RENDERPROGS_GL_H__ */