/*
===========================================================================

KROOM 3 GPL Source Code

This file is part of the KROOM 3 Source Code, originally based
on the Doom 3 with bits and pieces from Doom 3 BFG edition GPL Source Codes both published in 2011 and 2012.

KROOM 3 Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Extra attributions can be found on the CREDITS.txt file

===========================================================================
*/
#ifndef __GUISCRIPT_H
#define __GUISCRIPT_H

#include "Window.h"
#include "Winvar.h"

struct idGSWinVar
{
	idGSWinVar()
	{
		var = NULL;
		own = false;
	}
	idWinVar* var;
	bool own;
};

class idGuiScriptList;

class idGuiScript
{
	friend class idGuiScriptList;
	friend class idWindow;

public:
	idGuiScript();
	~idGuiScript();

	bool Parse( idTokenParser* src );
	bool Parse( idParser* src );
	void Execute( idWindow* win )
	{
		if( handler )
		{
			handler( win, &parms );
		}
	}
	void FixupParms( idWindow* win );
	size_t Size()
	{
		int sz = sizeof( *this );
		for( int i = 0; i < parms.Num(); i++ )
		{
			sz += parms[i].var->Size();
		}
		return sz;
	}

	void WriteToSaveGame( idFile* savefile );
	void ReadFromSaveGame( idFile* savefile );

protected:
	int conditionReg;
	idGuiScriptList* ifList;
	idGuiScriptList* elseList;
	idList<idGSWinVar> parms;
	void ( *handler )( idWindow* window, idList<idGSWinVar>* src );

};


class idGuiScriptList
{
	idList<idGuiScript*> list;
public:
	idGuiScriptList()
	{
		list.SetGranularity( 4 );
	};
	~idGuiScriptList()
	{
		list.DeleteContents( true );
	};
	void Execute( idWindow* win );
	void Append( idGuiScript* gs )
	{
		list.Append( gs );
	}
	size_t Size()
	{
		int sz = sizeof( *this );
		for( int i = 0; i < list.Num(); i++ )
		{
			sz += list[i]->Size();
		}
		return sz;
	}
	void FixupParms( idWindow* win );
	void ReadFromDemoFile( class idDemoFile* f ) {};
	void WriteToDemoFile( class idDemoFile* f ) {};

	void WriteToSaveGame( idFile* savefile );
	void ReadFromSaveGame( idFile* savefile );
};

#endif // __GUISCRIPT_H
