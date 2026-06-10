/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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

#ifndef __CINEMATICFFMPEG_H__
#define __CINEMATICFFMPEG_H__

#undef nullptr

#if defined(USE_FFMPEG)
extern "C"
{
	#ifndef INT64_C
		#define INT64_C(c) (c ## LL)
		#define UINT64_C(c) (c ## ULL)
	#endif

	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libswresample/swresample.h>
	#include <libavutil/imgutils.h>
}

#include <queue>
#define NUM_LAG_FRAMES 15	// SRS - Lag audio by 15 frames (~1/2 sec at 30 fps) for ffmpeg bik decoder AV sync

#endif

#include "../sound/CinematicAudio.h"

/*
===============================================================================

	Bink cinematic

===============================================================================
*/

class idCinematicFFMPEG : public idCinematic
{
public:
public:
	idCinematicFFMPEG();
	virtual			~idCinematicFFMPEG();
	virtual bool	InitFromFile( const char* qpath, bool amilooping );
	virtual int		AnimationLength();
	virtual cinData_t	ImageForTime( int milliseconds );
	virtual void	Close();
	virtual void	ResetTime( int time );
	virtual int		GetStartTime();
	virtual bool    IsPlaying() const;
	virtual void	ExportToTGA( bool skipExisting = true );
	virtual float	GetFrameRate() const;

private:
	int						animationLength;
	int						startTime;
	float					frameRate;
	int						CIN_WIDTH, CIN_HEIGHT;
	cinStatus_t				status;
	bool					looping;
	idImage*				img;
	byte* 					image;

#if defined(USE_FFMPEG)
	int						video_stream_index;
	int						audio_stream_index;
	AVFormatContext*		fmt_ctx;
	AVFrame*				frame;
	AVFrame*				frame2;
	AVFrame*				frame3;
#if LIBAVCODEC_VERSION_MAJOR > 58
	const AVCodec*			dec;
	const AVCodec*			dec2;
#else
	AVCodec*				dec;
	AVCodec*				dec2;
#endif
	AVCodecContext*			dec_ctx;
	AVCodecContext*			dec_ctx2;
	SwsContext*				img_convert_ctx;
	bool					hasFrame;
	long					framePos;
	AVSampleFormat			dst_smp;
	bool					hasplanar;
	SwrContext*				swr_ctx;
	cinData_t				ImageForTimeFFMPEG( int milliseconds );
	bool					InitFromFFMPEGFile( const char* qpath, bool looping );
	void					FFMPEGReset();
	std::queue<uint8_t*>	lagBuffer;
	std::queue<int>			lagBufSize;
	bool					skipLag;
#endif

	CinematicAudio*			cinematicAudio = NULL;
};

#endif /* !__CINEMATICFFMPEG_H__ */
