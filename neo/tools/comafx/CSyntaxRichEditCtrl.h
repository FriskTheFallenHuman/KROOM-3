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

#ifndef __CSYNTAXRICHEDITCTR_H__
#define __CSYNTAXRICHEDITCTR_H__

/*
===============================================================================

	Rich Edit Control with:

	- syntax highlighting
	- braced section highlighting
	- braced section auto-indentation
	- multi-line tabs
	- keyword auto-completion
	- object member auto-completion
	- keyword tool tip
	- function parameter tool tip

===============================================================================
*/

// use #import on Vista to generate .tlh header to copy from intermediate compile directory to local directory for subsequent builds
//     rename: avoids warning C4278: 'FindText': identifier in type library 'riched20.dll' is already a macro; use the 'rename' qualifier
//     no_auto_exclude: avoids warnings
//     no_namespace: no longer using this option, which avoids variable redifinition compile errors on Vista
//#define GENERATE_TLH
#ifdef GENERATE_TLH
	#import "riched20.dll" raw_interfaces_only, raw_native_types, named_guids, no_auto_exclude, no_implementation, rename( "FindText", "FindShit" )
#else
	#include "riched20.tlh"
#endif

static const char* 		FONT_NAME				= "Courier";
static const int		FONT_HEIGHT				= 10;
static const int		FONT_WIDTH				= 8;
static const int		TAB_SIZE				= 4;

static const COLORREF	SRE_COLOR_BLACK			= RGB( 0,   0,   0 );
static const COLORREF	SRE_COLOR_WHITE			= RGB( 255, 255, 255 );
static const COLORREF	SRE_COLOR_RED			= RGB( 255,   0,   0 );
static const COLORREF	SRE_COLOR_GREEN			= RGB( 0, 255,   0 );
static const COLORREF	SRE_COLOR_BLUE			= RGB( 0,   0, 255 );
static const COLORREF	SRE_COLOR_YELLOW		= RGB( 255, 255,   0 );
static const COLORREF	SRE_COLOR_MAGENTA		= RGB( 255,   0, 255 );
static const COLORREF	SRE_COLOR_CYAN			= RGB( 0, 255, 255 );
static const COLORREF	SRE_COLOR_ORANGE		= RGB( 255, 128,   0 );
static const COLORREF	SRE_COLOR_PURPLE		= RGB( 150,   0, 150 );
static const COLORREF	SRE_COLOR_PINK			= RGB( 186, 102, 123 );
static const COLORREF	SRE_COLOR_GREY			= RGB( 85,  85,  85 );
static const COLORREF	SRE_COLOR_BROWN			= RGB( 100,  90,  20 );
static const COLORREF	SRE_COLOR_LIGHT_GREY	= RGB( 170, 170, 170 );
static const COLORREF	SRE_COLOR_LIGHT_BROWN	= RGB( 170, 150,  20 );
static const COLORREF	SRE_COLOR_DARK_GREEN	= RGB( 0, 128,   0 );
static const COLORREF	SRE_COLOR_DARK_CYAN		= RGB( 0, 150, 150 );
static const COLORREF	SRE_COLOR_DARK_YELLOW	= RGB( 220, 200,  20 );

typedef struct
{
	const char* 		keyWord;
	COLORREF			color;
	const char* 		description;
} keyWord_t;

typedef bool ( *objectMemberCallback_t )( const char* objectName, CListBox& listBox );
typedef bool ( *toolTipCallback_t )( const char* name, CString& string );


class CSyntaxRichEditCtrl : public CRichEditCtrl
{
public:
	CSyntaxRichEditCtrl();
	~CSyntaxRichEditCtrl();

	void					Init();

	void					SetCaseSensitive( bool caseSensitive );
	void					AllowPathNames( bool allow );
	void					EnableKeyWordAutoCompletion( bool enable );
	void					SetKeyWords( const keyWord_t kws[] );
	bool					LoadKeyWordsFromFile( const char* fileName );
	void					SetObjectMemberCallback( objectMemberCallback_t callback );
	void					SetFunctionParmCallback( toolTipCallback_t callback );
	void					SetToolTipCallback( toolTipCallback_t callback );

	void					SetDefaultColor( const COLORREF color );
	void					SetCommentColor( const COLORREF color );
	void					SetStringColor( const COLORREF color, const COLORREF altColor = -1 );
	void					SetLiteralColor( const COLORREF color );

	COLORREF				GetForeColor( int charIndex ) const;
	COLORREF				GetBackColor( int charIndex ) const;

	void					GetCursorPos( int& line, int& column, int& character ) const;
	CHARRANGE				GetVisibleRange() const;

	void					GetText( idStr& text ) const;
	void					GetText( idStr& text, int startCharIndex, int endCharIndex ) const;
	void					SetText( const char* text );

	void					GoToLine( int line );
	bool					FindNext( const char* find, bool matchCase, bool matchWholeWords, bool searchForward );
	int						ReplaceAll( const char* find, const char* replace, bool matchCase, bool matchWholeWords );
	void					ReplaceText( int startCharIndex, int endCharIndex, const char* replace );

protected:
	virtual INT_PTR			OnToolHitTest( CPoint point, TOOLINFO* pTI ) const;
	afx_msg BOOL			OnToolTipNotify( UINT id, NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg UINT			OnGetDlgCode();
	afx_msg void			OnChar( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void			OnKeyDown( UINT nKey, UINT nRepCnt, UINT nFlags );
	afx_msg void			OnLButtonDown( UINT nFlags, CPoint point );
	afx_msg BOOL			OnMouseWheel( UINT nFlags, short zDelta, CPoint pt );
	afx_msg void			OnMouseMove( UINT nFlags, CPoint point );
	afx_msg void			OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg void			OnSize( UINT nType, int cx, int cy );
	afx_msg void			OnProtected( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void			OnChange();
	afx_msg void			OnAutoCompleteListBoxChange();
	afx_msg void			OnAutoCompleteListBoxDblClk();

	DECLARE_MESSAGE_MAP()

	// settings
	CHARFORMAT2				defaultCharFormat;
	COLORREF				defaultColor;
	COLORREF				singleLineCommentColor;
	COLORREF				multiLineCommentColor;
	COLORREF				stringColor[2];
	COLORREF				literalColor;
	COLORREF				braceHighlightColor;

	typedef enum
	{
		CT_WHITESPACE,
		CT_COMMENT,
		CT_STRING,
		CT_LITERAL,
		CT_NUMBER,
		CT_NAME,
		CT_PUNCTUATION
	} charType_t;

	int						charType[256];

	idList<keyWord_t>		keyWordsFromFile;
	const keyWord_t* 		keyWords;
	int* 					keyWordLengths;
	COLORREF* 				keyWordColors;
	idHashIndex				keyWordHash;

	bool					caseSensitive;
	bool					allowPathNames;
	bool					keyWordAutoCompletion;

	objectMemberCallback_t	GetObjectMembers;
	toolTipCallback_t		GetFunctionParms;
	toolTipCallback_t		GetToolTip;

	// run-time variables
	tom::ITextDocument* 	m_TextDoc;
	tom::ITextFont* 		m_DefaultFont;

	CHARRANGE				updateRange;
	bool					updateSyntaxHighlighting;
	int						stringColorIndex;
	int						stringColorLine;

	int						autoCompleteStart;
	CListBox				autoCompleteListBox;

	int						funcParmToolTipStart;
	CEdit					funcParmToolTip;

	int						bracedSection[2];

	CPoint					mousePoint;
	CToolTipCtrl* 			keyWordToolTip;
	TCHAR* 					m_pchTip;
	WCHAR* 					m_pwchTip;

protected:
	void					InitFont();
	void					InitSyntaxHighlighting();
	void					SetCharType( int first, int last, int type );
	void					SetDefaultFont( int startCharIndex, int endCharIndex );
	void					SetColor( int startCharIndex, int endCharIndex, COLORREF foreColor, COLORREF backColor, bool bold );

	void					FreeKeyWordsFromFile();
	int						FindKeyWord( const char* keyWord, int length ) const;

	void					HighlightSyntax( int startCharIndex, int endCharIndex );
	void					UpdateVisibleRange();

	bool					GetNameBeforeCurrentSelection( CString& name, int& charIndex ) const;
	bool					GetNameForMousePosition( idStr& name ) const;

	void					AutoCompleteInsertText();
	void					AutoCompleteUpdate();
	void					AutoCompleteShow( int charIndex );
	void					AutoCompleteHide();

	void					ToolTipShow( int charIndex, const char* string );
	void					ToolTipHide();

	bool					BracedSectionStart( char braceStartChar, char braceEndChar );
	bool					BracedSectionEnd( char braceStartChar, char braceEndChar );
	void					BracedSectionAdjustEndTabs();
	void					BracedSectionShow();
	void					BracedSectionHide();
};

#endif /* !__CSYNTAXRICHEDITCTR_H__ */
