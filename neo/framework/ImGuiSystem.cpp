/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2016 Daniel Gibson

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

#include "../extern/imgui/imgui_internal.h"
#include "../extern/imguizmo/ImGuizmo.h"
#include "../tools/imgui/ImGuiTools_local.h"
#include "renderer/RenderCommon.h"
#include "renderer/RenderBackend.h"

idCVar imgui_showDemoWindow( "imgui_showDemoWindow", "0", CVAR_GUI | CVAR_BOOL, "show big ImGui demo window" );

/*
===============================================================================

	Map engine side FLAGS_* bit values (idImGuiWindowFlags) onto
	ImGui's ImGuiWindowFlags_* bit values.

===============================================================================
*/
struct idImGuiWindowFlagRemap
{
	idImGuiWindowFlags	ourFlag;
	ImGuiWindowFlags_	imguiFlag;
};

static const idImGuiWindowFlagRemap windowFlagRemapTable[] =
{
	{ FLAGS_NOTITLEBAR,					ImGuiWindowFlags_NoTitleBar },
	{ FLAGS_NORESIZE,					ImGuiWindowFlags_NoResize },
	{ FLAGS_NOMOVE,						ImGuiWindowFlags_NoMove },
	{ FLAGS_NOSCROLLBAR,				ImGuiWindowFlags_NoScrollbar },
	{ FLAGS_NOSCROLLWITHMOUSE,			ImGuiWindowFlags_NoScrollWithMouse },
	{ FLAGS_NOCOLLAPSE,					ImGuiWindowFlags_NoCollapse },
	{ FLAGS_ALWAYSAUTORESIZE,			ImGuiWindowFlags_AlwaysAutoResize },
	{ FLAGS_NOBACKGROUND,				ImGuiWindowFlags_NoBackground },
	{ FLAGS_NOSAVEDSETTINGS,			ImGuiWindowFlags_NoSavedSettings },
	{ FLAGS_NOMOUSEINPUTS,				ImGuiWindowFlags_NoMouseInputs },
	{ FLAGS_MENUBAR,					ImGuiWindowFlags_MenuBar },
	{ FLAGS_HORIZONTALSCROLLBAR,		ImGuiWindowFlags_HorizontalScrollbar },
	{ FLAGS_NOFOCUSONAPPEARING,			ImGuiWindowFlags_NoFocusOnAppearing },
	{ FLAGS_NOBRINGTOFRONTONFOCUS,		ImGuiWindowFlags_NoBringToFrontOnFocus },
	{ FLAGS_ALWAYSVERTICALSCROLLBAR,	ImGuiWindowFlags_AlwaysVerticalScrollbar },
	{ FLAGS_ALWAYSHORIZONTALSCROLLBAR,	ImGuiWindowFlags_AlwaysHorizontalScrollbar },
	{ FLAGS_NONAVINPUTS,				ImGuiWindowFlags_NoNavInputs },
	{ FLAGS_NONAVFOCUS,					ImGuiWindowFlags_NoNavFocus },
	{ FLAGS_UNSAVEDDOCUMENT,			ImGuiWindowFlags_UnsavedDocument },
	{ FLAGS_NODOCKING,					ImGuiWindowFlags_NoDocking },
};

static ImGuiWindowFlags RemapWindowFlags( idImGuiWindowFlags flags )
{
	ImGuiWindowFlags result = ImGuiWindowFlags_None;

	for( const idImGuiWindowFlagRemap& entry : windowFlagRemapTable )
	{
		if( flags & entry.ourFlag )
		{
			result |= entry.imguiFlag;
		}
	}

	return result;
}

/*
===============================================================================

	idImGuiWindow

===============================================================================
*/

void idImGuiWindow::Draw()
{
	bool showTool = IsShown();

	const ImGuiWindowFlags imguiFlags = RemapWindowFlags( GetBaseWindowFlags() );

	if( ImGui::Begin( GetDisplayTitle(), &showTool, imguiFlags ) )
	{
		DrawContents( showTool );
	}
	ImGui::End();

	if( IsShown() && !showTool )
	{
		ShowIt( false );
		OnClosed();
	}
}

/*
===============================================================================

	idImGuiSystemLocal - the ImGui hooks to integrate it into the engine

===============================================================================
*/

class idImGuiSystemLocal : public idImGuiSystem
{
public:
	idImGuiSystemLocal();
	virtual ~idImGuiSystemLocal();

	virtual bool Init( int windowWidth, int windowHeight );
	virtual void Destroy();
	virtual idImGuiEditor* GetEditor();
	virtual void RegisterWindow( idImGuiWindow& window );
	virtual void ReleaseMouse( bool doRelease );
	virtual void NotifyDisplaySizeChanged( int width, int height );
	virtual bool InjectSysEvent( const sysEvent_t* keyEvent );
	virtual bool InjectMouseWheel( int delta );
	virtual void NewFrame();
	virtual bool IsReadyToRender();
	virtual void Render();
	virtual bool IsInitialized() const;
	virtual bool RightMouseActive() const;
	virtual void RegisterDockWindow( const char* windowName, DockRegion region );
	virtual bool UseInput() const;
	virtual bool UseInputForUsercmd() const;
	virtual void DrawWindows();

private:
	void Clear();
	ImGuiKey MapCustomKeyToImGuiKey( keyNum_t keyNum ) const;
	bool HandleKeyEvent( const sysEvent_t& keyEvent );
	const char* GetClipboardText( void* userData );
	void SetClipboardText( void* userData, const char* text );
	bool ShowWindows() const;
	void SetupDefaultDockLayout();

	bool isInitialized;
	double lastFrameTime;
	bool mousePressed[5];
	float mouseWheel;
	ImVec2 mousePos;
	ImVec2 displaySize;
	ImGuiContext* engineContext;
	bool haveNewFrame;
	bool releaseMouse;
	idList<idImGuiWindow*> windows;
	struct DockWindowRequest
	{
		DockWindowRequest();

		idStr name;
		DockRegion region;
	};

	idList<DockWindowRequest> dockWindowRequests;
	bool dockLayoutDirty;
	idImGuiEditorLocal editor;
};

static idImGuiSystemLocal localImGuiSystem;
idImGuiSystem* imguiSystem = &localImGuiSystem;

/*
=================
idImGuiSystemLocal::DockWindowRequest::DockWindowRequest
=================
*/
idImGuiSystemLocal::DockWindowRequest::DockWindowRequest()
	: name( "" ), region( DOCK_REGION_CENTER )
{
}

/*
=================
idImGuiSystemLocal::idImGuiSystemLocal
=================
*/
idImGuiSystemLocal::idImGuiSystemLocal()
	: editor( this )
{
	Clear();
}

/*
=================
idImGuiSystemLocal::~idImGuiSystemLocal
=================
*/
idImGuiSystemLocal::~idImGuiSystemLocal()
{
}

/*
=================
idImGuiSystemLocal::Clear
=================
*/
void idImGuiSystemLocal::Clear()
{
	isInitialized = false;
	lastFrameTime = 0.0f;

	for( int i = 0; i < 5; ++i )
	{
		mousePressed[i] = false;
	}

	mouseWheel = 0.0f;
	mousePos = ImVec2( -1.0f, -1.0f );
	displaySize = ImVec2( 0.0f, 0.0f );
	engineContext = nullptr;
	haveNewFrame = false;
	releaseMouse = false;
	windows.Clear();

	editor.ReleaseMouse( false );
	editor.SetRightMouseActive( false );
	dockWindowRequests.Clear();
	dockLayoutDirty = true;
}

/*
=================
idImGuiSystemLocal::Init
=================
*/
bool idImGuiSystemLocal::Init( int windowWidth, int windowHeight )
{
	if( IsInitialized() )
	{
		Destroy();
	}

	idLib::Printf( "--------- Initializing ImGui ----------\n" );

	IMGUI_CHECKVERSION();
	idLib::Printf( "Version: %s\n", ImGui::GetVersion() );

	engineContext = ImGui::CreateContext();
	ImGui::SetCurrentContext( engineContext );

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	displaySize.x = windowWidth;
	displaySize.y = windowHeight;
	io.DisplaySize = displaySize;
	io.SetClipboardTextFn = []( void* userData, const char* text )
	{
		static_cast<idImGuiSystemLocal*>( userData )->SetClipboardText( userData, text );
	};
	io.GetClipboardTextFn = []( void* userData )
	{
		return static_cast<idImGuiSystemLocal*>( userData )->GetClipboardText( userData );
	};
	io.ClipboardUserData = this;

	ImGui::StyleColorsDark();

	static idStr iniPath;
	iniPath = cvarSystem->GetCVarString( "fs_savepath" );
	iniPath += "/imgui.ini";
	io.IniFilename = iniPath.c_str();

	isInitialized = true;

	idLib::Printf( "imgui initialized.\n" );
	idLib::Printf( "--------------------------------------\n" );

	return true;
}

/*
=================
idImGuiSystemLocal::Destroy
=================
*/
void idImGuiSystemLocal::Destroy()
{
	if( IsInitialized() )
	{
		idLib::Printf( "------------ ImGui Shutdown -----------\n" );

		ImGui::DestroyContext( engineContext );
		engineContext = nullptr;
		isInitialized = false;
		haveNewFrame = false;

		idLib::Printf( "--------------------------------------\n" );
	}
}

/*
=================
idImGuiSystemLocal::GetEditor
=================
*/
idImGuiEditor* idImGuiSystemLocal::GetEditor()
{
	return &editor;
}

/*
=================
idImGuiSystemLocal::RegisterWindow
=================
*/
void idImGuiSystemLocal::RegisterWindow( idImGuiWindow& window )
{
	for( int i = 0; i < windows.Num(); ++i )
	{
		if( windows[i] == &window )
		{
			return;
		}
	}

	windows.Append( &window );

	RegisterDockWindow( window.GetWindowName(), window.GetDockRegion() );
}

/*
=================
idImGuiSystemLocal::ReleaseMouse
=================
*/
void idImGuiSystemLocal::ReleaseMouse( bool doRelease )
{
	releaseMouse = doRelease;
}

/*
=================
idImGuiSystemLocal::HandleKeyEvent

Map custom key codes to ImGui key codes
=================
*/
ImGuiKey idImGuiSystemLocal::MapCustomKeyToImGuiKey( keyNum_t keyNum ) const
{
	switch( keyNum )
	{
		case K_TAB:
			return ImGuiKey_Tab;
		case K_LEFTARROW:
			return ImGuiKey_LeftArrow;
		case K_RIGHTARROW:
			return ImGuiKey_RightArrow;
		case K_UPARROW:
			return ImGuiKey_UpArrow;
		case K_DOWNARROW:
			return ImGuiKey_DownArrow;
		case K_PGUP:
			return ImGuiKey_PageUp;
		case K_PGDN:
			return ImGuiKey_PageDown;
		case K_HOME:
			return ImGuiKey_Home;
		case K_END:
			return ImGuiKey_End;
		case K_DEL:
			return ImGuiKey_Delete;
		case K_BACKSPACE:
			return ImGuiKey_Backspace;
		case K_ENTER:
			return ImGuiKey_Enter;
		case K_ESCAPE:
			return ImGuiKey_Escape;
		case K_LCTRL:
			return ImGuiKey_LeftCtrl;
		case K_RCTRL:
			return ImGuiKey_RightCtrl;
		case K_LSHIFT:
			return ImGuiKey_LeftShift;
		case K_RSHIFT:
			return ImGuiKey_RightShift;
		case K_LALT:
			return ImGuiKey_LeftAlt;
		case K_RALT:
			return ImGuiKey_RightAlt;
		default:
			break;
	}

	// Try to map character keys (A-Z)
	if( keyNum >= K_A && keyNum <= K_Z )
	{
		return ( ImGuiKey )( ImGuiKey_A + ( keyNum - K_A ) );
	}

	// Try to map number keys (0-9)
	if( keyNum >= K_0 && keyNum <= K_9 )
	{
		return ( ImGuiKey )( ImGuiKey_0 + ( keyNum - K_0 ) );
	}

	return ImGuiKey_None;
}

/*
=================
idImGuiSystemLocal::HandleKeyEvent
=================
*/
bool idImGuiSystemLocal::HandleKeyEvent( const sysEvent_t& keyEvent )
{
	assert( keyEvent.evType == SE_KEY );

	keyNum_t keyNum = static_cast<keyNum_t>( keyEvent.evValue );
	bool pressed = keyEvent.evValue2 > 0;

	ImGuiIO& io = ImGui::GetIO();

	if( keyNum == K_MOUSE2 )
	{
		// RB: allow navigation like in a level editor
		mousePressed[1] = pressed;

		imguiSystem->GetEditor()->SetRightMouseActive( pressed );
		imguiSystem->GetEditor()->ReleaseMouse( !pressed );

		//common->Printf( "mouse2 pressed %d\n", int( pressed ) );

		return true;
	}

	if( mousePressed[1] )
	{
		return false;
	}

	if( keyNum < K_JOY1 )
	{
		ImGuiKey imguiKey = MapCustomKeyToImGuiKey( keyNum );
		if( imguiKey != ImGuiKey_None )
		{
			io.AddKeyEvent( imguiKey, pressed );
		}

		// Update modifier keys state
		io.AddKeyEvent( ImGuiKey_LeftCtrl, usercmdGen->KeyState( K_LCTRL ) == 1 );
		io.AddKeyEvent( ImGuiKey_RightCtrl, usercmdGen->KeyState( K_RCTRL ) == 1 );
		io.AddKeyEvent( ImGuiKey_LeftShift, usercmdGen->KeyState( K_LSHIFT ) == 1 );
		io.AddKeyEvent( ImGuiKey_RightShift, usercmdGen->KeyState( K_RSHIFT ) == 1 );
		io.AddKeyEvent( ImGuiKey_LeftAlt, usercmdGen->KeyState( K_LALT ) == 1 );
		io.AddKeyEvent( ImGuiKey_RightAlt, usercmdGen->KeyState( K_RALT ) == 1 );
		io.AddKeyEvent( ImGuiMod_Ctrl, usercmdGen->KeyState( K_LCTRL ) == 1 || usercmdGen->KeyState( K_RCTRL ) == 1 );
		io.AddKeyEvent( ImGuiMod_Shift, usercmdGen->KeyState( K_LSHIFT ) == 1 || usercmdGen->KeyState( K_RSHIFT ) == 1 );
		io.AddKeyEvent( ImGuiMod_Alt, usercmdGen->KeyState( K_LALT ) == 1 || usercmdGen->KeyState( K_RALT ) == 1 );

		return true;
	}
	else if( keyNum >= K_MOUSE1 && keyNum <= K_MOUSE5 )
	{
		int buttonIdx = keyNum - K_MOUSE1;

		// K_MOUSE* are contiguous, so they can be used as indexes into imgui's
		// mousePressed[] - ImGui uses the same order (left, right, middle, X1, X2)
		mousePressed[buttonIdx] = pressed;

		return true; // let's pretend we also handle mouse up events
	}

	return false;
}



/*
=================
idImGuiSystemLocal::GetClipboardText

Sys_GetClipboardData() expects that you Mem_Free() its returned data
ImGui can't do that, of course, so copy it into a static buffer here,
Mem_Free() and return the copy
=================
*/
const char* idImGuiSystemLocal::GetClipboardText( void* )
{
	char* txt = Sys_GetClipboardData();
	if( txt == NULL )
	{
		return NULL;
	}

	static idStr clipboardBuf;
	clipboardBuf = txt;

	Mem_Free( txt );

	return clipboardBuf.c_str();
}

/*
=================
idImGuiSystemLocal::SetClipboardText
=================
*/
void idImGuiSystemLocal::SetClipboardText( void*, const char* text )
{
	Sys_SetClipboardData( text );
}


/*
=================
idImGuiSystemLocal::ShowWindows
=================
*/
bool idImGuiSystemLocal::ShowWindows() const
{
	if( editor.AreEditorsActive() || imgui_showDemoWindow.GetBool() )
	{
		return true;
	}

	for( int i = 0; i < windows.Num(); ++i )
	{
		if( windows[i]->IsShown() )
		{
			return true;
		}
	}

	return false;
}

/*
=================
idImGuiSystemLocal::RegisterDockWindow
=================
*/
void idImGuiSystemLocal::RegisterDockWindow( const char* windowName, DockRegion region )
{
	if( windowName == NULL || windowName[0] == '\0' )
	{
		return;
	}

	if( region == DOCK_REGION_NONE )
	{
		return;
	}

	for( int i = 0; i < dockWindowRequests.Num(); ++i )
	{
		if( dockWindowRequests[i].name == windowName )
		{
			dockWindowRequests[i].region = region;
			dockLayoutDirty = true;
			return;
		}
	}

	DockWindowRequest request;
	request.name = windowName;
	request.region = region;
	dockWindowRequests.Append( request );
	dockLayoutDirty = true;
}

/*
=================
idImGuiSystemLocal::SetupDefaultDockLayout
=================
*/
void idImGuiSystemLocal::SetupDefaultDockLayout()
{
	if( !dockLayoutDirty )
	{
		return;
	}
	if( dockWindowRequests.Num() == 0 )
	{
		return;
	}

	const ImGuiID dockspaceId = ImHashStr( "Kroom3MainDockSpace" );
	if( ImGui::DockBuilderGetNode( dockspaceId ) != NULL )
	{
		ImGui::DockBuilderRemoveNode( dockspaceId );
	}

	ImGui::DockBuilderAddNode( dockspaceId, ImGuiDockNodeFlags_DockSpace );
	ImGui::DockBuilderSetNodeSize( dockspaceId, ImGui::GetMainViewport()->WorkSize );

	ImGuiID rightId = 0;
	ImGuiID centerId = dockspaceId;
	ImGui::DockBuilderSplitNode( dockspaceId, ImGuiDir_Right, 0.30f, &rightId, &centerId );
	ImGuiID bottomId = 0;
	ImGuiID leftId = 0;
	ImGuiID mainId = centerId;
	ImGui::DockBuilderSplitNode( centerId, ImGuiDir_Down, 0.25f, &bottomId, &mainId );
	ImGui::DockBuilderSplitNode( mainId, ImGuiDir_Left, 0.20f, &leftId, &mainId );

	for( int i = 0; i < dockWindowRequests.Num(); ++i )
	{
		ImGuiID regionId = mainId;
		switch( dockWindowRequests[i].region )
		{
			case DOCK_REGION_RIGHT:
				regionId = rightId;
				break;
			case DOCK_REGION_BOTTOM:
				regionId = bottomId;
				break;
			case DOCK_REGION_LEFT:
				regionId = leftId;
				break;
			case DOCK_REGION_CENTER:
				break;
		}
		ImGui::DockBuilderDockWindow( dockWindowRequests[i].name.c_str(), regionId );
	}
	ImGui::DockBuilderFinish( dockspaceId );
	dockLayoutDirty = false;
}

/*
=================
idImGuiSystemLocal::NotifyDisplaySizeChanged
=================
*/
void idImGuiSystemLocal::NotifyDisplaySizeChanged( int width, int height )
{
	if( displaySize.x != width || displaySize.y != height )
	{
		displaySize = ImVec2( ( float )width, ( float )height );

		if( IsInitialized() )
		{
			Destroy();
			Init( width, height );

			// reuse the default ImGui font
			const idMaterial* image = declManager->FindMaterial( "_imguiFont" );

			ImGuiIO& io = ImGui::GetIO();

			byte* pixels = NULL;
			io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );

			io.Fonts->TexID = ( void* )image;
		}
	}
}

/*
=================
idImGuiSystemLocal::UseInputForUsercmd

is a imgui windows requestion input?
=================
*/
bool idImGuiSystemLocal::UseInput() const
{
	return releaseMouse || editor.IsMouseRelease() || imgui_showDemoWindow.GetBool();
}

/*
=================
idImGuiSystemLocal::UseInputForUsercmd
=================
*/
bool idImGuiSystemLocal::UseInputForUsercmd() const
{
	return UseInput() && !editor.IsFreeCameraActive();
}

/*
=================
idImGuiSystemLocal::DrawWindows
=================
*/
void idImGuiSystemLocal::DrawWindows()
{
	for( int i = 0; i < windows.Num(); ++i )
	{
		if( windows[i]->IsShown() )
		{
			windows[i]->Draw();
		}
	}
}

/*
=================
idImGuiSystemLocal::InjectSysEvent

inject a sys event
=================
*/
bool idImGuiSystemLocal::InjectSysEvent( const sysEvent_t* event )
{
	if( !IsInitialized() )
	{
		if( event == NULL )
		{
			assert( 0 ); // I think this shouldn't happen
			return false;
		}
	}

	const sysEvent_t& ev = *event;

	if( ev.evType == SE_KEY && static_cast<keyNum_t>( ev.evValue ) == K_MOUSE2 )
	{
		const bool pressed = ev.evValue2 > 0;
		mousePressed[1] = pressed;
		editor.SetRightMouseActive( pressed );
		editor.ReleaseMouse( !pressed );
		return true;
	}

	if( UseInput() || ( editor.IsFreeCameraActive() && RightMouseActive() ) )
	{
		switch( ev.evType )
		{
			case SE_KEY:
				return HandleKeyEvent( ev );

			case SE_MOUSE_ABSOLUTE:
				mousePos.x = ev.evValue;
				mousePos.y = ev.evValue2;
				return true;

			case SE_CHAR:
				if( ev.evValue < 0x10000 )
				{
					ImGui::GetIO().AddInputCharacter( ev.evValue );
					return true;
				}
				break;

			case SE_MOUSE_LEAVE:
				mousePos = ImVec2( -1.0f, -1.0f );
				return true;

			default:
				break;
		}
	}
	return false;
}

/*
=================
idImGuiSystemLocal::RightMouseActive
=================
*/
bool idImGuiSystemLocal::RightMouseActive() const
{
	return mousePressed[1];
}

/*
=================
idImGuiSystemLocal::InjectMouseWheel
=================
*/
bool idImGuiSystemLocal::InjectMouseWheel( int delta )
{
	if( IsInitialized() && UseInput() && delta != 0 )
	{
		mouseWheel = ( delta > 0 ) ? 1 : -1;
		return true;
	}
	return false;
}

/*
=================
idImGuiSystemLocal::NewFrame
=================
*/
void idImGuiSystemLocal::NewFrame()
{
	if( !haveNewFrame && IsInitialized() && ShowWindows() )
	{
		ImGuiIO& io = ImGui::GetIO();

		// Setup display size (every frame to accommodate for window resizing)
		io.DisplaySize = displaySize;

		// Setup time step
		int	time = Sys_Milliseconds();
		double current_time = time * 0.001;
		io.DeltaTime = lastFrameTime > 0.0 ? ( float )( current_time - lastFrameTime ) : ( float )( 1.0f / 60.0f );

		if( io.DeltaTime <= 0.0F )
		{
			io.DeltaTime = ( 1.0f / 60.0f );
		}

		lastFrameTime = current_time;

		// Setup inputs
		io.MousePos = mousePos;

		// If a mouse press event came, always pass it as "mouse held this frame",
		// so we don't miss click-release events that are shorter than 1 frame.
		for( int i = 0; i < 5; ++i )
		{
			io.MouseDown[i] = mousePressed[i] || usercmdGen->KeyState( K_MOUSE1 + i ) == 1;
		}

		io.MouseWheel = mouseWheel;
		mouseWheel = 0.0f;

		// Hide OS mouse cursor if ImGui is drawing it TODO: hide mousecursor?
		// ShowCursor(io.MouseDrawCursor ? 0 : 1);

		ImGui::GetIO().MouseDrawCursor = UseInput();

		// Start the frame
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		haveNewFrame = true;
	}
}

/*
=================
idImGuiSystemLocal::IsReadyToRender
=================
*/
bool idImGuiSystemLocal::IsReadyToRender()
{
	if( IsInitialized() && ShowWindows() )
	{
		if( !haveNewFrame )
		{
			// for screenshots etc, where we didn't go through idCommonLocal::Frame()
			// before idRenderSystemLocal::SwapCommandBuffers_FinishRendering()
			NewFrame();
		}

		return true;
	}

	return false;
}

/*
=================
idImGuiSystemLocal::Render
=================
*/
void idImGuiSystemLocal::Render()
{
	if( IsInitialized() && ShowWindows() )
	{
		if( !haveNewFrame )
		{
			// for screenshots etc, where we didn't go through idCommonLocal::Frame()
			// before idRenderSystemLocal::SwapCommandBuffers_FinishRendering()
			NewFrame();
		}

		SetupDefaultDockLayout();

		// make dockspace transparent
		static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
		ImGui::DockSpaceOverViewport( ImHashStr( "Kroom3MainDockSpace" ), NULL, dockspaceFlags, NULL );

		DrawWindows();
		editor.DrawWindows();

		if( imgui_showDemoWindow.GetBool() )
		{
			ImGui::ShowDemoWindow();
		}

		//ImGui::End();

		ImGui::Render();
		idRenderBackend::ImGui_RenderDrawLists( ImGui::GetDrawData() );
		haveNewFrame = false;
	}
}

bool idImGuiSystemLocal::IsInitialized() const
{
	// checks if imgui is up and running
	return isInitialized;
}
