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

#include "win_local.h"
#include "../../renderer/RenderCommon.h"

#include <windowsx.h>

LONG WINAPI MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

static bool s_alttab_disabled;

extern idCVar r_windowX;
extern idCVar r_windowY;
extern idCVar r_windowWidth;
extern idCVar r_windowHeight;

void WIN_Sizing( WORD side, RECT* rect )
{
	if( !renderSystem->IsInitialized() || renderSystem->GetWidth() <= 0 || renderSystem->GetHeight() <= 0 )
	{
		return;
	}

	// restrict to a standard aspect ratio
	int width = rect->right - rect->left;
	int height = rect->bottom - rect->top;

	// Adjust width/height for window decoration
	RECT decoRect = { 0, 0, 0, 0 };
	AdjustWindowRect( &decoRect, WINDOW_STYLE | WS_SYSMENU, FALSE );
	int decoWidth = decoRect.right - decoRect.left;
	int decoHeight = decoRect.bottom - decoRect.top;

	width -= decoWidth;
	height -= decoHeight;

	// Clamp to a minimum size
	if( width < SCREEN_WIDTH / 4 )
	{
		width = SCREEN_WIDTH / 4;
	}
	if( height < SCREEN_HEIGHT / 4 )
	{
		height = SCREEN_HEIGHT / 4;
	}

	const int minWidth = height * 4 / 3;
	const int maxHeight = width * 3 / 4;

	const int maxWidth = height * 16 / 9;
	const int minHeight = width * 9 / 16;

	// Set the new size
	switch( side )
	{
		case WMSZ_LEFT:
			rect->left = rect->right - width - decoWidth;
			rect->bottom = rect->top + idMath::ClampInt( minHeight, maxHeight, height ) + decoHeight;
			break;
		case WMSZ_RIGHT:
			rect->right = rect->left + width + decoWidth;
			rect->bottom = rect->top + idMath::ClampInt( minHeight, maxHeight, height ) + decoHeight;
			break;
		case WMSZ_BOTTOM:
		case WMSZ_BOTTOMRIGHT:
			rect->bottom = rect->top + height + decoHeight;
			rect->right = rect->left + idMath::ClampInt( minWidth, maxWidth, width ) + decoWidth;
			break;
		case WMSZ_TOP:
		case WMSZ_TOPRIGHT:
			rect->top = rect->bottom - height - decoHeight;
			rect->right = rect->left + idMath::ClampInt( minWidth, maxWidth, width ) + decoWidth;
			break;
		case WMSZ_BOTTOMLEFT:
			rect->bottom = rect->top + height + decoHeight;
			rect->left = rect->right - idMath::ClampInt( minWidth, maxWidth, width ) - decoWidth;
			break;
		case WMSZ_TOPLEFT:
			rect->top = rect->bottom - height - decoHeight;
			rect->left = rect->right - idMath::ClampInt( minWidth, maxWidth, width ) - decoWidth;
			break;
	}
}

/*
====================
MainWndProc

main window procedure
====================
*/
LONG WINAPI MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	int key;
	switch( uMsg )
	{
		case WM_WINDOWPOSCHANGED:
			if( renderSystem->IsInitialized() )
			{
				RECT rect;
				if( ::GetClientRect( win32.hWnd, &rect ) )
				{

					if( rect.right > rect.left && rect.bottom > rect.top )
					{
						glConfig.nativeScreenWidth = rect.right - rect.left;
						glConfig.nativeScreenHeight = rect.bottom - rect.top;

						// save the window size in cvars if we aren't fullscreen
						int style = GetWindowLong( hWnd, GWL_STYLE );
						if( ( style & WS_POPUP ) == 0 )
						{
							r_windowWidth.SetInteger( glConfig.nativeScreenWidth );
							r_windowHeight.SetInteger( glConfig.nativeScreenHeight );
						}
					}
				}
			}
			break;
		case WM_MOVE:
		{
			int		xPos, yPos;
			RECT r;

			// save the window origin in cvars if we aren't fullscreen
			int style = GetWindowLong( hWnd, GWL_STYLE );
			if( ( style & WS_POPUP ) == 0 )
			{
				xPos = ( short ) LOWORD( lParam ); // horizontal position
				yPos = ( short ) HIWORD( lParam ); // vertical position

				r.left   = 0;
				r.top    = 0;
				r.right  = 1;
				r.bottom = 1;

				AdjustWindowRect( &r, style, FALSE );

				r_windowX.SetInteger( xPos + r.left );
				r_windowY.SetInteger( yPos + r.top );
			}
			break;
		}
		case WM_CREATE:

			win32.hWnd = hWnd;

			// do the OpenGL setup
			void GLW_WM_CREATE( HWND hWnd );
			GLW_WM_CREATE( hWnd );

			break;

		case WM_DESTROY:
			// let sound and input know about this?
			win32.hWnd = NULL;
			break;

		case WM_CLOSE:
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit" );
			break;

		case WM_ACTIVATE:
			// if we got here because of an alt-tab or maximize,
			// we should activate immediately.  If we are here because
			// the mouse was clicked on a title bar or drag control,
			// don't activate until the mouse button is released
		{
			int	fActive, fMinimized;

			fActive = LOWORD( wParam );
			fMinimized = ( BOOL ) HIWORD( wParam );

			win32.activeApp = ( fActive != WA_INACTIVE );
			if( win32.activeApp )
			{
				idKeyInput::ClearStates();
				com_editorActive = false;
				Sys_GrabMouseCursor( true );
				if( common->IsInitialized() )
				{
					SetCursor( NULL );
				}
			}

			if( fActive == WA_INACTIVE )
			{
				win32.movingWindow = false;
				if( common->IsInitialized() )
				{
					SetCursor( LoadCursor( 0, IDC_ARROW ) );
				}
			}

			// start playing the game sound world
			session->SetPlayingSoundWorld();

			// we do not actually grab or release the mouse here,
			// that will be done next time through the main loop
		}
		break;
		case WM_TIMER:
		{
			if( win32.win_timerUpdate.GetBool() )
			{
				common->Frame();
			}
			break;
		}
		case WM_SYSCOMMAND:
			if( wParam == SC_SCREENSAVE || wParam == SC_KEYMENU )
			{
				return 0;
			}
			break;

		case WM_SYSKEYDOWN:
			if( wParam == 13 )  	// alt-enter toggles full-screen
			{
				cvarSystem->SetCVarBool( "r_fullscreen", !renderSystem->IsFullScreen() );
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
				return 0;
			}
		// fall through for other keys
		case WM_KEYDOWN:
			key = ( ( lParam >> 16 ) & 0xFF ) | ( ( ( lParam >> 24 ) & 1 ) << 7 );
			if( key == K_LCTRL || key == K_LALT || key == K_RCTRL || key == K_RALT )
			{
				// let direct-input handle this because windows sends Alt-Gr
				// as two events (ctrl then alt)
				break;
			}
			// D
			if( key == K_NUMLOCK )
			{
				key = K_PAUSE;
			}
			else if( key == K_PAUSE )
			{
				key = K_NUMLOCK;
			}
			Sys_QueEvent( SE_KEY, key, true, 0, NULL, 0 );

			break;

		case WM_SYSKEYUP:
		case WM_KEYUP:
			key = ( ( lParam >> 16 ) & 0xFF ) | ( ( ( lParam >> 24 ) & 1 ) << 7 );
			if( key == K_PRINTSCREEN )
			{
				// don't queue printscreen keys.  Since windows doesn't send us key
				// down events for this, we handle queueing them with DirectInput
				break;
			}
			else if( key == K_LCTRL || key == K_LALT || key == K_RCTRL || key == K_RALT )
			{
				// let direct-input handle this because windows sends Alt-Gr
				// as two events (ctrl then alt)
				break;
			}
			Sys_QueEvent( SE_KEY, key, false, 0, NULL, 0 );
			break;

		case WM_CHAR:
			// DG: make sure it's an utf-16 non-surrogate character (and thus a valid utf-32 character as well)
			// TODO: will there ever be two messages with surrogate characters that should be combined?
			//       (probably not, some people claim it's actually UCS-2, not UTF-16)
			if( wParam < 0xD800 || wParam > 0xDFFF )
			{
				Sys_QueEvent( SE_CHAR, wParam, 0, 0, NULL, 0 );
			}
			break;

		// DG: support utf-32 input via WM_UNICHAR
		case WM_UNICHAR:
			Sys_QueEvent( SE_CHAR, wParam, 0, 0, NULL, 0 );
			break;
		// DG end

		case WM_NCLBUTTONDOWN:
//			win32.movingWindow = true;
			break;

		case WM_ENTERSIZEMOVE:
			win32.movingWindow = true;
			break;

		case WM_EXITSIZEMOVE:
			win32.movingWindow = false;
			break;

		case WM_SIZING:
			WIN_Sizing( wParam, ( RECT* )lParam );
			break;
		case WM_MOUSEMOVE:
		{
			break;
		}
		case WM_LBUTTONDOWN:
		{
			Sys_QueEvent( SE_KEY, K_MOUSE1, 1, 0, NULL, 0 );
			return 0;
		}
		case WM_LBUTTONUP:
		{
			Sys_QueEvent( SE_KEY, K_MOUSE1, 0, 0, NULL, 0 );
			return 0;
		}
		case WM_RBUTTONDOWN:
		{
			Sys_QueEvent( SE_KEY, K_MOUSE2, 1, 0, NULL, 0 );
			return 0;
		}
		case WM_RBUTTONUP:
		{
			Sys_QueEvent( SE_KEY, K_MOUSE2, 0, 0, NULL, 0 );
			return 0;
		}
		case WM_MBUTTONDOWN:
		{
			Sys_QueEvent( SE_KEY, K_MOUSE3, 1, 0, NULL, 0 );
			return 0;
		}
		case WM_MBUTTONUP:
		{
			Sys_QueEvent( SE_KEY, K_MOUSE3, 0, 0, NULL, 0 );
			return 0;
		}
		case WM_XBUTTONDOWN:
		{
			// RB begin
#if defined(__MINGW32__)
			Sys_QueEvent( SE_KEY, K_MOUSE4, 1, 0, NULL, 0 );
#else
			int button = GET_XBUTTON_WPARAM( wParam );
			if( button == 1 )
			{
				Sys_QueEvent( SE_KEY, K_MOUSE4, 1, 0, NULL, 0 );
			}
			else if( button == 2 )
			{
				Sys_QueEvent( SE_KEY, K_MOUSE5, 1, 0, NULL, 0 );
			}
#endif
			// RB end
			return 0;
		}
		case WM_XBUTTONUP:
		{
			// RB begin
#if defined(__MINGW32__)
			Sys_QueEvent( SE_KEY, K_MOUSE4, 0, 0, NULL, 0 );
#else
			int button = GET_XBUTTON_WPARAM( wParam );
			if( button == 1 )
			{
				Sys_QueEvent( SE_KEY, K_MOUSE4, 0, 0, NULL, 0 );
			}
			else if( button == 2 )
			{
				Sys_QueEvent( SE_KEY, K_MOUSE5, 0, 0, NULL, 0 );
			}
#endif
			// RB end
			return 0;
		}
		case WM_MOUSEWHEEL:
		{
			int delta = GET_WHEEL_DELTA_WPARAM( wParam ) / WHEEL_DELTA;
			int key = delta < 0 ? K_MWHEELDOWN : K_MWHEELUP;
			delta = abs( delta );
			while( delta-- > 0 )
			{
				Sys_QueEvent( SE_KEY, key, true, 0, NULL, 0 );
				Sys_QueEvent( SE_KEY, key, false, 0, NULL, 0 );
			}
			break;
		}
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}
