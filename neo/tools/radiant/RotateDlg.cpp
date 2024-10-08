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

#include "qe3.h"
#include "Radiant.h"
#include "RotateDlg.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

// CRotateDlg dialog

CRotateDlg::CRotateDlg( CWnd* pParent )
	: CDialogEx( CRotateDlg::IDD, pParent )
{
	m_strX = _T( "" );
	m_strY = _T( "" );
	m_strZ = _T( "" );
}

void CRotateDlg::DoDataExchange( CDataExchange* pDX )
{
	CDialogEx::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_SPIN3, m_wndSpin3 );
	DDX_Control( pDX, IDC_SPIN2, m_wndSpin2 );
	DDX_Control( pDX, IDC_SPIN1, m_wndSpin1 );
	DDX_Text( pDX, IDC_ROTX, m_strX );
	DDX_Text( pDX, IDC_ROTY, m_strY );
	DDX_Text( pDX, IDC_ROTZ, m_strZ );
}


BEGIN_MESSAGE_MAP( CRotateDlg, CDialogEx )
	ON_BN_CLICKED( IDC_APPLY, OnApply )
	ON_NOTIFY( UDN_DELTAPOS, IDC_SPIN1, OnDeltaposSpin1 )
	ON_NOTIFY( UDN_DELTAPOS, IDC_SPIN2, OnDeltaposSpin2 )
	ON_NOTIFY( UDN_DELTAPOS, IDC_SPIN3, OnDeltaposSpin3 )
END_MESSAGE_MAP()

// CRotateDlg message handlers

void CRotateDlg::OnOK()
{
	OnApply();
	CDialogEx::OnOK();
}

void CRotateDlg::OnApply()
{
	UpdateData( TRUE );
	float f = atof( m_strX );
	if( f != 0.0 )
	{
		Select_RotateAxis( 0, f );
		f = atof( m_strY );
	}
	if( f != 0.0 )
	{
		Select_RotateAxis( 1, f );
		f = atof( m_strZ );
	}
	if( f != 0.0 )
	{
		Select_RotateAxis( 2, f );
	}
}

BOOL CRotateDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_wndSpin1.SetRange( 0, 359 );
	m_wndSpin2.SetRange( 0, 359 );
	m_wndSpin3.SetRange( 0, 359 );
	return TRUE;
}

void CRotateDlg::OnDeltaposSpin1( NMHDR* pNMHDR, LRESULT* pResult )
{
	NM_UPDOWN* pNMUpDown = ( NM_UPDOWN* )pNMHDR;
	Select_RotateAxis( 0, pNMUpDown->iDelta );
	*pResult = 0;
}

void CRotateDlg::OnDeltaposSpin2( NMHDR* pNMHDR, LRESULT* pResult )
{
	NM_UPDOWN* pNMUpDown = ( NM_UPDOWN* )pNMHDR;
	Select_RotateAxis( 1, pNMUpDown->iDelta );
	*pResult = 0;
}

void CRotateDlg::OnDeltaposSpin3( NMHDR* pNMHDR, LRESULT* pResult )
{
	NM_UPDOWN* pNMUpDown = ( NM_UPDOWN* )pNMHDR;
	Select_RotateAxis( 2, pNMUpDown->iDelta );
	*pResult = 0;
}

void CRotateDlg::ApplyNoPaint()
{
}
