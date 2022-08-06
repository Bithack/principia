//
// Copyright (c) 2009 Brandon Jones
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//  claim that you wrote the original software. If you use this software
//  in a product, an acknowledgment in the product documentation would be
//  appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//  misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source
//  distribution.
//

#if !defined(SQRAT_TEST_VECTOR_H)
#define SQRAT_TEST_VECTOR_H

#include <sqrat.h>

namespace Sqrat {
// A simple Vector class used to demonstrate binding
class Vec2 {
public:
    float x, y;

    Vec2( void );
    Vec2( const Vec2 &v );
    Vec2( const float vx, const float vy );

    bool operator ==( const Vec2 &v ) const;
    Vec2 operator -( void ) const;
    Vec2 operator +( const Vec2& v ) const;
    Vec2 operator -( const Vec2& v ) const;
    Vec2 operator *( const float f ) const;
    Vec2 operator /( const float f ) const;
    Vec2& operator =( const Vec2& v );

    float Length( void ) const;
    float Distance( const Vec2 &v ) const;
    Vec2& Normalize( void );
    float Dot( const Vec2 &v ) const;
};
}

#endif
