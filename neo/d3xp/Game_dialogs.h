/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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

#ifndef __GAME_DIALOGS_H__
#define	__GAME_DIALOGS_H__

/*
===============================================================================

	Local implementation of the public dialogs interface.

===============================================================================
*/

class idGameDialogsLocal : public idGameDialogs
{
public:
	// ---------------------- Public idGameDialogs Interface -------------------

	idGameDialogsLocal();
	virtual         ~idGameDialogsLocal();

	//virtual void				Init();
	//virtual void				Shutdown();
	virtual void				Restart();

	virtual void				Render( bool loading );

	virtual void				AddDialog( gameDialogMessages_t msg, dialogType_t type, idDialogCallback* acceptCallback, idDialogCallback* cancelCallback, bool pause, const char* location = NULL, int lineNumber = 0, bool leaveOnMapHeapReset = false, bool waitOnAtlas = false, bool renderDuringLoad = false );
	virtual void				AddDynamicDialog( gameDialogMessages_t msg, const idStaticList< idDialogCallback*, 4 >& callbacks, const idStaticList< idStrId, 4 >& optionText, bool pause, idStrStatic< 256 > overrideMsg, bool leaveOnMapHeapReset = false, bool waitOnAtlas = false, bool renderDuringLoad = false );

	virtual void				AddDialogIntVal( const char* name, int val );

	virtual void				ClearDialog( gameDialogMessages_t msg, const char* location = NULL, int lineNumber = 0 );
	virtual void				ClearDialogs( bool forceClear = false );
	virtual void				ClearAllDialogHack();

	virtual bool				HasDialogMsg( gameDialogMessages_t msg, bool* isNowActive );
	virtual bool				HasAnyActiveDialog() const;
	virtual bool				IsDialogPausing() const
	{
		return dialogPause;
	}

	//virtual void				ShowSaveIndicator( bool show );
	//virtual bool				HandleDialogEvent( const sysEvent_t* sev );
	//virtual bool				IsDialogActive() const;

	virtual idStr				GetDialogMsg( gameDialogMessages_t msg, idStr& outMessage, idStr& outTitle );

	//virtual bool				IsRendererLoaded() const;
	//virtual bool				IsRendererActive() const;
	//virtual void				ActivateRenderer( bool active );

	//virtual bool				IsSaveIndicatorActive() const;
	//virtual void				RenderDialog( int timeMicroseconds );
	//virtual void				RenderSaveIndicator( int timeMicroseconds );

	//virtual void				SetRendererGlobalInt( const char* name, int val );
	//virtual void				SetRendererGlobalString( const char* name, const char* val );

	virtual void				AddRefCallback( idDialogCallback* cb )
	{
		if( cb )
		{
			cb->AddRef();
		}
	}
	virtual void				ReleaseCallback( idDialogCallback* cb )
	{
		if( cb )
		{
			cb->Release();
		}
	}
	virtual void				InvokeCallback( idDialogCallback* cb )
	{
		if( cb )
		{
			cb->Call();
		}
	}

	//virtual void				BindDialogToRenderer( const idDialogInfo& info );

	virtual void				AddDialogInternal( idDialogInfo& info );
	virtual void				ShowDialog( const idDialogInfo& info );
	virtual void				ShowNextDialog();
	virtual void				ActivateDialog( bool activate );
	virtual void				RemoveWaitDialogs();
	virtual void				ReleaseCallBacks( int index );

	// ---------------------- Public idGameDialogsLocal Interface -------------------

	static bool DialogMsgShouldWait( gameDialogMessages_t msg );

// SWF DIALOG START HERE
public:
	virtual void    Init();
	virtual void    Shutdown();

	virtual void    ShowSaveIndicator( bool show );
	virtual bool    HandleDialogEvent( const sysEvent_t* sev );

	virtual bool	IsDialogActive() const;

protected:

	virtual bool    IsRendererLoaded() const;
	virtual bool    IsRendererActive() const;
	virtual void    ActivateRenderer( bool active );
	virtual bool    IsSaveIndicatorActive()  const;
	virtual void    RenderDialog( int timeMicroseconds );
	virtual void    RenderSaveIndicator( int timeMicroseconds );

	virtual void    SetRendererGlobalInt( const char* name, int val );
	virtual void    SetRendererGlobalString( const char* name, const char* val );

	virtual void    AddRefCallback( void* cb );
	virtual void    ReleaseCallback( void* cb );
	virtual void    InvokeCallback( void* cb ) ;

	virtual void    BindDialogToRenderer( const idDialogInfo& info );

private:
	void			InitImp();

	static idSWFScriptFunction* AsSWF( void* cb )
	{
		return static_cast< idSWFScriptFunction* >( cb );
	}

	idSWF* dialog;
	idSWF* saveIndicator;
// SWF DIALOG ENDS HERE

protected:
	bool    dialogPause;
	bool    dialogInUse;
	bool    dialogShowingSaveIndicatorRequested;

	int     startSaveTime;
	int     stopSaveTime;

	idStaticList< idDialogInfo, MAX_DIALOGS >   messageList;
};

#endif /* !__GAME_DIALOGS_H__ */