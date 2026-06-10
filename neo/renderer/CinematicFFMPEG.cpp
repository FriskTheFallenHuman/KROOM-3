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

#include "precompiled.h"
#pragma hdrstop

#include "RenderCommon.h"

#include "CinematicFFMPEG.h"

#if defined(_MSC_VER) && !defined(USE_OPENAL)
	#include <xaudio2.h>
	#include "../sound/XAudio2/XA2_CinematicAudio.h"
#else
	#include "../sound/OpenAL/AL_CinematicAudio.h"
#endif

extern idCVar s_noSound;
extern idCVar s_playCinematicAudio;

const int DEFAULT_CIN_WIDTH = 512;
const int DEFAULT_CIN_HEIGHT = 512;

/*
==============
idCinematicFFMPEG::idCinematicFFMPEG
==============
*/
idCinematicFFMPEG::idCinematicFFMPEG()
{
#if defined(USE_FFMPEG)
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
	frame = av_frame_alloc();
	frame2 = av_frame_alloc();
	frame3 = av_frame_alloc();
#else
	frame = avcodec_alloc_frame();
	frame2 = avcodec_alloc_frame();
	frame3 = avcodec_alloc_frame();
#endif // LIBAVCODEC_VERSION_INT
	dec_ctx = NULL;
	dec_ctx2 = NULL;
	fmt_ctx = NULL;
	video_stream_index = -1;
	audio_stream_index = -1;
	hasplanar = false;
	swr_ctx = NULL;
	img_convert_ctx = NULL;
	hasFrame = false;
	framePos = -1;
	skipLag = false;
#endif

	image = NULL;
	status = FMV_EOF;
	img = globalImages->AllocStandaloneImage( "_cinematic" );
	if( img != NULL )
	{
		idImageOpts opts;
		opts.format = FMT_RGBA8;
		opts.colorFormat = CFM_DEFAULT;
		opts.width = 32;
		opts.height = 32;
		opts.numLevels = 1;
		img->AllocImage( opts, TF_LINEAR, TR_REPEAT );
	}

}

/*
==============
idCinematicFFMPEG::~idCinematicFFMPEG
==============
*/
idCinematicFFMPEG::~idCinematicFFMPEG()
{
	Close();

#if defined(USE_FFMPEG)
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
	av_frame_free( &frame );
	av_frame_free( &frame2 );
	av_frame_free( &frame3 );
#else
	av_freep( &frame );
	av_freep( &frame2 );
	av_freep( &frame3 );
#endif
#endif

	delete img;
	img = NULL;

	if( cinematicAudio )
	{
		cinematicAudio->ShutdownAudio();
		delete cinematicAudio;
		cinematicAudio = NULL;
	}
}

#if defined(USE_FFMPEG)
/*
==============
GetSampleFormat
==============
*/
const char* GetSampleFormat( AVSampleFormat sample_fmt )
{
	switch( sample_fmt )
	{
		case AV_SAMPLE_FMT_U8:
		case AV_SAMPLE_FMT_U8P:
		{
			return "8-bit";
		}
		case AV_SAMPLE_FMT_S16:
		case AV_SAMPLE_FMT_S16P:
		{
			return "16-bit";
		}
		case AV_SAMPLE_FMT_S32:
		case AV_SAMPLE_FMT_S32P:
		{
			return "32-bit";
		}
		case AV_SAMPLE_FMT_FLT:
		case AV_SAMPLE_FMT_FLTP:
		{
			return "Float";
		}
		case AV_SAMPLE_FMT_DBL:
		case AV_SAMPLE_FMT_DBLP:
		{
			return "Double";
		}
		default:
		{
			return "Unknown";
		}
	}
}

/*
==============
idCinematicFFMPEG::InitFromFFMPEGFile
==============
*/
bool idCinematicFFMPEG::InitFromFFMPEGFile( const char* qpath, bool amilooping )
{
	int ret;
	int ret2;
	int file_size;
	char error[64];
	looping = amilooping;
	startTime = 0;
	CIN_HEIGHT = DEFAULT_CIN_HEIGHT;
	CIN_WIDTH  =  DEFAULT_CIN_WIDTH;

	idStr fullpath;
	idFile* testFile = fileSystem->OpenFileRead( qpath );
	if( testFile )
	{
		fullpath = testFile->GetFullPath();
		file_size = testFile->Length();
		fileSystem->CloseFile( testFile );
	}
	else if( idStr::Cmpn( qpath, "sound/vo", 8 ) == 0 )
	{
		idStr newPath( qpath );
		newPath.Replace( "sound/vo", "sound/VO" );

		testFile = fileSystem->OpenFileRead( newPath );
		if( testFile )
		{
			fullpath = testFile->GetFullPath();
			file_size = testFile->Length();
			fileSystem->CloseFile( testFile );
		}
		else
		{
			common->Warning( "idCinematicFFMPEG: Cannot open FFMPEG video file: '%s', %d\n", qpath, looping );
			return false;
		}
	}

	if( ( ret = avformat_open_input( &fmt_ctx, fullpath, NULL, NULL ) ) < 0 )
	{
		if( ret < 0 )
		{
			common->Warning( "idCinematicFFMPEG: Cannot open FFMPEG video file: '%s', %d\n", qpath, looping );
			return false;
		}
	}

	if( ( ret = avformat_find_stream_info( fmt_ctx, NULL ) ) < 0 )
	{
		common->Warning( "idCinematicFFMPEG: Cannot find stream info: '%s', %d\n", qpath, looping );
		return false;
	}

	// select the video stream
	ret = av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0 );
	if( ret < 0 )
	{
		common->Warning( "idCinematicFFMPEG: Cannot find a video stream in: '%s', %d\n", qpath, looping );
		return false;
	}

	video_stream_index = ret;

	dec_ctx = avcodec_alloc_context3( dec );
	if( ( ret = avcodec_parameters_to_context( dec_ctx, fmt_ctx->streams[video_stream_index]->codecpar ) ) < 0 )
	{
		av_strerror( ret, error, sizeof( error ) );
		common->Warning( "idCinematicFFMPEG: Failed to create video codec context from codec parameters with error: %s\n", error );
	}

	dec_ctx->framerate = fmt_ctx->streams[video_stream_index]->avg_frame_rate;
	dec_ctx->pkt_timebase = fmt_ctx->streams[video_stream_index]->time_base;		// SRS - packet timebase for frame->pts timestamps
	
	// init the video decoder
	if( ( ret = avcodec_open2( dec_ctx, dec, NULL ) ) < 0 )
	{
		av_strerror( ret, error, sizeof( error ) );
		common->Warning( "idCinematicFFMPEG: Cannot open video decoder for: '%s', %d, with error: %s\n", qpath, looping, error );
		return false;
	}

	// After the video decoder is open then try to open audio decoder
	ret2 = av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &dec2, 0 );
	if( ret2 >= 0 && s_playCinematicAudio.GetBool() )  //Make audio optional (only intro video has audio no other)
	{
		audio_stream_index = ret2;
		dec_ctx2 = avcodec_alloc_context3( dec2 );
		if( ( ret2 = avcodec_parameters_to_context( dec_ctx2, fmt_ctx->streams[audio_stream_index]->codecpar ) ) < 0 )
		{
			av_strerror( ret2, error, sizeof( error ) );
			common->Warning( "idCinematicFFMPEG: Failed to create audio codec context from codec parameters with error: %s\n", error );
		}

		dec_ctx2->framerate = fmt_ctx->streams[audio_stream_index]->avg_frame_rate;
		dec_ctx2->pkt_timebase = fmt_ctx->streams[audio_stream_index]->time_base;	// SRS - packet timebase for frame3->pts timestamps
		
		if( ( ret2 = avcodec_open2( dec_ctx2, dec2, NULL ) ) < 0 )
		{
			common->Warning( "idCinematicFFMPEG: Cannot open audio decoder for: '%s', %d\n", qpath, looping );
			//return false;
		}

		// SRS - Planar formats start at AV_SAMPLE_FMT_U8P
		if( dec_ctx2->sample_fmt >= AV_SAMPLE_FMT_U8P )
		{
			dst_smp = static_cast<AVSampleFormat>( dec_ctx2->sample_fmt - AV_SAMPLE_FMT_U8P );	// SRS - Setup context to convert from planar to packed
#if	LIBSWRESAMPLE_VERSION_INT >= AV_VERSION_INT(4,7,100)
			if( ( ret2 = swr_alloc_set_opts2( &swr_ctx, &dec_ctx2->ch_layout, dst_smp, dec_ctx2->sample_rate, &dec_ctx2->ch_layout, dec_ctx2->sample_fmt, dec_ctx2->sample_rate, 0, NULL ) ) < 0 )
			{
				av_strerror( ret2, error, sizeof( error ) );
				common->Warning( "idCinematicFFMPEG: Failed to create audio resample context with error: %s\n", error );
			}
#else
			swr_ctx = swr_alloc_set_opts( NULL, dec_ctx2->channel_layout, dst_smp, dec_ctx2->sample_rate, dec_ctx2->channel_layout, dec_ctx2->sample_fmt, dec_ctx2->sample_rate, 0, NULL );
#endif
			ret2 = swr_init( swr_ctx );
			hasplanar = true;
		}
		else
		{
			dst_smp = dec_ctx2->sample_fmt;														// SRS - Must always define the destination format
			hasplanar = false;
		}

		common->DPrintf( "Cinematic audio stream found: Sample Rate=%d Hz, Channels=%d, Format=%s, Planar=%d\n", dec_ctx2->sample_rate,
#if	LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59,37,100)
						 dec_ctx2->ch_layout.nb_channels,
#else
						 dec_ctx2->channels,
#endif
						 GetSampleFormat( dec_ctx2->sample_fmt ), hasplanar );

#if defined(_MSC_VER) && !defined(USE_OPENAL)
		cinematicAudio = new( TAG_AUDIO ) CinematicAudio_XAudio2( false );
#else
		cinematicAudio = new( TAG_AUDIO ) CinematicAudio_OpenAL( false );
#endif
		cinematicAudio->InitAudio( dec_ctx2 );
	}

	CIN_WIDTH = dec_ctx->width;
	CIN_HEIGHT = dec_ctx->height;

	// Calculate Duration in seconds
	// This is the fundamental unit of time (in seconds) in terms
	// of which frame timestamps are represented. For fixed-fps content,
	// timebase should be 1/framerate and timestamp increments should be identically 1.
	// - encoding: MUST be set by user.
	// - decoding: Set by libavcodec.

	// SRS - Must use consistent duration and timebase parameters in the durationSec calculation (don't mix fmt_ctx duration with dec_ctx timebase)
	float durationSec = static_cast<double>( fmt_ctx->streams[video_stream_index]->duration ) * av_q2d( fmt_ctx->streams[video_stream_index]->time_base );
	
	// GK: No duration is given. Check if we get at least bitrate to calculate the length, otherwise set it to a fixed 100 seconds (should it be lower ?)
	if( durationSec < 0 )
	{
		// SRS - First check the file context bit rate and estimate duration using file size and overall bit rate
		if( fmt_ctx->bit_rate > 0 )
		{
			durationSec = file_size * 8.0 / fmt_ctx->bit_rate;
		}
		// SRS - Likely an RoQ file, so use the video bit rate tolerance plus audio bit rate to estimate duration, then add 10% to correct for variable bit rate
		else if( dec_ctx->bit_rate_tolerance > 0 )
		{
			durationSec = file_size * 8.0 / ( dec_ctx->bit_rate_tolerance + ( dec_ctx2 ? dec_ctx2->bit_rate : 0 ) ) * 1.1;
		}
		// SRS - Otherwise just set a large max duration
		else
		{
			durationSec = 100.0;
		}
	}
	animationLength = durationSec * 1000;
	frameRate = av_q2d( fmt_ctx->streams[video_stream_index]->avg_frame_rate );
	common->Printf( S_COLOR_GRAY "  ...Movie file: " S_COLOR_WHITE "'%s'\n", qpath );

	// SRS - Get image buffer size (dimensions mod 32 for bik & webm codecs, subsumes mod 16 for mp4 codec), then allocate image and fill with correct parameters
	int bufWidth = ( CIN_WIDTH + 31 ) & ~31;
	int bufHeight = ( CIN_HEIGHT + 31 ) & ~31;
	int img_bytes = av_image_get_buffer_size( AV_PIX_FMT_BGR32, bufWidth, bufHeight, 1 );
	image = ( byte* )Mem_Alloc( img_bytes, TAG_CINEMATIC );
	av_image_fill_arrays( frame2->data, frame2->linesize, image, AV_PIX_FMT_BGR32, CIN_WIDTH, CIN_HEIGHT, 1 ); // GK: Straight out of the FFMPEG source code
	img_convert_ctx = sws_getContext( dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt, CIN_WIDTH, CIN_HEIGHT, AV_PIX_FMT_BGR32, SWS_BICUBIC, NULL, NULL, NULL );

	status = FMV_PLAY;
	hasFrame = false;
	framePos = -1;
	ImageForTime( 0 );
	status = ( looping ) ? FMV_PLAY : FMV_IDLE;

	return true;
}

/*
==============
idCinematicFFMPEG::FFMPEGReset
==============
*/
void idCinematicFFMPEG::FFMPEGReset()
{
	// RB: don't reset startTime here because that breaks video replays in the PDAs
	//startTime = 0;

	framePos = -1;

	// SRS - If we have cinematic audio, reset audio to release any stale buffers and avoid AV drift if looping
	if( cinematicAudio )
	{
		cinematicAudio->ResetAudio();

		while( !lagBuffer.empty() )
		{
			av_freep( &lagBuffer.front() );
			lagBuffer.pop();
			lagBufSize.pop();
		}
	}

	// SRS - For non-RoQ (i.e. bik, mp4, webm, etc) files, use standard frame seek to rewind the stream
	if( dec_ctx->codec_id != AV_CODEC_ID_ROQ && av_seek_frame( fmt_ctx, video_stream_index, 0, 0 ) >= 0 )
	{
		status = FMV_LOOPED;
	}
	// SRS - Special handling for RoQ files: only byte seek works and ffmpeg RoQ decoder needs reset
	else if( dec_ctx->codec_id == AV_CODEC_ID_ROQ && av_seek_frame( fmt_ctx, video_stream_index, 0, AVSEEK_FLAG_BYTE ) >= 0 )
	{
#if LIBAVCODEC_VERSION_MAJOR <= 61
		// Close and reopen the ffmpeg RoQ codec without clearing the context - this seems to reset the decoder properly
		// Note avcodec_close( dec_ctx ) is deprecated and removed as of ffmpeg version 8 or LIBAVCODEC_VERSION_MAJOR 62
		avcodec_close( dec_ctx );
		avcodec_open2( dec_ctx, dec, NULL );
#endif

		status = FMV_LOOPED;
	}
	// SRS - Can't rewind the stream so we really are at EOF
	else
	{
		status = FMV_EOF;
	}
}
#endif
/*
==============
idCinematicFFMPEG::InitFromFile
==============
*/
bool idCinematicFFMPEG::InitFromFile( const char* qpath, bool amilooping )
{
	animationLength = 0;
#if defined(USE_FFMPEG)
	return InitFromFFMPEGFile( qpath, amilooping );
#else
	return false;
#endif
}

/*
==============
idCinematicFFMPEG::ExportToTGA
==============
*/
void idCinematicFFMPEG::ExportToTGA( bool skipExisting )
{
}

/*
==============
idCinematicFFMPEG::GetFrameRate
==============
*/
float idCinematicFFMPEG::GetFrameRate() const
{
	return frameRate;
}


/*
==============
idCinematicFFMPEG::Close
==============
*/
void idCinematicFFMPEG::Close()
{
	if( image )
	{
		Mem_Free( ( void* )image );
		image = NULL;
		status = FMV_EOF;
	}
#if defined(USE_FFMPEG)
	if( img_convert_ctx )
	{
		sws_freeContext( img_convert_ctx );
		img_convert_ctx = NULL;
	}

	// SRS - If we have cinematic audio, free audio codec context, resample context, and any lagged audio buffers
	if( cinematicAudio )
	{
		if( dec_ctx2 )
		{
			avcodec_free_context( &dec_ctx2 );
		}

		// SRS - Free resample context if we were decoding planar audio
		if( swr_ctx )
		{
			swr_free( &swr_ctx );
		}

		while( !lagBuffer.empty() )
		{
			av_freep( &lagBuffer.front() );
			lagBuffer.pop();
			lagBufSize.pop();
		}
	}

	if( dec_ctx )
	{
		avcodec_free_context( &dec_ctx );
	}

	if( fmt_ctx )
	{
		avformat_close_input( &fmt_ctx );
	}
#endif
	status = FMV_EOF;
}

/*
==============
idCinematicFFMPEG::AnimationLength
==============
*/
int idCinematicFFMPEG::AnimationLength()
{
	return animationLength;
}

/*
==============
idCinematicFFMPEG::IsPlaying
==============
*/
bool idCinematicFFMPEG::IsPlaying() const
{
	return ( status == FMV_PLAY );
}

/*
==============
 idCinematicFFMPEG::GetStartTime
==============
*/
int idCinematicFFMPEG::GetStartTime()
{
	return startTime;
}

/*
==============
idCinematicFFMPEG::ResetTime
==============
*/
void idCinematicFFMPEG::ResetTime( int time )
{
	startTime = time;
	status = FMV_PLAY;
}

/*
==============
idCinematicFFMPEG::ImageForTime
==============
*/
cinData_t idCinematicFFMPEG::ImageForTime( int thisTime )
{
#if defined(USE_FFMPEG)
	return ImageForTimeFFMPEG( thisTime );
#else
	cinData_t c;
	memset( &c, 0, sizeof( c ) );
	return c;
#endif
}

#if defined(USE_FFMPEG)
/*
==============
idCinematicFFMPEG::ImageForTimeFFMPEG
==============
*/
cinData_t idCinematicFFMPEG::ImageForTimeFFMPEG( int thisTime )
{
	cinData_t	cinData;
	char		error[64];
	uint8_t*	audioBuffer = NULL;
	int			num_bytes = 0;
	bool		syncLost = false;

	if( thisTime <= 0 )
	{
		thisTime = Sys_Milliseconds();
	}

	memset( &cinData, 0, sizeof( cinData ) );
	if( r_skipVideo.GetBool() || status == FMV_EOF || status == FMV_IDLE )
	{
		return cinData;
	}

	if( !fmt_ctx )
	{
		// .bik requested but not found
		return cinData;
	}

	if( ( !hasFrame ) || startTime == -1 )
	{
		if( startTime == -1 )
		{
			FFMPEGReset();
		}
		startTime = thisTime;
	}

	long desiredFrame = ( ( thisTime - startTime ) * frameRate ) / 1000;
	if( desiredFrame < 0 )
	{
		desiredFrame = 0;
	}

	if( desiredFrame < framePos )
	{
		FFMPEGReset();
		hasFrame = false;
		status = FMV_PLAY;
	}

	if( hasFrame && desiredFrame == framePos )
	{
		cinData.imageWidth = CIN_WIDTH;
		cinData.imageHeight = CIN_HEIGHT;
		cinData.status = status;
		cinData.image = img;
		return cinData;
	}

	AVPacket packet;
	while( framePos < desiredFrame )
	{
		int frameFinished = -1;
		int res = 0;

		// Do a single frame by getting packets until we have a full frame
		while( frameFinished != 0 )
		{
			// if we got to the end or failed
			if( av_read_frame( fmt_ctx, &packet ) < 0 )
			{
				// can't read any more, set to EOF
				status = FMV_EOF;
				if( looping )
				{
					desiredFrame = 0;
					FFMPEGReset();
					hasFrame = false;
					startTime = thisTime;
					if( av_read_frame( fmt_ctx, &packet ) < 0 )
					{
						status = FMV_IDLE;
						return cinData;
					}
					status = FMV_PLAY;
				}
				else
				{
					hasFrame = false;
					status = FMV_IDLE;
					return cinData;
				}
			}
			// Is this a packet from the video stream?
			if( packet.stream_index == video_stream_index )
			{
				// Decode video frame
				if( ( res = avcodec_send_packet( dec_ctx, &packet ) ) != 0 )
				{
					av_strerror( res, error, sizeof( error ) );
					common->Warning( "idCinematicFFMEPG: Failed to send video packet for decoding with error: %s\n", error );
				}
				else
				{
					frameFinished = avcodec_receive_frame( dec_ctx, frame );
					if( frameFinished != 0 && frameFinished != AVERROR( EAGAIN ) )
					{
						av_strerror( frameFinished, error, sizeof( error ) );
						common->Warning( "idCinematicFFMEPG: Failed to receive video frame from decoding with error: %s\n", error );
					}
				}
			}
			else if( cinematicAudio && packet.stream_index == audio_stream_index ) //Check if it found any audio data
			{
				res = avcodec_send_packet( dec_ctx2, &packet );
				if( res != 0 && res != AVERROR( EAGAIN ) )
				{
					av_strerror( res, error, sizeof( error ) );
					common->Warning( "idCinematicFFMEPG: Failed to send audio packet for decoding with error: %s\n", error );
				}

				//SRS - Separate frame finisher for audio since there can be multiple audio frames per video frame (e.g. at bik startup)
				int frameFinished1 = 0;
				while( frameFinished1 == 0 )
				{
					if( ( frameFinished1 = avcodec_receive_frame( dec_ctx2, frame3 ) ) != 0 )
					{
						if( frameFinished1 != AVERROR( EAGAIN ) )
						{
							av_strerror( frameFinished1, error, sizeof( error ) );
							common->Warning( "idCinematicFFMEPG: Failed to receive audio frame from decoding with error: %s\n", error );
						}
					}
					// SRS - Allocate audio buffer, convert to packed format, save in queue, and play synced audio for desired frame
					else
					{
						// SRS - Since destination sample format is packed (non-planar), returned bufflinesize equals num_bytes
#if	LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59,37,100)
						res = av_samples_alloc( &audioBuffer, &num_bytes, frame3->ch_layout.nb_channels, frame3->nb_samples, dst_smp, 0 );
#else
						res = av_samples_alloc( &audioBuffer, &num_bytes, frame3->channels, frame3->nb_samples, dst_smp, 0 );
#endif
						if( res < 0 || res != num_bytes )
						{
							common->Warning( "idCinematicFFMEPG: Failed to allocate audio buffer with result: %d\n", res );
						}
						if( hasplanar )
						{
							// SRS - Convert from planar to packed format keeping sample count the same
							res = swr_convert( swr_ctx, &audioBuffer, frame3->nb_samples, ( const uint8_t** )frame3->extended_data, frame3->nb_samples );
							if( res < 0 || res != frame3->nb_samples )
							{
								common->Warning( "idCinematicFFMEPG: Failed to convert planar audio data to packed format with result: %d\n", res );
							}
						}
						else
						{
							// SRS - Since audio is already in packed format, just copy into audio buffer
							if( num_bytes > 0 )
							{
								memcpy( audioBuffer, frame3->extended_data[0], num_bytes );
							}
						}

						// SRS - If we have cinematic audio data, save the current frame onto the back of the queue
						if( num_bytes > 0 )
						{
							// SRS - If queue is at max size we have lost a/v sync: drop frame and set syncLost flag
							if( lagBuffer.size() == ( skipLag ? 1 : NUM_LAG_FRAMES ) )
							{
								av_freep( &lagBuffer.front() );
								lagBuffer.pop();
								lagBufSize.pop();

								syncLost = true;
							}

							// SRS - Save the current (new) audio buffer and its size to play during the desired frame
							lagBuffer.push( audioBuffer );
							lagBufSize.push( num_bytes );
						}
						// SRS - Not sure if an audioBuffer can ever be allocated on failure, but check and free just in case
						else if( audioBuffer )
						{
							av_freep( &audioBuffer );
						}

						// SRS - If we have any synced audio frames available for the desired frame, play now and drain queue
						if( framePos + 1 == desiredFrame )
						{
							if( syncLost )
							{
								// SRS - If we have lost sync, reset / resync audio stream before starting to play again
								cinematicAudio->ResetAudio();
								syncLost = false;
							}

							while( !lagBuffer.empty() )
							{
								// SRS - Note that PlayAudio() is responsible for releasing any audio buffers sent to it
								if( !s_noSound.GetBool() )
								{
									cinematicAudio->PlayAudio( lagBuffer.front(), lagBufSize.front() );
								}
								else
								{
									av_freep( &lagBuffer.front() );
								}

								lagBuffer.pop();
								lagBufSize.pop();
							}
						}

						//common->Printf( "idCinematicFFMEPG: video pts = %7.3f, audio pts = %7.3f, samples = %4d, num_bytes = %5d\n", static_cast<double>( frame->pts ) * av_q2d( dec_ctx->pkt_timebase ), static_cast<double>( frame3->pts ) * av_q2d( dec_ctx2->pkt_timebase ), frame3->nb_samples, num_bytes );
					}
				}
			}

			// Free the packet that was allocated by av_read_frame
			av_packet_unref( &packet );
		}

		framePos++;
	}

	// We have reached the desired frame
	// Convert the image from its native format to RGB
	sws_scale( img_convert_ctx, frame->data, frame->linesize, 0, dec_ctx->height, frame2->data, frame2->linesize );
	cinData.imageWidth = CIN_WIDTH;
	cinData.imageHeight = CIN_HEIGHT;
	cinData.status = status;
	img->UploadScratch( image, CIN_WIDTH, CIN_HEIGHT );
	hasFrame = true;
	cinData.image = img;

	return cinData;
}
#endif