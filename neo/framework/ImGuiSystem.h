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

typedef int idImGuiWindowFlags;

// Carbon copy of imgui flags
typedef enum
{
	FLAGS_NONE						= 0,

	FLAGS_NOTITLEBAR				= BIT( 0 ),		// Disable title-bar
	FLAGS_NORESIZE					= BIT( 1 ),		// Disable user resizing with the lower-right grip
	FLAGS_NOMOVE					= BIT( 2 ),		// Disable user moving the window
	FLAGS_NOSCROLLBAR				= BIT( 3 ),		// Disable scrollbars (window can still scroll with mouse or programmatically)
	FLAGS_NOSCROLLWITHMOUSE			= BIT( 4 ),		// Disable user vertically scrolling with mouse wheel. On child window, mouse wheel will be forwarded to the parent unless NoScrollbar is also set.
	FLAGS_NOCOLLAPSE				= BIT( 5 ),		// Disable user collapsing window by double-clicking on it. Also referred to as Window Menu Button (e.g. within a docking node).
	FLAGS_ALWAYSAUTORESIZE			= BIT( 6 ),		// Resize every window to its content every frame
	FLAGS_NOBACKGROUND				= BIT( 7 ),		// Disable drawing background color (WindowBg, etc.) and outside border. Similar as using SetNextWindowBgAlpha(0.0f).
	FLAGS_NOSAVEDSETTINGS			= BIT( 8 ),		// Never load/save settings in .ini file
	FLAGS_NOMOUSEINPUTS				= BIT( 9 ),		// Disable catching mouse, hovering test with pass through.
	FLAGS_MENUBAR					= BIT( 10 ),	// Has a menu-bar
	FLAGS_HORIZONTALSCROLLBAR		= BIT( 11 ),	// Allow horizontal scrollbar to appear (off by default). You may use SetNextWindowContentSize(ImVec2(width,0.0f)); prior to calling Begin() to specify width. Read code in imgui_demo in the "Horizontal Scrolling" section.
	FLAGS_NOFOCUSONAPPEARING		= BIT( 12 ),	// Disable taking focus when transitioning from hidden to visible state
	FLAGS_NOBRINGTOFRONTONFOCUS		= BIT( 13 ),	// Disable bringing window to front when taking focus (e.g. clicking on it or programmatically giving it focus)
	FLAGS_ALWAYSVERTICALSCROLLBAR	= BIT( 14 ),	// Always show vertical scrollbar (even if ContentSize.y < Size.y)
	FLAGS_ALWAYSHORIZONTALSCROLLBAR	= BIT( 15 ),	// Always show horizontal scrollbar (even if ContentSize.x < Size.x)
	FLAGS_NONAVINPUTS				= BIT( 16 ),	// No keyboard/gamepad navigation within the window
	FLAGS_NONAVFOCUS				= BIT( 17 ),	// No focusing toward this window with keyboard/gamepad navigation (e.g. skipped by Ctrl+Tab)
	FLAGS_UNSAVEDDOCUMENT			= BIT( 18 ),	// Display a dot next to the title. When used in a tab/docking context, tab is selected when clicking the X + closure is not assumed (will wait for user to stop submitting the tab). Otherwise closure is assumed when pressing the X, so if you keep submitting the tab may reappear at end of tab bar.
	FLAGS_NODOCKING					= BIT( 19 ),	// Disable docking of this window

	FLAGS_NONAV						= FLAGS_NONAVINPUTS | FLAGS_NONAVFOCUS,
	FLAGS_NODECORATION				= FLAGS_NOTITLEBAR | FLAGS_NORESIZE | FLAGS_NOSCROLLBAR | FLAGS_NOCOLLAPSE,
	FLAGS_NOINPUTS					= FLAGS_NOMOUSEINPUTS | FLAGS_NONAVINPUTS | FLAGS_NONAVFOCUS,

} idImGuiWindowFlags_;

enum DockRegion
{
	DOCK_REGION_NONE = 0,
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

	// Full title shown in Begin(). Defaults to GetWindowName()
	virtual const char* GetDisplayTitle() const
	{
		return GetWindowName();
	}

	// Returns the default docking region assigned to this window.
	virtual DockRegion GetDockRegion() const = 0;

	// Returns whether the window should be submitted during the current frame.
	virtual bool IsShown() const = 0;

	// Shows or hides this window.
	virtual void ShowIt( bool show ) = 0;

	// Returns whether this window currently owns the editor free camera.
	virtual bool IsFreeCameraActive() const
	{
		return false;
	}

	// Extra ImGuiWindowFlags on top of the derived base flags.
	virtual idImGuiWindowFlags GetExtraWindowFlags() const
	{
		return FLAGS_NONE;
	}

	// Draws this window's contents.
	virtual void DrawContents( bool& showTool ) {}

	// Called once, the frame this window transitions from shown to
	// hidden.
	virtual void OnClosed() {}

	// Derives NoDocking from GetDockRegion
	idImGuiWindowFlags GetBaseWindowFlags() const
	{
		idImGuiWindowFlags flags = GetExtraWindowFlags();
		if( GetDockRegion() == DOCK_REGION_NONE )
		{
			flags |= FLAGS_NODOCKING;
		}
		return flags;
	}

	// Submits this window's ImGui widgets for the current frame.
	virtual void Draw();
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
