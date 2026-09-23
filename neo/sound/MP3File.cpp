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
idMp3File::Close
====================
*/
void idMp3File::Close()
{
	if( mp3 )
	{
		drmp3_uninit( mp3 );
		Mem_Free( mp3 );
		mp3 = NULL;
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
idMp3File::GetFormat
====================
*/
void idMp3File::GetFormat( idWaveFile::waveFmt_t& format )
{
	format.basic.samplesPerSec = this->mp3 ? ( int )this->mp3->sampleRate : 0;
	format.basic.numChannels = this->mp3 ? ( int )this->mp3->channels   : 0;
	format.basic.bitsPerSample = sizeof( short ) * 8;
	format.basic.formatTag = idWaveFile::FORMAT_PCM;
	format.basic.blockSize = format.basic.numChannels * format.basic.bitsPerSample / 8;
	format.basic.avgBytesPerSec = format.basic.samplesPerSec * format.basic.blockSize;
}

/*
====================
idMp3File::Seek
====================
*/
void idMp3File::Seek( int samplePos )
{
	if( mp3 )
	{
		drmp3_seek_to_pcm_frame( mp3, ( drmp3_uint64 )samplePos );
	}
}

/*
====================
idMp3File::IsEOS
====================
*/
bool idMp3File::IsEOS()
{
	if( mp3 == NULL )
	{
		return true;
	}
	if( mp3->totalPCMFrameCount == DRMP3_UINT64_MAX )
	{
		// Unknown total length.
		return mp3->atEnd != DRMP3_FALSE;
	}
	return mp3->currentPCMFrame >= mp3->totalPCMFrameCount;
}

/*
====================
idMp3File::Size
====================
*/
int64 idMp3File::Size()
{
	if( mp3 == NULL || mp3->totalPCMFrameCount == DRMP3_UINT64_MAX )
	{
		return 0;
	}
	return ( int64 )( mp3->totalPCMFrameCount * mp3->channels * sizeof( drmp3_int16 ) );
}

/*
====================
idMp3File::CompressedSize
====================
*/
int64 idMp3File::CompressedSize()
{
	if( mp3 == NULL || mp3->totalPCMFrameCount == DRMP3_UINT64_MAX )
	{
		return 0;
	}
	return ( int64 )mp3->totalPCMFrameCount;
}

/*
====================
idMp3File::Read
====================
*/
int idMp3File::Read( void* pBuffer, int dwSizeToRead )
{
	if( mp3 == NULL || pBuffer == NULL || dwSizeToRead <= 0 )
	{
		return 0;
	}

	const int channels = ( int )mp3->channels;
	const int bytesPerFrame = channels * ( int )sizeof( drmp3_int16 );
	const drmp3_uint64 framesToRead = ( drmp3_uint64 )( dwSizeToRead / bytesPerFrame );
	if( framesToRead == 0 )
	{
		return 0;
	}

	const drmp3_uint64 framesRead = drmp3_read_pcm_frames_s16( mp3, framesToRead, ( drmp3_int16* )pBuffer );

	return ( int )( framesRead * bytesPerFrame );
}

/*
====================
idMp3File::Open
====================
*/
bool idMp3File::Open( const char* fileName )
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

	mp3 = ( drmp3* )Mem_Alloc( sizeof( drmp3 ), TAG_AUDIO );
	if( mp3 == NULL || !drmp3_init_memory( mp3, fileData, fileSize, NULL ) )
	{
		if( mp3 )
		{
			Mem_Free( mp3 );
			mp3 = NULL;
		}
		Mem_Free( fileData );
		fileData = NULL;
		fileSystem->CloseFile( mhmmio );
		mhmmio = NULL;

		common->Warning( "Opening MP3 file '%s' with dr_mp3 failed\n", fileName );
		return false;
	}

	return true;
}