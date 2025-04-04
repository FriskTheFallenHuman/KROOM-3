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
#ifndef __ROQ_H__
#define __ROQ_H__

//#define JPEG_INTERNALS
//#include <jpeglib.h> // DG: unused

#include "gdefs.h"
#include "roqParam.h"
#include "quaddefs.h"

class codec;
class roqParam;

class NSBitmapImageRep
{
public:

	NSBitmapImageRep();
	NSBitmapImageRep( const char* filename );
	NSBitmapImageRep( int wide, int high );
	~NSBitmapImageRep();

	NSBitmapImageRep& 	operator=( const NSBitmapImageRep& a );

	int					samplesPerPixel();
	int					pixelsWide();
	int					pixelsHigh();
	byte* 				bitmapData();
	bool				hasAlpha();
	bool				isPlanar();

private:

	byte* 				bmap;
	int					width;
	int					height;
	ID_TIME_T				timestamp;

};

class roq
{
public:
	roq();
	~roq();

	//void				WriteLossless();
	void				LoadAndDisplayImage( const char* filename );
	void				CloseRoQFile( bool which );
	void				InitRoQFile( const char* roqFilename );
	void				InitRoQPatterns();
	void				EncodeStream( const char* paramInputFile );
	void				EncodeQuietly( bool which );
	bool				IsQuiet();
	bool				IsLastFrame();
	NSBitmapImageRep* 	CurrentImage();
	void				MarkQuadx( int xat, int yat, int size, float cerror, int choice );
	void				WritePuzzleFrame( quadcel* pquad );
	void				WriteFrame( quadcel* pquad );
	void				WriteCodeBook( byte* codebook );
	void				WwriteCodeBookToStream( byte* codes, int csize, word cflags );
	int					PreviousFrameSize();
	bool				MakingVideo();
	bool				ParamNoAlpha();
	bool				SearchType();
	bool				HasSound();
	const char* 		CurrentFilename();
	int					NormalFrameSize();
	int					FirstFrameSize();
	bool				Scaleable();
	void				WriteHangFrame();
	int					NumberOfFrames();
private:
	void				Write16Word( word* aWord, idFile* stream );
	void				Write32Word( unsigned int* aWord, idFile* stream );
	int					SizeFile( idFile* ftosize );
	void				CloseRoQFile();
	void				WriteCodeBookToStream( byte* codebook, int csize, word cflags );

#if 0
	static	void		JPEGInitDestination( j_compress_ptr cinfo );
	static	boolean		JPEGEmptyOutputBuffer( j_compress_ptr cinfo );
	static	void		JPEGTermDestination( j_compress_ptr cinfo );

	void				JPEGStartCompress( j_compress_ptr cinfo, bool write_all_tables );
	JDIMENSION			JPEGWriteScanlines( j_compress_ptr cinfo, JSAMPARRAY scanlines, JDIMENSION num_lines );
	void				JPEGDest( j_compress_ptr cinfo, byte* outfile, int size );
	void				JPEGSave( char* filename, int quality, int image_width, int image_height, unsigned char* image_buffer );
#endif

	codec* 				encoder;
	roqParam* 			paramFile;

	idFile* 			RoQFile;
	NSBitmapImageRep* 	image;
	int					numQuadCels;
	bool				quietMode;
	bool				lastFrame;
	idStr				roqOutfile;
	idStr				currentFile;
	int					numberOfFrames;
	int					previousSize;
	byte				codes[4096];
	bool				dataStuff;

};

extern roq* theRoQ;				// current roq

#endif /* !__ROQ_H__ */
