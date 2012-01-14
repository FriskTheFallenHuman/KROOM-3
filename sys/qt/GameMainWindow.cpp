/*
===========================================================================

Techyon GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 
Copyright (C) 2011 Robert Beckebans

This file is part of the Techyon GPL Source Code (Doom 3 Source Code).

Techyon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Techyon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Techyon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Techyon Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Techyon Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

//#include <QTimer>
//#include <QHBoxLayout>
#include <QGridLayout>
#include <QKeyEvent>

#include "GameMainWindow.h"

#include "../../idlib/precompiled.h"
#include "../../renderer/tr_local.h"
#include "../win32/win_local.h"
#pragma hdrstop

GameMainWindow::GameMainWindow(int argc, const char **argv)
	//:QGLWidget(QGLFormat(QGL::SampleBuffers))
{
	glWidget = new GameGLWidget();
	
	QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(glWidget);
	mainLayout->setMargin(0);
    setLayout(mainLayout);

	//initDoom3Engine(argc, argv);
}

void GameMainWindow::initDoom3Engine(int argc, const char **argv)
{
	//win32.hInstance = hInstance;
	//idStr::Copynz( sys_cmdline, lpCmdLine, sizeof( sys_cmdline ) );

	// done before Com/Sys_Init since we need this for error output
	//Sys_CreateConsole();

	// no abort/retry/fail errors
	//SetErrorMode( SEM_FAILCRITICALERRORS );

	Sys_InitCriticalSections();

	// get the initial time base
	Sys_Milliseconds();

#ifdef DEBUG
	// disable the painfully slow MS heap check every 1024 allocs
	_CrtSetDbgFlag( 0 );
#endif

//	Sys_FPU_EnableExceptions( TEST_FPU_EXCEPTIONS );
	//Sys_FPU_SetPrecision( FPU_PRECISION_DOUBLE_EXTENDED );

	GLenum glewResult = glewInit();
	if(GLEW_OK != glewResult)
	{
		// glewInit failed, something is seriously wrong
		//common->Printf( "^3GLimp_Init() - GLEW could not load OpenGL subsystem: %s", glewGetErrorString(glewResult));
		printf( "^3GLimp_Init() - GLEW could not load OpenGL subsystem: %s", glewGetErrorString(glewResult));
	}
	else
	{
		common->Printf( "Using GLEW %s\n", glewGetString(GLEW_VERSION));
		printf( "Using GLEW %s\n", glewGetString(GLEW_VERSION));
	}

	common->Init( argc, argv, NULL );

#if TEST_FPU_EXCEPTIONS != 0
	common->Printf( Sys_FPU_GetState() );
#endif

#ifndef	ID_DEDICATED
	//if ( win32.win_notaskkeys.GetInteger() ) {
	//	DisableTaskKeys( TRUE, FALSE, /*( win32.win_notaskkeys.GetInteger() == 2 )*/ FALSE );
	//}
#endif

	//Sys_StartAsyncThread();

	// hide or show the early console as necessary
	//if ( win32.win_viewlog.GetInteger() || com_skipRenderer.GetBool() || idAsyncNetwork::serverDedicated.GetInteger() ) {
	//	Sys_ShowConsole( 1, true );
	// else {
	//	Sys_ShowConsole( 0, false );
	//}

#ifdef SET_THREAD_AFFINITY 
	// give the main thread an affinity for the first cpu
	SetThreadAffinityMask( GetCurrentThread(), 1 );
#endif
}

void GameMainWindow::keyPressEvent(QKeyEvent *event)
{
	//common->Printf("GameMainWindow::keyPressEvent(%s)\n", event->text().toStdString().c_str());

	QChar qch( event->key() );
	int ch;

	if( event->key() == Qt::Key_AsciiCircum)
	{
		ch = Qt::Key_AsciiCircum;
		Sys_QueEvent( Sys_Milliseconds(), SE_KEY, ch, 1, 0, NULL );
	}
	else
	{
		ch = QKeyToDKey(event);
	
		if(ch != -1)
		{
			Sys_QueEvent( Sys_Milliseconds(), SE_KEY, ch, 1, 0, NULL );
		}
		else
		{
			if( qch.isLetterOrNumber() )
			{
				if(!(event->modifiers() & Qt::ShiftModifier))
				{
					qch = qch.toLower();
				}
				ch = qch.toAscii();
				Sys_QueEvent( Sys_Milliseconds(), SE_CHAR, ch, 1, 0, NULL );
			}
			else if( qch.isSpace() )
			{
				if(!(event->modifiers() & Qt::ShiftModifier))
				{
					//qch = qch.toLower();
					ch = qch.toAscii();
					Sys_QueEvent( Sys_Milliseconds(), SE_CHAR, ch, 1, 0, NULL );
				}
			}
			else if( qch.isPunct() )
			{
				if(!(event->modifiers() & Qt::ShiftModifier))
				{
					qch = qch.toLower();
				}
				ch = qch.toAscii();
				Sys_QueEvent( Sys_Milliseconds(), SE_CHAR, ch, 1, 0, NULL );
			}
		}
	}
}

void GameMainWindow::keyReleaseEvent(QKeyEvent *event)
{
	//common->Printf("GameMainWindow::keyReleaseEvent(%s)\n", event->text().toStdString().c_str());

	QChar qch( event->key() );
	int ch;

	if( event->key() == Qt::Key_AsciiCircum)
	{
		ch = Qt::Key_AsciiCircum;
		Sys_QueEvent( Sys_Milliseconds(), SE_KEY, ch, 0, 0, NULL );
	}
	else
	{
		ch = QKeyToDKey(event);
	
		if(ch != -1)
		{
			Sys_QueEvent( Sys_Milliseconds(), SE_KEY, ch, 0, 0, NULL );
		}
	}
	/*
	else if( qch.isLetterOrNumber() || qch.isSpace() || qch.isPunct())
	{
		//ch = qch.toAscii();
		//Sys_QueEvent( Sys_Milliseconds(), SE_CHAR, ch, 0, 0, NULL );
	}
	else
	{
		ch = QKeyToDKey(event);
		Sys_QueEvent( Sys_Milliseconds(), SE_KEY, ch, 0, 0, NULL );
	}
	*/
}

void GameMainWindow::mousePressEvent(QMouseEvent *event)
{

}

void GameMainWindow::mouseReleaseEvent(QMouseEvent *event)
{

}

void GameMainWindow::mouseMoveEvent(QMouseEvent *event)
{

}

int  GameMainWindow::QKeyToDKey(QKeyEvent *event)
{
	struct TransKey
	{
		int DKey;
		int QKey;
	};

	static TransKey transKeys[] = {
		{K_TAB, Qt::Key_Tab},
		{K_ENTER, Qt::Key_Enter},
		{K_ENTER, Qt::Key_Return},
		{K_ESCAPE, Qt::Key_Escape},
		//{K_SPACE, Qt::Key_Space},

		{K_BACKSPACE, Qt::Key_Backspace},

		//{K_COMMAND, Qt::Key_Command},
		{K_CAPSLOCK, Qt::Key_CapsLock},
		{K_SCROLL, Qt::Key_ScrollLock},
		{K_POWER, Qt::Key_PowerOff},
		{K_PAUSE, Qt::Key_Pause},

		{K_UPARROW, Qt::Key_Up},
		{K_DOWNARROW, Qt::Key_Down},
		{K_LEFTARROW, Qt::Key_Left},
		{K_RIGHTARROW, Qt::Key_Right},

		// The 3 windows keys
		{K_LWIN, Qt::Key_Meta},
		{K_RWIN, Qt::Key_Meta},
		{K_MENU, Qt::Key_Menu},

		{K_ALT, Qt::Key_Alt},
		{K_CTRL, Qt::Key_Control},
		//{K_SHIFT, Qt::Key_Shift},
		{K_INS, Qt::Key_Insert},
		{K_DEL, Qt::Key_Delete},
		{K_PGDN, Qt::Key_PageDown},
		{K_PGUP, Qt::Key_PageUp},
		{K_HOME, Qt::Key_Home},
		{K_END, Qt::Key_End},

		{K_F1, Qt::Key_F1},
		{K_F2, Qt::Key_F2},
		{K_F3, Qt::Key_F3},
		{K_F4, Qt::Key_F4},
		{K_F5, Qt::Key_F5},
		{K_F6, Qt::Key_F6},
		{K_F7, Qt::Key_F7},
		{K_F8, Qt::Key_F8},
		{K_F9, Qt::Key_F9},
		{K_F10, Qt::Key_F10},
		{K_F11, Qt::Key_F11},
		{K_F12, Qt::Key_F12},
		//{K_INVERTED_EXCLAMATION, Qt::Key_},
		{K_F13, Qt::Key_F13},
		{K_F14, Qt::Key_F14},
		{K_F15, Qt::Key_F15},

		/*
		{K_KP_HOME, Qt::Key_Keyboard_ho},
		{K_KP_UPARROW, Qt::Key_},
		{K_KP_PGUP, Qt::Key_},
		{K_KP_LEFTARROW, Qt::Key_},
		{K_KP_5, Qt::Key_},
		{K_KP_RIGHTARROW, Qt::Key_},
		{K_KP_END, Qt::Key_},
		{K_KP_DOWNARROW, Qt::Key_},
		{K_KP_PGDN, Qt::Key_},
		{K_KP_ENTER, Qt::Key_},
		{K_KP_INS, Qt::Key_},
		{K_KP_DEL, Qt::Key_},
		{K_KP_SLASH, Qt::Key_},
		{K_SUPERSCRIPT_TWO, Qt::Key_},
		{K_KP_MINUS, Qt::Key_},
		{K_ACUTE_ACCENT, Qt::Key_},
		{K_KP_PLUS, Qt::Key_},
		{K_KP_NUMLOCK, Qt::Key_},
		{K_KP_STAR, Qt::Key_},
		{K_KP_EQUALS, Qt::Key_},
		*/
	};

	for(int i = 0; i < sizeof(transKeys)/sizeof(TransKey); i++)
	{
		const TransKey& tk = transKeys[i];

		if(event->key() == tk.QKey)
			return tk.DKey;
	}

	return -1;
}

/*
void GameMainWindow::initializeGL()
{

}

void GameMainWindow::paintGL()
{
#ifdef DEBUG
	Sys_MemFrame();
#endif

	// run the game
	common->Frame();
}

void GameMainWindow::resizeGL(int width, int height)
{
	glConfig.vidWidth = width;
	glConfig.vidHeight = height;
}

void GameMainWindow::onTimer()
{
	updateGL();
}
*/