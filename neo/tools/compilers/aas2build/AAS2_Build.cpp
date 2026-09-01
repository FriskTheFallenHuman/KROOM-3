/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2026 Justin Marshall(justinmarshall20@gmail.com)

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

#include "precompiled.h"
#pragma hdrstop

#include "AAS2_Build.h"

/*
============
CompileBrushes
============
*/
BuildResult CompileBrushes( const std::vector<ConvexBrush>& source, const BuildSettings& settings, File& output )
{
	output = File{};
	BuildResult result;

	if( settings.progress != nullptr )
	{
		settings.progress( "configuration space", 0, source.size(), settings.progressUserData );
	}
	std::vector<ConvexBrush> expanded;

	result.configurationSpace = ExpandConfigurationSpace( source, settings.agent, expanded );
	if( !result.configurationSpace )
	{
		result.error = BuildError::configurationSpace;
		return result;
	}

	if( settings.progress != nullptr )
	{
		settings.progress( "configuration space", source.size(), source.size(), settings.progressUserData );
	}

	SourceGeometry floors;
	result.brushSurface = BuildBrushFloorGeometry( expanded, settings.navigation.maxFloorSlopeDegrees, settings.geometryEpsilon, floors, settings.progress, settings.progressUserData );
	if( !result.brushSurface )
	{
		result.error = BuildError::brushSurface;
		return result;
	}

	result.surface = CompileSurface( floors, settings.navigation, output, settings.progress, settings.progressUserData );
	if( !result.surface )
	{
		result.error = BuildError::surfaceCompiler;
	}
	return result;
}

/*
============
BuildErrorName
============
*/
const char* BuildErrorName( BuildError error )
{
	switch( error )
	{
		case BuildError::none:
			return "none";
		case BuildError::configurationSpace:
			return "configuration space";
		case BuildError::brushSurface:
			return "brush surface";
		case BuildError::surfaceCompiler:
			return "surface compiler";
	}
	return "unknown";
}