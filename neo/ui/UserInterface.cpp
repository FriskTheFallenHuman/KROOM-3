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

#include "precompiled.h"
#pragma hdrstop

#include "ListGUILocal.h"
#include "Window.h"
#include "UserInterfaceLocal.h"

extern idCVar r_skipGuiShaders;		// 1 = don't render any gui elements on surfaces

idUserInterfaceManagerLocal	uiManagerLocal;
idUserInterfaceManager* 	uiManager = &uiManagerLocal;

// These used to be in every window, but they all pointed at the same one in idUserInterfaceManagerLocal.
// Made a global so it can be switched out dynamically to test optimized versions.
idDeviceContext* dc;

idCVar g_useNewGuiCode(	"g_useNewGuiCode",	"1", CVAR_GAME | CVAR_INTEGER, "use optimized device context code, 2 = toggle on/off every frame" );
//idCVar binaryLoadGuis( "binaryLoadGuis", "1", CVAR_NEW, "enable binary load/write of particle decls" );

extern idCVar sys_lang;

/*
===============================================================================

	idUserInterfaceManagerLocal

===============================================================================
*/

void idUserInterfaceManagerLocal::Init()
{
	screenRect = idRectangle( 0, 0, 640, 480 );
	dcOld.Init();
	dcOptimized.Init();

	SetDrawingDC();

}

void idUserInterfaceManagerLocal::Shutdown()
{
	guis.DeleteContents( true );
	demoGuis.DeleteContents( true );
	dcOld.Shutdown();
	dcOptimized.Shutdown();
	//mapParser.Clear();
}

void idUserInterfaceManagerLocal::SetDrawingDC()
{
	static int toggle;

	// to make it more obvious that there is a difference between the old and
	// new paths, toggle between them every frame if g_useNewGuiCode is set to 2
	toggle++;

	if( g_useNewGuiCode.GetInteger() == 1 ||
			( g_useNewGuiCode.GetInteger() == 2 && ( toggle & 1 ) ) )
	{
		dc = &dcOptimized;
	}
	else
	{
		dc = &dcOld;
	}
}

void idUserInterfaceManagerLocal::Touch( const char* name )
{
	idUserInterface* gui = Alloc();
	gui->InitFromFile( name );
//	delete gui;
}

void idUserInterfaceManagerLocal::WritePrecacheCommands( idFile* f )
{

	int c = guis.Num();
	for( int i = 0; i < c; i++ )
	{
		char	str[1024];
		idStr::snPrintf( str, sizeof( str ), "touchGui %s\n", guis[i]->Name() );
		common->Printf( "%s", str );
		f->Printf( "%s", str );
	}
}

void idUserInterfaceManagerLocal::SetSize( float width, float height )
{
	dc->SetSize( width, height );
}

/*
void idUserInterfaceManagerLocal::Preload( const char* mapName )
{
	// RB: allow skipping binary preloading so modders can add new .gui files
	if( !binaryLoadGuis.GetBool() )
	{
		return;
	}

	if( mapName != NULL && mapName[ 0 ] != '\0' )
	{
		mapParser.LoadFromFile( va( "generated/guis/%s.bgui", mapName ) );
	}
}
*/

void idUserInterfaceManagerLocal::BeginLevelLoad()
{
	for( int i = 0; i < guis.Num(); i++ )
	{
		if( ( guis[ i ]->GetDesktop()->GetFlags() & WIN_MENUGUI ) == 0 )
		{
			guis[ i ]->ClearRefs();
		}
	}
}

void idUserInterfaceManagerLocal::EndLevelLoad( const char* mapName )
{
	int c = guis.Num();
	for( int i = 0; i < c; i++ )
	{
		// foresthale 2014-05-28: Brian Harris suggested the editors should never purge assets, because of potential for crashes on improperly refcounted assets
		if( guis[i]->GetRefs() == 0 && !( com_editors ) )
		{
			//common->Printf( "purging %s.\n", guis[i]->GetSourceFile() );

			// use this to make sure no materials still reference this gui
			bool remove = true;
			for( int j = 0; j < declManager->GetNumDecls( DECL_MATERIAL ); j++ )
			{
				const idMaterial* material = static_cast<const idMaterial*>( declManager->DeclByIndex( DECL_MATERIAL, j, false ) );
				if( material->GlobalGui() == guis[i] )
				{
					remove = false;
					break;
				}
			}
			if( remove )
			{
				delete guis[ i ];
				// foresthale 2014-06-08: changed the ~idUserInterfaceLocal method to remove its index automatically, so we don't want this line anymore
				//guis.RemoveIndex( i );
				i--;
				c--;
			}
		}
		session->PacifierUpdate();
	}
	/*
	if( cvarSystem->GetCVarBool( "fs_buildresources" ) && mapName != NULL && mapName[ 0 ] != '\0' )
	{
		mapParser.WriteToFile( va( "generated/guis/%s.bgui", mapName ) );
		idFile* f = fileSystem->OpenFileRead( va( "generated/guis/%s.bgui", mapName ) );
		delete f;
	}
	*/
	dcOld.Init();
	dcOptimized.Init();
}

void idUserInterfaceManagerLocal::Reload( bool all )
{
	ID_TIME_T ts;

	int c = guis.Num();
	for( int i = 0; i < c; i++ )
	{
		if( !all )
		{
			fileSystem->ReadFile( guis[i]->GetSourceFile(), NULL, &ts );
			if( ts <= guis[i]->GetTimeStamp() )
			{
				continue;
			}
		}

		guis[i]->InitFromFile( guis[i]->GetSourceFile() );
		common->Printf( "reloading %s.\n", guis[i]->GetSourceFile() );
	}
}

void idUserInterfaceManagerLocal::ListGuis() const
{
	int c = guis.Num();
	common->Printf( "\n   size   refs   name\n" );
	size_t total = 0;
	int copies = 0;
	int unique = 0;
	for( int i = 0; i < c; i++ )
	{
		idUserInterfaceLocal* gui = guis[i];
		size_t sz = gui->Size();
		bool isUnique = guis[i]->interactive;
		if( isUnique )
		{
			unique++;
		}
		else
		{
			copies++;
		}
		common->Printf( "%6.1fk %4i (%s) %s ( %i transitions )\n", sz / 1024.0f, guis[i]->GetRefs(), isUnique ? "unique" : "copy", guis[i]->GetSourceFile(), guis[i]->desktop->NumTransitions() );
		total += sz;
	}
	common->Printf( "===========\n  %i total Guis ( %i copies, %i unique ), %.2f total Mbytes", c, copies, unique, total / ( 1024.0f * 1024.0f ) );
}

bool idUserInterfaceManagerLocal::CheckGui( const char* qpath ) const
{
	idFile* file = fileSystem->OpenFileRead( qpath );
	if( file )
	{
		fileSystem->CloseFile( file );
		return true;
	}
	return false;
}

idUserInterface* idUserInterfaceManagerLocal::Alloc() const
{
	return new idUserInterfaceLocal();
}

void idUserInterfaceManagerLocal::DeAlloc( idUserInterface* gui )
{
	if( gui )
	{
		int c = guis.Num();
		for( int i = 0; i < c; i++ )
		{
			if( guis[i] == gui )
			{
				delete guis[i];
				// foresthale 2014-05-29: added the remove to the dtor, so this is no longer needed
				//guis.RemoveIndex( i ); // motorsep 06-02-2014 was disabled by foresthale
				return;
			}
		}
	}
}

idUserInterface* idUserInterfaceManagerLocal::FindGui( const char* qpath, bool autoLoad, bool needUnique, bool forceNOTUnique )
{
	int c = guis.Num();

	for( int i = 0; i < c; i++ )
	{
		idUserInterfaceLocal* gui = guis[i];
		if( gui == NULL )
		{
			continue;
		}

		if( !idStr::Icmp( gui->GetSourceFile(), qpath ) )
		{
			if( !forceNOTUnique && ( needUnique || guis[i]->IsInteractive() ) )
			{
				break;
			}
			// Reload the gui if it's been cleared
			if( guis[i]->GetRefs() == 0 )
			{
				guis[i]->InitFromFile( guis[i]->GetSourceFile() );
			}
			guis[i]->AddRef();
			return guis[i];
		}
	}

	if( autoLoad )
	{
		idUserInterface* gui = Alloc();
		if( gui->InitFromFile( qpath ) )
		{
			gui->SetUniqued( forceNOTUnique ? false : needUnique );
			return gui;
		}
		else
		{
			delete gui;
		}
	}
	return NULL;
}

idUserInterface* idUserInterfaceManagerLocal::FindDemoGui( const char* qpath )
{
	int c = demoGuis.Num();
	for( int i = 0; i < c; i++ )
	{
		if( !idStr::Icmp( demoGuis[i]->GetSourceFile(), qpath ) )
		{
			return demoGuis[i];
		}
	}
	return NULL;
}

idListGUI* 	idUserInterfaceManagerLocal::AllocListGUI() const
{
	return new idListGUILocal();
}

void idUserInterfaceManagerLocal::FreeListGUI( idListGUI* listgui )
{
	delete listgui;
}

/*
===============================================================================

	idUserInterfaceLocal

===============================================================================
*/

idUserInterfaceLocal::idUserInterfaceLocal()
{
	cursorX = cursorY = 0.0;
	desktop = NULL;
	loading = false;
	active = false;
	interactive = false;
	uniqued = false;
	bindHandler = NULL;
	//so the reg eval in gui parsing doesn't get bogus values
	time = 0;
	refs = 1;
}

idUserInterfaceLocal::~idUserInterfaceLocal()
{
	// foresthale 2014-05-29: editGUIs shutdown code doesn't remove the gui before it gets destroyed
	uiManagerLocal.guis.Remove( this ); // motorsep 06-02-2014 was introduced by foresthale, that's what crashes game (bug # 0000213)
	delete desktop;
	desktop = NULL;
}

const char* idUserInterfaceLocal::Name() const
{
	return source;
}

const char* idUserInterfaceLocal::Comment() const
{
	if( desktop )
	{
		return desktop->GetComment();
	}
	return "";
}

bool idUserInterfaceLocal::IsInteractive() const
{
	return interactive;
}

bool idUserInterfaceLocal::InitFromFile( const char* qpath, bool rebuild, bool cache )
{

	if( !( qpath && *qpath ) )
	{
		// FIXME: Memory leak!!
		return false;
	}

	int sz = sizeof( idWindow );
	sz = sizeof( idSimpleWindow );
	loading = true;

	if( rebuild )
	{
		delete desktop;
		desktop = new idWindow( this );
	}
	else if( desktop == NULL )
	{
		desktop = new idWindow( this );
	}

	// First try loading the localized version
	// Then fall back to the english version
	for( int i = 0; i < 2; i++ )
	{
		source = qpath;
		idStr trySource = qpath;
		trySource.ToLower();
		trySource.BackSlashesToSlashes();
		if( i == 0 )
		{
			trySource.Replace( "guis/", va( "guis/%s/", sys_lang.GetString() ) );
		}
		fileSystem->ReadFile( trySource, NULL, &timeStamp );
		if( timeStamp != FILE_NOT_FOUND_TIMESTAMP )
		{
			source = trySource;
			break;
		}
	}
	state.Set( "text", "Test Text!" );

	if( com_editors )
	{
		idParser src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

		//Load the timestamp so reload guis will work correctly
		fileSystem->ReadFile( qpath, NULL, &timeStamp );

		src.LoadFile( qpath );

		if( src.IsLoaded() )
		{
			idToken token;
			while( src.ReadToken( &token ) )
			{
				if( idStr::Icmp( token, "windowDef" ) == 0 )
				{
					if( desktop->Parse( &src, rebuild ) )
					{
						desktop->SetFlag( WIN_DESKTOP );
						desktop->FixupParms();
					}
					continue;
				}
			}

			state.Set( "name", qpath );
		}
		else
		{
			desktop->SetFlag( WIN_DESKTOP );
			desktop->name = "Desktop";
			desktop->text = va( "Invalid GUI: %s", qpath );
			desktop->rect = idRectangle( 0.0f, 0.0f, 640.0f, 480.0f );
			desktop->drawRect = desktop->rect;
			desktop->foreColor = idVec4( 1.0f, 1.0f, 1.0f, 1.0f );
			desktop->backColor = idVec4( 0.0f, 0.0f, 0.0f, 1.0f );
			desktop->SetupFromState();
			common->Warning( "Couldn't load gui: '%s'", source.c_str() );
			loading = false;
			return false;
		}
	}
	else
	{
		idTokenParser& bsrc = uiManagerLocal.GetBinaryParser();
		if( rebuild || !bsrc.IsLoaded() || !bsrc.StartParsing( source ) )
		{
			idParser src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
			src.LoadFile( source );
			if( src.IsLoaded() )
			{
				bsrc.LoadFromParser( src, source );
				ID_TIME_T ts = fileSystem->GetTimestamp( source );
				bsrc.UpdateTimeStamp( ts );
			}
		}
		if( bsrc.IsLoaded() && bsrc.StartParsing( source ) )
		{
			idToken token;
			while( bsrc.ReadToken( &token ) )
			{
				if( idStr::Icmp( token, "windowDef" ) == 0 )
				{
					if( desktop->Parse( &bsrc, rebuild ) )
					{
						desktop->SetFlag( WIN_DESKTOP );
						desktop->FixupParms();
					}
					continue;
				}
			}
			state.Set( "name", qpath );	// don't use localized name
			bsrc.DoneParsing();
		}
		else
		{
			desktop->SetFlag( WIN_DESKTOP );
			desktop->name = "Desktop";
			desktop->text = va( "Invalid GUI: %s", qpath );
			desktop->rect = idRectangle( 0.0f, 0.0f, 640.0f, 480.0f );
			desktop->drawRect = desktop->rect;
			desktop->foreColor = idVec4( 1.0f, 1.0f, 1.0f, 1.0f );
			desktop->backColor = idVec4( 0.0f, 0.0f, 0.0f, 1.0f );
			desktop->SetupFromState();
			common->Warning( "Couldn't load gui: '%s'", source.c_str() );
			loading = false;
			return false;
		}
	}

	interactive = desktop->Interactive();

	if( uiManagerLocal.guis.Find( this ) == NULL )
	{
		uiManagerLocal.guis.Append( this );
	}

	loading = false;

	return true;
}

const char* idUserInterfaceLocal::HandleEvent( const sysEvent_t* event, int _time, bool* updateVisuals )
{

	time = _time;

	if( bindHandler && event->evType == SE_KEY && event->evValue2 == 1 )
	{
		const char* ret = bindHandler->HandleEvent( event, updateVisuals );
		bindHandler = NULL;
		return ret;
	}

	if( event->evType == SE_MOUSE )
	{
		if( !desktop || ( desktop->GetFlags() & WIN_MENUGUI ) )
		{
			// DG: this is a fullscreen GUI, scale the mousedelta added to cursorX/Y
			// by 640/w, because the GUI pretends that everything is 640x480
			// even if the actual resolution is higher => mouse moved too fast
			float w = renderSystem->GetWidth();
			float h = renderSystem->GetHeight();
			if( w <= 0.0f || h <= 0.0f )
			{
				w = 640.0f;
				h = 480.0f;
			}
			cursorX += event->evValue * ( 640.0f / w );
			cursorY += event->evValue2 * ( 480.0f / h );
		}
		else
		{
			// not a fullscreen GUI but some ingame thing - no scaling needed
			cursorX += event->evValue;
			cursorY += event->evValue2;
		}

		if( cursorX < 0 )
		{
			cursorX = 0;
		}
		if( cursorY < 0 )
		{
			cursorY = 0;
		}
	}

	if( desktop )
	{
		return desktop->HandleEvent( event, updateVisuals );
	}

	return "";
}

void idUserInterfaceLocal::HandleNamedEvent( const char* eventName )
{
	desktop->RunNamedEvent( eventName );
}

void idUserInterfaceLocal::Redraw( int _time, bool hud )
{
	if( r_skipGuiShaders.GetInteger() > 5 )
	{
		return;
	}
	if( !loading && desktop )
	{
		time = _time;
		dc->PushClipRect( uiManagerLocal.screenRect );
		desktop->Redraw( 0, 0, hud );
		dc->PopClipRect();
	}
}

void idUserInterfaceLocal::DrawCursor()
{
	if( !desktop || desktop->GetFlags() & WIN_MENUGUI )
	{
		dc->DrawCursor( &cursorX, &cursorY, 22.0f );
	}
	else
	{
		dc->DrawCursor( &cursorX, &cursorY, 56.0f );
	}
}

const idDict& idUserInterfaceLocal::State() const
{
	return state;
}

void idUserInterfaceLocal::DeleteStateVar( const char* varName )
{
	state.Delete( varName );
}

void idUserInterfaceLocal::SetStateString( const char* varName, const char* value )
{
	state.Set( varName, value );
}

void idUserInterfaceLocal::SetStateBool( const char* varName, const bool value )
{
	state.SetBool( varName, value );
}

void idUserInterfaceLocal::SetStateInt( const char* varName, const int value )
{
	state.SetInt( varName, value );
}

void idUserInterfaceLocal::SetStateFloat( const char* varName, const float value )
{
	state.SetFloat( varName, value );
}

const char* idUserInterfaceLocal::GetStateString( const char* varName, const char* defaultString ) const
{
	return state.GetString( varName, defaultString );
}

bool idUserInterfaceLocal::GetStateBool( const char* varName, const char* defaultString ) const
{
	return state.GetBool( varName, defaultString );
}

int idUserInterfaceLocal::GetStateInt( const char* varName, const char* defaultString ) const
{
	return state.GetInt( varName, defaultString );
}

float idUserInterfaceLocal::GetStateFloat( const char* varName, const char* defaultString ) const
{
	return state.GetFloat( varName, defaultString );
}

void idUserInterfaceLocal::StateChanged( int _time, bool redraw )
{
	time = _time;
	if( desktop )
	{
		desktop->StateChanged( redraw );
	}
	if( state.GetBool( "noninteractive" ) )
	{
		interactive = false;
	}
	else
	{
		if( desktop )
		{
			interactive = desktop->Interactive();
		}
		else
		{
			interactive = false;
		}
	}
}

const char* idUserInterfaceLocal::Activate( bool activate, int _time )
{
	time = _time;
	active = activate;
	if( desktop )
	{
		activateStr = "";
		desktop->Activate( activate, activateStr );
		return activateStr;
	}
	return "";
}

void idUserInterfaceLocal::Trigger( int _time )
{
	time = _time;
	if( desktop )
	{
		desktop->Trigger();
	}
}

void idUserInterfaceLocal::ReadFromDemoFile( class idDemoFile* f )
{
	idStr work;
	f->ReadDict( state );
	source = state.GetString( "name" );

	if( desktop == NULL )
	{
		f->Log( "creating new gui\n" );
		desktop = new idWindow( this );
		desktop->SetFlag( WIN_DESKTOP );
		desktop->ReadFromDemoFile( f );
	}
	else
	{
		f->Log( "re-using gui\n" );
		desktop->ReadFromDemoFile( f, false );
	}

	f->ReadFloat( cursorX );
	f->ReadFloat( cursorY );

	bool add = true;
	int c = uiManagerLocal.demoGuis.Num();
	for( int i = 0; i < c; i++ )
	{
		if( uiManagerLocal.demoGuis[i] == this )
		{
			add = false;
			break;
		}
	}

	if( add )
	{
		uiManagerLocal.demoGuis.Append( this );
	}
}

void idUserInterfaceLocal::WriteToDemoFile( class idDemoFile* f )
{
	idStr work;
	f->WriteDict( state );
	if( desktop )
	{
		desktop->WriteToDemoFile( f );
	}

	f->WriteFloat( cursorX );
	f->WriteFloat( cursorY );
}

bool idUserInterfaceLocal::WriteToSaveGame( idFile* savefile ) const
{
	int len;
	const idKeyValue* kv;
	const char* string;

	int num = state.GetNumKeyVals();
	savefile->Write( &num, sizeof( num ) );

	for( int i = 0; i < num; i++ )
	{
		kv = state.GetKeyVal( i );
		len = kv->GetKey().Length();
		string = kv->GetKey().c_str();
		savefile->Write( &len, sizeof( len ) );
		savefile->Write( string, len );

		len = kv->GetValue().Length();
		string = kv->GetValue().c_str();
		savefile->Write( &len, sizeof( len ) );
		savefile->Write( string, len );
	}

	savefile->Write( &active, sizeof( active ) );
	savefile->Write( &interactive, sizeof( interactive ) );
	savefile->Write( &uniqued, sizeof( uniqued ) );
	savefile->Write( &time, sizeof( time ) );
	len = activateStr.Length();
	savefile->Write( &len, sizeof( len ) );
	savefile->Write( activateStr.c_str(), len );
	len = pendingCmd.Length();
	savefile->Write( &len, sizeof( len ) );
	savefile->Write( pendingCmd.c_str(), len );
	len = returnCmd.Length();
	savefile->Write( &len, sizeof( len ) );
	savefile->Write( returnCmd.c_str(), len );

	savefile->Write( &cursorX, sizeof( cursorX ) );
	savefile->Write( &cursorY, sizeof( cursorY ) );

	desktop->WriteToSaveGame( savefile );

	return true;
}

bool idUserInterfaceLocal::ReadFromSaveGame( idFile* savefile )
{
	int num;
	int i, len;
	idStr key;
	idStr value;

	savefile->Read( &num, sizeof( num ) );

	state.Clear();
	for( i = 0; i < num; i++ )
	{
		savefile->Read( &len, sizeof( len ) );
		key.Fill( ' ', len );
		savefile->Read( &key[0], len );

		savefile->Read( &len, sizeof( len ) );
		value.Fill( ' ', len );
		savefile->Read( &value[0], len );

		state.Set( key, value );
	}

	savefile->Read( &active, sizeof( active ) );
	savefile->Read( &interactive, sizeof( interactive ) );
	savefile->Read( &uniqued, sizeof( uniqued ) );
	savefile->Read( &time, sizeof( time ) );

	savefile->Read( &len, sizeof( len ) );
	activateStr.Fill( ' ', len );
	savefile->Read( &activateStr[0], len );
	savefile->Read( &len, sizeof( len ) );
	pendingCmd.Fill( ' ', len );
	savefile->Read( &pendingCmd[0], len );
	savefile->Read( &len, sizeof( len ) );
	returnCmd.Fill( ' ', len );
	savefile->Read( &returnCmd[0], len );

	savefile->Read( &cursorX, sizeof( cursorX ) );
	savefile->Read( &cursorY, sizeof( cursorY ) );

	desktop->ReadFromSaveGame( savefile );

	return true;
}

size_t idUserInterfaceLocal::Size()
{
	size_t sz = sizeof( *this ) + state.Size() + source.Allocated();
	if( desktop )
	{
		sz += desktop->Size();
	}
	return sz;
}

void idUserInterfaceLocal::RecurseSetKeyBindingNames( idWindow* window )
{
	int i;
	idWinVar* v = window->GetWinVarByName( "bind" );
	if( v )
	{
		SetStateString( v->GetName(), idKeyInput::KeysFromBinding( v->GetName() ) );
	}
	i = 0;
	while( i < window->GetChildCount() )
	{
		idWindow* next = window->GetChild( i );
		if( next )
		{
			RecurseSetKeyBindingNames( next );
		}
		i++;
	}
}

/*
==============
idUserInterfaceLocal::SetKeyBindingNames
==============
*/
void idUserInterfaceLocal::SetKeyBindingNames()
{
	if( !desktop )
	{
		return;
	}
	// walk the windows
	RecurseSetKeyBindingNames( desktop );
}

/*
==============
idUserInterfaceLocal::SetCursor
==============
*/
void idUserInterfaceLocal::SetCursor( float x, float y )
{
	cursorX = x;
	cursorY = y;
}
