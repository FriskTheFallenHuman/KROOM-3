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

#include "precompiled.h"
#pragma hdrstop

#include "../RenderCommon.h"
#include "BufferObject_GL.h"

extern idCVar r_showBuffers;

//static const GLenum bufferUsage = GL_STATIC_DRAW;
static const GLenum bufferUsage = GL_DYNAMIC_DRAW;

/*
================================================================================================

	Buffer Objects

================================================================================================
*/

/*
========================
UnbindBufferObjects
========================
*/
void UnbindBufferObjects()
{
	if( !glBindBufferARB )
	{
		return;
	}

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

/*
========================
idVertexBuffer::Create
========================
*/
idVertexBuffer* idVertexBuffer::Create()
{
	return new idVertexBufferGL();
}

/*
========================
idIndexBuffer::Create
========================
*/
idIndexBuffer* idIndexBuffer::Create()
{
	return new idIndexBufferGL();
}

/*
========================
idUniformBuffer::Create
========================
*/
idUniformBuffer* idUniformBuffer::Create()
{
	return new idUniformBufferGL();
}

/*
================================================================================================

idVertexBufferGL

================================================================================================
*/

/*
========================
idVertexBufferGL::idVertexBufferGL
========================
*/
idVertexBufferGL::idVertexBufferGL()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = 0xFFFF;
	buffer = NULL;
	SetUnmapped();
}

/*
========================
idVertexBufferGL::~idVertexBufferGL
========================
*/
idVertexBufferGL::~idVertexBufferGL()
{
	FreeBufferObject();
}

/*
========================
idVertexBufferGL::AllocBufferObject
========================
*/
bool idVertexBufferGL::AllocBufferObject( const void* data, int allocSize, bufferUsageType_t _usage )
{
	assert( apiObject == 0xFFFF );
	assert_16_byte_aligned( data );

	if( allocSize <= 0 )
	{
		idLib::Error( "idVertexBufferGL::AllocBufferObject: allocSize = %i", allocSize );
	}

	size = allocSize;
	usage = _usage;

	bool allocationFailed = false;

	int numBytes = GetAllocedSize();

	// clear out any previous error
	GL_CheckErrors();

	glGenBuffers( 1, ( GLuint* ) &apiObject );
	if( apiObject == 0xFFFF )
	{
		idLib::FatalError( "idVertexBufferGL::AllocBufferObject: failed" );
	}

	glBindBuffer( GL_ARRAY_BUFFER, apiObject );

	// these are rewritten every frame
	glBufferDataARB( GL_ARRAY_BUFFER, numBytes, NULL, bufferUsage );

	GLenum err = glGetError();
	if( err == GL_OUT_OF_MEMORY )
	{
		idLib::Warning( "idVertexBufferGL::AllocBufferObject: allocation failed" );
		allocationFailed = true;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "vertex buffer alloc %p, api %p (%i bytes)\n", this, ( GLuint* )&apiObject, GetSize() );
	}

	// copy the data
	if( data != NULL )
	{
		Update( data, allocSize, 0 );
	}

	return !allocationFailed;
}

/*
========================
idVertexBufferGL::FreeBufferObject
========================
*/
void idVertexBufferGL::FreeBufferObject()
{
	if( IsMapped() )
	{
		UnmapBuffer();
	}

	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if( OwnsBuffer() == false )
	{
		ClearWithoutFreeing();
		return;
	}

	if( apiObject == 0xFFFF )
	{
		return;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "vertex buffer free %p, api %p (%i bytes)\n", this, ( GLuint* )&apiObject, GetSize() );
	}

	glDeleteBuffers( 1, ( GLuint* )&apiObject );

	ClearWithoutFreeing();
}

/*
========================
idVertexBufferGL::Update
========================
*/
void idVertexBufferGL::Update( const void* data, int updateSize, int offset ) const
{
	assert( apiObject != 0xFFFF );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );

	if( updateSize > GetSize() )
	{
		idLib::FatalError( "idVertexBufferGL::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}

	int numBytes = ( updateSize + 15 ) & ~15;

	if( usage == BU_DYNAMIC )
	{
		CopyBuffer( ( byte* )buffer + offset, ( const byte* )data, numBytes );
	}
	else
	{
		glBindBuffer( GL_ARRAY_BUFFER, apiObject );
		glBufferSubData( GL_ARRAY_BUFFER, GetOffset() + offset, ( GLsizeiptrARB )numBytes, data );
	}
}

/*
========================
idVertexBufferGL::MapBuffer
========================
*/
void* idVertexBufferGL::MapBuffer( bufferMapType_t mapType )
{
	assert( apiObject != 0xFFFF );
	assert( IsMapped() == false );

	buffer = NULL;

	glBindBuffer( GL_ARRAY_BUFFER, apiObject );
	if( mapType == BM_READ )
	{
		buffer = glMapBufferRange( GL_ARRAY_BUFFER_ARB, 0, GetAllocedSize(), GL_MAP_READ_BIT /*| GL_MAP_UNSYNCHRONIZED_BIT*/ );
		if( buffer != NULL )
		{
			buffer = ( byte* )buffer + GetOffset();
		}
	}
	else if( mapType == BM_WRITE )
	{
		// RB: removed GL_MAP_INVALIDATE_RANGE_BIT as it breaks with an optimization in the Nvidia WHQL drivers >= 344.11
		buffer = glMapBufferRange( GL_ARRAY_BUFFER, 0, GetAllocedSize(), GL_MAP_WRITE_BIT /*| GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT*/ );
		if( buffer != NULL )
		{
			buffer = ( byte* )buffer + GetOffset();
		}
		// assert( IsWriteCombined( buffer ) ); // commented out because it spams the console
	}
	else
	{
		assert( false );
	}

	SetMapped();

	if( buffer == NULL )
	{
		idLib::FatalError( "idVertexBufferGL::MapBuffer: failed" );
	}
	return buffer;
}

/*
========================
idVertexBufferGL::UnmapBuffer
========================
*/
void idVertexBufferGL::UnmapBuffer()
{
	assert( apiObject != 0xFFFF );
	assert( IsMapped() );

	glBindBuffer( GL_ARRAY_BUFFER, apiObject );
	if( !glUnmapBuffer( GL_ARRAY_BUFFER ) )
	{
		idLib::Printf( "idVertexBufferGL::UnmapBuffer failed\n" );
	}

	SetUnmapped();
}

/*
========================
idVertexBufferGL::ClearWithoutFreeing
========================
*/
void idVertexBufferGL::ClearWithoutFreeing()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = 0xFFFF;
}

/*
========================
idVertexBufferGL::CopyBackendState
========================
*/
void idVertexBufferGL::CopyBackendState( const idBufferObject& other )
{
	apiObject = static_cast< const idVertexBufferGL& >( other ).apiObject;
}

/*
================================================================================================

idIndexBufferGL

================================================================================================
*/

/*
========================
idIndexBufferGL::idIndexBufferGL
========================
*/
idIndexBufferGL::idIndexBufferGL()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = 0xFFFF;
	buffer = NULL;
	SetUnmapped();
}

/*
========================
idIndexBufferGL::~idIndexBufferGL
========================
*/
idIndexBufferGL::~idIndexBufferGL()
{
	FreeBufferObject();
}

/*
========================
idIndexBufferGL::AllocBufferObject
========================
*/
bool idIndexBufferGL::AllocBufferObject( const void* data, int allocSize, bufferUsageType_t _usage )
{
	assert( apiObject == 0xFFFF );
	assert_16_byte_aligned( data );

	if( allocSize <= 0 )
	{
		idLib::Error( "idIndexBufferGL::AllocBufferObject: allocSize = %i", allocSize );
	}

	size = allocSize;
	usage = _usage;

	bool allocationFailed = false;

	int numBytes = GetAllocedSize();


	// clear out any previous error
	GL_CheckErrors();

	glGenBuffersARB( 1, ( GLuint* )&apiObject );
	if( apiObject == 0xFFFF )
	{
		GLenum error = glGetError();
		idLib::FatalError( "idIndexBufferGL:AllocBufferObject: failed - GL_Error %d", error );
	}
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, apiObject );

	// these are rewritten every frame
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, numBytes, NULL, bufferUsage );

	GLenum err = glGetError();
	if( err == GL_OUT_OF_MEMORY )
	{
		idLib::Warning( "idIndexBufferGL:AllocBufferObject: allocation failed" );
		allocationFailed = true;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "index buffer alloc %p, api %p (%i bytes)\n", this, ( GLuint* )&apiObject, GetSize() );
	}

	// copy the data
	if( data != NULL )
	{
		Update( data, allocSize, 0 );
	}

	return !allocationFailed;
}

/*
========================
idIndexBufferGL::FreeBufferObject
========================
*/
void idIndexBufferGL::FreeBufferObject()
{
	if( IsMapped() )
	{
		UnmapBuffer();
	}

	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if( OwnsBuffer() == false )
	{
		ClearWithoutFreeing();
		return;
	}

	if( apiObject == 0xFFFF )
	{
		return;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "index buffer free %p, api %p (%i bytes)\n", this, ( GLuint* )&apiObject, GetSize() );
	}

	glDeleteBuffers( 1, ( GLuint* )&apiObject );

	ClearWithoutFreeing();
}

/*
========================
idIndexBufferGL::Update
========================
*/
void idIndexBufferGL::Update( const void* data, int updateSize, int offset ) const
{
	assert( apiObject != 0xFFFF );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );

	if( updateSize > GetSize() )
	{
		idLib::FatalError( "idIndexBufferGL::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}

	int numBytes = ( updateSize + 15 ) & ~15;

	if( usage == BU_DYNAMIC )
	{
		CopyBuffer( ( byte* )buffer + offset, ( const byte* )data, numBytes );
	}
	else
	{
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, apiObject );
		glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, GetOffset() + offset, ( GLsizeiptrARB )numBytes, data );
	}
}

/*
========================
idIndexBufferGL::MapBuffer
========================
*/
void* idIndexBufferGL::MapBuffer( bufferMapType_t mapType )
{
	assert( apiObject != 0xFFFF );
	assert( IsMapped() == false );

	buffer = NULL;

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, apiObject );
	if( mapType == BM_READ )
	{
		//buffer = glMapBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, GL_READ_ONLY_ARB );
		buffer = glMapBufferRange( GL_ELEMENT_ARRAY_BUFFER, 0, GetAllocedSize(), GL_MAP_READ_BIT /*| GL_MAP_UNSYNCHRONIZED_BIT*/ );
		if( buffer != NULL )
		{
			buffer = ( byte* )buffer + GetOffset();
		}
	}
	else if( mapType == BM_WRITE )
	{
		//buffer = glMapBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB );

		// RB: removed GL_MAP_INVALIDATE_RANGE_BIT as it breaks with an optimization in the Nvidia WHQL drivers >= 344.11
		buffer = glMapBufferRange( GL_ELEMENT_ARRAY_BUFFER, 0, GetAllocedSize(), GL_MAP_WRITE_BIT /*| GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT*/ );
		if( buffer != NULL )
		{
			buffer = ( byte* )buffer + GetOffset();
		}
		// assert( IsWriteCombined( buffer ) ); // commented out because it spams the console
	}
	else
	{
		assert( false );
	}

	SetMapped();

	if( buffer == NULL )
	{
		idLib::FatalError( "idIndexBufferGL::MapBuffer: failed" );
	}
	return buffer;
}

/*
========================
idIndexBufferGL::UnmapBuffer
========================
*/
void idIndexBufferGL::UnmapBuffer()
{
	assert( apiObject != 0xFFFF );
	assert( IsMapped() );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, apiObject );
	if( !glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER ) )
	{
		idLib::Printf( "idIndexBufferGL::UnmapBuffer failed\n" );
	}

	buffer = NULL;

	SetUnmapped();
}

/*
========================
idIndexBufferGL::ClearWithoutFreeing
========================
*/
void idIndexBufferGL::ClearWithoutFreeing()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = 0xFFFF;
}

/*
========================
idIndexBufferGL::CopyBackendState
========================
*/
void idIndexBufferGL::CopyBackendState( const idBufferObject& other )
{
	apiObject = static_cast< const idIndexBufferGL& >( other ).apiObject;
}

/*
================================================================================================

idUniformBufferGL

================================================================================================
*/

/*
========================
idUniformBufferGL::idUniformBufferGL
========================
*/
idUniformBufferGL::idUniformBufferGL()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = 0xFFFF;
	buffer = NULL;
	SetUnmapped();
}

/*
========================
idUniformBufferGL::~idUniformBufferGL
========================
*/
idUniformBufferGL::~idUniformBufferGL()
{
	FreeBufferObject();
}

/*
========================
idUniformBufferGL::AllocBufferObject
========================
*/
bool idUniformBufferGL::AllocBufferObject( const void* data, int allocSize, bufferUsageType_t _usage )
{
	assert( apiObject == 0xFFFF );
	assert_16_byte_aligned( data );

	if( allocSize <= 0 )
	{
		idLib::Error( "idUniformBufferGL::AllocBufferObject: allocSize = %i", allocSize );
	}

	size = allocSize;
	usage = _usage;

	bool allocationFailed = false;

	const int numBytes = GetAllocedSize();

	glGenBuffers( 1, ( GLuint* )&apiObject );
	glBindBuffer( GL_UNIFORM_BUFFER, apiObject );
	glBufferData( GL_UNIFORM_BUFFER, numBytes, NULL, GL_STREAM_DRAW_ARB );
	glBindBuffer( GL_UNIFORM_BUFFER, 0 );

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "joint buffer alloc %p, api %p (%i joints)\n", this, ( GLuint* )&apiObject, GetSize() );
	}

	// copy the data
	if( data != NULL )
	{
		Update( data, allocSize, 0 );
	}

	return !allocationFailed;
}

/*
========================
idUniformBufferGL::FreeBufferObject
========================
*/
void idUniformBufferGL::FreeBufferObject()
{
	if( IsMapped() )
	{
		UnmapBuffer();
	}

	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if( OwnsBuffer() == false )
	{
		ClearWithoutFreeing();
		return;
	}

	if( apiObject == 0xFFFF )
	{
		return;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "joint buffer free %p, api %p (%i size)\n", this, ( GLuint* )&apiObject, GetSize() );
	}

	glBindBuffer( GL_UNIFORM_BUFFER, 0 );
	glDeleteBuffers( 1, ( GLuint* )&apiObject );

	ClearWithoutFreeing();
}

/*
========================
idUniformBufferGL::Update
========================
*/
void idUniformBufferGL::Update( const void* data, int updateSize, int offset ) const
{
	assert( apiObject != 0xFFFF );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );

	if( updateSize > GetSize() )
	{
		idLib::FatalError( "idUniformBufferGL::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}

	const int numBytes = ( updateSize + 15 ) & ~15;

	if( usage == BU_DYNAMIC )
	{
		CopyBuffer( ( byte* )buffer + offset, ( const byte* )data, numBytes );
	}
	else
	{
		glBindBuffer( GL_ARRAY_BUFFER, apiObject );
		glBufferSubData( GL_ARRAY_BUFFER, GetOffset() + offset, ( GLsizeiptr )numBytes, data );
	}
}

/*
========================
idUniformBufferGL::MapBuffer
========================
*/
void* idUniformBufferGL::MapBuffer( bufferMapType_t mapType )
{
	assert( IsMapped() == false );
	assert( mapType == BM_WRITE );
	assert( apiObject != 0xFFFF );

	int numBytes = GetAllocedSize();

	buffer = NULL;

	glBindBuffer( GL_UNIFORM_BUFFER, apiObject );
	numBytes = numBytes;
	assert( GetOffset() == 0 );

	// RB: removed GL_MAP_INVALIDATE_RANGE_BIT as it breaks with an optimization in the Nvidia WHQL drivers >= 344.11
	buffer = glMapBufferRange( GL_UNIFORM_BUFFER, 0, GetAllocedSize(), GL_MAP_WRITE_BIT /*| GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT*/ );
	if( buffer != NULL )
	{
		buffer = ( byte* )buffer + GetOffset();
	}

	SetMapped();

	if( buffer == NULL )
	{
		idLib::FatalError( "idUniformBufferGL::MapBuffer: failed" );
	}
	return ( float* ) buffer;
}

/*
========================
idUniformBufferGL::UnmapBuffer
========================
*/
void idUniformBufferGL::UnmapBuffer()
{
	assert( apiObject != 0xFFFF );
	assert( IsMapped() );

	glBindBuffer( GL_UNIFORM_BUFFER, apiObject );
	if( !glUnmapBuffer( GL_UNIFORM_BUFFER ) )
	{
		idLib::Printf( "idUniformBufferGL::UnmapBuffer failed\n" );
	}

	buffer = NULL;

	SetUnmapped();
}

/*
========================
idUniformBufferGL::ClearWithoutFreeing
========================
*/
void idUniformBufferGL::ClearWithoutFreeing()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = 0xFFFF;
}

/*
========================
idUniformBufferGL::CopyBackendState
========================
*/
void idUniformBufferGL::CopyBackendState( const idBufferObject& other )
{
	apiObject = static_cast< const idUniformBufferGL& >( other ).apiObject;
}
