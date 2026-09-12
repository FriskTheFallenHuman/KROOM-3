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

#ifndef __REGISTRYOPTIONS_H__
#define __REGISTRYOPTIONS_H__

class wxWindow;
class wxListCtrl;

class rvRegistryOptions
{
public:
	static const int MAX_MRU_SIZE = 4;

	rvRegistryOptions();

	// Sets namespace prefix
	void			Init( const char* fileName, const char* keyPrefix );

	// Save all options
	bool			Save();

	// Load options
	bool			Load();

	// Window placement
	void			SetWindowPlacement( const char* name, wxWindow* win );
	bool			GetWindowPlacement( const char* name, wxWindow* win );

	// List view column sizes
	void			SetColumnWidths( const char* name, wxListCtrl* list );
	bool			GetColumnWidths( const char* name, wxListCtrl* list );

	// Setters
	void			SetFloat( const char* name, float v );
	void			SetLong( const char* name, long v );
	void			SetBool( const char* name, bool v );
	void			SetString( const char* name, const char* v );
	void			SetVec4( const char* name, const idVec4& v );
	void			SetBinary( const char* name, const unsigned char* data, int size );

	// Getters
	float			GetFloat( const char* name, float defaultVal = 0.0f );
	long			GetLong( const char* name, long defaultVal = 0 );
	bool			GetBool( const char* name, bool defaultVal = false );
	const char*		GetString( const char* name, const char* defaultVal = "" );
	idVec4			GetVec4( const char* name, const idVec4& defaultVal = vec4_zero );
	void			GetBinary( const char* name, unsigned char* data, int size );

	// MRU methods
	void			AddRecentFile( const char* filename );
	const char*		GetRecentFile( int index );
	int				GetRecentFileCount();

private:
	idStr			MakeKeyName( const char* name ) const;

	idList<idStr>	mRecentFiles;
	idDict			mValues;
	idStr			mBaseKey;
	idStr			mFileName;
};

ID_INLINE idStr rvRegistryOptions::MakeKeyName( const char* name ) const
{
	if( mBaseKey.IsEmpty() )
	{
		return idStr( name );
	}
	return idStr( va( "%s.%s", mBaseKey.c_str(), name ) );
}

ID_INLINE void rvRegistryOptions::SetFloat( const char* name, float v )
{
	mValues.SetFloat( MakeKeyName( name ), v );
}

ID_INLINE void rvRegistryOptions::SetLong( const char* name, long v )
{
	mValues.SetInt( MakeKeyName( name ), ( int )v );
}

ID_INLINE void rvRegistryOptions::SetBool( const char* name, bool v )
{
	mValues.SetBool( MakeKeyName( name ), v );
}

ID_INLINE void rvRegistryOptions::SetString( const char* name, const char* v )
{
	mValues.Set( MakeKeyName( name ), v );
}

ID_INLINE void rvRegistryOptions::SetVec4( const char* name, const idVec4& v )
{
	mValues.SetVec4( MakeKeyName( name ), v );
}

ID_INLINE float rvRegistryOptions::GetFloat( const char* name, float defaultVal )
{
	return mValues.GetFloat( MakeKeyName( name ), va( "%f", defaultVal ) );
}

ID_INLINE long rvRegistryOptions::GetLong( const char* name, long defaultVal )
{
	return mValues.GetInt( MakeKeyName( name ), va( "%ld", defaultVal ) );
}

ID_INLINE bool rvRegistryOptions::GetBool( const char* name, bool defaultVal )
{
	return mValues.GetBool( MakeKeyName( name ), defaultVal ? "1" : "0" );
}

ID_INLINE const char* rvRegistryOptions::GetString( const char* name, const char* defaultVal )
{
	return mValues.GetString( MakeKeyName( name ), defaultVal );
}

ID_INLINE idVec4 rvRegistryOptions::GetVec4( const char* name, const idVec4& defaultVal )
{
	return mValues.GetVec4( MakeKeyName( name ), va( "%f %f %f %f", defaultVal.x, defaultVal.y, defaultVal.z, defaultVal.w ) );
}

ID_INLINE int rvRegistryOptions::GetRecentFileCount()
{
	return mRecentFiles.Num();
}

ID_INLINE const char* rvRegistryOptions::GetRecentFile( int index )
{
	return mRecentFiles[index].c_str();
}

#endif /* !__REGISTRYOPTIONS_H__ */
