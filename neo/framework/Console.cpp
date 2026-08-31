/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2017-2024 Robert Beckebans

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
#include "ConsoleHistory.h"
#include "ConsoleWatch.h"
#include "../renderer/RenderCommon.h"
#include "../renderer/Font.h"
#include "Common_local.h"

#define CON_MAX_LINES			4096
#define	NUM_CON_TIMES			4
#define CONSOLE_FIRSTREPEAT		200
#define CONSOLE_REPEAT			100

#define	COMMAND_HISTORY			64

struct overlayText_t
{
	idStr			text;
	idVec4			textColor;
	justify_t		justify;
	int				time;
	bool			showbackground = false;
};

// the console will query the cvar and command systems for
// command completion information

class idConsoleLocal : public idConsole
{
public:
	virtual	void		Init();
	virtual void		LoadGraphics();
	virtual void		Shutdown();
	virtual	bool		ProcessEvent( const sysEvent_t* event, bool forceAccept );
	virtual	bool		Active();
	virtual	void		ClearNotifyLines();
	virtual void		Open();
	virtual	void		Close( bool clearNotify = true );
	virtual	void		Print( const char* text );
	virtual	void		Draw( bool forceFullScreen, bool skipNotifyLines = false );

	virtual void		PrintOverlay( idOverlayHandle& handle, justify_t justify, VERIFY_FORMAT_STRING const char* text, idVec4& textColor, bool showbackground, ... );

	virtual idDebugGraph* 	CreateGraph( int numItems );
	virtual void			DestroyGraph( idDebugGraph* graph );

	void				Dump( const char* toFile );
	void				Clear();
	void				Activate( float fraction );

private:
	void				KeyDownEvent( int key );

	void				Linefeed( int now );
	void				PrintInternal( const char* text );
	void				PumpThreadLines();

	void				PageUp();
	void				PageDown();
	void				Top();
	void				Bottom();

	void				DrawInput();
	void				DrawNotify();
	void				DrawSolidConsole( float frac );

	void				Scroll();
	void				SetDisplayFraction( float frac );
	void				UpdateDisplayFraction();

	void				DrawTextLeftAlign( float x, float& y, const idVec4& textColor, const char* text, ... );
	void				DrawTextRightAlign( float x, float& y, const idVec4& textColor, const char* text, ... );

	float				DrawFPS( float y );
	float				DrawMemoryUsage( float y );

	void				DrawOverlayText( float& leftY, float& rightY, float& centerY );
	void				DrawDebugGraphs();
	void				UpdateScreenLayout();

	//============================

	// allow these constants to be adjusted for HMD
	int					LOCALSAFE_LEFT;
	int					LOCALSAFE_RIGHT;
	int					LOCALSAFE_TOP;
	int					LOCALSAFE_BOTTOM;
	int					LOCALSAFE_WIDTH;
	int					LOCALSAFE_HEIGHT;
	int					LINE_WIDTH;
	int					TOTAL_LINES;

	bool				keyCatching;

	idList< short >		textLines[CON_MAX_LINES];
	int					current;		// line where next message will be printed
	int					lineOffset;		// offset in current logical line for next print
	int					display;		// bottom of console displays this line
	int					lastKeyEvent;	// time of last key event for scroll delay
	int					nextKeyEvent;	// keyboard repeat rate

	float				displayFrac;	// approaches finalFrac at con_speed
	float				finalFrac;		// 0.0 to 1.0 lines of console to display
	int					fracTime;		// time of last displayFrac update

	int					vislines;		// in scanlines

	int					times[NUM_CON_TIMES];	// cls.realtime time the line was generated
	// for transparent notify lines
	idVec4				color;

	idEditField			historyEditLines[COMMAND_HISTORY];

	int					nextHistoryLine;// the last line in the history buffer, not masked
	int					historyLine;	// the line being displayed from history buffer
	// will be <= nextHistoryLine

	idEditField			consoleField;

	idList< overlayText_t >	overlayText;
	idList< idDebugGraph*> debugGraphs;
	idList< idStr >		queuedPrints;
	idSysMutex			printMutex;
	bool				captureWatchText;
	idStr				watchText;

	static idCVar		con_speed;
	static idCVar		con_notifyTime;
	static idCVar		con_fontName;
	static idCVar		con_fontSize;
	static idCVar		con_transparency;
	static idCVar		con_noPrint;
};

static idConsoleLocal localConsole;
idConsole* console = &localConsole;

idCVar idConsoleLocal::con_speed( "con_speed", "3", CVAR_SYSTEM, "speed at which the console moves up and down" );
idCVar idConsoleLocal::con_notifyTime( "con_notifyTime", "3", CVAR_SYSTEM, "time messages are displayed onscreen when console is pulled up" );
idCVar idConsoleLocal::con_fontName( "con_fontName", "Courier", CVAR_SYSTEM | CVAR_ARCHIVE, "font used by the in-game console" );
idCVar idConsoleLocal::con_fontSize( "con_fontSize", "7", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "console character width in virtual pixels", 4.0f, 32.0f );
idCVar idConsoleLocal::con_transparency( "con_transparency", "1.0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "console background opacity", 0.0f, 1.0f );
#ifdef DEBUG
	idCVar idConsoleLocal::con_noPrint( "con_noPrint", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "print on the console but not onscreen when console is pulled up" );
#else
	idCVar idConsoleLocal::con_noPrint( "con_noPrint", "1", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "print on the console but not onscreen when console is pulled up" );
#endif

static idFont* consoleRenderFont = NULL;

static idFont* Con_GetFont()
{
	return consoleRenderFont;
}

// BFG's GUI model always projects a 640x480 surface and stretches it to the
// current mode. Tech 5's console is authored in square screen pixels instead.
// Use a wider logical console at widescreen aspect ratios, then map only X
// back into BFG's virtual surface so glyphs retain the font atlas aspect.
static float Con_LogicalScreenWidth()
{
	const int renderWidth = renderSystem->GetWidth();
	const int renderHeight = renderSystem->GetHeight();
	if( renderWidth <= 0 || renderHeight <= 0 )
	{
		return static_cast<float>( SCREEN_WIDTH );
	}
	return SCREEN_HEIGHT * ( static_cast<float>( renderWidth ) / renderHeight );
}

static float Con_ToVirtualX( float x )
{
	return x * ( SCREEN_WIDTH / Con_LogicalScreenWidth() );
}

static float Con_ToVirtualWidth( float width )
{
	return width * ( SCREEN_WIDTH / Con_LogicalScreenWidth() );
}

static void Con_DrawConsoleFilled( const idVec4& color, float x, float y, float width, float height )
{
	renderSystem->DrawFilled( color, Con_ToVirtualX( x ), y, Con_ToVirtualWidth( width ), height );
}

float Con_ConsoleCharWidth()
{
	return cvarSystem->GetCVarFloat( "con_fontSize" );
}

static float Con_FontScale()
{
	idFont* font = Con_GetFont();
	const float naturalWidth = font != NULL ? font->GetGlyphWidth( 1.0f, 'M' ) : 0.0f;
	return naturalWidth > 0.0f ? Con_ConsoleCharWidth() / naturalWidth : 1.0f;
}

float Con_ConsoleLineHeight()
{
	idFont* font = Con_GetFont();
	const float height = font != NULL ? font->GetLineHeight( Con_FontScale() ) : 0.0f;
	return Max( Con_ConsoleCharWidth() + 2.0f, height );
}

float Con_ConsoleStringWidth( const char* text )
{
	if( text == NULL )
	{
		return 0.0f;
	}
	int column = 0;
	int maximum = 0;
	while( *text != '\0' )
	{
		if( idStr::IsColor( text ) )
		{
			text += 2;
			continue;
		}
		if( *text++ == '\n' )
		{
			maximum = Max( maximum, column );
			column = 0;
		}
		else
		{
			++column;
		}
	}
	return Max( maximum, column ) * Con_ConsoleCharWidth();
}

void Con_DrawConsoleChar( float x, float y, uint32 character, const idVec4& color )
{
	if( character == ' ' )
	{
		return;
	}
	idFont* font = Con_GetFont();
	if( font == NULL )
	{
		return;
	}
	const float scale = Con_FontScale();
	scaledGlyphInfo_t glyph;
	font->GetScaledGlyph( scale, character, glyph );
	if( glyph.material == NULL )
	{
		return;
	}
	const float cellOffset = 0.5f * ( Con_ConsoleCharWidth() - glyph.xSkip );
	const float baseline = y + font->GetAscender( scale );
	renderSystem->SetColor( color );
	renderSystem->DrawStretchPic( Con_ToVirtualX( x + cellOffset + glyph.left ), baseline - glyph.top,
								  Con_ToVirtualWidth( glyph.width + 1.0f ), glyph.height + 1.0f,
								  glyph.s1, glyph.t1, glyph.s2, glyph.t2, glyph.material );
}

void Con_DrawConsoleString( float x, float y, const char* text, const idVec4& defaultColor, bool forceColor )
{
	if( text == NULL )
	{
		return;
	}
	const float startX = x;
	idVec4 color = defaultColor;
	while( *text != '\0' )
	{
		if( idStr::IsColor( text ) )
		{
			if( !forceColor )
			{
				color = text[1] == C_COLOR_DEFAULT ? defaultColor : idStr::ColorForIndex( text[1] );
				color.w = defaultColor.w;
			}
			text += 2;
			continue;
		}
		const unsigned char character = static_cast<unsigned char>( *text++ );
		if( character == '\n' )
		{
			x = startX;
			y += Con_ConsoleLineHeight();
			continue;
		}
		Con_DrawConsoleChar( x, y, character, color );
		x += Con_ConsoleCharWidth();
	}
	renderSystem->SetColor( colorWhite );
}

/*
=============================================================================

	Misc stats

=============================================================================
*/

/*
==================
idConsoleLocal::DrawTextLeftAlign
==================
*/
void idConsoleLocal::DrawTextLeftAlign( float x, float& y, const idVec4& textColor, const char* text, ... )
{
	char string[MAX_STRING_CHARS];
	va_list argptr;
	va_start( argptr, text );
	idStr::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );
	Con_DrawConsoleString( x, y + 2, string, textColor, true );
	y += Con_ConsoleLineHeight() + 4.0f;
}

/*
==================
idConsoleLocal::DrawTextRightAlign
==================
*/
void idConsoleLocal::DrawTextRightAlign( float x, float& y, const idVec4& textColor, const char* text, ... )
{
	char string[MAX_STRING_CHARS];
	va_list argptr;
	va_start( argptr, text );
	idStr::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );
	Con_DrawConsoleString( x - Con_ConsoleStringWidth( string ) - 22, y + 2, string, textColor, true );
	y += Con_ConsoleLineHeight() + 4.0f;
}

/*
==================
idConsoleLocal::DrawFPS
==================
*/
#define	FPS_FRAMES	6
#define FPS_FRAMES_HISTORY 90
float idConsoleLocal::DrawFPS( float y )
{
	extern idCVar com_smp;

	static float previousTimes[FPS_FRAMES];
	static float previousTimesNormalized[FPS_FRAMES_HISTORY];
	static int index;
	static int previous;
	static int valuesOffset = 0;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	int t = Sys_Milliseconds();
	int frameTime = t - previous;
	previous = t;

	int fps = 0;

	const float milliSecondsPerFrame = 1000.0f / com_engineHz_latched;

	previousTimes[index % FPS_FRAMES] = frameTime;
	previousTimesNormalized[index % FPS_FRAMES_HISTORY] = frameTime / milliSecondsPerFrame;
	valuesOffset = ( valuesOffset + 1 ) % FPS_FRAMES_HISTORY;
	index++;
	if( index > FPS_FRAMES )
	{
		// average multiple frames together to smooth changes out a bit
		int total = 0;
		for( int i = 0 ; i < FPS_FRAMES ; i++ )
		{
			total += previousTimes[i];
		}
		if( !total )
		{
			total = 1;
		}
		fps = 1000000 * FPS_FRAMES / total;
		fps = ( fps + 500 ) / 1000;

		idStr s = va( "%ifps", fps );
		int w = strlen( s ) * BIGCHAR_WIDTH;

		if( com_showFPS.GetInteger() == 1 )
		{
			CREATE_OVERLAY( fps1, s, JUSTIFY_RIGHT, colorWhite, false );
		}
	}

	y += Con_ConsoleLineHeight() + 4.0f;

	// DG: "com_showFPS 1" means: show FPS only, like in classic doom3
	if( com_showFPS.GetInteger() == 1 )
	{
		return y;
	}
	// DG end

	// SRS - Shouldn't use these getters since they access and return int-sized variables measured in milliseconds
	//const uint64 gameThreadTotalTime = commonLocal.GetGameThreadTotalTime();
	//const uint64 gameThreadGameTime = commonLocal.GetGameThreadGameTime();
	//const uint64 gameThreadRenderTime = commonLocal.GetGameThreadRenderTime();

	const uint64 gameThreadTotalTime	= commonLocal.mainFrameTiming.finishDrawTime - commonLocal.mainFrameTiming.startGameTime;
	const uint64 gameThreadGameTime		= commonLocal.mainFrameTiming.finishGameTime - commonLocal.mainFrameTiming.startGameTime;
	const uint64 gameThreadRenderTime	= commonLocal.mainFrameTiming.finishDrawTime - commonLocal.mainFrameTiming.finishGameTime;

	const uint64 rendererBackEndTime = commonLocal.GetRendererBackEndMicroseconds();
	const uint64 rendererMaskedOcclusionCullingTime = commonLocal.GetRendererMaskedOcclusionRasterizationMicroseconds();
	// SRS - GPU idle time calculation depends on whether game is operating in smp mode or not
	const uint64 rendererGPUIdleTime = commonLocal.GetRendererIdleMicroseconds() - ( com_smp.GetInteger() > 0 && com_editors == 0 ? 0 : gameThreadTotalTime );
	const uint64 rendererGPUTime = commonLocal.GetRendererGPUMicroseconds();
	const uint64 rendererGPUEarlyZTime = commonLocal.GetRendererGpuEarlyZMicroseconds();
	const uint64 rendererGPU_SSAOTime = commonLocal.GetRendererGpuSSAOMicroseconds();
	const uint64 rendererGPU_SSRTime = commonLocal.GetRendererGpuSSRMicroseconds();
	const uint64 rendererGPUAmbientPassTime = commonLocal.GetRendererGpuAmbientPassMicroseconds();
	const uint64 rendererGPUInteractionsTime = commonLocal.GetRendererGpuInteractionsMicroseconds();
	const uint64 rendererGPUShaderPassesTime = commonLocal.GetRendererGpuShaderPassMicroseconds();
	const uint64 rendererGPUPostProcessingTime = commonLocal.GetRendererGpuPostProcessingMicroseconds();
	const int maxTime = int( 1000 / com_engineHz_latched ) * 1000;

	// SRS - Get GPU sync time at the start of a frame (com_smp = 1 or 0) and at the end of a frame (com_smp = -1)
	const uint64 rendererStartFrameSyncTime = commonLocal.GetRendererStartFrameSyncMicroseconds();
	const uint64 rendererEndFrameSyncTime = commonLocal.GetRendererEndFrameSyncMicroseconds();

	// SRS - Total CPU and Frame time calculations depend on whether game is operating in smp mode or not
	const uint64 totalCPUTime = ( com_smp.GetInteger() > 0 && com_editors == 0 ? Max( gameThreadTotalTime, rendererBackEndTime ) : gameThreadTotalTime + rendererBackEndTime );
	const uint64 totalFrameTime = ( com_smp.GetInteger() > 0 && com_editors == 0 ? Max( gameThreadTotalTime, rendererEndFrameSyncTime ) : gameThreadTotalTime + rendererEndFrameSyncTime ) + rendererStartFrameSyncTime;

#if defined( USE_VULKAN )
	const char* API = "Vulkan";
#else
	const char* API = "OpenGL";
#endif

	extern idCVar r_antiAliasing;
	static const int aaNumValues = 5;
	static const char* aaValues[aaNumValues] =
	{
		"None",
		"SMAA 1X",
		"MSAA 2X",
		"MSAA 4X",
		"MSAA 8X"
	};

	compile_time_assert( aaNumValues == ( ANTI_ALIASING_MSAA_8X + 1 ) );

	const char* aaMode = aaValues[ r_antiAliasing.GetInteger() ];

	int width = renderSystem->GetWidth();
	int height = renderSystem->GetHeight();

	idStr timeStr;
	timeStr.Format( "===========  Performance Stats ===========" );
	CREATE_OVERLAY( gperfstats, timeStr, JUSTIFY_RIGHT, colorWhite, false );

	timeStr.Format( "API: %s, AA[%i, %i]: %s", API, width, height, aaMode );
	CREATE_OVERLAY( ggeneral, timeStr, JUSTIFY_RIGHT, colorCyan, false );

	if( com_showFPS.GetInteger() > 2 )
	{
		timeStr.Format( "Average FPS %i", fps );
		CREATE_OVERLAY( gfpsRelative, timeStr, JUSTIFY_RIGHT, colorWhite, false );

		timeStr.Format( "Relative - Frametime ms %f", previousTimesNormalized );
		CREATE_OVERLAY( gfpsRelativeMS, timeStr, JUSTIFY_RIGHT, colorWhite, false );
	}
	else
	{
		timeStr.Format( "Average FPS %i", fps );
		CREATE_OVERLAY( gfps, timeStr, JUSTIFY_RIGHT, fps < com_engineHz_latched ? colorRed : colorYellow, false );
	}

	timeStr.Format( "GENERAL: views:%i draws:%i tris:%i (shdw:%i)",
					commonLocal.stats_frontend.c_numViews,
					commonLocal.stats_backend.c_drawElements + commonLocal.stats_backend.c_shadowElements,
					( commonLocal.stats_backend.c_drawIndexes + commonLocal.stats_backend.c_shadowIndexes ) / 3,
					commonLocal.stats_backend.c_shadowIndexes / 3 );
	CREATE_OVERLAY( gviews, timeStr, JUSTIFY_RIGHT, colorCyan, false );

	if( com_showFPS.GetInteger() > 2 )
	{
		timeStr.Format( "DYNAMIC: callback:%-2i md5:%i dfrmVerts:%i dfrmTris:%i tangTris:%i guis:%i",
						commonLocal.stats_frontend.c_entityDefCallbacks,
						commonLocal.stats_frontend.c_generateMd5,
						commonLocal.stats_frontend.c_deformedVerts,
						commonLocal.stats_frontend.c_deformedIndexes / 3,
						commonLocal.stats_frontend.c_tangentIndexes / 3,
						commonLocal.stats_frontend.c_guiSurfs );
		CREATE_OVERLAY( gdyncallbacks, timeStr, JUSTIFY_RIGHT, colorLtGrey, false );


		timeStr.Format( "MASKCULL: tests:%-3i lightCulls:%i surfCulls:%i verts:%i tris:%i",
						commonLocal.stats_frontend.c_mocTests,
						commonLocal.stats_frontend.c_mocCulledLights,
						commonLocal.stats_frontend.c_mocCulledSurfaces,
						commonLocal.stats_frontend.c_mocVerts,
						commonLocal.stats_frontend.c_mocIndexes );
		CREATE_OVERLAY( gmaskcull, timeStr, JUSTIFY_RIGHT, colorLtGrey, false );

		timeStr.Format( "ADDMODEL: callback:%-2i createInteractions:%i createShadowVolumes:%i",
						commonLocal.stats_frontend.c_entityDefCallbacks,
						commonLocal.stats_frontend.c_createInteractions,
						commonLocal.stats_frontend.c_createShadowVolumes );
		CREATE_OVERLAY( gaddModel, timeStr, JUSTIFY_RIGHT, colorLtGrey, false );

		timeStr.Format( "viewEntities:%-3i  shadowEntities:%-3i  viewLights:%i\n",	commonLocal.stats_frontend.c_visibleViewEntities,
						commonLocal.stats_frontend.c_shadowViewEntities,
						commonLocal.stats_frontend.c_viewLights );
		CREATE_OVERLAY( gviewEnts, timeStr, JUSTIFY_RIGHT, colorLtGrey, false );

		timeStr.Format( "UPDATES: entityUpdates:%-3i  entityRefs:%-3i  lightUpdates:%-2i  lightRefs:%i\n",
						commonLocal.stats_frontend.c_entityUpdates, commonLocal.stats_frontend.c_entityReferences,
						commonLocal.stats_frontend.c_lightUpdates, commonLocal.stats_frontend.c_lightReferences );
		CREATE_OVERLAY( gEntsupdates, timeStr, JUSTIFY_RIGHT, colorLtGrey, false );
	}

	timeStr.Format( "==============  CPU/GPU ==================" );
	CREATE_OVERLAY( gcpugpu, timeStr, JUSTIFY_RIGHT, colorWhite, false );

	timeStr.Format( "Game+RF: %5llu us   EarlyZ:       %5llu us", gameThreadTotalTime, rendererGPUEarlyZTime );
	CREATE_OVERLAY( grf, timeStr, JUSTIFY_RIGHT, gameThreadTotalTime > maxTime ? colorRed : colorWhite, false );

	timeStr.Format( "Game:    %5llu us   SSAO:         %5llu us", gameThreadGameTime, rendererGPU_SSAOTime );
	CREATE_OVERLAY( gthread, timeStr, JUSTIFY_RIGHT, gameThreadGameTime > maxTime ? colorRed : colorWhite, false );

	timeStr.Format( "RF:      %5llu us   SSR:          %5llu us", gameThreadRenderTime, rendererGPU_SSRTime );
	CREATE_OVERLAY( gthreadrf, timeStr, JUSTIFY_RIGHT, gameThreadRenderTime > maxTime ? colorRed : colorWhite, false );

	timeStr.Format( "RB:      %5llu us   AmbientPass:  %5llu us", rendererBackEndTime, rendererGPUAmbientPassTime );
	CREATE_OVERLAY( rb, timeStr, JUSTIFY_RIGHT, rendererBackEndTime > maxTime ? colorRed : colorWhite, false );

	timeStr.Format( "MOC: %5llu us   Interactions: %5llu us", rendererMaskedOcclusionCullingTime, rendererGPUInteractionsTime );
	CREATE_OVERLAY( rbsv, timeStr, JUSTIFY_RIGHT, rendererMaskedOcclusionCullingTime > maxTime ? colorRed : colorWhite, false );

	timeStr.Format( "                    ShaderPass:   %5llu us", rendererGPUShaderPassesTime );
	CREATE_OVERLAY( rbgpuShader, timeStr, JUSTIFY_RIGHT, rendererGPUShaderPassesTime > maxTime ? colorRed : colorWhite, false );

	timeStr.Format( "                    PostFX:       %5llu us", rendererGPUPostProcessingTime );
	CREATE_OVERLAY( rbgpuPostFX, timeStr, JUSTIFY_RIGHT, rendererGPUPostProcessingTime > maxTime ? colorRed : colorWhite, false );

	timeStr.Format( "Total:   %5llu us   Total:        %5llu us", totalCPUTime, rendererGPUTime );
	CREATE_OVERLAY( rbgtotal, timeStr, JUSTIFY_RIGHT, ( totalCPUTime > maxTime || rendererGPUTime > maxTime ) ? colorRed : colorWhite, false );

	timeStr.Format( "Frame:   %5llu us   Idle:         %5llu us", totalFrameTime, rendererGPUIdleTime );
	CREATE_OVERLAY( rbgframetime, timeStr, JUSTIFY_RIGHT, totalFrameTime > maxTime ? colorRed : colorWhite, false );

	return y + Con_ConsoleLineHeight() + 4.0f;
}

/*
==================
idConsoleLocal::DrawMemoryUsage
==================
*/
float idConsoleLocal::DrawMemoryUsage( float y )
{
	return y;
}

//=========================================================================

/*
==============
Con_Clear_f
==============
*/
static void Con_Clear_f( const idCmdArgs& args )
{
	localConsole.Clear();
}

/*
==============
Con_Dump_f
==============
*/
static void Con_Dump_f( const idCmdArgs& args )
{
	if( args.Argc() != 2 )
	{
		common->Printf( "usage: conDump <filename>\n" );
		return;
	}

	idStr fileName = args.Argv( 1 );
	fileName.DefaultFileExtension( ".txt" );

	common->Printf( "Dumped console text to %s.\n", fileName.c_str() );

	localConsole.Dump( fileName.c_str() );
}

/*
==============
Con_Print_f
==============
*/
static void Con_Print_f( const idCmdArgs& args )
{
	common->Printf( "%s\n", args.Args( 1 ) );
}

/*
==============
Con_Activate_f
==============
*/
static void Con_Activate_f( const idCmdArgs& args )
{
	const float fraction = args.Argc() > 1 ? atof( args.Argv( 1 ) ) : 0.5f;
	localConsole.Activate( idMath::ClampFloat( 0.05f, 1.0f, fraction ) );
}

/*
==============
idConsoleLocal::Init
==============
*/
void idConsoleLocal::Init()
{
	keyCatching = false;
	captureWatchText = false;
	watchText.Clear();

	UpdateScreenLayout();

	LINE_WIDTH = Max( 1, idMath::Ftoi( LOCALSAFE_WIDTH / Con_ConsoleCharWidth() ) - 2 );
	TOTAL_LINES = CON_MAX_LINES;

	lastKeyEvent = -1;
	nextKeyEvent = CONSOLE_FIRSTREPEAT;

	consoleField.Clear();
	consoleField.SetWidthInChars( LINE_WIDTH );

	for( int i = 0 ; i < COMMAND_HISTORY ; i++ )
	{
		historyEditLines[i].Clear();
		historyEditLines[i].SetWidthInChars( LINE_WIDTH );
	}

	cmdSystem->AddCommand( "clear", Con_Clear_f, CMD_FL_SYSTEM, "clears the console" );
	cmdSystem->AddCommand( "conDump", Con_Dump_f, CMD_FL_SYSTEM, "dumps the console text to a file" );
	cmdSystem->AddCommand( "print", Con_Print_f, CMD_FL_SYSTEM, "prints text to the console" );
	cmdSystem->AddCommand( "activateConsole", Con_Activate_f, CMD_FL_SYSTEM, "opens the console to an optional screen fraction" );
	ConsoleWatch_Init();
	Clear();
}
/*
==============
idConsoleLocal::UpdateScreenLayout
==============
*/
void idConsoleLocal::UpdateScreenLayout()
{
	// Unlike game HUDs, the Tech 5 console does not use title-safe gutters.
	LOCALSAFE_LEFT = 0;
	LOCALSAFE_RIGHT = idMath::Ftoi( Con_LogicalScreenWidth() );
	LOCALSAFE_TOP = 0;
	LOCALSAFE_BOTTOM = SCREEN_HEIGHT;
	LOCALSAFE_WIDTH = LOCALSAFE_RIGHT - LOCALSAFE_LEFT;
	LOCALSAFE_HEIGHT = LOCALSAFE_BOTTOM - LOCALSAFE_TOP;
}

/*
==============
idConsoleLocal::LoadGraphics

Font construction resolves its atlas through the decl manager. This must be
done on the main thread before the game/draw worker starts.
==============
*/
void idConsoleLocal::LoadGraphics()
{
	consoleRenderFont = renderSystem->RegisterFont( con_fontName.GetString() );
}

/*
==============
idConsoleLocal::Shutdown
==============
*/
void idConsoleLocal::Shutdown()
{
	cmdSystem->RemoveCommand( "clear" );
	cmdSystem->RemoveCommand( "conDump" );
	cmdSystem->RemoveCommand( "print" );
	cmdSystem->RemoveCommand( "activateConsole" );
	ConsoleWatch_Shutdown();

	debugGraphs.DeleteContents( true );
}

/*
================
idConsoleLocal::Active
================
*/
bool	idConsoleLocal::Active()
{
	return keyCatching;
}

/*
================
idConsoleLocal::ClearNotifyLines
================
*/
void	idConsoleLocal::ClearNotifyLines()
{
	int		i;

	for( i = 0 ; i < NUM_CON_TIMES ; i++ )
	{
		times[i] = 0;
	}
}

/*
================
idConsoleLocal::Open
================
*/
void	idConsoleLocal::Open()
{
	if( keyCatching )
	{
		return; // already open
	}

	consoleField.ClearAutoComplete();
	consoleField.Clear();
	keyCatching = true;
	SetDisplayFraction( 0.5f );
}

/*
================
idConsoleLocal::Close
================
*/
void	idConsoleLocal::Close( bool clearNotify )
{
	keyCatching = false;
	SetDisplayFraction( 0 );
	displayFrac = 0;	// don't scroll to that point, go immediately
	if( clearNotify )
	{
		ClearNotifyLines();
	}
}

/*
================
idConsoleLocal::Activate
================
*/
void idConsoleLocal::Activate( float fraction )
{
	keyCatching = true;
	SetDisplayFraction( fraction );
}

/*
================
idConsoleLocal::Clear
================
*/
void idConsoleLocal::Clear()
{
	for( int i = 0; i < CON_MAX_LINES; ++i )
	{
		textLines[i].Clear();
	}
	current = 0;
	display = 0;
	lineOffset = 0;
	ClearNotifyLines();
}

/*
================
idConsoleLocal::Dump

Save the console contents out to a file
================
*/
void idConsoleLocal::Dump( const char* fileName )
{
	PumpThreadLines();
	idFile* f = fileSystem->OpenFileWrite( fileName );
	if( !f )
	{
		common->Warning( "couldn't open %s", fileName );
		return;
	}
	const int firstLine = Max( 0, current - CON_MAX_LINES + 1 );
	for( int lineNumber = firstLine; lineNumber <= current; ++lineNumber )
	{
		const idList< short >& line = textLines[lineNumber & ( CON_MAX_LINES - 1 )];
		idStr plain;
		for( int i = 0; i < line.Num(); ++i )
		{
			plain.Append( static_cast<char>( line[i] & 0xff ) );
		}
		plain.StripTrailingWhitespace();
		f->Printf( "%s\r\n", plain.c_str() );
	}
	fileSystem->CloseFile( f );
}

/*
================
idConsoleLocal::PageUp
================
*/
void idConsoleLocal::PageUp()
{
	display -= 2;
	if( current - display >= TOTAL_LINES )
	{
		display = current - TOTAL_LINES + 1;
	}
}

/*
================
idConsoleLocal::PageDown
================
*/
void idConsoleLocal::PageDown()
{
	display += 2;
	if( display > current )
	{
		display = current;
	}
}

/*
================
idConsoleLocal::Top
================
*/
void idConsoleLocal::Top()
{
	display = Max( 0, current - CON_MAX_LINES + 1 );
}

/*
================
idConsoleLocal::Bottom
================
*/
void idConsoleLocal::Bottom()
{
	display = current;
}


/*
=============================================================================

CONSOLE LINE EDITING

==============================================================================
*/

/*
====================
KeyDownEvent

Handles history and console scrollback
====================
*/
void idConsoleLocal::KeyDownEvent( int key )
{

	// Execute F key bindings
	if( key >= K_F1 && key <= K_F12 )
	{
		idKeyInput::ExecKeyBinding( key );
		return;
	}

	// ctrl-L clears screen
	if( key == K_L && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) )
	{
		Clear();
		return;
	}

	// enter finishes the line
	if( key == K_ENTER || key == K_KP_ENTER ||
			( key == K_M && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) ) )
	{
		if( consoleField.AcceptAutoComplete() )
		{
			return;
		}

		common->Printf( "]%s\n", consoleField.GetBuffer() );

		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, consoleField.GetBuffer() );	// valid command
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "\n" );

		// copy line to history buffer

		if( consoleField.GetBuffer()[ 0 ] != '\n' && consoleField.GetBuffer()[ 0 ] != '\0' )
		{
			consoleHistory.AddToHistory( consoleField.GetBuffer() );
		}

		consoleField.Clear();
		consoleField.SetWidthInChars( LINE_WIDTH );

		const bool captureToImage = false;
		common->UpdateScreen( captureToImage );// force an update, because the command
		// may take some time
		return;
	}

	// command completion

	if( key == K_TAB )
	{
		consoleField.AutoComplete( idKeyInput::IsDown( K_LSHIFT ) || idKeyInput::IsDown( K_RSHIFT ) );
		return;
	}

	// command history (ctrl-p ctrl-n for unix style)

	if( ( key == K_UPARROW ) ||
			( key == K_P && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) ) )
	{
		idStr hist = consoleHistory.RetrieveFromHistory( true );
		if( !hist.IsEmpty() )
		{
			consoleField.SetBuffer( hist );
		}
		return;
	}

	if( ( key == K_DOWNARROW ) ||
			( key == K_N && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) ) )
	{
		idStr hist = consoleHistory.RetrieveFromHistory( false );
		if( !hist.IsEmpty() )
		{
			consoleField.SetBuffer( hist );
		}
		else // DG: if no more lines are in the history, show a blank line again
		{
			consoleField.Clear();
		} // DG end

		return;
	}

	// console scrolling
	if( key == K_PGUP )
	{
		PageUp();
		lastKeyEvent = eventLoop->Milliseconds();
		nextKeyEvent = CONSOLE_FIRSTREPEAT;
		return;
	}

	if( key == K_PGDN )
	{
		PageDown();
		lastKeyEvent = eventLoop->Milliseconds();
		nextKeyEvent = CONSOLE_FIRSTREPEAT;
		return;
	}

	if( key == K_MWHEELUP )
	{
		PageUp();
		return;
	}

	if( key == K_MWHEELDOWN )
	{
		PageDown();
		return;
	}

	// ctrl-home = top of console
	if( key == K_HOME && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) )
	{
		Top();
		return;
	}

	// ctrl-end = bottom of console
	if( key == K_END && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) )
	{
		Bottom();
		return;
	}

	// pass to the normal editline routine
	consoleField.KeyDownEvent( key );
}

/*
==============
Scroll
deals with scrolling text because we don't have key repeat
==============
*/
void idConsoleLocal::Scroll( )
{
	if( lastKeyEvent == -1 || ( lastKeyEvent + 200 ) > eventLoop->Milliseconds() )
	{
		return;
	}
	// console scrolling
	if( idKeyInput::IsDown( K_PGUP ) )
	{
		PageUp();
		nextKeyEvent = CONSOLE_REPEAT;
		return;
	}

	if( idKeyInput::IsDown( K_PGDN ) )
	{
		PageDown();
		nextKeyEvent = CONSOLE_REPEAT;
		return;
	}
}

/*
==============
SetDisplayFraction

Causes the console to start opening the desired amount.
==============
*/
void idConsoleLocal::SetDisplayFraction( float frac )
{
	finalFrac = frac;
	fracTime = Sys_Milliseconds();
}

/*
==============
UpdateDisplayFraction

Scrolls the console up or down based on conspeed
==============
*/
void idConsoleLocal::UpdateDisplayFraction()
{
	if( con_speed.GetFloat() <= 0.1f )
	{
		fracTime = Sys_Milliseconds();
		displayFrac = finalFrac;
		return;
	}

	// scroll towards the destination height
	if( finalFrac < displayFrac )
	{
		displayFrac -= con_speed.GetFloat() * ( Sys_Milliseconds() - fracTime ) * 0.001f;
		if( finalFrac > displayFrac )
		{
			displayFrac = finalFrac;
		}
		fracTime = Sys_Milliseconds();
	}
	else if( finalFrac > displayFrac )
	{
		displayFrac += con_speed.GetFloat() * ( Sys_Milliseconds() - fracTime ) * 0.001f;
		if( finalFrac < displayFrac )
		{
			displayFrac = finalFrac;
		}
		fracTime = Sys_Milliseconds();
	}
}

/*
==============
ProcessEvent
==============
*/
bool	idConsoleLocal::ProcessEvent( const sysEvent_t* event, bool forceAccept )
{
	const bool consoleKey = event->evType == SE_KEY && event->evValue == K_GRAVE && com_allowConsole.GetBool();

	// we always catch the console key event
	if( !forceAccept && consoleKey )
	{
		// ignore up events
		if( event->evValue2 == 0 )
		{
			return true;
		}

		consoleField.ClearAutoComplete();

		// a down event will toggle the destination lines
		if( keyCatching )
		{
			Close();
			Sys_GrabMouseCursor( true );
		}
		else
		{
			consoleField.Clear();
			keyCatching = true;
			if( idKeyInput::IsDown( K_LSHIFT ) || idKeyInput::IsDown( K_RSHIFT ) )
			{
				// if the shift key is down, don't open the console as much
				SetDisplayFraction( 0.2f );
			}
			else
			{
				SetDisplayFraction( 0.5f );
			}
		}
		return true;
	}

	// if we aren't key catching, dump all the other events
	if( !forceAccept && !keyCatching )
	{
		return false;
	}

	// handle key and character events
	if( event->evType == SE_CHAR )
	{
		// never send the console key as a character
		if( event->evValue != '`' && event->evValue != '~' )
		{
			consoleField.CharEvent( event->evValue );
		}
		return true;
	}

	if( event->evType == SE_KEY )
	{
		// ignore up key events
		if( event->evValue2 == 0 )
		{
			return true;
		}

		KeyDownEvent( event->evValue );
		return true;
	}

	// we don't handle things like mouse, joystick, and network packets
	return false;
}

/*
==============================================================================

PRINTING

==============================================================================
*/

/*
===============
Linefeed
===============
*/
void idConsoleLocal::Linefeed( int now )
{
	times[current & ( NUM_CON_TIMES - 1 )] = now;
	if( display == current )
	{
		++display;
	}
	++current;
	lineOffset = 0;
	textLines[current & ( CON_MAX_LINES - 1 )].Clear();
	times[current & ( NUM_CON_TIMES - 1 )] = now;
}


/*
================
PumpThreadLines
================
*/
void idConsoleLocal::PumpThreadLines()
{
	if( !idLib::IsMainThread() )
	{
		return;
	}
	idList< idStr > pending;
	{
		idScopedCriticalSection lock( printMutex );
		pending = queuedPrints;
		queuedPrints.Clear();
	}
	for( int i = 0; i < pending.Num(); ++i )
	{
		PrintInternal( pending[i].c_str() );
	}
}

/*
================
PrintInternal
================
*/
void idConsoleLocal::PrintInternal( const char* input )
{
	if( input == NULL )
	{
		return;
	}
	if( captureWatchText )
	{
		watchText.Append( input );
		for( int newline = watchText.Find( '\n' ); newline >= 0; newline = watchText.Find( '\n' ) )
		{
			idStr line( watchText, 0, newline );
			line.StripTrailingWhitespace();
			consoleWatchResults.Append( line );
			const idStr remainder( watchText.c_str() + newline + 1 );
			watchText = remainder;
		}
		return;
	}

	const int now = Sys_Milliseconds();
	int color = idStr::ColorIndex( C_COLOR_WHITE );
	idList< short >* line = &textLines[current & ( CON_MAX_LINES - 1 )];
	while( *input != '\0' )
	{
		if( idStr::IsColor( input ) )
		{
			color = idStr::ColorIndex( input[1] == C_COLOR_DEFAULT ? C_COLOR_WHITE : input[1] );
			input += 2;
			continue;
		}
		const unsigned char character = static_cast<unsigned char>( *input++ );
		switch( character )
		{
			case '\n':
				line->SetNum( lineOffset );
				Linefeed( now );
				line = &textLines[current & ( CON_MAX_LINES - 1 )];
				break;
			case '\r':
				if( *input != '\n' )
				{
					lineOffset = 0;
				}
				break;
			case '\t':
				do
				{
					if( lineOffset >= line->Num() )
					{
						line->Append( 0 );
					}
					( *line )[lineOffset++] = static_cast<short>( ( color << 8 ) | ' ' );
				}
				while( ( lineOffset & 3 ) != 0 );
				break;
			default:
				if( lineOffset >= line->Num() )
				{
					line->Append( 0 );
				}
				( *line )[lineOffset++] = static_cast<short>( ( color << 8 ) | character );
				break;
		}
	}
	line->SetNum( lineOffset );
	times[current & ( NUM_CON_TIMES - 1 )] = now;
}

/*
================
Print

Handles cursor positioning, line wrapping, etc
================
*/
void idConsoleLocal::Print( const char* text )
{
	if( !idLib::IsMainThread() )
	{
		idScopedCriticalSection lock( printMutex );
		queuedPrints.Append( idStr( text != NULL ? text : "" ) );
		return;
	}
	PumpThreadLines();
	PrintInternal( text );
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
DrawInput

Draw the editline after a ] prompt
================
*/
void idConsoleLocal::DrawInput()
{
	const float charWidth = Con_ConsoleCharWidth();
	const float lineHeight = Con_ConsoleLineHeight();
	const int y = idMath::Ftoi( vislines - lineHeight * 2.0f );

	if( consoleField.GetAutoCompleteLength() != 0 )
	{
		const int autoCompleteLength = strlen( consoleField.GetBuffer() ) - consoleField.GetAutoCompleteLength();

		if( autoCompleteLength > 0 )
		{
			Con_DrawConsoleFilled( idVec4( 0.8f, 0.2f, 0.2f, 0.45f ),
								   LOCALSAFE_LEFT + 2.0f * charWidth + consoleField.GetAutoCompleteLength() * charWidth,
								   y + 2.0f, autoCompleteLength * charWidth, lineHeight - 2.0f );
		}
	}

	Con_DrawConsoleChar( LOCALSAFE_LEFT + charWidth, static_cast<float>( y ), ']', colorWhite );
	consoleField.Draw( idMath::Ftoi( LOCALSAFE_LEFT + 2.0f * charWidth ), y,
					   idMath::Ftoi( LOCALSAFE_WIDTH - 3.0f * charWidth ), true );
}


/*
================
DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void idConsoleLocal::DrawNotify()
{
	if( con_noPrint.GetBool() )
	{
		return;
	}
	const float charWidth = Con_ConsoleCharWidth();
	const float lineHeight = Con_ConsoleLineHeight();
	const int charactersWide = Max( 1, idMath::Ftoi( ( LOCALSAFE_WIDTH - charWidth ) / charWidth ) );
	float y = 0.0f;
	for( int lineNumber = current - NUM_CON_TIMES + 1; lineNumber <= current; ++lineNumber )
	{
		if( lineNumber < 0 )
		{
			continue;
		}
		const int lineTime = times[lineNumber & ( NUM_CON_TIMES - 1 )];
		if( lineTime == 0 || Sys_Milliseconds() - lineTime > con_notifyTime.GetFloat() * 1000.0f )
		{
			continue;
		}
		const idList< short >& line = textLines[lineNumber & ( CON_MAX_LINES - 1 )];
		int column = 0;
		for( int character = 0; character < line.Num(); ++character )
		{
			if( column >= charactersWide )
			{
				column = 0;
				y += lineHeight;
			}
			const int glyph = line[character] & 0xff;
			const idVec4& glyphColor = idStr::ColorForIndex( static_cast<char>( line[character] >> 8 ) );
			Con_DrawConsoleChar( LOCALSAFE_LEFT + ( ++column ) * charWidth, y, glyph, glyphColor );
		}
		y += lineHeight;
	}
	renderSystem->SetColor( colorWhite );
}

/*
================
DrawSolidConsole

Draws the console with the solid background
================
*/
void idConsoleLocal::DrawSolidConsole( float frac )
{
	int lines = idMath::Ftoi( SCREEN_HEIGHT * frac );
	if( lines <= 0 )
	{
		return;
	}
	if( lines > SCREEN_HEIGHT )
	{
		lines = SCREEN_HEIGHT;
	}

	float backgroundHeight = frac * SCREEN_HEIGHT;
	if( backgroundHeight < 1.0f )
	{
		backgroundHeight = 0.0f;
	}
	else
	{
		renderSystem->DrawFilled( idVec4( 0.0f, 0.0f, 0.0f, con_transparency.GetFloat() ),
								  0.0f, 0.0f, SCREEN_WIDTH, backgroundHeight );
	}
	renderSystem->DrawFilled( colorOrange, 0.0f, backgroundHeight, SCREEN_WIDTH, 2.0f );

	const float charWidth = Con_ConsoleCharWidth();
	const float lineHeight = Con_ConsoleLineHeight();
	static int versionColorCounter = 0;
	++versionColorCounter;
	int colorStep = versionColorCounter % 360;
	if( colorStep >= 180 )
	{
		colorStep = 360 - colorStep;
	}
	const float pulse = colorStep / 180.0f;
	idVec4 versionColor = idStr::ColorForIndex( C_COLOR_RED );
	versionColor.x *= 0.75f + pulse * 0.25f;
	versionColor.y *= 0.75f + pulse * 0.25f;
	versionColor.z *= 0.75f + pulse * 0.25f;

	const idStr version = va( "%s (Build:v%i.%i)", ENGINE_VERSION, BUILD_NUMBER, BUILD_NUMBER_MINOR );
	Con_DrawConsoleString( LOCALSAFE_RIGHT - Con_ConsoleStringWidth( version.c_str() ) - 12, lines - lineHeight * 1.5f, version.c_str(), versionColor, true );

	vislines = lines;
	int rows = idMath::Ftoi( ( lines - lineHeight ) / lineHeight );
	float y = lines - lineHeight * 3.0f;
	const int charactersWide = Max( 1, idMath::Ftoi( ( LOCALSAFE_WIDTH - charWidth ) / charWidth ) );
	if( display != current )
	{
		for( int column = 0; column < charactersWide; column += 4 )
		{
			Con_DrawConsoleChar( LOCALSAFE_LEFT + ( column + 1 ) * charWidth, y, '^', colorWhite );
		}
		y -= lineHeight;
		--rows;
	}

	int lineToDraw = display;
	for( int row = 0; row < rows; ++row, --lineToDraw )
	{
		if( lineToDraw < 0 )
		{
			break;
		}
		if( current - lineToDraw >= CON_MAX_LINES )
		{
			continue;
		}

		const idList< short >& line = textLines[lineToDraw & ( CON_MAX_LINES - 1 )];
		const idStr lineNumber = va( "%d", lineToDraw );
		int column = 0;
		for( int i = 0; i < lineNumber.Length(); ++i )
		{
			Con_DrawConsoleChar( LOCALSAFE_LEFT + ( ++column ) * charWidth, y,
								 lineNumber[i], idStr::ColorForIndex( C_COLOR_GREEN ) );
		}
		const char* separator = " : ";
		for( int i = 0; separator[i] != '\0'; ++i )
		{
			Con_DrawConsoleChar( LOCALSAFE_LEFT + ( ++column ) * charWidth, y,
								 separator[i], idStr::ColorForIndex( C_COLOR_BLUE ) );
		}
		const int firstTextColumn = column;
		int measureColumn = column;
		int wraps = 0;
		for( int character = 0; character < line.Num(); ++character )
		{
			if( measureColumn >= charactersWide )
			{
				measureColumn = firstTextColumn;
				++wraps;
			}
			else if( ( line[character] & 0xff ) > ' ' &&
					 ( character == 0 || ( line[character - 1] & 0xff ) <= ' ' ) )
			{
				int wordLength = 0;
				while( character + wordLength < line.Num() &&
						( line[character + wordLength] & 0xff ) > ' ' && wordLength < charactersWide )
				{
					++wordLength;
				}
				if( wordLength < charactersWide && measureColumn + wordLength >= charactersWide )
				{
					measureColumn = firstTextColumn;
					++wraps;
				}
			}
			++measureColumn;
		}

		float drawY = y - wraps * lineHeight;
		column = firstTextColumn;
		for( int character = 0; character < line.Num(); ++character )
		{
			if( column >= charactersWide )
			{
				column = firstTextColumn;
				drawY += lineHeight;
			}
			else if( ( line[character] & 0xff ) > ' ' &&
					 ( character == 0 || ( line[character - 1] & 0xff ) <= ' ' ) )
			{
				int wordLength = 0;
				while( character + wordLength < line.Num() &&
						( line[character + wordLength] & 0xff ) > ' ' && wordLength < charactersWide )
				{
					++wordLength;
				}
				if( wordLength < charactersWide && column + wordLength >= charactersWide )
				{
					column = firstTextColumn;
					drawY += lineHeight;
				}
			}
			Con_DrawConsoleChar( LOCALSAFE_LEFT + ( ++column ) * charWidth, drawY,
								 line[character] & 0xff, idStr::ColorForIndex( static_cast<char>( line[character] >> 8 ) ) );
		}
		y = drawY - ( wraps + 1 ) * lineHeight;
		rows -= wraps;
	}

	DrawInput();
	renderSystem->SetColor( colorWhite );
}


/*
==============
Draw

ForceFullScreen is used by the editor
==============
*/
void idConsoleLocal::Draw( bool forceFullScreen, bool skipNotifyLines )
{
	PumpThreadLines();
	UpdateScreenLayout();
	LINE_WIDTH = Max( 1, idMath::Ftoi( LOCALSAFE_WIDTH / Con_ConsoleCharWidth() ) - 2 );
	consoleField.SetWidthInChars( LINE_WIDTH );
	if( forceFullScreen )
	{
		// if we are forced full screen because of a disconnect,
		// we want the console closed when we go back to a session state
		Close();
		// we are however catching keyboard input
		keyCatching = true;
	}

	Scroll();

	UpdateDisplayFraction();

	if( forceFullScreen )
	{
		DrawSolidConsole( 1.0f );
	}
	else if( displayFrac )
	{
		DrawSolidConsole( displayFrac );
	}
	else
	{
		// only draw the notify lines if the developer cvar is set,
		// or we are a debug build
		if( !con_noPrint.GetBool() && !skipNotifyLines )
		{
			DrawNotify();
		}
	}

	float lefty = LOCALSAFE_TOP;
	float righty = LOCALSAFE_TOP;
	float centery = LOCALSAFE_TOP;
	if( com_showFPS.GetBool() )
	{
		righty = DrawFPS( righty );
	}
	if( com_showMemoryUsage.GetBool() )
	{
		righty = DrawMemoryUsage( righty );
	}
	DrawOverlayText( lefty, righty, centery );
	DrawDebugGraphs();
}

/*
========================
idConsoleLocal::PrintOverlay
========================
*/
void idConsoleLocal::PrintOverlay( idOverlayHandle& handle, justify_t justify, VERIFY_FORMAT_STRING const char* text, idVec4& textColor, bool showbackground, ... )
{
	if( handle.index >= 0 && handle.index < overlayText.Num() )
	{
		if( overlayText[handle.index].time == handle.time )
		{
			return;
		}
	}

	char string[MAX_PRINT_MSG];
	va_list argptr;
	va_start( argptr, showbackground );
	idStr::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );

	overlayText_t& overlay = overlayText.Alloc();
	overlay.text = string;
	overlay.textColor = textColor;
	overlay.justify = justify;
	overlay.time = Sys_Milliseconds();
	overlay.showbackground = showbackground;

	handle.index = overlayText.Num() - 1;
	handle.time = overlay.time;
}

/*
========================
idConsoleLocal::DrawOverlayText
========================
*/
void idConsoleLocal::DrawOverlayText( float& leftY, float& rightY, float& centerY )
{
	for( int i = 0; i < overlayText.Num(); i++ )
	{
		const idStr& text = overlayText[i].text;

		int maxWidth = 0;
		int numLines = 0;
		for( int j = 0; j < text.Length(); j++ )
		{
			int width = 1;
			for( ; j < text.Length() && text[j] != '\n'; j++ )
			{
				width++;
			}
			numLines++;
			if( width > maxWidth )
			{
				maxWidth = width;
			}
		}

		const float width = maxWidth * Con_ConsoleCharWidth();
		const float height = numLines * ( Con_ConsoleLineHeight() + 4.0f );

		if( overlayText[i].showbackground )
		{
			idVec4 bgColor( 0.0f, 0.0f, 0.0f, 0.75f );

			const float bgAdjust = -0.5f * Con_ConsoleCharWidth();
			if( overlayText[i].justify == JUSTIFY_LEFT )
			{
				Con_DrawConsoleFilled( bgColor, LOCALSAFE_LEFT + bgAdjust, leftY, width, height );
			}
			else if( overlayText[i].justify == JUSTIFY_RIGHT )
			{
				Con_DrawConsoleFilled( bgColor, LOCALSAFE_RIGHT - width + bgAdjust, rightY, width, height );
			}
			else if( overlayText[i].justify == JUSTIFY_CENTER_LEFT || overlayText[i].justify == JUSTIFY_CENTER_RIGHT )
			{
				Con_DrawConsoleFilled( bgColor, LOCALSAFE_LEFT + ( LOCALSAFE_WIDTH - width + bgAdjust ) * 0.5f, centerY, width, height );
			}
			else
			{
				assert( false );
			}
		}

		idStr singleLine;
		for( int j = 0; j < text.Length(); j += singleLine.Length() + 1 )
		{
			singleLine = "";
			for( int k = j; k < text.Length() && text[k] != '\n'; k++ )
			{
				singleLine.Append( text[k] );
			}
			if( overlayText[i].justify == JUSTIFY_LEFT )
			{
				DrawTextLeftAlign( LOCALSAFE_LEFT, leftY, overlayText[i].textColor, "%s", singleLine.c_str() );
			}
			else if( overlayText[i].justify == JUSTIFY_RIGHT )
			{
				DrawTextRightAlign( LOCALSAFE_RIGHT, rightY, overlayText[i].textColor, "%s", singleLine.c_str() );
			}
			else if( overlayText[i].justify == JUSTIFY_CENTER_LEFT )
			{
				DrawTextLeftAlign( LOCALSAFE_LEFT + ( LOCALSAFE_WIDTH - width ) * 0.5f, centerY, overlayText[i].textColor, "%s", singleLine.c_str() );
			}
			else if( overlayText[i].justify == JUSTIFY_CENTER_RIGHT )
			{
				DrawTextRightAlign( LOCALSAFE_LEFT + ( LOCALSAFE_WIDTH + width ) * 0.5f, centerY, overlayText[i].textColor, "%s", singleLine.c_str() );
			}
			else
			{
				assert( false );
			}
		}
	}

	overlayText.SetNum( 0 );

	consoleWatchResults.Clear();
	for( int watchIndex = 0; watchIndex < consoleWatchList.Num(); ++watchIndex )
	{
		idConsoleWatch& watch = consoleWatchList[watchIndex];
		watchText.Clear();
		captureWatchText = true;
		cmdSystem->ExecuteCommandText( watch.watchString.c_str() );
		captureWatchText = false;
		if( !watchText.IsEmpty() )
		{
			watchText.StripTrailingWhitespace();
			consoleWatchResults.Append( watchText );
			watchText.Clear();
		}
		const float drawX = watch.drawX >= 0 ? static_cast<float>( watch.drawX ) : LOCALSAFE_LEFT;
		float drawY = watch.drawY >= 0 ? static_cast<float>( watch.drawY ) : leftY;
		for( int result = 0; result < consoleWatchResults.Num(); ++result )
		{
			Con_DrawConsoleString( drawX, drawY + 2.0f, consoleWatchResults[result].c_str(), colorWhite, false );
			drawY += Con_ConsoleLineHeight() + 4.0f;
		}
		if( watch.drawY < 0 )
		{
			leftY = drawY;
		}
		consoleWatchResults.Clear();
	}
}

/*
========================
idConsoleLocal::CreateGraph
========================
*/
idDebugGraph* idConsoleLocal::CreateGraph( int numItems )
{
	idDebugGraph* graph = new( TAG_SYSTEM ) idDebugGraph( numItems );
	debugGraphs.Append( graph );
	return graph;
}

/*
========================
idConsoleLocal::DestroyGraph
========================
*/
void idConsoleLocal::DestroyGraph( idDebugGraph* graph )
{
	debugGraphs.Remove( graph );
	delete graph;
}

/*
========================
idConsoleLocal::DrawDebugGraphs
========================
*/
void idConsoleLocal::DrawDebugGraphs()
{
	for( int i = 0; i < debugGraphs.Num(); i++ )
	{
		debugGraphs[i]->Render( renderSystem );
	}
}
