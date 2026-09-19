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

#ifndef __MATERIALPREVIEWVIEW_H__
#define __MATERIALPREVIEWVIEW_H__

#include <wx/wx.h>
#include <wx/timer.h>

#include "MaterialEditor.h"
#include "MaterialView.h"
#include "MaterialDocManager.h"

class MaterialPreviewView;

class idGLDrawableView
{

public:
	idGLDrawableView();
	~idGLDrawableView();

	void setMedia( const char* name );
	void draw( int x, int y, int w, int h );

	void buttonDown( int _button, float x, float y );
	void mouseMove( float x, float y );
	void buttonUp( int /*button*/ ) {}

	void UpdateCamera( renderView_t* refdef );
	void UpdateModel();
	void UpdateLights();
	void drawLights( renderView_t* refdef );
	void InitWorld();
	void ResetView();

	// Lighting controls.
	void addLight();
	void deleteLight( const int lightId );
	void setLightShader( const int lightId, const idStr shaderName );
	void setLightColor( const int lightId, const idVec3& value );
	void setLightRadius( const int lightId, const float radius );
	void setLightAllowMove( const int lightId, const bool move );

	// Model controls.
	void setObject( int id );
	void setCustomModel( const idStr modelName );
	void setShowLights( bool _showLights );

	// Shader parm controls.
	void setLocalParm( int parmNum, float value );
	void setGlobalParm( int parmNum, float value );

	// Light info
	int  GetLightCount() const
	{
		return viewLights.Num();
	}
	bool GetLightInfo( int lightId, idStr& shaderName, idVec3& color, float& radius, bool& allowMove ) const;

private:
	idRenderWorld*      world;
	idRenderModel*      worldModel;
	const idMaterial*   material;

	bool                showLights;

	idVec3              viewOrigin;
	idAngles            viewRotation;
	float               viewDistance;

	renderEntity_t      worldEntity;
	qhandle_t           modelDefHandle;

	int                 objectId;
	idStr               customModelName;

	float               globalParms[MAX_GLOBAL_SHADER_PARMS];

	// Mouse drag state.
	int                 button;
	bool                handleMove;
	float               pressX;
	float               pressY;

	typedef struct
	{
		renderLight_t       renderLight;
		qhandle_t           lightDefHandle;
		idVec3              origin;
		const idMaterial*   shader;
		float               radius;
		idVec3              color;
		bool                allowMove;
	} lightInfo_t;

	idList<lightInfo_t> viewLights;
};

class MaterialPreviewView : public wxPanel, public MaterialView
{
public:
	explicit MaterialPreviewView( wxWindow* parent );
	virtual ~MaterialPreviewView();

	// Called by MaterialPreviewPropView.
	void	OnModelChange( int modelId );
	void	OnCustomModelChange( idStr modelName );
	void	OnShowLightsChange( bool showLights );

	void	OnLocalParmChange( int parmNum, float value );
	void	OnGlobalParmChange( int parmNum, float value );

	void	OnLightShaderChange( int lightId, idStr shaderName );
	void	OnLightRadiusChange( int lightId, float radius );
	void	OnLightColorChange( int lightId, idVec3& color );
	void	OnLightAllowMoveChange( int lightId, bool move );

	void	OnAddLight();
	void	OnDeleteLight( int lightId );

	int		GetLightCount() const;
	bool	GetLightInfo( int lightId, idStr& shaderName, idVec3& color, float& radius, bool& allowMove ) const;

	// Called once per engine frame, from MaterialEditorRun(), while the main
	// GL context is current. Advances the render-to-texture pipeline: reads
	// back the previous call's capture (if any) and, if a repaint is
	// pending, issues a fresh render+capture for the frame after that.
	void	RenderPreviewFrame();

public: // MaterialView interface
	virtual void MV_OnMaterialSelectionChange( MaterialDoc* pMaterial ) override;

private:
	void	OnPaint( wxPaintEvent& event );
	void	OnSize( wxSizeEvent& event );
	void	OnEraseBackground( wxEraseEvent& event );

	void	OnMouseDown( wxMouseEvent& event );
	void	OnMouseUp( wxMouseEvent& event );
	void	OnMouseMotion( wxMouseEvent& event );
	void	OnMouseWheel( wxMouseEvent& event );
	void	OnMouseLeave( wxMouseEvent& event );

	void	ReadBackCapture();

	idGLDrawableView    m_renderedView;
	idStr               m_currentMaterial;
	wxTimer             m_repaintTimer;
	bool                m_needsRepaint;
	bool                m_captureReady;   // a capture from a previous RenderPreviewFrame() call is safe to read back
	wxBitmap            m_previewBitmap;
};

ID_INLINE int  MaterialPreviewView::GetLightCount() const
{
	return m_renderedView.GetLightCount();
}

ID_INLINE bool MaterialPreviewView::GetLightInfo( int lightId, idStr& shaderName, idVec3& color, float& radius, bool& allowMove ) const
{
	return m_renderedView.GetLightInfo( lightId, shaderName, color, radius, allowMove );
}

#endif /* !__MATERIALPREVIEWVIEW_H__ */