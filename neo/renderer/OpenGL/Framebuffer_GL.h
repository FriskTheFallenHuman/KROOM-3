/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2014-2016 Robert Beckebans

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

#ifndef __FRAMEBUFFER_GL_H__
#define __FRAMEBUFFER_GL_H__

#include "../Framebuffer.h"

class FramebufferGL : public Framebuffer
{
public:
	FramebufferGL( const char* name, int width, int height );
	virtual			~FramebufferGL();

	virtual void	Bind();

	virtual void	AttachImage2D( int target, const idImage* image, int index, int mipmapLod = 0 );
	virtual void	AttachImageDepth( int target, const idImage* image );
	virtual void	AttachImageDepthLayer( const idImage* image, int layer );

	virtual void	Check();

	virtual bool	ReadbackColorRGB16F( int w, int h, byte* rgb16f );
};

#endif /* !__FRAMEBUFFER_GL_H__ */
