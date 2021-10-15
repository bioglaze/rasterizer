// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

typedef struct
{
    float m[ 16 ];
} Matrix44;

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

void makeLookat( Vec3 eye, Vec3 center, Matrix44* mat )
{
    Vec3 zAxis = normalized( sub( center, eye ) );

    Vec3 up = { 0, 1, 0 };
    Vec3 xAxis = normalized( cross( up, zAxis ) );
    Vec3 yAxis = normalized( cross( zAxis, xAxis ) );

    mat->m[ 0 ] = xAxis.x; mat->m[ 1 ] = xAxis.y; mat->m[ 2 ] = xAxis.z; mat->m[ 3 ] = -dot( xAxis, eye );
    mat->m[ 4 ] = yAxis.x; mat->m[ 5 ] = yAxis.y; mat->m[ 6 ] = yAxis.z; mat->m[ 7 ] = -dot( yAxis, eye );
    mat->m[ 8 ] = zAxis.x; mat->m[ 9 ] = zAxis.y; mat->m[ 10 ] = zAxis.z; mat->m[ 11 ] = -dot( zAxis, eye );
    mat->m[ 12 ] = 0; mat->m[ 13 ] = 0; mat->m[ 14 ] = 0; mat->m[ 15 ] = 1;
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
#if _MSC_VER
    float result[ 16 ];
#else
    alignas( 16 ) float result[ 16 ];
#endif
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
    return ((x3 - x1) * (y3 - y2) - (x3 - x2) * (y3 - y1)) < 0;
}

// 1 is rectangle.
// If width is dominant, value increases.
// If height is dominant, the value reduces.
// Experience shows that triangles with value 0.4-1.6 should be rasterized using block-based approach.
// All other cases should not use block-based approach. 
// (Source: P. Mileff et al. Accelerated Half-Space Triangle Rasterization)
float getRatio( Vec3 aabbMin, Vec3 aabbMax )
{
    return (aabbMax.x - aabbMin.x) / (aabbMax.y - aabbMin.y);
}

void getCorners( Vec3 vMin, Vec3 vMax, Vec3 outCorners[ 8 ] )
{
    outCorners[ 0 ] = (Vec3){ vMin.x, vMin.y, vMin.z };
    outCorners[ 1 ] = (Vec3){ vMax.x, vMin.y, vMin.z };
    outCorners[ 2 ] = (Vec3){ vMin.x, vMax.y, vMin.z };
    outCorners[ 3 ] = (Vec3){ vMin.x, vMin.y, vMax.z };
    outCorners[ 4 ] = (Vec3){ vMax.x, vMax.y, vMin.z };
    outCorners[ 5 ] = (Vec3){ vMin.x, vMax.y, vMax.z };
    outCorners[ 6 ] = (Vec3){ vMax.x, vMax.y, vMax.z };
    outCorners[ 7 ] = (Vec3){ vMax.x, vMin.y, vMax.z };
}

void getMinMax( const Vec3* aPoints, int count, Vec3* outMin, Vec3* outMax )
{
    *outMin = aPoints[ 0 ];
    *outMax = aPoints[ 0 ];

    for (int i = 1; i < count; ++i)
    {
        Vec3 point = aPoints[ i ];

        if (point.x < outMin->x)
        {
            outMin->x = point.x;
        }

        if (point.y < outMin->y)
        {
            outMin->y = point.y;
        }

        if (point.z < outMin->z)
        {
            outMin->z = point.z;
        }

        if (point.x > outMax->x)
        {
            outMax->x = point.x;
        }

        if (point.y > outMax->y)
        {
            outMax->y = point.y;
        }

        if (point.z > outMax->z)
        {
            outMax->z = point.z;
        }
    }
}

