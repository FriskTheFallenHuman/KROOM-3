/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2015 Daniel Gibson

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

#ifndef __IMGUIGUISYSTEM_H_
#define __IMGUIGUISYSTEM_H_

enum DockRegion
{
	DOCK_REGION_RIGHT,
	DOCK_REGION_BOTTOM,
	DOCK_REGION_LEFT,
	DOCK_REGION_CENTER
};

class idImGuiWindow
{
public:
	virtual ~idImGuiWindow() {}

	// Returns the stable ImGui window name used for docking and identification.
	virtual const char* GetWindowName() const = 0;

	// Returns the default docking region assigned to this window.
	virtual DockRegion GetDockRegion() const = 0;

	// Returns whether the window should be submitted during the current frame.
	virtual bool IsShown() const = 0;

	// Returns whether this window currently owns the editor free camera.
	virtual bool IsFreeCameraActive() const
	{
		return false;
	}

	// Submits this window's ImGui widgets for the current frame.
	virtual void Draw() = 0;
};

class idImGuiEditor;

class idImGuiSystem
{
public:
	virtual ~idImGuiSystem() {}

	// Initializes the ImGui context and connects it to the engine window.
	virtual bool Init( int windowWidth, int windowHeight ) = 0;

	// Releases the ImGui context and all renderer-owned ImGui resources.
	virtual void Destroy() = 0;

	// Returns the editor subsystem used to manage in-game ImGui tools.
	virtual idImGuiEditor* GetEditor() = 0;

	// Registers a ImGui window with the system.
	virtual void RegisterWindow( idImGuiWindow& window ) = 0;

	// Sets whether a ImGui window may release the engine mouse cursor.
	virtual void ReleaseMouse( bool doRelease ) = 0;

	// Updates the display size after the engine window changes dimensions.
	virtual void NotifyDisplaySizeChanged( int width, int height ) = 0;

	// Injects an engine system event into ImGui input processing.
	virtual bool InjectSysEvent( const sysEvent_t* keyEvent ) = 0;

	// Injects the current engine mouse-wheel delta into ImGui.
	virtual bool InjectMouseWheel( int delta ) = 0;

	// Starts a new ImGui frame after engine input has been collected.
	virtual void NewFrame() = 0;

	// Ensures a frame exists when rendering is requested outside the normal loop.
	virtual bool IsReadyToRender() = 0;

	// Renders all registered ImGui windows through the engine renderer.
	virtual void Render() = 0;

	// Returns whether the ImGui context is initialized.
	virtual bool IsInitialized() const = 0;

	// Returns whether the right mouse button is currently held.
	virtual bool RightMouseActive() const = 0;

	// Registers an additional named window in the docking layout.
	virtual void RegisterDockWindow( const char* windowName, DockRegion region ) = 0;

	// Returns whether ImGui should consume engine input.
	virtual bool UseInput() const = 0;

	// Returns whether ImGui should inhibit normal player user commands.
	virtual bool UseInputForUsercmd() const = 0;

	// Draws every registered non-editor ImGui window that is currently shown.
	virtual void DrawWindows() = 0;

};

extern idImGuiSystem* imguiSystem;

#endif /* !__IMGUIGUISYSTEM_H_ */
