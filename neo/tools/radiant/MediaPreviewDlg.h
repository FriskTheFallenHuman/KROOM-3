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

// CMediaPreviewDlg dialog

class CMediaPreviewDlg : public CDialogEx
{

	DECLARE_DYNAMIC( CMediaPreviewDlg )

public:
	enum { MATERIALS, GUIS };
	CMediaPreviewDlg( CWnd* pParent = nullptr );   // standard constructor
	virtual ~CMediaPreviewDlg();

	void SetMode( int _mode )
	{
		mode = _mode;
	}

	void SetMedia( const char* _media );
	void Refresh();

	// Dialog Data
	enum { IDD = IDD_DIALOG_EDITPREVIEW };

protected:
	idGLDrawable testDrawable;
	idGLDrawableMaterial drawMaterial;
	idGLWidget wndPreview;
	int mode;
	idStr media;

	virtual void DoDataExchange( CDataExchange* pDX ) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog() override;
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDown( UINT nFlags, CPoint point );
	afx_msg void OnLButtonUp( UINT nFlags, CPoint point );
	afx_msg void OnMouseMove( UINT nFlags, CPoint point );
};