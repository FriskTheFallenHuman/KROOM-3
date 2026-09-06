/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Daniel Gibson
Copyright (C) 2020-2023 Robert Beckebans

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

#ifndef __IMGUI_TOOLS_H__
#define __IMGUI_TOOLS_H__

class idImGuiEditor
{
public:
	virtual ~idImGuiEditor() {}

	// Registers an editor window and its default docking location.
	virtual void RegisterWindow( idImGuiWindow& window ) = 0;

	// Initializes a tool.
	virtual void InitTool( const toolFlag_t tool, const idDict* dict, idEntity* entity = NULL ) = 0;

	// Updates whether editor tools may release the engine mouse cursor.
	virtual void ReleaseMouse( bool doRelease ) = 0;

	// Updates the editor's right-mouse-button state.
	virtual void SetRightMouseActive( bool active ) = 0;

	// Returns whether an editor mode currently requires ImGui handling.
	virtual bool AreEditorsActive() const = 0;

	// Returns whether editor tools released the engine mouse cursor.
	virtual bool IsMouseRelease() const = 0;

	// Returns whether an editor currently owns the free camera.
	virtual bool IsFreeCameraActive() const = 0;

	// Draws every registered editor window that is currently shown.
	virtual void DrawWindows() = 0;
};

#endif /* !__IMGUI_TOOLS_H__ */
