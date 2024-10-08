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

#include "precompiled.h"
#pragma hdrstop

/*
============
idRotation::ToAngles
============
*/
idAngles idRotation::ToAngles() const
{
	return ToMat3().ToAngles();
}

/*
============
idRotation::ToQuat
============
*/
idQuat idRotation::ToQuat() const
{
	float a, s, c;

	a = angle * ( idMath::M_DEG2RAD * 0.5f );
	idMath::SinCos( a, s, c );
	return idQuat( vec.x * s, vec.y * s, vec.z * s, c );
}

/*
============
idRotation::toMat3
============
*/
const idMat3& idRotation::ToMat3() const
{
	float wx, wy, wz;
	float xx, yy, yz;
	float xy, xz, zz;
	float x2, y2, z2;
	float a, c, s, x, y, z;

	if( axisValid )
	{
		return axis;
	}

	a = angle * ( idMath::M_DEG2RAD * 0.5f );
	idMath::SinCos( a, s, c );

	x = vec[0] * s;
	y = vec[1] * s;
	z = vec[2] * s;

	x2 = x + x;
	y2 = y + y;
	z2 = z + z;

	xx = x * x2;
	xy = x * y2;
	xz = x * z2;

	yy = y * y2;
	yz = y * z2;
	zz = z * z2;

	wx = c * x2;
	wy = c * y2;
	wz = c * z2;

	axis[ 0 ][ 0 ] = 1.0f - ( yy + zz );
	axis[ 0 ][ 1 ] = xy - wz;
	axis[ 0 ][ 2 ] = xz + wy;

	axis[ 1 ][ 0 ] = xy + wz;
	axis[ 1 ][ 1 ] = 1.0f - ( xx + zz );
	axis[ 1 ][ 2 ] = yz - wx;

	axis[ 2 ][ 0 ] = xz - wy;
	axis[ 2 ][ 1 ] = yz + wx;
	axis[ 2 ][ 2 ] = 1.0f - ( xx + yy );

	axisValid = true;

	return axis;
}

/*
============
idRotation::ToMat4
============
*/
idMat4 idRotation::ToMat4() const
{
	return ToMat3().ToMat4();
}

/*
============
idRotation::ToAngularVelocity
============
*/
idVec3 idRotation::ToAngularVelocity() const
{
	return vec * DEG2RAD( angle );
}

/*
============
idRotation::Normalize180
============
*/
void idRotation::Normalize180()
{
	angle -= floor( angle / 360.0f ) * 360.0f;
	if( angle > 180.0f )
	{
		angle -= 360.0f;
	}
	else if( angle < -180.0f )
	{
		angle += 360.0f;
	}
}

/*
============
idRotation::Normalize360
============
*/
void idRotation::Normalize360()
{
	angle -= floor( angle / 360.0f ) * 360.0f;
	if( angle > 360.0f )
	{
		angle -= 360.0f;
	}
	else if( angle < 0.0f )
	{
		angle += 360.0f;
	}
}
