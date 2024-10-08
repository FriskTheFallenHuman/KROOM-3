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

#ifndef __MRU_H__
#define __MRU_H__

#define NBMRUMENUSHOW   6       // Default number of MRU showed in the menu File
#define NBMRUMENU       9       // Default number of MRU stored
#define IDMRU           8000    // Default First ID of MRU
#ifdef  OFS_MAXPATHNAME
	#define MAXSIZEMRUITEM  OFS_MAXPATHNAME
#else
	#define MAXSIZEMRUITEM  128     // Default max size of an entry
#endif

typedef struct
{
	WORD wNbItemFill;
	WORD wNbLruShow;
	WORD wNbLruMenu;
	WORD wMaxSizeLruItem;
	WORD wIdMru;
	LPSTR lpMRU;
} MRUMENU;

typedef MRUMENU FAR* LPMRUMENU;

#ifdef __cplusplus
LPMRUMENU       CreateMruMenu( WORD wNbLruShowInit = NBMRUMENUSHOW,
							   WORD wNbLruMenuInit = NBMRUMENU,
							   WORD wMaxSizeLruItemInit = MAXSIZEMRUITEM,
							   WORD wIdMruInit = IDMRU );
#else
LPMRUMENU       CreateMruMenu( WORD wNbLruShowInit,
							   WORD wNbLruMenuInit,
							   WORD wMaxSizeLruItemInit,
							   WORD wIdMruInit );
#endif

LPMRUMENU       CreateMruMenuDefault();
void            DeleteMruMenu( LPMRUMENU lpMruMenu );

void            SetNbLruShow( LPMRUMENU lpMruMenu, WORD wNbLruShowInit );
BOOL            SetMenuItem( LPMRUMENU lpMruMenu, WORD wItem,
							 LPSTR lpItem );
BOOL            GetMenuItem( LPMRUMENU lpMruMenu, WORD wItem,
							 BOOL fIDMBased, LPSTR lpItem, UINT uiSize );
BOOL            DelMenuItem( LPMRUMENU lpMruMenu, WORD wItem, BOOL fIDMBased );
void            AddNewItem( LPMRUMENU lpMruMenu, LPSTR lpItem );
void            PlaceMenuMRUItem( LPMRUMENU lpMruMenu, HMENU hMenu, UINT uiItem );

BOOL            SaveMruInIni( LPMRUMENU lpMruMenu, LPSTR lpszSection, LPSTR lpszFile );
BOOL            LoadMruInIni( LPMRUMENU lpMruMenu, LPSTR lpszSection, LPSTR lpszFile );
#ifdef WIN32
	BOOL            SaveMruInReg( LPMRUMENU lpMruMenu, LPSTR lpszKey );
	BOOL            LoadMruInReg( LPMRUMENU lpMruMenu, LPSTR lpszKey );
#endif


//////////////////////////////////////////////////////////////
#endif
