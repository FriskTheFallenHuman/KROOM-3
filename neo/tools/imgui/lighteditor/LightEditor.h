/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Daniel Gibson
Copyright (C) 2020-2023 Robert Beckebans

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

#ifndef __LIGHTEDITOR_H_
#define __LIGHTEDITOR_H_

#include "../imguizmo/ImGuizmo.h"

enum ELightType
{
	LIGHT_POINT,
	LIGHT_SPOT,
	LIGHT_SUN
};

class LightInfo
{
public:
	ELightType	lightType;

	idStr		strTexture;
	bool		equalRadius;
	bool		explicitStartEnd;
	idVec3		lightStart;
	idVec3		lightEnd;
	idVec3		lightUp;
	idVec3		lightRight;
	idVec3		lightTarget;
	idVec3		lightCenter;
	idVec3		color;

	idVec3		origin;
	idAngles	angles;			// RBDOOM specific, saved to map as "angles"
	idVec3		scale;			// not saved to .map

	idVec3		lightRadius;
	bool		castShadows;
	bool		skipSpecular;
	bool		hasCenter;

	int			lightStyle;

	LightInfo();

	void		Defaults();

	void		DefaultPoint();
	void		DefaultProjected();
	void		DefaultSun();
	void		FromDict( const idDict* e );
	void		ToDict( idDict* e );
};

class LightEditor : public idImGuiWindow
{
private:
	bool				isShown;

	idStr				title;
	idStr				entityName;
	idVec3				entityPos;

	LightInfo			original;
	LightInfo			cur; // current status of the light
	LightInfo			curNotMoving;

	idEntity*			lightEntity;

	idList<idStr>		textureNames;
	int					currentTextureIndex;
	idImage*			currentTexture;
	const idMaterial*	currentTextureMaterial;

	idList<idStr>		styleNames;
	int					currentStyleIndex;

	ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	ImGuizmo::MODE		mCurrentGizmoMode = ImGuizmo::WORLD;

	bool				useSnap = false;
	float				gridSnap[3] = { 4.0f, 4.0f, 4.0f };
	float				angleSnap = 15.0f;
	float				scaleSnap = 0.1f;
	float				bounds[6] = { -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
	float				boundsSnap[3] = { 0.1f, 0.1f, 0.1f };
	bool				boundSizing = false;
	bool				boundSizingSnap = false;

	bool				shortcutSaveMapEnabled;
	bool				shortcutDuplicateLightEnabled;

	void				LoadLightStyles();
	static const char* 	StyleItemsGetter( void* data, int idx );

	void				Init( const idDict* dict, idEntity* light );
	void				Reset();

	void				LoadLightTextures();
	static const char* 	TextureItemsGetter( void* data, int idx );
	void				LoadCurrentTexture();
	bool				DrawLightTextureBrowser();

	void				TempApplyChanges();
	void				SaveChanges( bool saveMap );
	void				CancelChanges();

	void				DuplicateLight();

	LightEditor()
	{
		isShown = false;

		Reset();
	}

public:
	const char* GetWindowName() const override
	{
		return "###LightEditor";
	}
	const char* GetDisplayTitle() const override
	{
		return title.c_str();
	}
	DockRegion GetDockRegion() const override
	{
		return DOCK_REGION_RIGHT;
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
		return IsShown();
	}
	idImGuiWindowFlags GetExtraWindowFlags() const override
	{
		return FLAGS_NOCOLLAPSE;
	}

	void DrawContents( bool& showTool ) override;
	void OnClosed() override;

	static LightEditor&	Instance();
	static void			ReInit( const idDict* dict, idEntity* light );
};

#endif /* !__LIGHTEDITOR_H_ */
