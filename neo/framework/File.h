/*
===========================================================================

KROOM 3 GPL Source Code

This file is part of the KROOM 3 Source Code, originally based
on the Doom 3 with bits and pieces from Doom 3 BFG edition GPL Source Codes both published in 2011 and 2012.

KROOM 3 Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Extra attributions can be found on the CREDITS.txt file

===========================================================================
*/

#ifndef __FILE_H__
#define __FILE_H__

/*
==============================================================

  File Streams.

==============================================================
*/

// mode parm for Seek
typedef enum
{
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
} fsOrigin_t;

class idFileSystemLocal;


class idFile
{
public:
	virtual					~idFile() {};
	// Get the name of the file.
	virtual const char* 	GetName() const;
	// Get the full file path.
	virtual const char* 	GetFullPath() const;
	// Read data from the file to the buffer.
	virtual int				Read( void* buffer, int len );
	// Write data from the buffer to the file.
	virtual int				Write( const void* buffer, int len );
	// Returns the length of the file.
	virtual int				Length() const;
	// Return a time value for reload operations.
	virtual ID_TIME_T		Timestamp() const;
	// Returns offset in file.
	virtual int				Tell() const;
	// Forces flush on files being writting to.
	virtual void			ForceFlush();
	// Causes any buffered data to be written to the file.
	virtual void			Flush();
	// Seek on a file.
	virtual int				Seek( long offset, fsOrigin_t origin );
	// Go back to the beginning of the file.
	virtual void			Rewind();
	// Like fprintf.
	virtual int				Printf( VERIFY_FORMAT_STRING const char* fmt, ... );
	// Like fprintf but with argument pointer
	virtual int				VPrintf( const char* fmt, va_list arg );
	// Write a string with high precision floating point numbers to the file.
	virtual int				WriteFloatString( VERIFY_FORMAT_STRING const char* fmt, ... );

	// Endian portable alternatives to Read(...)
	virtual int				ReadInt( int& value );
	virtual int				ReadUnsignedInt( unsigned int& value );
	virtual int				ReadShort( short& value );
	virtual int				ReadUnsignedShort( unsigned short& value );
	virtual int				ReadChar( char& value );
	virtual int				ReadUnsignedChar( unsigned char& value );
	virtual int				ReadFloat( float& value );
	virtual int				ReadBool( bool& value );
	virtual int				ReadString( idStr& string );
	virtual int				ReadVec2( idVec2& vec );
	virtual int				ReadVec3( idVec3& vec );
	virtual int				ReadVec4( idVec4& vec );
	virtual int				ReadVec6( idVec6& vec );
	virtual int				ReadMat3( idMat3& mat );

	// Endian portable alternatives to Write(...)
	virtual int				WriteInt( const int value );
	virtual int				WriteUnsignedInt( const unsigned int value );
	virtual int				WriteShort( const short value );
	virtual int				WriteUnsignedShort( unsigned short value );
	virtual int				WriteChar( const char value );
	virtual int				WriteUnsignedChar( const unsigned char value );
	virtual int				WriteFloat( const float value );
	virtual int				WriteBool( const bool value );
	virtual int				WriteString( const char* string );
	virtual int				WriteVec2( const idVec2& vec );
	virtual int				WriteVec3( const idVec3& vec );
	virtual int				WriteVec4( const idVec4& vec );
	virtual int				WriteVec6( const idVec6& vec );
	virtual int				WriteMat3( const idMat3& mat );

	template<class type> ID_INLINE size_t ReadBig( type& c )
	{
		size_t r = Read( &c, sizeof( c ) );
		idSwap::Big( c );
		return r;
	}

	template<class type> ID_INLINE size_t ReadBigArray( type* c, int count )
	{
		size_t r = Read( c, sizeof( c[0] ) * count );
		idSwap::BigArray( c, count );
		return r;
	}

	template<class type> ID_INLINE size_t WriteBig( const type& c )
	{
		type b = c;
		idSwap::Big( b );
		return Write( &b, sizeof( b ) );
	}

	template<class type> ID_INLINE size_t WriteBigArray( const type* c, int count )
	{
		size_t r = 0;
		for( int i = 0; i < count; i++ )
		{
			r += WriteBig( c[i] );
		}
		return r;
	}
};

/*
================================================
idFile_Memory
================================================
*/
class idFile_Memory : public idFile
{
	friend class			idFileSystemLocal;

public:
	idFile_Memory();	// file for writing without name
	idFile_Memory( const char* name );	// file for writing
	idFile_Memory( const char* name, char* data, int length );	// file for writing
	idFile_Memory( const char* name, const char* data, int length );	// file for reading
	virtual					~idFile_Memory();

	virtual const char* 	GetName() const
	{
		return name.c_str();
	}
	virtual const char* 	GetFullPath() const
	{
		return name.c_str();
	}
	virtual int				Read( void* buffer, int len );
	virtual int				Write( const void* buffer, int len );
	virtual int				Length() const;
	virtual void			SetLength( size_t len );
	virtual ID_TIME_T		Timestamp() const;
	virtual int				Tell() const;
	virtual void			ForceFlush();
	virtual void			Flush();
	virtual int				Seek( long offset, fsOrigin_t origin );

	// Set the given length and don't allow the file to grow.
	void					SetMaxLength( size_t len );
	// changes memory file to read only
	void					MakeReadOnly();
	// Change the file to be writable
	void					MakeWritable();
	// clear the file
	virtual void			Clear( bool freeMemory = true );
	// set data for reading
	void					SetData( const char* data, int length );
	// returns const pointer to the memory buffer
	const char* 			GetDataPtr() const
	{
		return filePtr;
	}
	// returns pointer to the memory buffer
	char* 					GetDataPtr()
	{
		return filePtr;
	}
	// set the file granularity
	void					SetGranularity( int g )
	{
		assert( g > 0 );
		granularity = g;
	}
	void					PreAllocate( size_t len );

	// Doesn't change how much is allocated, but allows you to set the size of the file to smaller than it should be.
	// Useful for stripping off a checksum at the end of the file
	void					TruncateData( size_t len );

	void					TakeDataOwnership();

	size_t					GetMaxLength()
	{
		return maxSize;
	}
	size_t					GetAllocated()
	{
		return allocated;
	}

protected:
	idStr					name;			// name of the file
private:
	int						mode;			// open mode
	size_t					maxSize;		// maximum size of file
	size_t					fileSize;		// size of the file
	size_t					allocated;		// allocated size
	int						granularity;	// file granularity
	char* 					filePtr;		// buffer holding the file data
	char* 					curPtr;			// current read/write pointer
};


class idFile_BitMsg : public idFile
{
	friend class			idFileSystemLocal;

public:
	idFile_BitMsg( idBitMsg& msg );
	idFile_BitMsg( const idBitMsg& msg );
	virtual					~idFile_BitMsg();

	virtual const char* 	GetName() const
	{
		return name.c_str();
	}
	virtual const char* 	GetFullPath() const
	{
		return name.c_str();
	}
	virtual int				Read( void* buffer, int len );
	virtual int				Write( const void* buffer, int len );
	virtual int				Length() const;
	virtual ID_TIME_T		Timestamp() const;
	virtual int				Tell() const;
	virtual void			ForceFlush();
	virtual void			Flush();
	virtual int				Seek( long offset, fsOrigin_t origin );

private:
	idStr					name;			// name of the file
	int						mode;			// open mode
	idBitMsg* 				msg;
};


class idFile_Permanent : public idFile
{
	friend class			idFileSystemLocal;

public:
	idFile_Permanent();
	virtual					~idFile_Permanent();

	virtual const char* 	GetName() const
	{
		return name.c_str();
	}
	virtual const char* 	GetFullPath() const
	{
		return fullPath.c_str();
	}
	virtual int				Read( void* buffer, int len );
	virtual int				Write( const void* buffer, int len );
	virtual int				Length() const;
	virtual ID_TIME_T		Timestamp() const;
	virtual int				Tell() const;
	virtual void			ForceFlush();
	virtual void			Flush();
	virtual int				Seek( long offset, fsOrigin_t origin );

	// returns file pointer
	idFileHandle			GetFilePtr()
	{
		return o;
	}

private:
	idStr					name;			// relative path of the file - relative path
	idStr					fullPath;		// full file path - OS path
	int						mode;			// open mode
	int						fileSize;		// size of the file
	idFileHandle			o;				// file handle
	bool					handleSync;		// true if written data is immediately flushed
};

class idFile_Cached : public idFile_Permanent
{
	friend class			idFileSystemLocal;
public:
	idFile_Cached();
	virtual					~idFile_Cached();

	void					CacheData( uint64_t offset, uint64_t length );

	virtual int				Read( void* buffer, int len );

	virtual int				Tell() const;
	virtual int				Seek( long offset, fsOrigin_t origin );

private:
	uint64_t				internalFilePos;
	uint64_t				bufferedStartOffset;
	uint64_t				bufferedEndOffset;
	byte* 				buffered;
};


class idFile_InZip : public idFile
{
	friend class			idFileSystemLocal;

public:
	idFile_InZip();
	virtual					~idFile_InZip();

	virtual const char* 	GetName() const
	{
		return name.c_str();
	}
	virtual const char* 	GetFullPath() const
	{
		return fullPath.c_str();
	}
	virtual int				Read( void* buffer, int len );
	virtual int				Write( const void* buffer, int len );
	virtual int				Length() const;
	virtual ID_TIME_T		Timestamp() const;
	virtual int				Tell() const;
	virtual void			ForceFlush();
	virtual void			Flush();
	virtual int				Seek( long offset, fsOrigin_t origin );

private:
	idStr					name;			// name of the file in the pak
	idStr					fullPath;		// full file path including pak file name
#ifdef _WIN32
	unsigned __int64		zipFilePos;		// zip file info position in pak - should be ZPOS64_T, but I don't want miniz.h (through Unzip.h) in this header
#else
	unsigned long long int	zipFilePos;		// zip file info position in pak
#endif
	int						fileSize;		// size of the file
	void* 					z;				// unzip info
};


/*
================================================================================================

idFileLocal

================================================================================================
*/

/*
================================================
idFileLocal is a FileStream wrapper that automatically closes a file when the
class variable goes out of scope. Note that the pointer passed in to the constructor can be for
any type of File Stream that ultimately inherits from idFile, and that this is not actually a
SmartPointer, as it does not keep a reference count.
================================================
*/
class idFileLocal
{
public:
	// Constructor that accepts and stores the file pointer.
	idFileLocal( idFile* _file )	: file( _file )
	{
	}

	// Destructor that will destroy (close) the file when this wrapper class goes out of scope.
	~idFileLocal();

	// Cast to a file pointer.
	operator idFile* () const
	{
		return file;
	}

	// Member access operator for treating the wrapper as if it were the file, itself.
	idFile* operator -> () const
	{
		return file;
	}

protected:
	idFile* file;	// The managed file pointer.
};

/*
========================
idFileLocal::~idFileLocal

Destructor that will destroy (close) the managed file when this wrapper class goes out of scope.
========================
*/
ID_INLINE idFileLocal::~idFileLocal()
{
	if( file != NULL )
	{
		delete file;
		file = NULL;
	}
}

#endif /* !__FILE_H__ */
