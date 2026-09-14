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

#ifndef __SYNTAXKEYWORDS_H__
#define __SYNTAXKEYWORDS_H__

#include <vector>

class SyntaxKeywords
{
public:
	struct Entry
	{
		idStr   name;
		idStr   description;
		idVec3  color;

		Entry() : color( 0.0f, 0.0f, 1.0f ) {}
	};

	SyntaxKeywords();
	~SyntaxKeywords();

	bool LoadFromFile( const char* fileName );
	bool LoadFromMemory( const char* buffer, int length, const char* name );

	void Add( const char* name, const char* description = "", const idVec3& color = idVec3( 0.0f, 0.0f, 1.0f ) );
	void Clear();

	int             GetCount() const;
	const Entry&    GetEntry( int index ) const;

	const Entry*    Find( const char* name, int length = -1 ) const;

	const char*     GetConcatenated() const;

	void            CollectByPrefix( const char* prefix, int prefixLength, std::vector<idStr>& out ) const;

private:
	bool    LoadFlat( idLexer& src, const idToken& firstToken );
	bool    LoadRich( idLexer& src );

	void    RebuildIndex();

	idList<Entry>           m_entries;
	mutable idStr           m_concatenated;
	idHashIndex             m_hash;
};

#endif /* !__SYNTAXKEYWORDS_H__ */