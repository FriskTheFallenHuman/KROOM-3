/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#ifndef __MP3FILE_H
#define __MP3FILE_H

/*
================================================================================================
Contains the Mp3File declaration.
================================================================================================
*/

#define DR_MP3_NO_STDIO
#pragma warning(push, 0)
#include "drlibs/dr_mp3.h"
#pragma warning(pop)

/*
================================================
idMp3File is used for reading generic MP3 files.
================================================
*/
class idMp3File
{
public:
	ID_INLINE 	idMp3File();
	ID_INLINE 	~idMp3File();

	bool	Open( const char* fileName );
	void	Close();
	bool	IsEOS();
	void	Seek( int samplePos );
	int		Read( void* buffer, int bufferSize );
	int64	Size();
	int64	CompressedSize();
	void	GetFormat( idWaveFile::waveFmt_t& format );

private:
	drmp3*	mp3;
	byte*	fileData;	// dr_mp3 does not copy the input, so we must keep this alive
	idFile*	mhmmio;
};

/*
========================
idMp3File::idMp3File
========================
*/
ID_INLINE idMp3File::idMp3File() :
	mp3( NULL ),
	fileData( NULL ),
	mhmmio( NULL )
{
}

/*
========================
idMp3File::~idMp3File
========================
*/
ID_INLINE idMp3File::~idMp3File()
{
	Close();
}

#endif // !__MP3FILE_H