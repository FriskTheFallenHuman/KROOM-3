/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans
Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>  (MS ADPCM decoder)
Copyright (c) 2011 Chris Robinson <chris.kcat@gmail.com> (OpenAL helpers)
Copyright (C) 2021 George Kalmpokis

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

#include "../snd_local.h"

/*
========================
DecodeOggAudio
========================
*/
static bool DecodeOggAudio( byte** audio, int* len, int* rate, ALenum* sample, const char* filename )
{
	idOggFile ogg;
	if( !ogg.Open( filename ) )
	{
		return false;
	}

	idWaveFile::waveFmt_t fmt;
	ogg.GetFormat( fmt );

	int64 totalBytes64 = ogg.Size();
	if( totalBytes64 <= 0 || totalBytes64 > INT_MAX )
	{
		ogg.Close();
		return false;
	}

	const int totalBytes = ( int )totalBytes64;
	byte* pcm = ( byte* )malloc( totalBytes );
	if( pcm == NULL )
	{
		ogg.Close();
		return false;
	}

	const int bytesRead = ogg.Read( pcm, totalBytes );
	ogg.Close();

	if( bytesRead <= 0 )
	{
		free( pcm );
		return false;
	}

	*audio = pcm;
	*len = bytesRead;
	*rate = fmt.basic.samplesPerSec;
	*sample = ( fmt.basic.numChannels == 1 ) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	return true;
}


/*
========================
DecodeFlacAudio
========================
*/
static bool DecodeFlacAudio( byte** audio, int* len, int* rate, ALenum* sample, const char* filename )
{
	idFlacFile in;
	if( !in.Open( filename ) )
	{
		return false;
	}

	idWaveFile::waveFmt_t fmt;
	in.GetFormat( fmt );

	const int64 size64 = in.Size();
	if( size64 <= 0 || size64 > INT_MAX )
	{
		in.Close();
		return false;
	}

	const int size = ( int )size64;
	byte* pcm = ( byte* )malloc( size );
	if( pcm == NULL )
	{
		in.Close();
		return false;
	}

	const int read = in.Read( pcm, size );
	in.Close();

	if( read <= 0 )
	{
		free( pcm );
		return false;
	}

	*audio = pcm;
	*len = read;
	*rate = fmt.basic.samplesPerSec;
	*sample = ( fmt.basic.numChannels == 1 ) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	return true;
}


/*
========================
DecodeMp3Audio
========================
*/
static bool DecodeMp3Audio( byte** audio, int* len, int* rate, ALenum* sample, const char* filename )
{
	idMp3File in;
	if( !in.Open( filename ) )
	{
		return false;
	}

	idWaveFile::waveFmt_t fmt;
	in.GetFormat( fmt );

	const int64 size64 = in.Size();
	if( size64 <= 0 || size64 > INT_MAX )
	{
		in.Close();
		return false;
	}

	const int size = ( int )size64;
	byte* pcm = ( byte* )malloc( size );
	if( pcm == NULL )
	{
		in.Close();
		return false;
	}

	const int read = in.Read( pcm, size );
	in.Close();

	if( read <= 0 )
	{
		free( pcm );
		return false;
	}

	*audio = pcm;
	*len = read;
	*rate = fmt.basic.samplesPerSec;
	*sample = ( fmt.basic.numChannels == 1 ) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	return true;
}


/*
========================
DecodeALAudio
========================
*/
bool DecodeALAudio( byte** audio, int* len, int* rate, ALenum* sample, const char* filename, int* /*loopStart*/, int* /*loopEnd*/ )
{
	if( audio == NULL || len == NULL || rate == NULL || sample == NULL || filename == NULL )
	{
		return false;
	}

	*audio = NULL;
	*len = 0;
	*rate = 0;
	*sample = 0;

	if( idStr::CheckExtension( filename, "ogg" ) )
	{
		return DecodeOggAudio( audio, len, rate, sample, filename );
	}
	if( idStr::CheckExtension( filename, "flac" ) )
	{
		return DecodeFlacAudio( audio, len, rate, sample, filename );
	}
	if( idStr::CheckExtension( filename, "mp3" ) )
	{
		return DecodeMp3Audio( audio, len, rate, sample, filename );
	}

	return false;
}

/*
========================
GetSampleName
========================
*/
const char* GetSampleName( ALenum sample )
{
	switch( sample )
	{
		case AL_FORMAT_MONO8:
			return "Mono 8-bit";
		case AL_FORMAT_MONO16:
			return "Mono 16-bit";
		case AL_FORMAT_STEREO8:
			return "Stereo 8-bit";
		case AL_FORMAT_STEREO16:
			return "Stereo 16-bit";
		case AL_FORMAT_STEREO_FLOAT32:
			return "Stereo Float Point";
		case AL_FORMAT_MONO_FLOAT32:
			return "Mono Float Point";
	}
	return "";
}