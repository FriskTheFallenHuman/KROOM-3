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

#ifndef __VECTORSET_H__
#define __VECTORSET_H__

/*
===============================================================================

	Vector Set

	Creates a set of vectors without duplicates.

===============================================================================
*/

template< class type, int dimension >
class idVectorSet : public idList<type>
{
public:
	idVectorSet();
	idVectorSet( const type& mins, const type& maxs, const int boxHashSize, const int initialSize );

	// returns total size of allocated memory
	size_t					Allocated() const
	{
		return idList<type>::Allocated() + hash.Allocated();
	}
	// returns total size of allocated memory including size of type
	size_t					Size() const
	{
		return sizeof( *this ) + Allocated();
	}

	void					Init( const type& mins, const type& maxs, const int boxHashSize, const int initialSize );
	void					ResizeIndex( const int newSize );
	void					Clear();

	int						FindVector( const type& v, const float epsilon );

private:
	idHashIndex				hash;
	type					mins;
	type					maxs;
	int						boxHashSize;
	float					boxInvSize[dimension];
	float					boxHalfSize[dimension];
};

template< class type, int dimension >
ID_INLINE idVectorSet<type, dimension>::idVectorSet()
{
	hash.Clear( idMath::IPow( boxHashSize, dimension ), 128 );
	boxHashSize = 16;
	memset( boxInvSize, 0, dimension * sizeof( boxInvSize[0] ) );
	memset( boxHalfSize, 0, dimension * sizeof( boxHalfSize[0] ) );
}

template< class type, int dimension >
ID_INLINE idVectorSet<type, dimension>::idVectorSet( const type& mins, const type& maxs, const int boxHashSize, const int initialSize )
{
	Init( mins, maxs, boxHashSize, initialSize );
}

template< class type, int dimension >
ID_INLINE void idVectorSet<type, dimension>::Init( const type& mins, const type& maxs, const int boxHashSize, const int initialSize )
{
	int i;
	float boxSize;

	idList<type>::AssureSize( initialSize );
	idList<type>::SetNum( 0 );

	hash.Clear( idMath::IPow( boxHashSize, dimension ), initialSize );

	this->mins = mins;
	this->maxs = maxs;
	this->boxHashSize = boxHashSize;

	for( i = 0; i < dimension; i++ )
	{
		boxSize = ( maxs[i] - mins[i] ) / ( float ) boxHashSize;
		boxInvSize[i] = 1.0f / boxSize;
		boxHalfSize[i] = boxSize * 0.5f;
	}
}

template< class type, int dimension >
ID_INLINE void idVectorSet<type, dimension>::ResizeIndex( const int newSize )
{
	idList<type>::Resize( newSize );
	hash.ResizeIndex( newSize );
}

template< class type, int dimension >
ID_INLINE void idVectorSet<type, dimension>::Clear()
{
	idList<type>::Clear();
	hash.Clear();
}

template< class type, int dimension >
ID_INLINE int idVectorSet<type, dimension>::FindVector( const type& v, const float epsilon )
{
	int i, j, k, hashKey, partialHashKey[dimension];

	for( i = 0; i < dimension; i++ )
	{
		assert( epsilon <= boxHalfSize[i] );
		partialHashKey[i] = ( int )( ( v[i] - mins[i] - boxHalfSize[i] ) * boxInvSize[i] );
	}

	for( i = 0; i < ( 1 << dimension ); i++ )
	{

		hashKey = 0;
		for( j = 0; j < dimension; j++ )
		{
			hashKey *= boxHashSize;
			hashKey += partialHashKey[j] + ( ( i >> j ) & 1 );
		}

		for( j = hash.First( hashKey ); j >= 0; j = hash.Next( j ) )
		{
			const type& lv = ( *this )[j];
			for( k = 0; k < dimension; k++ )
			{
				if( idMath::Fabs( lv[k] - v[k] ) > epsilon )
				{
					break;
				}
			}
			if( k >= dimension )
			{
				return j;
			}
		}
	}

	hashKey = 0;
	for( i = 0; i < dimension; i++ )
	{
		hashKey *= boxHashSize;
		hashKey += ( int )( ( v[i] - mins[i] ) * boxInvSize[i] );
	}

	hash.Add( hashKey, idList<type>::Num() );
	this->Append( v );
	return idList<type>::Num() - 1;
}


/*
===============================================================================

	Vector Subset

	Creates a subset without duplicates from an existing list with vectors.

===============================================================================
*/

template< class type, int dimension >
class idVectorSubset
{
public:
	idVectorSubset();
	idVectorSubset( const type& mins, const type& maxs, const int boxHashSize, const int initialSize );

	// returns total size of allocated memory
	size_t					Allocated() const
	{
		return idList<type>::Allocated() + hash.Allocated();
	}
	// returns total size of allocated memory including size of type
	size_t					Size() const
	{
		return sizeof( *this ) + Allocated();
	}

	void					Init( const type& mins, const type& maxs, const int boxHashSize, const int initialSize );
	void					Clear();

	// returns either vectorNum or an index to a previously found vector
	int						FindVector( const type* vectorList, const int vectorNum, const float epsilon );

private:
	idHashIndex				hash;
	type					mins;
	type					maxs;
	int						boxHashSize;
	float					boxInvSize[dimension];
	float					boxHalfSize[dimension];
};

template< class type, int dimension >
ID_INLINE idVectorSubset<type, dimension>::idVectorSubset()
{
	hash.Clear( idMath::IPow( boxHashSize, dimension ), 128 );
	boxHashSize = 16;
	memset( boxInvSize, 0, dimension * sizeof( boxInvSize[0] ) );
	memset( boxHalfSize, 0, dimension * sizeof( boxHalfSize[0] ) );
}

template< class type, int dimension >
ID_INLINE idVectorSubset<type, dimension>::idVectorSubset( const type& mins, const type& maxs, const int boxHashSize, const int initialSize )
{
	Init( mins, maxs, boxHashSize, initialSize );
}

template< class type, int dimension >
ID_INLINE void idVectorSubset<type, dimension>::Init( const type& mins, const type& maxs, const int boxHashSize, const int initialSize )
{
	int i;
	float boxSize;

	hash.Clear( idMath::IPow( boxHashSize, dimension ), initialSize );

	this->mins = mins;
	this->maxs = maxs;
	this->boxHashSize = boxHashSize;

	for( i = 0; i < dimension; i++ )
	{
		boxSize = ( maxs[i] - mins[i] ) / ( float ) boxHashSize;
		boxInvSize[i] = 1.0f / boxSize;
		boxHalfSize[i] = boxSize * 0.5f;
	}
}

template< class type, int dimension >
ID_INLINE void idVectorSubset<type, dimension>::Clear()
{
	idList<type>::Clear();
	hash.Clear();
}

template< class type, int dimension >
ID_INLINE int idVectorSubset<type, dimension>::FindVector( const type* vectorList, const int vectorNum, const float epsilon )
{
	int i, j, k, hashKey, partialHashKey[dimension];
	const type& v = vectorList[vectorNum];

	for( i = 0; i < dimension; i++ )
	{
		assert( epsilon <= boxHalfSize[i] );
		partialHashKey[i] = ( int )( ( v[i] - mins[i] - boxHalfSize[i] ) * boxInvSize[i] );
	}

	for( i = 0; i < ( 1 << dimension ); i++ )
	{

		hashKey = 0;
		for( j = 0; j < dimension; j++ )
		{
			hashKey *= boxHashSize;
			hashKey += partialHashKey[j] + ( ( i >> j ) & 1 );
		}

		for( j = hash.First( hashKey ); j >= 0; j = hash.Next( j ) )
		{
			const type& lv = vectorList[j];
			for( k = 0; k < dimension; k++ )
			{
				if( idMath::Fabs( lv[k] - v[k] ) > epsilon )
				{
					break;
				}
			}
			if( k >= dimension )
			{
				return j;
			}
		}
	}

	hashKey = 0;
	for( i = 0; i < dimension; i++ )
	{
		hashKey *= boxHashSize;
		hashKey += ( int )( ( v[i] - mins[i] ) * boxInvSize[i] );
	}

	hash.Add( hashKey, vectorNum );
	return vectorNum;
}

#endif /* !__VECTORSET_H__ */
