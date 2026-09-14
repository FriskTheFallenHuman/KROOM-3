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

#include "ImageUtils.h"

namespace ImageUtils
{

wxImage BitmapFromRaw( const unsigned char* pixelData, int width, int height, int bytesPerPixel )
{
	if( !pixelData || width <= 0 || height <= 0 )
	{
		return wxImage();
	}

	if( bytesPerPixel != 2 && bytesPerPixel != 3 && bytesPerPixel != 4 )
	{
		return wxImage();
	}

	const size_t pixelCount = ( size_t )width * ( size_t )height;

	wxImage image( width, height, false );

	unsigned char* rgb = image.GetData();
	unsigned char* alpha = NULL;

	if( bytesPerPixel == 4 )
	{
		image.InitAlpha();
		alpha = image.GetAlpha();
	}

	for( size_t i = 0; i < pixelCount; i++ )
	{
		const unsigned char* src = pixelData + i * bytesPerPixel;

		if( bytesPerPixel == 4 )
		{
			rgb[i * 3 + 0] = src[0];
			rgb[i * 3 + 1] = src[1];
			rgb[i * 3 + 2] = src[2];
			if( alpha )
			{
				alpha[i] = src[3];
			}
		}
		else if( bytesPerPixel == 3 )
		{
			rgb[i * 3 + 0] = src[0];
			rgb[i * 3 + 1] = src[1];
			rgb[i * 3 + 2] = src[2];
		}
		else // bytesPerPixel == 2, RGB565
		{
			const unsigned short p = *( const unsigned short* )src;
			const unsigned char r = ( unsigned char )( ( ( p >> 11 ) & 0x1F ) * 255 / 31 );
			const unsigned char g = ( unsigned char )( ( ( p >>  5 ) & 0x3F ) * 255 / 63 );
			const unsigned char b = ( unsigned char )( ( p        & 0x1F ) * 255 / 31 );
			rgb[i * 3 + 0] = r;
			rgb[i * 3 + 1] = g;
			rgb[i * 3 + 2] = b;
		}
	}

	return image;
}

wxBitmap BitmapFromRawToBitmap( const unsigned char* pixelData, int width, int height, int bytesPerPixel )
{
	const wxImage image = BitmapFromRaw( pixelData, width, height, bytesPerPixel );
	if( !image.IsOk() )
	{
		return wxBitmap();
	}

	// 32-bit depth preserves the alpha channel.
	return wxBitmap( image, 32 );
}

wxIcon IconFromRaw( const unsigned char* pixelData, int width, int height, int bytesPerPixel )
{
	const wxBitmap bmp = BitmapFromRawToBitmap( pixelData, width, height, bytesPerPixel );
	if( !bmp.IsOk() )
	{
		return wxIcon();
	}

	wxIcon icon;
	icon.CopyFromBitmap( bmp );
	return icon;
}

wxIcon IconFromEmbedded( const unsigned char* pixelData, unsigned int width, unsigned int height, unsigned int bytesPerPixel )
{
	return IconFromRaw( pixelData, ( int )width, ( int )height, ( int )bytesPerPixel );
}

} // namespace ImageUtils