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

#ifndef TILE_MAP_H
#define TILE_MAP_H

// Tile specifies the position and size within a texture atlas.
struct Tile
{
	Tile():
		size( 0.0f )
	{
	}

	idVec2 position;
	float size;
};

// TileNode of a quad-tree that efficiently packs all tiles in a limited area.
struct TileNode
{
	TileNode():
		level( 0 ),
		minLevel( 0 )
	{
		for( unsigned int i = 0; i < 4; i++ )
		{
			childIndices[i] = -1;
		}
	}

	idVec2 position;
	int childIndices[4];
	unsigned int level;
	unsigned int minLevel;
};

// TileMap
//
// Quad-tree that manages tiles in a power of two/ squared texture atlas. At initialization the quad-tree is build so that
// all nodes already have the information of the position for the corresponding tile. All nodes are kept in a cache-friendly
// manner in one linear list, which makes clearing the quad-tree very fast. Therefore instead of pointer indirections, indices
// into the underlying list are used.
// At runtime each relevant light will request per frame a tile with a size that corresponds to the screen-space light-area of
// the light. Thereby the size is clamped between a min/ max resolution. To determine the level of the requested tile first the
// next power of two size is determined which is larger than the requested size. However, instead of using the power of two size
// of the determined tile, the actual incoming dynamically changing size is used. In this way unpleasant popping of shadows can
// be avoided, which would occur otherwise when discrete power of two steps would be used.
// Since this operation is working with a O(n) complexity, the quad-tree is held on software-side, which is faster than keeping
// the quad-tree on the GPU.
class TileMap
{
public:
	TileMap():
		mapSize( 0.0f ),
		log2MapSize( 0 ),
		minAbsTileSize( 0.0f ),
		maxAbsTileSize( 0.0f ),
		numLevels( 0 ),
		numNodes( 0 ),
		nodeIndex( 0 ),
		foundNode( NULL )
	{
	}

	~TileMap()
	{
		Release();
	}

	void Release();

	bool Init( unsigned int mapSize, unsigned int maxAbsTileSize, unsigned int numLevels );

	void Clear();

	bool GetTile( float size, Tile& tile );

private:
	void BuildTree( TileNode& parentNode, unsigned int level );

	void FindNode( TileNode& parentNode, unsigned int level );

	float			mapSize;
	unsigned int	log2MapSize;
	float			minAbsTileSize;
	float			maxAbsTileSize;
	unsigned int	numLevels;
	idList<TileNode> tileNodeList;
	unsigned int	numNodes;
	unsigned int	nodeIndex;
	TileNode*		foundNode;
};


#endif
