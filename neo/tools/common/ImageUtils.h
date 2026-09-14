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

#ifndef __IMAGEUTILS__
#define __IMAGEUTILS__

#include <wx/wx.h>
#include <wx/image.h>

// Base uppon this: https://wiki.wxwidgets.org/Embedding_PNG_Images
namespace ImageUtils
{

/**
 * Build a wxImage from a raw pixel buffer.
 *
 * Supported formats:
 *   bytesPerPixel == 3  ->  RGB
 *   bytesPerPixel == 4  ->  RGBA
 *   bytesPerPixel == 2  ->  RGB565 (packed, native endian)
 *
 * Returns an invalid wxImage on failure.  The pixel buffer is copied; the
 * caller retains ownership of the input.
 */
wxImage BitmapFromRaw( const unsigned char* pixelData, int width, int height, int bytesPerPixel );

/**
 * Convenience wrapper: raw pixels -> wxBitmap.
 */
wxBitmap BitmapFromRawToBitmap( const unsigned char* pixelData, int width, int height, int bytesPerPixel );

/**
 * Convenience wrapper: raw pixels => wxIcon.
 *
 * The icon owns its own copy of the image data, so the input buffer can be
 * released immediately after this call.
 */
wxIcon IconFromRaw( const unsigned char* pixelData, int width, int height, int bytesPerPixel );

/**
 * Build a wxIcon from a GIMP-generated embedded image struct.
 *
 * Use like:
 *
 *     #include "doom_ico.h"
 *     wxIcon icon = wx_image_util::IconFromEmbedded(
 *         doom_icon.pixel_data,
 *         doom_icon.width,
 *         doom_icon.height,
 *         doom_icon.bytes_per_pixel );
 *
 * which is a thin wrapper around IconFromRaw().
 */
wxIcon IconFromEmbedded( const unsigned char* pixelData, unsigned int width, unsigned int height, unsigned int bytesPerPixel );

} // namespace ImageUtils

#endif /* !__IMAGEUTILS__ */