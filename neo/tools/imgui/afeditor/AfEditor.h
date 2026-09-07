/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2022 Stephen Pridham

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

#ifndef __AFEDITOR_H__
#define __AFEDITOR_H__

#include "AfBodyEditor.h"
#include "AfConstraintEditor.h"
#include "AfPropertyEditor.h"

/**
* Articulated figure imgui editor.
*/
class AfEditor : public idImGuiWindow
{
public:
	AfEditor( AfEditor const& ) = delete;
	void operator=( AfEditor const& ) = delete;

private:
	struct AfList
	{
		AfList() : names(), shouldPopulate( false ) {}
		void populate();

		idList<idStr>	names;
		bool			shouldPopulate;
	};

	bool					isShown;
	bool					showDockedTool;
	int						fileSelection;
	int						currentAf;
	int						currentConstraint;
	int						currentBodySelection;
	int						currentEntity;
	idDeclAF*				decl;
	idDeclAF_Body*			body;
	idDeclAF_Constraint*	constraint;

	// Editor dialogs
	AfPropertyEditor*			propertyEditor;
	idList<AfBodyEditor*, TAG_AF>		bodyEditors;
	idList<AfConstraintEditor*, TAG_AF> constraintEditors;

	AfList					afList; // list with idDeclAF names
	idList<idStr>			afFiles;
	idStr					fileName;

	struct IndexEntityDef
	{
		int index;
		idStr name;
	};
	idList<IndexEntityDef>	entities;

	void				Reset();

	void				OnNewDecl( idDeclAF* newDecl );

	AfEditor()
	{
		isShown = false;
		showDockedTool = false;

		Reset();
	}
public:
	virtual	~AfEditor();

	const char* GetWindowName() const override
	{
		return "###ArticulatedFigureEditor";
	}
	DockRegion GetDockRegion() const override
	{
		return DOCK_REGION_LEFT;
	}
	bool IsShown() const override
	{
		return isShown;
	}
	void ShowIt( bool show ) override
	{
		isShown = show;
	}
	bool IsFreeCameraActive() const override
	{
		return false;
	}

	void Draw() override;

	static AfEditor&	Instance();
	static void			Enable( const idCmdArgs& args );
};

#endif /* !__AFEDITOR_H__ */