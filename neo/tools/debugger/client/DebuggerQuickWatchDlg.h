/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 1999-2011 Raven Software
Copyright (C) 2021 Harrie van Ginneken

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

#ifndef __DEBUGGERQUICKWATCHDLG_H__
#define __DEBUGGERQUICKWATCHDLG_H__

#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/sizer.h>

class rvDebuggerWindow;

class rvDebuggerQuickWatchDlg : public wxDialog
{
public:
	rvDebuggerQuickWatchDlg( wxWindow* parent, rvDebuggerWindow* debuggerWin, int callstackDepth, const char* variable = NULL );
	virtual ~rvDebuggerQuickWatchDlg();

	bool	DoModal();

private:
	enum
	{
		ID_QUICKWATCH_ADDWATCH = wxID_HIGHEST + 1,
		ID_QUICKWATCH_RECALC,
		ID_QUICKWATCH_CLOSE,
	};

	void	OnAddWatch( wxCommandEvent& event );
	void	OnRecalc( wxCommandEvent& event );
	void	OnClose( wxCommandEvent& event );
	void	OnVariableChange( wxCommandEvent& event );


protected:

	int					mCallstackDepth;
	idStr				mVariable;
	rvDebuggerWindow*	mDebuggerWindow;

	void				SetVariable( const char* varname, bool force = false );

private:

	wxTextCtrl*     mVarText;
	wxListView*     mValueList;
	wxButton*       mAddWatchBtn;
	wxButton*       mRecalcBtn;
	wxButton*       mCloseBtn;
	wxStaticText*   mValueLabel;

	wxDECLARE_EVENT_TABLE();
};

#endif /* !__DEBUGGERQUICKWATCHDLG_H__ */