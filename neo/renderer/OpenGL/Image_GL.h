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

#ifndef __IMAGE_GL_H__
#define __IMAGE_GL_H__

#include "../Image.h"

/*
================================================================================================

Contains the Image implementation for OpenGL.

================================================================================================
*/

class idImageGL : public idImage
{
public:
	idImageGL( const char* name );
	virtual ~idImageGL();

	virtual void	Bind();

	virtual bool	IsLoaded() const;

	virtual void	PurgeImage();

	virtual void	CopyFramebuffer( int x, int y, int imageWidth, int imageHeight );
	virtual void	CopyDepthbuffer( int x, int y, int imageWidth, int imageHeight );

	virtual void	SubImageUpload( int mipLevel, int destX, int destY, int destZ,
									int width, int height, const void* data,
									int pixelPitch );

	virtual void	SetTexParameters();

	virtual void*	GetImGuiTextureID();

protected:
	virtual void	AllocImage();

	virtual void	SetSamplerState( textureFilter_t tf, textureRepeat_t tr );

private:
	// we could derive these in subImageUpload each time if necessary
	GLuint			internalFormat;
	GLuint			dataFormat;
	GLuint			dataType;
};

#endif /* !__IMAGE_GL_H__ */
