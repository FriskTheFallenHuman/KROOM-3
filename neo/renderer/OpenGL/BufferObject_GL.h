/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans
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

#ifndef __BUFFEROBJECT_GL_H__
#define __BUFFEROBJECT_GL_H__

#include "../BufferObject.h"

/*
================================================================================================

idVertexBufferGL

================================================================================================
*/
class idVertexBufferGL : public idVertexBuffer
{
public:
	idVertexBufferGL();
	virtual ~idVertexBufferGL();

	GLintptr			GetAPIObject() const
	{
		return apiObject;
	}

	virtual bool		AllocBufferObject( const void* data, int allocSize, bufferUsageType_t usage );
	virtual void		FreeBufferObject();
	virtual void		Update( const void* data, int size, int offset = 0 ) const;
	virtual void* 		MapBuffer( bufferMapType_t mapType );
	virtual void		UnmapBuffer();

protected:
	virtual void		CopyBackendState( const idBufferObject& other );

private:
	void				ClearWithoutFreeing();

	GLintptr			apiObject;
	void* 				buffer;

	DISALLOW_COPY_AND_ASSIGN( idVertexBufferGL );
};

/*
================================================================================================

idIndexBufferGL

================================================================================================
*/
class idIndexBufferGL : public idIndexBuffer
{
public:
	idIndexBufferGL();
	virtual ~idIndexBufferGL();

	GLintptr			GetAPIObject() const
	{
		return apiObject;
	}

	virtual bool		AllocBufferObject( const void* data, int allocSize, bufferUsageType_t usage );
	virtual void		FreeBufferObject();
	virtual void		Update( const void* data, int size, int offset = 0 ) const;
	virtual void* 		MapBuffer( bufferMapType_t mapType );
	virtual void		UnmapBuffer();

protected:
	virtual void		CopyBackendState( const idBufferObject& other );

private:
	void				ClearWithoutFreeing();

	GLintptr			apiObject;
	void* 				buffer;

	DISALLOW_COPY_AND_ASSIGN( idIndexBufferGL );
};

/*
================================================================================================

idUniformBufferGL

================================================================================================
*/
class idUniformBufferGL : public idUniformBuffer
{
public:
	idUniformBufferGL();
	virtual ~idUniformBufferGL();

	GLintptr			GetAPIObject() const
	{
		return apiObject;
	}

	virtual bool		AllocBufferObject( const void* data, int allocSize, bufferUsageType_t usage );
	virtual void		FreeBufferObject();
	virtual void		Update( const void* data, int size, int offset = 0 ) const;
	virtual void* 		MapBuffer( bufferMapType_t mapType );
	virtual void		UnmapBuffer();

protected:
	virtual void		CopyBackendState( const idBufferObject& other );

private:
	void				ClearWithoutFreeing();

	GLintptr			apiObject;
	void* 				buffer;

	DISALLOW_COPY_AND_ASSIGN( idUniformBufferGL );
};

#endif /* !__BUFFEROBJECT_GL_H__ */
