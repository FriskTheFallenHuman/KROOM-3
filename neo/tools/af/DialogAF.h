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
#pragma once

class idDeclAF;

class DialogAFView;
class DialogAFProperties;
class DialogAFBody;
class DialogAFConstraint;


// DialogAF dialog

class DialogAF : public CDialog
{

	DECLARE_DYNAMIC( DialogAF )

public:
	DialogAF( CWnd* pParent = NULL );   // standard constructor
	virtual				~DialogAF();
	void				LoadFile( idDeclAF* af );
	void				SaveFile();
	void				ReloadFile();
	void				SetFileModified();

	enum				{ IDD = IDD_DIALOG_AF };

protected:
	virtual BOOL		OnInitDialog();
	virtual void		DoDataExchange( CDataExchange* pDX );    // DDX/DDV support
	afx_msg BOOL		OnToolTipNotify( UINT id, NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void		OnSetFocus( CWnd* pOldWnd );
	afx_msg void		OnDestroy();
	afx_msg void		OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized );
	afx_msg void		OnMove( int x, int y );
	afx_msg void		OnTcnSelchangeTabMode( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void		OnCbnSelchangeComboAf();
	afx_msg void		OnBnClickedButtonAfNew();
	afx_msg void		OnBnClickedButtonAfDelete();
	afx_msg void		OnBnClickedButtonAfSpawn();
	afx_msg void		OnBnClickedButtonAfTpose();
	afx_msg void		OnBnClickedButtonAfKill();
	afx_msg void		OnBnClickedButtonAfSave();
	afx_msg void		OnBnClickedCancel();

	DECLARE_MESSAGE_MAP()

private:
	CTabCtrl* 			wndTabs;
	CWnd* 				wndTabDisplay;
	DialogAFView* 		viewDlg;
	DialogAFProperties* propertiesDlg;
	DialogAFBody* 		bodyDlg;
	DialogAFConstraint* constraintDlg;

	idDeclAF* 			file;				// file being edited

	//{{AFX_DATA(DialogAF)
	CComboBox			AFList;				// list with .af files
	//}}AFX_DATA

	static toolTip_t	toolTips[];

private:
	void				InitAFList();
	void				AddTabItem( int id, const char* name );
	void				SetTab( int id );
	void				SetTabChildPos();
};

void AFDialogSetFileModified();
void AFDialogReloadFile();
