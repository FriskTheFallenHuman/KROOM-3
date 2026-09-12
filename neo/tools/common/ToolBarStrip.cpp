/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 1999-2011 Raven Software
Copyright (C) 2021 Harrie van Ginneken

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

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/mstream.h>

#include "ToolBarStrip.h"

rvToolbarImageStrip::rvToolbarImageStrip()
	: m_iconWidth( 16 )
	, m_iconHeight( 16 )
{
}

rvToolbarImageStrip::~rvToolbarImageStrip()
{
}

bool rvToolbarImageStrip::Load( const char* relativePath, int iconWidth, int iconHeight, const wxColour& transparentKey )
{
	if( !relativePath || !*relativePath )
	{
		return false;
	}
	if( iconWidth  <= 0 )
	{
		return false;
	}
	if( iconHeight <= 0 )
	{
		return false;
	}

	void* buffer = NULL;
	int len = fileSystem->ReadFile( relativePath, &buffer, NULL );
	if( len <= 0 || buffer == NULL )
	{
		return false;
	}

	bool ok = LoadFromMemory( buffer, len, iconWidth, iconHeight, transparentKey );
	fileSystem->FreeFile( buffer );
	return ok;
}

bool rvToolbarImageStrip::LoadFromMemory( const void* data, int size, int iconWidth, int iconHeight, const wxColour& transparentKey )
{
	if( !data || size <= 0 )
	{
		return false;
	}
	if( iconWidth  <= 0 )
	{
		return false;
	}
	if( iconHeight <= 0 )
	{
		return false;
	}

	// Decode the image
	wxMemoryInputStream stream( data, size );
	wxImage strip;

	if( !strip.LoadFile( stream, wxBITMAP_TYPE_ANY ) )
	{
		if( !DecodeBMP( data, size, strip ) )
		{
			return false;
		}
	}

	if( !strip.IsOk() || strip.GetWidth() <= 0 || strip.GetHeight() <= 0 )
	{
		return false;
	}

	// Determine which color becomes transparent.
	if( !strip.HasAlpha() )
	{
		wxColour key = transparentKey;
		if( !key.IsOk() )
		{
			key = wxColour( strip.GetRed( 0, 0 ), strip.GetGreen( 0, 0 ), strip.GetBlue( 0, 0 ) );
		}

		strip.InitAlpha();

		const int w = strip.GetWidth();
		const int h = strip.GetHeight();
		const unsigned char kr = key.Red();
		const unsigned char kg = key.Green();
		const unsigned char kb = key.Blue();

		for( int y = 0; y < h; ++y )
		{
			for( int x = 0; x < w; ++x )
			{
				if( strip.GetRed( x, y ) == kr && strip.GetGreen( x, y ) == kg && strip.GetBlue( x, y ) == kb )
				{
					strip.SetAlpha( x, y, 0 );
				}
			}
		}
	}

	if( strip.GetHeight() < iconHeight || ( strip.GetHeight() % iconHeight ) != 0 )
	{
		const int W = strip.GetWidth();
		const int H = ( ( strip.GetHeight() + iconHeight - 1 ) / iconHeight ) * iconHeight;

		wxImage padded( W, H, false );
		padded.InitAlpha();

		for( int y = 0; y < strip.GetHeight(); ++y )
		{
			for( int x = 0; x < W; ++x )
			{
				padded.SetRGB( x, y, strip.GetRed( x, y ), strip.GetGreen( x, y ), strip.GetBlue( x, y ) );
				padded.SetAlpha( x, y, strip.HasAlpha() ? strip.GetAlpha( x, y ) : 255 );
			}
		}

		// if empty, transparent.
		for( int y = strip.GetHeight(); y < H; ++y )
		{
			for( int x = 0; x < W; ++x )
			{
				padded.SetRGB( x, y, 0, 0, 0 );
				padded.SetAlpha( x, y, 0 );
			}
		}

		strip = padded;
	}

	const int w = strip.GetWidth();
	const int h = strip.GetHeight();

	const int cols = w / iconWidth;
	const int rows = h / iconHeight;
	if( cols < 1 || rows < 1 )
	{
		return false;
	}

	m_strip      = strip;
	m_iconWidth  = iconWidth;
	m_iconHeight = iconHeight;
	SliceIntoIcons();
	return !m_icons.empty();
}

bool rvToolbarImageStrip::DecodeBMP( const void* data, int size, wxImage& out )
{
	if( !data || size < 54 )
	{
		return false;
	}

	const unsigned char* p = ( const unsigned char* )data;

	if( p[0] != 'B' || p[1] != 'M' )
	{
		return false;
	}

	unsigned int pixelOffset = p[10] | ( p[11] << 8 ) | ( p[12] << 16 ) | ( p[13] << 24 );
	unsigned int headerSize  = p[14] | ( p[15] << 8 ) | ( p[16] << 16 ) | ( p[17] << 24 );
	int width                = p[18] | ( p[19] << 8 ) | ( p[20] << 16 ) | ( p[21] << 24 );
	int height               = p[22] | ( p[23] << 8 ) | ( p[24] << 16 ) | ( p[25] << 24 );
	unsigned short bpp       = p[28] | ( p[29] << 8 );
	unsigned int compression = p[30] | ( p[31] << 8 ) | ( p[32] << 16 ) | ( p[33] << 24 );

	if( headerSize != 40 )
	{
		return false;
	}
	if( compression != 0 )
	{
		return false;    // BI_RGB only
	}
	if( bpp != 24 && bpp != 32 )
	{
		return false;
	}

	const bool topDown = ( height < 0 );
	const int W = width;
	const int H = topDown ? -height : height;
	if( W <= 0 || H <= 0 )
	{
		return false;
	}

	const int bytesPerPixel = bpp / 8;
	const int srcStride     = ( ( W * bytesPerPixel ) + 3 ) & ~3;
	const unsigned int needed = pixelOffset + ( unsigned int )( srcStride * H );
	if( needed > ( unsigned int )size )
	{
		return false;
	}

	out.Create( W, H, false );
	out.InitAlpha();

	const unsigned char* src = p + pixelOffset;
	for( int y = 0; y < H; ++y )
	{
		const int srcY = topDown ? y : ( H - 1 - y );
		const unsigned char* row = src + srcY * srcStride;
		for( int x = 0; x < W; ++x )
		{
			const unsigned char b = row[x * bytesPerPixel + 0];
			const unsigned char g = row[x * bytesPerPixel + 1];
			const unsigned char r = row[x * bytesPerPixel + 2];
			out.SetRGB( x, y, r, g, b );
			out.SetAlpha( x, y, 255 );
		}
	}

	return true;
}

void rvToolbarImageStrip::SliceIntoIcons()
{
	m_icons.clear();

	const int cols = m_strip.GetWidth()  / m_iconWidth;
	const int rows = m_strip.GetHeight() / m_iconHeight;

	m_icons.reserve( cols * rows );

	for( int r = 0; r < rows; ++r )
	{
		for( int c = 0; c < cols; ++c )
		{
			wxRect cell( c * m_iconWidth, r * m_iconHeight,
						 m_iconWidth, m_iconHeight );
			wxImage icon = m_strip.GetSubImage( cell );
			m_icons.push_back( wxBitmap( icon, 32 ) );
		}
	}
}

const wxBitmap& rvToolbarImageStrip::GetIcon( int index ) const
{
	static wxBitmap s_empty;
	if( index < 0 || index >= ( int )m_icons.size() )
	{
		return s_empty;
	}
	return m_icons[ index ];
}

wxImageList* rvToolbarImageStrip::CreateImageList() const
{
	if( m_icons.empty() )
	{
		return NULL;
	}

	wxImageList* list = new wxImageList( m_iconWidth, m_iconHeight, true, ( int )m_icons.size() );
	for( size_t i = 0; i < m_icons.size(); ++i )
	{
		list->Add( m_icons[ i ] );
	}
	return list;
}