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

#include "MaterialEditor.h"
#include "../common/registryoptions.h"

class MEMainFrame;

/**
* Dialog that provides an input box and several checkboxes to define
* the parameters of a search. These parameters include: text string, search
* scope and search only name flag.
*/
class FindDialog : public CDialog
{

public:
	enum { IDD = IDD_FIND };

public:
	FindDialog( CWnd* pParent = NULL );
	virtual ~FindDialog();

	BOOL					Create();

protected:
	DECLARE_DYNAMIC( FindDialog )

	//Overrides
	virtual void			DoDataExchange( CDataExchange* pDX );
	virtual BOOL			OnInitDialog();

	//Messages
	afx_msg void			OnBnClickedFindNext();
	virtual void			OnCancel();
	DECLARE_MESSAGE_MAP()

	//Protected Operations
	void					LoadFindSettings();
	void					SaveFindSettings();

protected:
	MEMainFrame*			parent;
	MaterialSearchData_t	searchData;
	rvRegistryOptions		registry;
};
