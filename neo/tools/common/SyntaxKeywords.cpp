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

#include "SyntaxKeywords.h"

static const int kKeywordLexerFlags = LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS |
									  LEXFL_ALLOWPATHNAMES | LEXFL_NOFATALERRORS;

/*
================
SyntaxKeywords::SyntaxKeywords
================
*/
SyntaxKeywords::SyntaxKeywords()
{
	m_hash.Clear( 256, 256 );
}

/*
================
SyntaxKeywords::~SyntaxKeywords
================
*/
SyntaxKeywords::~SyntaxKeywords()
{
}

/*
================
SyntaxKeywords::Clear
================
*/
void SyntaxKeywords::Clear()
{
	m_entries.Clear();
	m_concatenated.Clear();
	m_hash.Clear( 256, 256 );
}

/*
================
SyntaxKeywords::GetCount
================
*/
int SyntaxKeywords::GetCount() const
{
	return m_entries.Num();
}

/*
================
SyntaxKeywords::GetEntry
================
*/
const SyntaxKeywords::Entry& SyntaxKeywords::GetEntry( int index ) const
{
	static Entry empty;
	if( index < 0 || index >= m_entries.Num() )
	{
		return empty;
	}
	return m_entries[index];
}

/*
================
SyntaxKeywords::Add
================
*/
void SyntaxKeywords::Add( const char* name, const char* description, const idVec3& color )
{
	if( !name || !name[0] )
	{
		return;
	}

	Entry e;
	e.name = name;
	e.description = description ? description : "";
	e.color = color;

	m_entries.Append( e );

	const int index = m_entries.Num() - 1;
	m_hash.Add( idStr::IHash( e.name.c_str() ), index );

	m_concatenated.Clear();
}

/*
================
SyntaxKeywords::Find
================
*/
const SyntaxKeywords::Entry* SyntaxKeywords::Find( const char* name, int length ) const
{
	if( !name || !name[0] )
	{
		return nullptr;
	}

	if( length < 0 )
	{
		length = ( int )strlen( name );
	}
	if( length == 0 )
	{
		return nullptr;
	}

	if( length != ( int )strlen( name ) )
	{
		for( int i = 0; i < m_entries.Num(); i++ )
		{
			if( idStr::Icmpn( m_entries[i].name.c_str(), name, length ) == 0 )
			{
				return &m_entries[i];
			}
		}
		return nullptr;
	}

	const int hashKey = idStr::IHash( name, length );
	for( int i = m_hash.First( hashKey ); i != -1; i = m_hash.Next( i ) )
	{
		if( !m_entries[i].name.Icmp( name ) )
		{
			return &m_entries[i];
		}
	}

	return nullptr;
}

/*
================
SyntaxKeywords::GetConcatenated
================
*/
const char* SyntaxKeywords::GetConcatenated() const
{
	if( m_concatenated.Length() > 0 )
	{
		return m_concatenated.c_str();
	}

	for( int i = 0; i < m_entries.Num(); i++ )
	{
		if( i > 0 )
		{
			m_concatenated += " ";
		}
		m_concatenated += m_entries[i].name;
	}

	return m_concatenated.c_str();
}

/*
================
SyntaxKeywords::CollectByPrefix
================
*/
void SyntaxKeywords::CollectByPrefix( const char* prefix, int prefixLength, std::vector<idStr>& out ) const
{
	if( prefixLength < 0 )
	{
		prefixLength = prefix ? ( int )strlen( prefix ) : 0;
	}

	for( int i = 0; i < m_entries.Num(); i++ )
	{
		const idStr& name = m_entries[i].name;

		if( prefixLength == 0 ||
				idStr::Icmpn( name.c_str(), prefix, prefixLength ) == 0 )
		{
			out.push_back( name );
		}
	}
}

/*
================
SyntaxKeywords::LoadFromFile
================
*/
bool SyntaxKeywords::LoadFromFile( const char* fileName )
{
	if( !fileName || !fileName[0] )
	{
		return false;
	}

	idLexer src( fileName, kKeywordLexerFlags );
	if( !src.IsLoaded() )
	{
		return false;
	}

	Clear();

	idToken firstToken;
	if( !src.ReadToken( &firstToken ) )
	{
		return true;
	}

	if( !firstToken.Icmp( "keywords" ) )
	{
		return LoadRich( src );
	}

	return LoadFlat( src, firstToken );
}

/*
================
SyntaxKeywords::LoadFromMemory
================
*/
bool SyntaxKeywords::LoadFromMemory( const char* buffer, int length, const char* name )
{
	if( !buffer || length <= 0 )
	{
		return false;
	}

	idLexer src( buffer, length, name ? name : "*keywords*", kKeywordLexerFlags );

	Clear();

	idToken firstToken;
	if( !src.ReadToken( &firstToken ) )
	{
		return true;
	}

	if( !firstToken.Icmp( "keywords" ) )
	{
		return LoadRich( src );
	}

	return LoadFlat( src, firstToken );
}

/*
================
SyntaxKeywords::LoadFlat
================
*/
bool SyntaxKeywords::LoadFlat( idLexer& src, const idToken& firstToken )
{
	Add( firstToken.c_str() );

	idToken token;
	while( src.ReadToken( &token ) )
	{
		Add( token.c_str() );
	}

	RebuildIndex();
	return true;
}

/*
================
SyntaxKeywords::LoadRich
================
*/
bool SyntaxKeywords::LoadRich( idLexer& src )
{
	if( !src.ExpectTokenString( "{" ) )
	{
		return false;
	}

	idToken token;
	while( src.ReadToken( &token ) )
	{
		if( token == "}" )
		{
			break;
		}
		if( token != "{" )
		{
			continue;
		}

		Entry e;

		// Name
		idToken nameToken;
		if( !src.ExpectTokenType( TT_STRING, 0, &nameToken ) )
		{
			return false;
		}
		e.name = nameToken.c_str();

		if( !src.ExpectTokenString( "," ) )
		{
			return false;
		}

		// colour ( r, g, b )
		idToken rTok, gTok, bTok;
		if( !src.ExpectTokenString( "(" ) )
		{
			return false;
		}
		if( !src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &rTok ) )
		{
			return false;
		}
		if( !src.ExpectTokenString( "," ) )
		{
			return false;
		}
		if( !src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &gTok ) )
		{
			return false;
		}
		if( !src.ExpectTokenString( "," ) )
		{
			return false;
		}
		if( !src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &bTok ) )
		{
			return false;
		}
		if( !src.ExpectTokenString( ")" ) )
		{
			return false;
		}

		e.color.Set( rTok.GetIntValue() / 255.0f,
					 gTok.GetIntValue() / 255.0f,
					 bTok.GetIntValue() / 255.0f );

		if( !src.ExpectTokenString( "," ) )
		{
			return false;
		}

		// Description
		idToken descToken;
		if( !src.ExpectTokenType( TT_STRING, 0, &descToken ) )
		{
			return false;
		}
		e.description = descToken.c_str();

		if( !src.ExpectTokenString( "}" ) )
		{
			return false;
		}

		m_entries.Append( e );
		m_hash.Add( idStr::IHash( e.name.c_str() ), m_entries.Num() - 1 );
	}

	RebuildIndex();
	return true;
}

/*
================
SyntaxKeywords::RebuildIndex
================
*/
void SyntaxKeywords::RebuildIndex()
{
	m_concatenated.Clear();
}