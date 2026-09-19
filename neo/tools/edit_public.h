/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __EDIT_PUBLIC_H__
#define __EDIT_PUBLIC_H__

/*
===============================================================================

	Editors.

===============================================================================
*/

class	idProgram;
class	idInterpreter;

// in-game Light Editor
void	LightEditorInit( const idDict* spawnArgs, idEntity* ent );

// in-game Sound Editor
void	SoundEditorInit( const idDict* spawnArgs, idEntity* ent );

// in-game Articulated Figure Editor
void	AFEditorInit();


// in-game Particle Editor
void	ParticleEditorInit( const idDict* spawnArgs, idEntity* ent );

// in-game PDA Editor
//void	PDAEditorInit( const idDict* spawnArgs );
//void	PDAEditorShutdown();
//void	PDAEditorRun();


// in-game Script Editor
void	ScriptEditorInit( const idDict* spawnArgs );

// in-game Declaration Browser
void	DeclBrowserInit();

// Script Debugger (Server)
bool	DebuggerServerInit();
void	DebuggerServerShutdown();
void	DebuggerServerPrint( const char* text );
void	DebuggerServerCheckBreakpoint( idInterpreter* interpreter, idProgram* program, int instructionPointer );

#ifdef ID_ALLOW_TOOLS
	// Radiant Level Editor
	//void	RadiantInit();
	//void	RadiantShutdown();
	//void	RadiantRun();
	//void	RadiantPrint( const char* text );
	//void	RadiantSync( const char* mapName, const idVec3& viewOrg, const idAngles& viewAngles );

	// GUI Editor
	//void	GUIEditorInit();
	//void	GUIEditorShutdown();
	//void	GUIEditorRun();
	//bool	GUIEditorHandleMessage( void* msg );


	// Script Debugger (Client)
	void	DebuggerClientLaunch();
	void	DebuggerClientInit( const char* cmdline );

	//Material Editor
	void	MaterialEditorInit();
	bool	MaterialEditorRun();
	void	MaterialEditorShutdown();
	void	MaterialEditorPrintConsole( const char* msg );
#endif

#endif /* !__EDIT_PUBLIC_H__ */
