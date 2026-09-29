/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 1999-2011 Raven Software

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

#ifndef __GEITEMPROPSDLG_H__
#define __GEITEMPROPSDLG_H__

#include <wx/wx.h>
#include <wx/propdlg.h>
#include <wx/clrpicker.h>

#include "GEWindowWrapper.h"

class idDict;
class idWindow;

bool GEItemPropsDlg_DoModal( wxWindow* parent, idWindow* window, idDict& outDict );

class rvGEAlphaButton : public wxButton
{
public:
	rvGEAlphaButton( wxWindow* parent, wxWindowID id, float alpha = 1.0f );

	void SetAlpha( float a );
	float GetAlpha() const;

private:
	void OnClick( wxCommandEvent& );

	float m_alpha;
};

class rvGEItemPropsGeneralPage : public wxPanel
{
public:
	rvGEItemPropsGeneralPage( wxWindow* parent, idDict* dict, rvGEWindowWrapper::EWindowType type );

	bool SetActive();
	bool KillActive();

private:
	idDict* m_dict;
	rvGEWindowWrapper::EWindowType m_type;
	wxTextCtrl* m_name;
	wxStaticText* m_typeLabel;
	wxCheckBox* m_visible;
	wxCheckBox* m_noEvents;
	wxCheckBox* m_noTime;
	wxCheckBox* m_noClip;
	wxCheckBox* m_noCursor;
};

class rvGEItemPropsImagePage : public wxPanel
{
public:
	rvGEItemPropsImagePage( wxWindow* parent, idDict* dict );

	bool SetActive();
	bool KillActive();

private:
	void OnToggle( wxCommandEvent& );
	void UpdateCheckedStates();

	idDict* m_dict;
	wxCheckBox* m_useBackcolor;
	wxColourPickerCtrl* m_backColor;
	rvGEAlphaButton* m_backAlpha;
	wxCheckBox* m_useMaterial;
	wxTextCtrl* m_background;
	wxTextCtrl* m_matScaleX;
	wxTextCtrl* m_matScaleY;
	wxCheckBox* m_variableBackground;
	wxSpinCtrl* m_borderSize;
	wxRadioButton* m_useBorderColor;
	wxRadioButton* m_useBorderMaterial;
	wxColourPickerCtrl* m_borderColor;
	rvGEAlphaButton* m_borderAlpha;
	wxTextCtrl* m_borderMaterial;
	wxColourPickerCtrl* m_matColor;
	rvGEAlphaButton* m_matAlpha;
};

class rvGEItemPropsTextPage : public wxPanel
{
public:
	rvGEItemPropsTextPage( wxWindow* parent, idDict* dict );

	bool SetActive();
	bool KillActive();

private:
	void OnToggle( wxCommandEvent& );
	void UpdateCheckedStates();

	idDict* m_dict;
	wxCheckBox* m_useText;
	wxTextCtrl* m_text;
	wxComboBox* m_font;
	wxTextCtrl* m_scale;
	wxColourPickerCtrl* m_foreColor;
	rvGEAlphaButton* m_foreAlpha;
	wxComboBox* m_align;
	wxTextCtrl* m_alignX;
	wxTextCtrl* m_alignY;
	wxCheckBox* m_nowrap;
};

class rvGEItemPropsKeysPage : public wxPanel
{
public:
	rvGEItemPropsKeysPage( wxWindow* parent, idDict* dict, rvGEWindowWrapper* wrapper );

	bool SetActive();
	bool KillActive();

private:
	void RefreshList();
	void OnAddKey( wxCommandEvent& );
	void OnEditKey( wxCommandEvent& );
	void OnDeleteKey( wxCommandEvent& );
	void OnSelChanged( wxListEvent& );

	struct KeyEditResult
	{
		wxString key;
		wxString value;
		bool ok = false;
	};

	bool EditKeyDialog( const wxString& inKey, const wxString& inValue, bool allowKeyEdit, KeyEditResult& out );

	idDict* m_dict;
	rvGEWindowWrapper* m_wrapper;
	wxListCtrl* m_list;
};

#endif /* !__GEITEMPROPSDLG_H__ */