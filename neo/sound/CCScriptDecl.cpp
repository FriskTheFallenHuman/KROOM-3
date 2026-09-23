/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2021 George Kalmpokis

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

#include "CCScriptDecl.h"

idCVar cc_debugCaptions( "cc_debugCaptions", "0", CVAR_BOOL | CVAR_SOUND | CVAR_NOCHEAT, "Enable/Disable Console Debug for the caption system" );

bool CCScriptDecl::LoadFile( const char* fileName, bool OSPath )
{
	idLexer src;

	src.SetFlags( DECL_LEXER_FLAGS );
	src.LoadFile( fileName, OSPath );
	if( !src.IsLoaded() )
	{
		return false;
	}

	while( !src.EndOfFile() )
	{
		idCaption* caption = new idCaption;
		if( ReadCaption( src, caption ) )
		{
			captions.Append( caption );
		}
	};

	return true;
}

bool CCScriptDecl::FindCaption( const char* name, idCaption** caption )
{
	for( int i = 0; i < captions.Num(); i++ )
	{
		if( !idStr::Icmp( name, captions[i]->GetName().c_str() ) )
		{
			*caption = captions[i];
			return true;
		}
	}
	return false;
}

bool CCScriptDecl::FindCaptionWithTimeCode( const char* name, int timecode, idCaption** caption )
{
	*caption = NULL;
	for( int i = 0; i < captions.Num(); i++ )
	{
		if( !idStr::Icmp( name, captions[i]->GetName() ) && timecode >= captions[i]->GetTimeCode() )
		{
			*caption = captions[i];
		}
	}
	return *caption != NULL;
}

bool CCScriptDecl::HasMultipleCaptions( const char* name )
{
	int count = 0;
	for( int i = 0; i < captions.Num(); i++ )
	{
		if( !idStr::Icmp( name, captions[i]->GetName().c_str() ) )
		{
			count++;
		}
	}
	return count > 1;
}

void CCScriptDecl::Clear()
{
	captions.DeleteContents( true );
}

bool CCScriptDecl::ReadCaption( idLexer& src, idCaption* caption )
{
	idToken name, token;

	src.SkipUntilString( "{" );

	while( 1 )
	{
		if( !src.ReadToken( &token ) )
		{
			return false;
		}

		if( !token.Icmp( "}" ) )
		{
			break;
		}

		if( !token.Icmp( "name" ) )
		{
			src.ParseRestOfLine( token );
			caption->SetName( token.c_str() );
		}
		else if( !token.Icmp( "color" ) )
		{
			float color[4];
			src.Parse1DMatrixJSON( 4, color );
			caption->SetColor( color[0], color[1], color[2], color[3] );
		}
		else if( !token.Icmp( "caption" ) )
		{
			int timecode = src.ParseInt();
			if( timecode > 0 )
			{
				caption->SetTimeCode( timecode );
			}
			else
			{
				caption->SetTimeCode( 0 );
			}

			src.ParseRestOfLine( token );
			caption->SetCaption( idLocalization::FindString( token.c_str() ) );
		}
		else if( !token.Icmp( "priority" ) )
		{
			caption->SetPriority( src.ParseInt() );
		}

	}

	return true;
}
