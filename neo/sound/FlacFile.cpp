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

#include "precompiled.h"
#pragma hdrstop

#include "snd_local.h"

/*
====================
idFlacFile::Close
====================
*/
void idFlacFile::Close()
{
	if( flac )
	{
		drflac_close( flac );	// frees the drflac object itself
		flac = NULL;
	}

	if( fileData )
	{
		Mem_Free( fileData );
		fileData = NULL;
	}

	if( mhmmio )
	{
		fileSystem->CloseFile( mhmmio );
		mhmmio = NULL;
	}
}

/*
====================
idFlacFile::GetFormat
====================
*/
void idFlacFile::GetFormat( idWaveFile::waveFmt_t& format )
{
	format.basic.samplesPerSec = this->flac ? ( int )this->flac->sampleRate : 0;
	format.basic.numChannels = this->flac ? ( int )this->flac->channels   : 0;
	format.basic.bitsPerSample = sizeof( short ) * 8;
	format.basic.formatTag = idWaveFile::FORMAT_PCM;
	format.basic.blockSize = format.basic.numChannels * format.basic.bitsPerSample / 8;
	format.basic.avgBytesPerSec = format.basic.samplesPerSec * format.basic.blockSize;
}

/*
====================
idFlacFile::Seek
====================
*/
void idFlacFile::Seek( int samplePos )
{
	if( flac )
	{
		drflac_seek_to_pcm_frame( flac, ( drflac_uint64 )samplePos );
	}
}

/*
====================
idFlacFile::IsEOS
====================
*/
bool idFlacFile::IsEOS()
{
	if( flac == NULL )
	{
		return true;
	}
	if( flac->totalPCMFrameCount == 0 )
	{
		// Unknown total length.
		return false;
	}
	return flac->currentPCMFrame >= flac->totalPCMFrameCount;
}

/*
====================
idFlacFile::Size
====================
*/
int64 idFlacFile::Size()
{
	if( flac == NULL || flac->totalPCMFrameCount == 0 )
	{
		return 0;
	}
	return ( int64 )( flac->totalPCMFrameCount * flac->channels * sizeof( drflac_int16 ) );
}

/*
====================
idFlacFile::CompressedSize
====================
*/
int64 idFlacFile::CompressedSize()
{
	if( flac == NULL || flac->totalPCMFrameCount == 0 )
	{
		return 0;
	}
	return ( int64 )flac->totalPCMFrameCount;
}

/*
====================
idFlacFile::Read
====================
*/
int idFlacFile::Read( void* pBuffer, int dwSizeToRead )
{
	if( flac == NULL || pBuffer == NULL || dwSizeToRead <= 0 )
	{
		return 0;
	}

	const int channels = ( int )flac->channels;
	const int bytesPerFrame = channels * ( int )sizeof( drflac_int16 );
	const drflac_uint64 framesToRead = ( drflac_uint64 )( dwSizeToRead / bytesPerFrame );
	if( framesToRead == 0 )
	{
		return 0;
	}

	const drflac_uint64 framesRead = drflac_read_pcm_frames_s16( flac, framesToRead, ( drflac_int16* )pBuffer );

	return ( int )( framesRead * bytesPerFrame );
}

/*
====================
idFlacFile::Open
====================
*/
bool idFlacFile::Open( const char* fileName )
{
	Close();

	mhmmio = fileSystem->OpenFileRead( fileName );
	if( mhmmio == NULL )
	{
		return false;
	}

	const int fileSize = mhmmio->Length();
	if( fileSize <= 0 )
	{
		fileSystem->CloseFile( mhmmio );
		mhmmio = NULL;
		return false;
	}

	fileData = ( byte* )Mem_Alloc( fileSize, TAG_CRAP );
	mhmmio->Read( fileData, fileSize );

	flac = drflac_open_memory( fileData, fileSize, NULL );
	if( flac == NULL )
	{
		Mem_Free( fileData );
		fileData = NULL;
		fileSystem->CloseFile( mhmmio );
		mhmmio = NULL;

		common->Warning( "Opening FLAC file '%s' with dr_flac failed\n", fileName );
		return false;
	}

	return true;
}