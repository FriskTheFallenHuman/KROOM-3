/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __GEPROPERTIES_H__
#define __GEPROPERTIES_H__

class rvGEWorkspace;
class rvGEWindowWrapper;

class rvGEProperties : public wxPanel
{
public:
	explicit rvGEProperties( wxWindow* parent );
	~rvGEProperties() override;

	void SetWorkspace( rvGEWorkspace* workspace );
	void Update();
	void EmbedTransformer( wxWindow* transformerPanel );

private:
	void OnPropertyChanged( wxPropertyGridEvent& e );

	bool AddModifier( const char* name, const char* value );

	wxPropertyGridManager* m_grid = nullptr;
	rvGEWindowWrapper*     m_wrapper = nullptr;
	rvGEWorkspace*         m_workspace = nullptr;
	bool                   m_updating = false;
};

#endif /* !__GEPROPERTIES_H__ */