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
#include <wx/toplevel.h>
#include <wx/listctrl.h>

#include "RegistryOptions.h"

/*
================
rvRegistryOptions::rvRegistryOptions
================
*/
rvRegistryOptions::rvRegistryOptions()
{
}

/*
================
rvRegistryOptions::Init
================
*/
void rvRegistryOptions::Init( const char* fileName, const char* keyPrefix )
{
	mFileName = fileName;
	mBaseKey = keyPrefix;
}

/*
================
rvRegistryOptions::Save

Save all options into the cfg file
================
*/
bool rvRegistryOptions::Save()
{
	idFile* file = fileSystem->OpenFileWrite( mFileName.c_str(), "fs_savepath" );
	if( !file )
	{
		return false;
	}

	file->Printf( "// Editor Configuration\n\n" );

	// Write the keyvalues
	for( int i = 0; i < mValues.GetNumKeyVals(); i++ )
	{
		const idKeyValue* keyVal = mValues.GetKeyVal( i );
		if( keyVal )
		{
			file->Printf( "\"%s\"\t\"%s\"\n", keyVal->GetKey().c_str(), keyVal->GetValue().c_str() );
		}
	}

	// Write MRUs
	for( int i = 0; i < mRecentFiles.Num(); i++ )
	{
		file->Printf( "\"MRU.%d\"\t\"%s\"\n", i, mRecentFiles[i].c_str() );
	}

	fileSystem->CloseFile( file );
	return true;
}

/*
================
rvRegistryOptions::Load

Parse the unified cfg file
================
*/
bool rvRegistryOptions::Load()
{
	mValues.Clear();
	mRecentFiles.Clear();

	idFile* file = fileSystem->OpenFileRead( mFileName.c_str(), "fs_savepath" );
	if( !file )
	{
		return false;
	}

	int len = file->Length();
	if( len <= 0 )
	{
		fileSystem->CloseFile( file );
		return false;
	}

	char* buffer = new char[len + 1];
	file->Read( buffer, len );
	buffer[len] = '\0';
	fileSystem->CloseFile( file );

	idParser parser( LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT | LEXFL_NOFATALERRORS );
	parser.LoadMemory( buffer, len, mFileName.c_str() );

	idToken keyToken, valToken;
	while( parser.ReadToken( &keyToken ) )
	{
		if( !parser.ReadToken( &valToken ) )
		{
			break;
		}

		if( !keyToken.IcmpPrefix( "MRU." ) )
		{
			AddRecentFile( valToken.c_str() );
		}
		else
		{
			mValues.Set( keyToken.c_str(), valToken.c_str() );
		}
	}

	delete[] buffer;

	return true;
}

/*
================
rvRegistryOptions::SetWindowPlacement
================
*/
void rvRegistryOptions::SetWindowPlacement( const char* name, wxWindow* win )
{
	if( !win )
	{
		return;
	}

	wxPoint pos = win->GetPosition();
	wxSize size = win->GetSize();

	wxTopLevelWindow* tlw = dynamic_cast<wxTopLevelWindow*>( win );
	bool isMaximized = tlw ? tlw->IsMaximized() : false;

	mValues.Set( MakeKeyName( name ), va( "%d %d %d %d %d", pos.x, pos.y, size.x, size.y, isMaximized ? 1 : 0 ) );
}

/*
================
rvRegistryOptions::GetWindowPlacement
================
*/
bool rvRegistryOptions::GetWindowPlacement( const char* name, wxWindow* win )
{
	if( !win )
	{
		return false;
	}

	idStr keyName = MakeKeyName( name );
	const idKeyValue* key = mValues.FindKey( keyName );
	if( !key )
	{
		return false;
	}

	int x, y, w, h, maxed = 0;
	if( sscanf( key->GetValue().c_str(), "%d %d %d %d %d", &x, &y, &w, &h, &maxed ) == 5 )
	{
		win->SetSize( x, y, w, h );
		if( maxed )
		{
			wxTopLevelWindow* tlw = dynamic_cast<wxTopLevelWindow*>( win );
			if( tlw )
			{
				tlw->Maximize( true );
			}
		}
		return true;
	}

	return false;
}

/*
================
rvRegistryOptions::SetColumnWidths
================
*/
void rvRegistryOptions::SetColumnWidths( const char* name, wxListCtrl* list )
{
	if( !list )
	{
		return;
	}

	idStr widths;
	int colCount = list->GetColumnCount();
	for( int i = 0; i < colCount; i++ )
	{
		widths += va( "%d ", list->GetColumnWidth( i ) );
	}

	mValues.Set( MakeKeyName( name ), widths.c_str() );
}

/*
================
rvRegistryOptions::GetColumnWidths
================
*/
bool rvRegistryOptions::GetColumnWidths( const char* name, wxListCtrl* list )
{
	if( !list )
	{
		return false;
	}

	idStr keyName = MakeKeyName( name );
	idStr widths = mValues.GetString( keyName );
	const char* parse = widths.c_str();
	const char* next;
	int index = 0;

	while( ( next = strchr( parse, ' ' ) ) != NULL )
	{
		int width = 0;
		sscanf( parse, "%d", &width );
		parse = next + 1;
		list->SetColumnWidth( index++, width );
	}

	return index > 0;
}

/*
================
rvRegistryOptions::AddRecentFile
================
*/
void rvRegistryOptions::AddRecentFile( const char* filename )
{
	idStr path = filename;

	for( int i = mRecentFiles.Num() - 1; i >= 0; i-- )
	{
		if( !mRecentFiles[i].Icmp( filename ) )
		{
			mRecentFiles.RemoveIndex( i );
			break;
		}
	}

	while( mRecentFiles.Num() >= MAX_MRU_SIZE )
	{
		mRecentFiles.RemoveIndex( 0 );
	}

	mRecentFiles.Append( path );
}

/*
================
rvRegistryOptions::SetBinary
================
*/
void rvRegistryOptions::SetBinary( const char* name, const unsigned char* data, int size )
{
	idStr binary;
	for( size--; size >= 0; size--, data++ )
	{
		binary += va( "%02x", *data );
	}
	mValues.Set( MakeKeyName( name ), binary );
}


/*
================
rvRegistryOptions::GetBinary
================
*/
void rvRegistryOptions::GetBinary( const char* name, unsigned char* data, int size )
{
	const char* parse = mValues.GetString( MakeKeyName( name ) );
	for( size--; size >= 0 && *parse && *( parse + 1 ); size--, parse += 2, data++ )
	{
		int value;
		sscanf( parse, "%02x", &value );
		*data = ( unsigned char )value;
	}
}