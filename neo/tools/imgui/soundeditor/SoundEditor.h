/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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

#ifndef __SOUNDEDITOR_H__
#define __SOUNDEDITOR_H__

#include "../imguizmo/ImGuizmo.h"

class SoundInfo
{
public:
	idStr name;
	idStr shader;
	idStr group;
	idVec3 origin;
	idAngles angles;
	float volume;
	float minDistance;
	float maxDistance;
	float leadThrough;
	float random;
	float wait;
	float shakes;
	bool omni;
	bool occlusion;
	bool plain;
	bool looping;
	bool unclamped;
	int waitForTrigger;

	SoundInfo();
	void FromDict( const idDict* dict );
	void ToDict( idDict* dict ) const;
};

class SoundEditor : public idImGuiWindow
{
private:
	bool isShown;
	idStr title;
	idStr entityName;
	idEntity* soundEntity;
	SoundInfo original;
	SoundInfo current;
	idList<idStr> shaderNames;
	int currentShaderIndex;
	ImGuizmo::OPERATION currentGizmoOperation;
	ImGuizmo::MODE currentGizmoMode;
	bool useSnap;
	float gridSnap[3];
	float angleSnap;

	void Init( const idDict* dict, idEntity* entity );
	void Reset();

	void ApplyChanges();
	void SaveChanges();
	void CancelChanges();
	void PlayShader( const char* shader ) const;
	void DrawGizmo( bool& changed );
	static const char* ShaderItemsGetter( void* data, int index );
	void LoadShaders();

	SoundEditor()
	{
		isShown = false;

		Reset();
	}

public:
	static SoundEditor& Instance();
	static void ReInit( const idDict* dict, idEntity* entity );

	const char* GetWindowName() const override
	{
		return "###SoundEditor";
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
		return isShown;
	}
	idImGuiWindowFlags GetExtraWindowFlags() const override
	{
		return FLAGS_NOCOLLAPSE;
	}

	void DrawContents( bool& showTool ) override;
	void OnClosed() override;
};

#endif /* !__SOUNDEDITOR_H__ */
