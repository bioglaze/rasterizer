#include <pmmintrin.h>

typedef struct
{
    float m[ 16 ];
} Matrix44;

typedef struct
{
    float x, y, z;
} Vec3;

typedef struct
{
    float x, y, z, w;
} Vec4;

float dot( Vec3 a, Vec3 b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 cross( Vec3 v1, Vec3 v2 )
{
    return (Vec3){ v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x };
}

void normalize( Vec3* v )
{
    const float invLen = 1.0f / sqrtf( v->x * v->x + v->y * v->y + v->z * v->z );
    v->x *= invLen;
    v->y *= invLen;
    v->z *= invLen;
}

Vec3 sub( Vec3 a, Vec3 b )
{
    return (Vec3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

typedef struct Plane
{
    Vec3 a;
    Vec3 b;
    Vec3 c;
    Vec3 normal; // Plane's normal pointing inside the frustum.
    float d;
} Plane;

typedef struct Frustum
{
    // Near clipping plane coordinates
    Vec3 nearTopLeft;
    Vec3 nearTopRight;
    Vec3 nearBottomLeft;
    Vec3 nearBottomRight;

    // Far clipping plane coordinates.
    Vec3 farTopLeft;
    Vec3 farTopRight;
    Vec3 farBottomLeft;
    Vec3 farBottomRight;

    Vec3 nearCenter; // Near clipping plane center coordinate.
    Vec3 farCenter;  // Far clipping plane center coordinate.
    float zNear, zFar;
    float nearWidth, nearHeight;
    float farWidth, farHeight;

    Plane planes[ 6 ];
} Frustum;

float planeDistance( Plane* plane, Vec3 point )
{
    return dot( plane->normal, point ) + plane->d;
}

void planeSetNormalAndPoint( Plane* plane, Vec3 normal, Vec3 point )
{
    plane->normal = normal;
    normalize( &plane->normal );
    plane->d = -dot( plane->normal, point );
}

int mini( int i1, int i2 )
{
    return i1 < i2 ? i1 : i2;
}

int maxi( int i1, int i2 )
{
    return i1 > i2 ? i1 : i2;
}

void makeProjection( float fovDegrees, float aspect, float nearDepth, float farDepth, Matrix44* outMatrix )
{
    const float f = 1.0f / tanf( (0.5f * fovDegrees) * 3.14159265358979f / 180.0f );

    const float proj[] =
    {
        f / aspect, 0, 0, 0,
        0, -f, 0, 0,
        0, 0, farDepth / (nearDepth - farDepth), -1,
        0, 0, (nearDepth * farDepth) / (nearDepth - farDepth), 0
    };

    for (int i = 0; i < 16; ++i)
    {
        outMatrix->m[ i ] = proj[ i ];
    }
}

void makeIdentity( Matrix44* mat )
{
    for (int i = 0; i < 15; ++i)
    {
        mat->m[ i ] = 0;
    }

    mat->m[ 0 ] = mat->m[ 5 ] = mat->m[ 10 ] = mat->m[ 15 ] = 1;
}

void makeRotationXYZ( float xDeg, float yDeg, float zDeg, Matrix44* outMat )
{
    const float deg2rad = 3.1415926535f / 180.0f;
    const float sx = sinf( xDeg * deg2rad );
    const float sy = sinf( yDeg * deg2rad );
    const float sz = sinf( zDeg * deg2rad );
    const float cx = cosf( xDeg * deg2rad );
    const float cy = cosf( yDeg * deg2rad );
    const float cz = cosf( zDeg * deg2rad );

    outMat->m[ 0 ] = cy * cz;
    outMat->m[ 1 ] = cz * sx * sy - cx * sz;
    outMat->m[ 2 ] = cx * cz * sy + sx * sz;
    outMat->m[ 3 ] = 0;
    outMat->m[ 4 ] = cy * sz;
    outMat->m[ 5 ] = cx * cz + sx * sy * sz;
    outMat->m[ 6 ] = -cz * sx + cx * sy * sz;
    outMat->m[ 7 ] = 0;
    outMat->m[ 8 ] = -sy;
    outMat->m[ 9 ] = cy * sx;
    outMat->m[ 10 ] = cx * cy;
    outMat->m[ 11 ] = 0;
    outMat->m[ 12 ] = 0;
    outMat->m[ 13 ] = 0;
    outMat->m[ 14 ] = 0;
    outMat->m[ 15 ] = 1;
}

void multiply( const Matrix44* a, const Matrix44* b, Matrix44* result )
{
    Matrix44 tmp;

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            tmp.m[ i * 4 + j ] = a->m[ i * 4 + 0 ] * b->m[ 0 * 4 + j ] +
                a->m[ i * 4 + 1 ] * b->m[ 1 * 4 + j ] +
                a->m[ i * 4 + 2 ] * b->m[ 2 * 4 + j ] +
                a->m[ i * 4 + 3 ] * b->m[ 3 * 4 + j ];
        }
    }

    for (int i = 0; i < 16; ++i)
    {
        result->m[ i ] = tmp.m[ i ];
    }
}

void transformPoint( Vec3 point, const Matrix44* mat, Vec3* out )
{
    Vec3 tmp;
    tmp.x = mat->m[ 0 ] * point.x + mat->m[ 4 ] * point.y + mat->m[ 8 ] * point.z + mat->m[ 12 ];
    tmp.y = mat->m[ 1 ] * point.x + mat->m[ 5 ] * point.y + mat->m[ 9 ] * point.z + mat->m[ 13 ];
    tmp.z = mat->m[ 2 ] * point.x + mat->m[ 6 ] * point.y + mat->m[ 10 ] * point.z + mat->m[ 14 ];

    out->x = tmp.x;
    out->y = tmp.y;
    out->z = tmp.z;
}

void multiplySSE( const Matrix44* ma, const Matrix44* mb, Matrix44* out )
{
    alignas( 16 ) float result[ 16 ];
    Matrix44 matA;
    for (int i = 0; i < 16; ++i)
    {
        matA.m[ i ] = ma->m[ i ];
    }
    Matrix44 matB;
    for (int i = 0; i < 16; ++i)
    {
        matB.m[ i ] = mb->m[ i ];
    }

    const float* a = matA.m;
    const float* b = matB.m;
    __m128 a_line, b_line, r_line;

    for (int i = 0; i < 16; i += 4)
    {
        // unroll the first step of the loop to avoid having to initialize r_line to zero
        a_line = _mm_load_ps( b );
        b_line = _mm_set1_ps( a[ i ] );
        r_line = _mm_mul_ps( a_line, b_line );

        for (int j = 1; j < 4; j++)
        {
            a_line = _mm_load_ps( &b[ j * 4 ] );
            b_line = _mm_set1_ps(  a[ i + j ] );
            r_line = _mm_add_ps(_mm_mul_ps( a_line, b_line ), r_line);
        }

        _mm_store_ps( &result[ i ], r_line );
    }

    for (int i = 0; i < 16; ++i)
    {
        out->m[ i ] = result[ i ];
    }
}


/*void transformPointSSE( Vec3* vec, Matrix44* mat, Vec3* out )
{
    alignas( 16 ) Matrix44 transpose;
    makeIdentity( &transpose );
    //mat.Transpose( transpose );
    alignas( 16 ) Vec4 v4 = { vec->x, vec->y, vec->z, 1 };

    __m128 vec4 = _mm_load_ps( &v4.x );
    __m128 row1 = _mm_load_ps( &transpose.m[  0 ] );
    __m128 row2 = _mm_load_ps( &transpose.m[  4 ] );
    __m128 row3 = _mm_load_ps( &transpose.m[  8 ] );
    __m128 row4 = _mm_load_ps( &transpose.m[ 12 ] );

    __m128 r1 = _mm_mul_ps( row1, vec4 );
    __m128 r2 = _mm_mul_ps( row2, vec4 );
    __m128 r3 = _mm_mul_ps( row3, vec4 );
    __m128 r4 = _mm_mul_ps( row4, vec4 );

    __m128 sum_01 = _mm_hadd_ps( r1, r2 );
    __m128 sum_23 = _mm_hadd_ps( r3, r4 );
    __m128 result = _mm_hadd_ps( sum_01, sum_23 );
    _mm_store_ps( &v4.x, result );
    out->x = v4.x;
    out->y = v4.y;
    out->z = v4.z;
    }*/

Vec3 localToRaster( Vec3 v, const Matrix44* localToClip )
{
    Vec3 vertexNDC;
    transformPoint( v, localToClip, &vertexNDC );

    Vec3 output;
    output.x = WIDTH * 0.5f + vertexNDC.x * WIDTH  * 0.5f / vertexNDC.z;
    output.y = HEIGHT * 0.5f + vertexNDC.y * HEIGHT * 0.5f / vertexNDC.z;
    output.z = vertexNDC.z;

    return output;
}

float toSRGB( float f )
{
    if (f > 1)
    {
        f = 1;
    }

    if (f < 0)
    {
        f = 0;
    }

    float s = f * 12.92f;

    if (s > 0.0031308f)
    {
        s = 1.055f * pow( f, 1 / 2.4f ) - 0.055f;
    }

    return s;
}

// http://entropymine.com/imageworsener/srgbformula/
float sRGBToLinear( float s )
{
    if (s > 1)
    {
        s = 1;
    }

    if (s < 0)
    {
        s = 0;
    }

    if (s >= 0 && s <= 0.0404482362771082f)
    {
        return s / 12.92f;
    }

    if (s > 0.0404482362771082f && s <= 1)
    {
        return pow( (s + 0.055f) / 1.055f, 2.4f );
    }

    return 0;
}

bool isBackface( float x1, float y1, float x2, float y2, float x3, float y3 )
{
    return ((x3 - x1) * (y3 - y2) - (x3 - x2)*(y3 - y1)) < 0;
}

void calculateNormal( Plane* plane )
{
    const Vec3 v1 = sub( plane->a, plane->b );
    const Vec3 v2 = sub( plane->c, plane->b );
    plane->normal = cross( v2, v1 );
    normalize( &plane->normal );
    plane->d = -( dot( plane->normal, plane->b ) );
}

void frustumSetProjection( Frustum* frustum, float fieldOfView, float aAspect, float aNear, float aFar )
{

}

void updateFrustum( Frustum* frustum, Vec3 cameraPosition, Vec3 cameraDirection )
{

}

bool BoxInFrustum( Frustum* frustum, Vec3 vMin, Vec3 vMax )
{
    bool result = true;

    Vec3 pos;

    // Determines positive and negative vertex in relation to the normal.
    for (unsigned p = 0; p < 6; ++p)
    {
        pos = vMin;

        if (frustum->planes[ p ].normal.x >= 0)
        {
            pos.x = vMax.x;
        }
        if (frustum->planes[ p ].normal.y >= 0)
        {
            pos.y = vMax.y;
        }
        if (frustum->planes[ p ].normal.z >= 0)
        {
            pos.z = vMax.z;
        }

        if (planeDistance( &frustum->planes[ p ], pos ) < 0)
        {
            return false;
        }
    }

    return result;
}

