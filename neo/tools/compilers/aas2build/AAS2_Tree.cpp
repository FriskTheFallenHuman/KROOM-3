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

#include "AAS2_Tree.h"

/*
============
CenterAxis
============
*/
static float CenterAxis( const Area& area, int axis )
{
	return axis == 0 ? area.center.x : axis == 1 ? area.center.y : area.center.z;
}

/*
============
AreaListBounds
============
*/
static Bounds AreaListBounds( const File& file, const std::vector<uint32>& areas )
{
	Bounds bounds;
	bounds.mins = {std::numeric_limits<float>::max(),
				   std::numeric_limits<float>::max(),
				   std::numeric_limits<float>::max()
				  };
	bounds.maxs = { -std::numeric_limits<float>::max(),
					-std::numeric_limits<float>::max(),
					-std::numeric_limits<float>::max()
				  };
	for( uint32 areaIndex : areas )
	{
		const Bounds& area = file.areas[areaIndex].bounds;
		bounds.mins.x = Min( bounds.mins.x, area.mins.x );
		bounds.mins.y = Min( bounds.mins.y, area.mins.y );
		bounds.mins.z = Min( bounds.mins.z, area.mins.z );
		bounds.maxs.x = Max( bounds.maxs.x, area.maxs.x );
		bounds.maxs.y = Max( bounds.maxs.y, area.maxs.y );
		bounds.maxs.z = Max( bounds.maxs.z, area.maxs.z );
	}
	return bounds;
}

/*
============
BuildNode
============
*/
static int32 BuildNode( File& file, std::vector<uint32> areas )
{
	if( areas.size() == 1 )
	{
		return -static_cast<int32>( areas.front() ) - 1;
	}
	const Bounds bounds = AreaListBounds( file, areas );
	const float extents[3] =
	{
		bounds.maxs.x - bounds.mins.x,
		bounds.maxs.y - bounds.mins.y,
		bounds.maxs.z - bounds.mins.z
	};
	int axis = extents[1] > extents[0] ? 1 : 0;
	if( extents[2] > extents[axis] )
	{
		axis = 2;
	}
	std::stable_sort( areas.begin(), areas.end(),
					  [&]( uint32 left, uint32 right )
	{
		const float leftCenter = CenterAxis( file.areas[left], axis );
		const float rightCenter = CenterAxis( file.areas[right], axis );
		return leftCenter == rightCenter ? left < right : leftCenter < rightCenter;
	} );
	const size_t middle = areas.size() / 2;
	std::vector<uint32> back( areas.begin(), areas.begin() + middle );
	std::vector<uint32> front( areas.begin() + middle, areas.end() );

	Node node;
	node.bounds = bounds;
	if( axis == 0 )
	{
		node.normal.x = 1.0f;
	}
	if( axis == 1 )
	{
		node.normal.y = 1.0f;
	}
	if( axis == 2 )
	{
		node.normal.z = 1.0f;
	}
	node.distance = ( CenterAxis( file.areas[back.back()], axis ) +
					  CenterAxis( file.areas[front.front()], axis ) ) * 0.5f;
	const int32 nodeIndex = static_cast<int32>( file.nodes.size() );
	file.nodes.push_back( node );
	file.nodes[nodeIndex].children[0] = BuildNode( file, std::move( front ) );
	file.nodes[nodeIndex].children[1] = BuildNode( file, std::move( back ) );
	return nodeIndex;
}

/*
============
BuildAreaTree
============
*/
void BuildAreaTree( File& file )
{
	file.nodes.clear();
	file.trees.clear();
	Tree tree;
	tree.areas.reserve( file.areas.size() );
	for( size_t area = 0; area < file.areas.size(); ++area )
	{
		tree.areas.push_back( static_cast<uint32>( area ) );
		file.areas[area].tree = 0;
	}
	if( !tree.areas.empty() )
	{
		tree.rootNode = BuildNode( file, tree.areas );
	}
	file.trees.push_back( std::move( tree ) );
}
