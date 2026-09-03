/*
 * ImGui integration into Doom3BFG/OpenTechEngine.
 * Based on ImGui SDL and OpenGL3 examples.
 *  Copyright (c) 2014-2015 Omar Cornut and ImGui contributors
 *
 * Doom3-specific Code (and ImGui::DragXYZ(), based on ImGui::DragFloatN())
 *  Copyright (C) 2015 Daniel Gibson
 *
 * This file is under MIT License, like the original code from ImGui.
 */

#include "precompiled.h"
#pragma hdrstop

#include "BFGimgui.h"
#include "../extern/imgui/imgui_internal.h"
#include "../extern/imguizmo/ImGuizmo.h"
#include "renderer/RenderCommon.h"
#include "renderer/RenderBackend.h"
#include "../tools/imgui/lighteditor/LightEditor.h"

extern idCVar g_editEntityMode;

idCVar imgui_showDemoWindow( "imgui_showDemoWindow", "0", CVAR_GUI | CVAR_BOOL, "show big ImGui demo window" );

// our custom ImGui functions from BFGimgui.h

// like DragFloat3(), but with "X: ", "Y: " or "Z: " prepended to each display_format, for vectors
// if !ignoreLabelWidth, it makes sure the label also fits into the current item width.
//    note that this screws up alignment with consecutive "value+label widgets" (like Drag* or ColorEdit*)
bool ImGui::DragVec3( const char* label, idVec3& v, float v_speed, float v_min, float v_max, const char* display_format, float power, bool ignoreLabelWidth )
{
	bool value_changed = false;
	ImGui::BeginGroup();
	ImGui::PushID( label );

	ImGuiStyle& style = ImGui::GetStyle();
	float wholeWidth = ImGui::CalcItemWidth() - 2.0f * style.ItemSpacing.x;
	float spacing = style.ItemInnerSpacing.x;
	float labelWidth = ignoreLabelWidth ? 0.0f : ( ImGui::CalcTextSize( label, NULL, true ).x + spacing );
	float coordWidth = ( wholeWidth - labelWidth - 2.0f * spacing ) * ( 1.0f / 3.0f ); // width of one x/y/z dragfloat

	ImGui::PushItemWidth( coordWidth );
	for( int i = 0; i < 3; i++ )
	{
		ImGui::PushID( i );
		char format[64];
		idStr::snPrintf( format, sizeof( format ), "%c: %s", "XYZ"[i], display_format );
		value_changed |= ImGui::DragFloat( "##v", &v[i], v_speed, v_min, v_max, format, power );

		ImGui::PopID();
		ImGui::SameLine( 0.0f, spacing );
	}
	ImGui::PopItemWidth();
	ImGui::PopID();

	const char* labelEnd = strstr( label, "##" );
	ImGui::TextUnformatted( label, labelEnd );

	ImGui::EndGroup();

	return value_changed;
}

// shortcut for DragXYZ with ignorLabelWidth = false
// very similar, but adjusts width to width of label to make sure it's not cut off
// sometimes useful, but might not align with consecutive "value+label widgets" (like Drag* or ColorEdit*)
bool ImGui::DragVec3fitLabel( const char* label, idVec3& v, float v_speed, float v_min, float v_max, const char* display_format, float power )
{
	return ImGui::DragVec3( label, v, v_speed, v_min, v_max, display_format, power, false );
}

// the ImGui hooks to integrate it into the engine
class idImGuiSystemLocal : public idImGuiSystem
{
public:
	idImGuiSystemLocal();
	virtual ~idImGuiSystemLocal();

	virtual bool Init( int windowWidth, int windowHeight );
	virtual void Destroy();
	virtual void RegisterWindow( idImGuiWindow& window );
	virtual void InitializeLightEditor( const idDict* dict, idEntity* entity );
	virtual void SetReleaseToolMouse( bool doRelease );
	virtual void NotifyDisplaySizeChanged( int width, int height );
	virtual bool InjectSysEvent( const sysEvent_t* keyEvent );
	virtual bool InjectMouseWheel( int delta );
	virtual void NewFrame();
	virtual bool IsReadyToRender();
	virtual void Render();
	virtual bool IsInitialized() const;
	virtual bool RightMouseActive() const;
	virtual void RegisterDockWindow( const char* windowName, DockRegion region );
	virtual bool AreEditorsActive() const;
	virtual bool ReleaseMouseForTools() const;
	virtual bool IsFreeCameraActive() const;
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

	struct DockWindowRequest
	{
		DockWindowRequest()
		{
			name = "";
			region = DOCK_REGION_CENTER;
		}
		
		idStr name;
		DockRegion region;
	};

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
	idList<DockWindowRequest> dockWindowRequests;
};

static idImGuiSystemLocal localImGuiSystem;
idImGuiSystem* imguiSystem = &localImGuiSystem;

idImGuiSystemLocal::idImGuiSystemLocal()
{
}

idImGuiSystemLocal::~idImGuiSystemLocal()
{
}

void idImGuiSystemLocal::Clear()
{
	isInitialized = false;
	lastFrameTime = 0.0f;

	for (int i = 0; i < 5; ++i)
	{
		mousePressed[i] = false;
	}

	mouseWheel = 0.0f;
	mousePos = ImVec2( -1.0f, -1.0f );
	displaySize = ImVec2( 0.0f, 0.0f );
	engineContext = nullptr;
	haveNewFrame = false;

	windows.Clear();
	dockWindowRequests.Clear();
}

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

void idImGuiSystemLocal::SetReleaseToolMouse( bool doRelease )
{
	releaseMouse = doRelease;
}

bool idImGuiSystemLocal::AreEditorsActive() const
{
	return g_editEntityMode.GetInteger() > 0 || com_editors != 0;
}

bool idImGuiSystemLocal::ReleaseMouseForTools() const
{
	return AreEditorsActive() && releaseMouse && !RightMouseActive();
}

bool idImGuiSystemLocal::IsFreeCameraActive() const
{
	for( int i = 0; i < windows.Num(); ++i )
	{
		if( windows[i]->IsShown() && windows[i]->IsFreeCameraActive() )
		{
			return true;
		}
	}
	return false;
}

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

void idImGuiSystemLocal::InitializeLightEditor( const idDict* dict, idEntity* ent )
{
	if( dict == NULL || ent == NULL )
	{
		return;
	}

	idassert( idStr::Icmp( dict->GetString( "spawnclass" ), "idLight" ) == 0
			  && "InitializeLightEditor() must only be called with light entities or NULL!" );

	LightEditor::Instance().ShowIt( true );
	RegisterWindow( LightEditor::Instance() );
	SetReleaseToolMouse( true );
	gameEdit->PlayerEnableFreeCam( true );
	RegisterDockWindow( "Light Texture Browser", DOCK_REGION_BOTTOM );

	LightEditor::ReInit( dict, ent );
}

// Map custom key codes to ImGui key codes
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

		imguiSystem->SetReleaseToolMouse( !pressed );

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

// Sys_GetClipboardData() expects that you Mem_Free() its returned data
// ImGui can't do that, of course, so copy it into a static buffer here,
// Mem_Free() and return the copy
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

void idImGuiSystemLocal::SetClipboardText( void*, const char* text )
{
	Sys_SetClipboardData( text );
}


bool idImGuiSystemLocal::ShowWindows() const
{
	return ( g_editEntityMode.GetInteger() > 0 || com_editors != 0 || imgui_showDemoWindow.GetBool() );
}

void idImGuiSystemLocal::RegisterDockWindow( const char* windowName, DockRegion region )
{
	if( windowName == NULL || windowName[0] == '\0' )
	{
		return;
	}

	for( int i = 0; i < dockWindowRequests.Num(); ++i )
	{
		if( dockWindowRequests[i].name == windowName )
		{
			dockWindowRequests[i].region = region;
			return;
		}
	}

	DockWindowRequest request;
	request.name = windowName;
	request.region = region;
	dockWindowRequests.Append( request );
}

void idImGuiSystemLocal::SetupDefaultDockLayout()
{
	static bool initialized = false;
	if( initialized )
	{
		return;
	}

	const ImGuiID dockspaceId = ImHashStr( "Kroom3MainDockSpace" );
	if( ImGui::DockBuilderGetNode( dockspaceId ) != NULL )
	{
		initialized = true;
		return;
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
	initialized = true;
}

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

// is a imgui windows requestion input?
bool idImGuiSystemLocal::UseInput() const
{
	return ReleaseMouseForTools() || imgui_showDemoWindow.GetBool();
}

bool idImGuiSystemLocal::UseInputForUsercmd() const
{
	return UseInput() && !IsFreeCameraActive();
}

// inject a sys event
bool idImGuiSystemLocal::InjectSysEvent( const sysEvent_t* event )
{
	if( IsInitialized() && ( UseInput() || RightMouseActive() ) )
	{
		if( event == NULL )
		{
			assert( 0 ); // I think this shouldn't happen
			return false;
		}

		const sysEvent_t& ev = *event;

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

bool idImGuiSystemLocal::RightMouseActive() const
{
	return mousePressed[1];
}

bool idImGuiSystemLocal::InjectMouseWheel( int delta )
{
	if( IsInitialized() && UseInput() && delta != 0 )
	{
		mouseWheel = ( delta > 0 ) ? 1 : -1;
		return true;
	}
	return false;
}

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

		imguiSystem->DrawWindows();

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
