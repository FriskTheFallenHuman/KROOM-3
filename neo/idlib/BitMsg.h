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
#ifndef __BITMSG_H__
#define __BITMSG_H__

/*
===============================================================================

  idBitMsg

  Handles byte ordering and avoids alignment errors.
  Allows concurrent writing and reading.
  The data set with Init is never freed.

===============================================================================
*/

class idBitMsg
{
public:
	idBitMsg();
	~idBitMsg() {}

	void			Init( byte* data, int length );
	void			Init( const byte* data, int length );
	byte* 			GetData();						// get data for writing
	const byte* 	GetData() const;					// get data for reading
	int				GetMaxSize() const;				// get the maximum message size
	void			SetAllowOverflow( bool set );			// generate error if not set and message is overflowed
	bool			IsOverflowed() const;				// returns true if the message was overflowed

	int				GetSize() const;					// size of the message in bytes
	void			SetSize( int size );					// set the message size
	int				GetWriteBit() const;				// get current write bit
	void			SetWriteBit( int bit );					// set current write bit
	int				GetNumBitsWritten() const;		// returns number of bits written
	int				GetRemainingWriteBits() const;	// space left in bits for writing
	void			SaveWriteState( int& s, int& b ) const;	// save the write state
	void			RestoreWriteState( int s, int b );		// restore the write state

	int				GetReadCount() const;				// bytes read so far
	void			SetReadCount( int bytes );				// set the number of bytes and bits read
	int				GetReadBit() const;				// get current read bit
	void			SetReadBit( int bit );					// set current read bit
	int				GetNumBitsRead() const;			// returns number of bits read
	int				GetRemainingReadBits() const;		// number of bits left to read
	void			SaveReadState( int& c, int& b ) const;	// save the read state
	void			RestoreReadState( int c, int b );		// restore the read state

	void			BeginWriting();					// begin writing
	int				GetRemainingSpace() const;		// space left in bytes
	void			WriteByteAlign();					// write up to the next byte boundary
	void			WriteBits( int value, int numBits );	// write the specified number of bits
	void			WriteChar( int c );
	void			WriteByte( int c );
	void			WriteShort( int c );
	void			WriteUShort( int c );
	void			WriteInt( int c );
	void			WriteFloat( float f );
	void			WriteFloat( float f, int exponentBits, int mantissaBits );
	void			WriteAngle8( float f );
	void			WriteAngle16( float f );
	void			WriteDir( const idVec3& dir, int numBits );
	void			WriteString( const char* s, int maxLength = -1, bool make7Bit = true );
	void			WriteData( const void* data, int length );
	void			WriteNetadr( const netadr_t adr );

	void			WriteDeltaChar( int oldValue, int newValue );
	void			WriteDeltaByte( int oldValue, int newValue );
	void			WriteDeltaShort( int oldValue, int newValue );
	void			WriteDeltaInt( int oldValue, int newValue );
	void			WriteDeltaFloat( float oldValue, float newValue );
	void			WriteDeltaFloat( float oldValue, float newValue, int exponentBits, int mantissaBits );
	void			WriteDeltaByteCounter( int oldValue, int newValue );
	void			WriteDeltaShortCounter( int oldValue, int newValue );
	void			WriteDeltaIntCounter( int oldValue, int newValue );
	bool			WriteDeltaDict( const idDict& dict, const idDict* base );

	void			BeginReading() const;				// begin reading.
	int				GetRemaingData() const;			// number of bytes left to read
	void			ReadByteAlign() const;			// read up to the next byte boundary
	int				ReadBits( int numBits ) const;			// read the specified number of bits
	int				ReadChar() const;
	int				ReadByte() const;
	int				ReadShort() const;
	int				ReadUShort() const;
	int				ReadInt() const;
	float			ReadFloat() const;
	float			ReadFloat( int exponentBits, int mantissaBits ) const;
	float			ReadAngle8() const;
	float			ReadAngle16() const;
	idVec3			ReadDir( int numBits ) const;
	int				ReadString( char* buffer, int bufferSize ) const;
	int				ReadData( void* data, int length ) const;
	void			ReadNetadr( netadr_t* adr ) const;

	int				ReadDeltaChar( int oldValue ) const;
	int				ReadDeltaByte( int oldValue ) const;
	int				ReadDeltaShort( int oldValue ) const;
	int				ReadDeltaInt( int oldValue ) const;
	float			ReadDeltaFloat( float oldValue ) const;
	float			ReadDeltaFloat( float oldValue, int exponentBits, int mantissaBits ) const;
	int				ReadDeltaByteCounter( int oldValue ) const;
	int				ReadDeltaShortCounter( int oldValue ) const;
	int				ReadDeltaIntCounter( int oldValue ) const;
	bool			ReadDeltaDict( idDict& dict, const idDict* base ) const;

	static int		DirToBits( const idVec3& dir, int numBits );
	static idVec3	BitsToDir( int bits, int numBits );

private:
	byte* 			writeData;			// pointer to data for writing
	const byte* 	readData;			// pointer to data for reading
	int				maxSize;			// maximum size of message in bytes
	int				curSize;			// current size of message in bytes
	int				writeBit;			// number of bits written to the last written byte
	mutable int		readCount;			// number of bytes read so far
	mutable int		readBit;			// number of bits read from the last read byte
	bool			allowOverflow;		// if false, generate an error when the message is overflowed
	bool			overflowed;			// set to true if the buffer size failed (with allowOverflow set)

private:
	bool			CheckOverflow( int numBits );
	byte* 			GetByteSpace( int length );
	void			WriteDelta( int oldValue, int newValue, int numBits );
	int				ReadDelta( int oldValue, int numBits ) const;
};


ID_INLINE void idBitMsg::Init( byte* data, int length )
{
	writeData = data;
	readData = data;
	maxSize = length;
}

ID_INLINE void idBitMsg::Init( const byte* data, int length )
{
	writeData = NULL;
	readData = data;
	maxSize = length;
}

ID_INLINE byte* idBitMsg::GetData()
{
	return writeData;
}

ID_INLINE const byte* idBitMsg::GetData() const
{
	return readData;
}

ID_INLINE int idBitMsg::GetMaxSize() const
{
	return maxSize;
}

ID_INLINE void idBitMsg::SetAllowOverflow( bool set )
{
	allowOverflow = set;
}

ID_INLINE bool idBitMsg::IsOverflowed() const
{
	return overflowed;
}

ID_INLINE int idBitMsg::GetSize() const
{
	return curSize;
}

ID_INLINE void idBitMsg::SetSize( int size )
{
	if( size > maxSize )
	{
		curSize = maxSize;
	}
	else
	{
		curSize = size;
	}
}

ID_INLINE int idBitMsg::GetWriteBit() const
{
	return writeBit;
}

ID_INLINE void idBitMsg::SetWriteBit( int bit )
{
	writeBit = bit & 7;
	if( writeBit )
	{
		writeData[curSize - 1] &= ( 1 << writeBit ) - 1;
	}
}

ID_INLINE int idBitMsg::GetNumBitsWritten() const
{
	return ( ( curSize << 3 ) - ( ( 8 - writeBit ) & 7 ) );
}

ID_INLINE int idBitMsg::GetRemainingWriteBits() const
{
	return ( maxSize << 3 ) - GetNumBitsWritten();
}

ID_INLINE void idBitMsg::SaveWriteState( int& s, int& b ) const
{
	s = curSize;
	b = writeBit;
}

ID_INLINE void idBitMsg::RestoreWriteState( int s, int b )
{
	curSize = s;
	writeBit = b & 7;
	if( writeBit )
	{
		writeData[curSize - 1] &= ( 1 << writeBit ) - 1;
	}
}

ID_INLINE int idBitMsg::GetReadCount() const
{
	return readCount;
}

ID_INLINE void idBitMsg::SetReadCount( int bytes )
{
	readCount = bytes;
}

ID_INLINE int idBitMsg::GetReadBit() const
{
	return readBit;
}

ID_INLINE void idBitMsg::SetReadBit( int bit )
{
	readBit = bit & 7;
}

ID_INLINE int idBitMsg::GetNumBitsRead() const
{
	return ( ( readCount << 3 ) - ( ( 8 - readBit ) & 7 ) );
}

ID_INLINE int idBitMsg::GetRemainingReadBits() const
{
	return ( curSize << 3 ) - GetNumBitsRead();
}

ID_INLINE void idBitMsg::SaveReadState( int& c, int& b ) const
{
	c = readCount;
	b = readBit;
}

ID_INLINE void idBitMsg::RestoreReadState( int c, int b )
{
	readCount = c;
	readBit = b & 7;
}

ID_INLINE void idBitMsg::BeginWriting()
{
	curSize = 0;
	overflowed = false;
	writeBit = 0;
}

ID_INLINE int idBitMsg::GetRemainingSpace() const
{
	return maxSize - curSize;
}

ID_INLINE void idBitMsg::WriteByteAlign()
{
	writeBit = 0;
}

ID_INLINE void idBitMsg::WriteChar( int c )
{
	WriteBits( c, -8 );
}

ID_INLINE void idBitMsg::WriteByte( int c )
{
	WriteBits( c, 8 );
}

ID_INLINE void idBitMsg::WriteShort( int c )
{
	WriteBits( c, -16 );
}

ID_INLINE void idBitMsg::WriteUShort( int c )
{
	WriteBits( c, 16 );
}

ID_INLINE void idBitMsg::WriteInt( int c )
{
	WriteBits( c, 32 );
}

ID_INLINE void idBitMsg::WriteFloat( float f )
{
	WriteBits( *reinterpret_cast<int*>( &f ), 32 );
}

ID_INLINE void idBitMsg::WriteFloat( float f, int exponentBits, int mantissaBits )
{
	int bits = idMath::FloatToBits( f, exponentBits, mantissaBits );
	WriteBits( bits, 1 + exponentBits + mantissaBits );
}

ID_INLINE void idBitMsg::WriteAngle8( float f )
{
	WriteByte( ANGLE2BYTE( f ) );
}

ID_INLINE void idBitMsg::WriteAngle16( float f )
{
	WriteShort( ANGLE2SHORT( f ) );
}

ID_INLINE void idBitMsg::WriteDir( const idVec3& dir, int numBits )
{
	WriteBits( DirToBits( dir, numBits ), numBits );
}

ID_INLINE void idBitMsg::WriteDeltaChar( int oldValue, int newValue )
{
	WriteDelta( oldValue, newValue, -8 );
}

ID_INLINE void idBitMsg::WriteDeltaByte( int oldValue, int newValue )
{
	WriteDelta( oldValue, newValue, 8 );
}

ID_INLINE void idBitMsg::WriteDeltaShort( int oldValue, int newValue )
{
	WriteDelta( oldValue, newValue, -16 );
}

ID_INLINE void idBitMsg::WriteDeltaInt( int oldValue, int newValue )
{
	WriteDelta( oldValue, newValue, 32 );
}

ID_INLINE void idBitMsg::WriteDeltaFloat( float oldValue, float newValue )
{
	WriteDelta( *reinterpret_cast<int*>( &oldValue ), *reinterpret_cast<int*>( &newValue ), 32 );
}

ID_INLINE void idBitMsg::WriteDeltaFloat( float oldValue, float newValue, int exponentBits, int mantissaBits )
{
	int oldBits = idMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = idMath::FloatToBits( newValue, exponentBits, mantissaBits );
	WriteDelta( oldBits, newBits, 1 + exponentBits + mantissaBits );
}

ID_INLINE void idBitMsg::BeginReading() const
{
	readCount = 0;
	readBit = 0;
}

ID_INLINE int idBitMsg::GetRemaingData() const
{
	return curSize - readCount;
}

ID_INLINE void idBitMsg::ReadByteAlign() const
{
	readBit = 0;
}

ID_INLINE int idBitMsg::ReadChar() const
{
	return ( signed char )ReadBits( -8 );
}

ID_INLINE int idBitMsg::ReadByte() const
{
	return ( unsigned char )ReadBits( 8 );
}

ID_INLINE int idBitMsg::ReadShort() const
{
	return ( short )ReadBits( -16 );
}

ID_INLINE int idBitMsg::ReadUShort() const
{
	return ( unsigned short )ReadBits( 16 );
}

ID_INLINE int idBitMsg::ReadInt() const
{
	return ReadBits( 32 );
}

ID_INLINE float idBitMsg::ReadFloat() const
{
	float value;
	*reinterpret_cast<int*>( &value ) = ReadBits( 32 );
	return value;
}

ID_INLINE float idBitMsg::ReadFloat( int exponentBits, int mantissaBits ) const
{
	int bits = ReadBits( 1 + exponentBits + mantissaBits );
	return idMath::BitsToFloat( bits, exponentBits, mantissaBits );
}

ID_INLINE float idBitMsg::ReadAngle8() const
{
	return BYTE2ANGLE( ReadByte() );
}

ID_INLINE float idBitMsg::ReadAngle16() const
{
	return SHORT2ANGLE( ReadShort() );
}

ID_INLINE idVec3 idBitMsg::ReadDir( int numBits ) const
{
	return BitsToDir( ReadBits( numBits ), numBits );
}

ID_INLINE int idBitMsg::ReadDeltaChar( int oldValue ) const
{
	return ( signed char )ReadDelta( oldValue, -8 );
}

ID_INLINE int idBitMsg::ReadDeltaByte( int oldValue ) const
{
	return ( unsigned char )ReadDelta( oldValue, 8 );
}

ID_INLINE int idBitMsg::ReadDeltaShort( int oldValue ) const
{
	return ( short )ReadDelta( oldValue, -16 );
}

ID_INLINE int idBitMsg::ReadDeltaInt( int oldValue ) const
{
	return ReadDelta( oldValue, 32 );
}

ID_INLINE float idBitMsg::ReadDeltaFloat( float oldValue ) const
{
	float value;
	*reinterpret_cast<int*>( &value ) = ReadDelta( *reinterpret_cast<int*>( &oldValue ), 32 );
	return value;
}

ID_INLINE float idBitMsg::ReadDeltaFloat( float oldValue, int exponentBits, int mantissaBits ) const
{
	int oldBits = idMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = ReadDelta( oldBits, 1 + exponentBits + mantissaBits );
	return idMath::BitsToFloat( newBits, exponentBits, mantissaBits );
}


/*
===============================================================================

  idBitMsgDelta

===============================================================================
*/

class idBitMsgDelta
{
public:
	idBitMsgDelta();
	~idBitMsgDelta() {}

	void			Init( const idBitMsg* base, idBitMsg* newBase, idBitMsg* delta );
	void			Init( const idBitMsg* base, idBitMsg* newBase, const idBitMsg* delta );
	bool			HasChanged() const;

	void			WriteBits( int value, int numBits );
	void			WriteChar( int c );
	void			WriteByte( int c );
	void			WriteShort( int c );
	void			WriteUShort( int c );
	void			WriteInt( int c );
	void			WriteFloat( float f );
	void			WriteFloat( float f, int exponentBits, int mantissaBits );
	void			WriteAngle8( float f );
	void			WriteAngle16( float f );
	void			WriteDir( const idVec3& dir, int numBits );
	void			WriteString( const char* s, int maxLength = -1 );
	void			WriteData( const void* data, int length );
	void			WriteDict( const idDict& dict );

	void			WriteDeltaChar( int oldValue, int newValue );
	void			WriteDeltaByte( int oldValue, int newValue );
	void			WriteDeltaShort( int oldValue, int newValue );
	void			WriteDeltaInt( int oldValue, int newValue );
	void			WriteDeltaFloat( float oldValue, float newValue );
	void			WriteDeltaFloat( float oldValue, float newValue, int exponentBits, int mantissaBits );
	void			WriteDeltaByteCounter( int oldValue, int newValue );
	void			WriteDeltaShortCounter( int oldValue, int newValue );
	void			WriteDeltaIntCounter( int oldValue, int newValue );

	int				ReadBits( int numBits ) const;
	int				ReadChar() const;
	int				ReadByte() const;
	int				ReadShort() const;
	int				ReadUShort() const;
	int				ReadInt() const;
	float			ReadFloat() const;
	float			ReadFloat( int exponentBits, int mantissaBits ) const;
	float			ReadAngle8() const;
	float			ReadAngle16() const;
	idVec3			ReadDir( int numBits ) const;
	void			ReadString( char* buffer, int bufferSize ) const;
	void			ReadData( void* data, int length ) const;
	void			ReadDict( idDict& dict );

	int				ReadDeltaChar( int oldValue ) const;
	int				ReadDeltaByte( int oldValue ) const;
	int				ReadDeltaShort( int oldValue ) const;
	int				ReadDeltaInt( int oldValue ) const;
	float			ReadDeltaFloat( float oldValue ) const;
	float			ReadDeltaFloat( float oldValue, int exponentBits, int mantissaBits ) const;
	int				ReadDeltaByteCounter( int oldValue ) const;
	int				ReadDeltaShortCounter( int oldValue ) const;
	int				ReadDeltaIntCounter( int oldValue ) const;

private:
	const idBitMsg* base;			// base
	idBitMsg* 		newBase;		// new base
	idBitMsg* 		writeDelta;		// delta from base to new base for writing
	const idBitMsg* readDelta;		// delta from base to new base for reading
	mutable bool	changed;		// true if the new base is different from the base

private:
	void			WriteDelta( int oldValue, int newValue, int numBits );
	int				ReadDelta( int oldValue, int numBits ) const;
};

ID_INLINE idBitMsgDelta::idBitMsgDelta()
{
	base = NULL;
	newBase = NULL;
	writeDelta = NULL;
	readDelta = NULL;
	changed = false;
}

ID_INLINE void idBitMsgDelta::Init( const idBitMsg* base, idBitMsg* newBase, idBitMsg* delta )
{
	this->base = base;
	this->newBase = newBase;
	this->writeDelta = delta;
	this->readDelta = delta;
	this->changed = false;
}

ID_INLINE void idBitMsgDelta::Init( const idBitMsg* base, idBitMsg* newBase, const idBitMsg* delta )
{
	this->base = base;
	this->newBase = newBase;
	this->writeDelta = NULL;
	this->readDelta = delta;
	this->changed = false;
}

ID_INLINE bool idBitMsgDelta::HasChanged() const
{
	return changed;
}

ID_INLINE void idBitMsgDelta::WriteChar( int c )
{
	WriteBits( c, -8 );
}

ID_INLINE void idBitMsgDelta::WriteByte( int c )
{
	WriteBits( c, 8 );
}

ID_INLINE void idBitMsgDelta::WriteShort( int c )
{
	WriteBits( c, -16 );
}

ID_INLINE void idBitMsgDelta::WriteUShort( int c )
{
	WriteBits( c, 16 );
}

ID_INLINE void idBitMsgDelta::WriteInt( int c )
{
	WriteBits( c, 32 );
}

ID_INLINE void idBitMsgDelta::WriteFloat( float f )
{
	WriteBits( *reinterpret_cast<int*>( &f ), 32 );
}

ID_INLINE void idBitMsgDelta::WriteFloat( float f, int exponentBits, int mantissaBits )
{
	int bits = idMath::FloatToBits( f, exponentBits, mantissaBits );
	WriteBits( bits, 1 + exponentBits + mantissaBits );
}

ID_INLINE void idBitMsgDelta::WriteAngle8( float f )
{
	WriteBits( ANGLE2BYTE( f ), 8 );
}

ID_INLINE void idBitMsgDelta::WriteAngle16( float f )
{
	WriteBits( ANGLE2SHORT( f ), 16 );
}

ID_INLINE void idBitMsgDelta::WriteDir( const idVec3& dir, int numBits )
{
	WriteBits( idBitMsg::DirToBits( dir, numBits ), numBits );
}

ID_INLINE void idBitMsgDelta::WriteDeltaChar( int oldValue, int newValue )
{
	WriteDelta( oldValue, newValue, -8 );
}

ID_INLINE void idBitMsgDelta::WriteDeltaByte( int oldValue, int newValue )
{
	WriteDelta( oldValue, newValue, 8 );
}

ID_INLINE void idBitMsgDelta::WriteDeltaShort( int oldValue, int newValue )
{
	WriteDelta( oldValue, newValue, -16 );
}

ID_INLINE void idBitMsgDelta::WriteDeltaInt( int oldValue, int newValue )
{
	WriteDelta( oldValue, newValue, 32 );
}

ID_INLINE void idBitMsgDelta::WriteDeltaFloat( float oldValue, float newValue )
{
	WriteDelta( *reinterpret_cast<int*>( &oldValue ), *reinterpret_cast<int*>( &newValue ), 32 );
}

ID_INLINE void idBitMsgDelta::WriteDeltaFloat( float oldValue, float newValue, int exponentBits, int mantissaBits )
{
	int oldBits = idMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = idMath::FloatToBits( newValue, exponentBits, mantissaBits );
	WriteDelta( oldBits, newBits, 1 + exponentBits + mantissaBits );
}

ID_INLINE int idBitMsgDelta::ReadChar() const
{
	return ( signed char )ReadBits( -8 );
}

ID_INLINE int idBitMsgDelta::ReadByte() const
{
	return ( unsigned char )ReadBits( 8 );
}

ID_INLINE int idBitMsgDelta::ReadShort() const
{
	return ( short )ReadBits( -16 );
}

ID_INLINE int idBitMsgDelta::ReadUShort() const
{
	return ( unsigned short )ReadBits( 16 );
}

ID_INLINE int idBitMsgDelta::ReadInt() const
{
	return ReadBits( 32 );
}

ID_INLINE float idBitMsgDelta::ReadFloat() const
{
	float value;
	*reinterpret_cast<int*>( &value ) = ReadBits( 32 );
	return value;
}

ID_INLINE float idBitMsgDelta::ReadFloat( int exponentBits, int mantissaBits ) const
{
	int bits = ReadBits( 1 + exponentBits + mantissaBits );
	return idMath::BitsToFloat( bits, exponentBits, mantissaBits );
}

ID_INLINE float idBitMsgDelta::ReadAngle8() const
{
	return BYTE2ANGLE( ReadByte() );
}

ID_INLINE float idBitMsgDelta::ReadAngle16() const
{
	return SHORT2ANGLE( ReadShort() );
}

ID_INLINE idVec3 idBitMsgDelta::ReadDir( int numBits ) const
{
	return idBitMsg::BitsToDir( ReadBits( numBits ), numBits );
}

ID_INLINE int idBitMsgDelta::ReadDeltaChar( int oldValue ) const
{
	return ( signed char )ReadDelta( oldValue, -8 );
}

ID_INLINE int idBitMsgDelta::ReadDeltaByte( int oldValue ) const
{
	return ( unsigned char )ReadDelta( oldValue, 8 );
}

ID_INLINE int idBitMsgDelta::ReadDeltaShort( int oldValue ) const
{
	return ( short )ReadDelta( oldValue, -16 );
}

ID_INLINE int idBitMsgDelta::ReadDeltaInt( int oldValue ) const
{
	return ReadDelta( oldValue, 32 );
}

ID_INLINE float idBitMsgDelta::ReadDeltaFloat( float oldValue ) const
{
	float value;
	*reinterpret_cast<int*>( &value ) = ReadDelta( *reinterpret_cast<int*>( &oldValue ), 32 );
	return value;
}

ID_INLINE float idBitMsgDelta::ReadDeltaFloat( float oldValue, int exponentBits, int mantissaBits ) const
{
	int oldBits = idMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = ReadDelta( oldBits, 1 + exponentBits + mantissaBits );
	return idMath::BitsToFloat( newBits, exponentBits, mantissaBits );
}

#endif /* !__BITMSG_H__ */
