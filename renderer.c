// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/*#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
*/

typedef struct
{
    float x, y, z;
    float u, v;
} Vertex;

typedef struct
{
    unsigned short a, b, c;
} VertexInd;

typedef struct
{
    float u, v;
} UV;

typedef struct
{
    Vec3* positions;
    unsigned vertexCount;
    Vec3* normals;
    UV* uvs;
    VertexInd* faces;
    unsigned faceCount;
    Vec3 aabbMin;
    Vec3 aabbMax;
} Mesh;

float edgeFunction( float ax, float ay, float bx, float by, float cx, float cy )
{
    return (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
}

float orient2D( float ax, float ay, float bx, float by, float cx, float cy )
{
    return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

// Vertices must be in CCW order! texture must be a 4-channel 32-bit format.
// Reference implementation, not optimized. Optimized version is below in drawTriangle2().
void drawTriangle( Vertex* v1, Vertex* v2, Vertex* v3, int rowPitch, int* texture, int texDim, float* zBuffer, int* outBuffer )
{
    float x1 = v1->x;
    float x2 = v2->x;
    float x3 = v3->x;
    float y1 = v1->y;
    float y2 = v2->y;
    float y3 = v3->y;

    float s1 = v1->u / v1->z;
    float s2 = v2->u / v2->z;
    float s3 = v3->u / v3->z;
    float t1 = v1->v / v1->z;
    float t2 = v2->v / v2->z;
    float t3 = v3->v / v3->z;

    float z1 = 1.0f / v1->z;
    float z2 = 1.0f / v2->z;
    float z3 = 1.0f / v3->z;

    int minx = fmin( x1, fmin( x2, x3 ) );
    int miny = fmin( y1, fmin( y2, y3 ) );
    int maxx = fmax( x1, fmax( x2, x3 ) );
    int maxy = fmax( y1, fmax( y2, y3 ) );

    // Clip against screen bounds
    minx = fmax( minx, 0 );
    miny = fmax( miny, 0 );
    maxx = fmin( maxx, WIDTH - 1 );
    maxy = fmin( maxy, HEIGHT - 1 );

    Uint32* target = (Uint32*)((Uint8*)outBuffer + miny * rowPitch);
    float* targetZ = (float*)((Uint8*)zBuffer + miny * rowPitch);

    float area = edgeFunction( x1, y1, x2, y2, x3, y3 );

    for (int y = miny; y <= maxy; ++y)
    {
        for (int x = minx; x <= maxx; ++x)
        {
            if ((x1 - x2) * (y - y1) - (y1 - y2) * (x - x1) > 0 &&
                (x2 - x3) * (y - y2) - (y2 - y3) * (x - x2) > 0 &&
                (x3 - x1) * (y - y3) - (y3 - y1) * (x - x3) > 0)
            {
                float w0 = edgeFunction( x2, y2, x3, y3, x, y );
                float w1 = edgeFunction( x3, y3, x1, y1, x, y );
                float w2 = edgeFunction( x1, y1, x2, y2, x, y );

                if (w0 < 0 || w1 < 0 || w2 < 0)
                {
                    //printf( "skip\n" );
                    continue;
                }

                w0 /= area;
                w1 /= area;
                w2 /= area;

                float s = w0 * s1 + w1 * s2 + w2 * s3;
                float t = w0 * t1 + w1 * t2 + w2 * t3;

                float z = 1.0f / (w0 * z1 + w1 * z2 + w2 * z3);

                //if (z > targetZ[ x ] )
                {
                //    continue;
                }

                targetZ[ x ] = z;

                s *= z;
                t *= z;

                int ix = s * ((float)texDim - 1.0f) + 0.5f;
                int iy = t * ((float)texDim - 1.0f) + 0.5f;

                target[ x ] = texture[ iy * texDim + ix ];
            }
        }

        target += WIDTH;
        targetZ += WIDTH;
    }
}

// Vertices must be in CCW order! texture must be a 4-channel 32-bit format.
// Optimized version of drawTriangle().
// texture dimension must be square (width == height)
void drawTriangle2( Vertex* v1, Vertex* v2, Vertex* v3, int rowPitch, int* texture, int texDim, float* zBuffer, int* outBuffer )
{
    float x1 = v1->x;
    float x2 = v2->x;
    float x3 = v3->x;
    float y1 = v1->y;
    float y2 = v2->y;
    float y3 = v3->y;

    float s1 = v1->u / v1->z;
    float s2 = v2->u / v2->z;
    float s3 = v3->u / v3->z;
    float t1 = v1->v / v1->z;
    float t2 = v2->v / v2->z;
    float t3 = v3->v / v3->z;

    float z1 = 1.0f / v1->z;
    float z2 = 1.0f / v2->z;
    float z3 = 1.0f / v3->z;

    int minx = round( fmin( x1, fmin( x2, x3 ) ) );
    int miny = round( fmin( y1, fmin( y2, y3 ) ) );
    int maxx = fmax( x1, fmax( x2, x3 ) );
    int maxy = fmax( y1, fmax( y2, y3 ) );

    // Clip against screen bounds
    minx = fmax( minx, 0 );
    miny = fmax( miny, 0 );
    maxx = fmin( maxx, WIDTH - 1 );
    maxy = fmin( maxy, HEIGHT - 1 );

    //assert( minx <= maxx && "minx == maxx" );
    //assert( miny <= maxy && "miny == maxy" );
    
    if (minx > maxx || miny > maxy)
        return;
        
    if (minx > WIDTH || miny > HEIGHT)
        return;
        
    if (maxx < 0 || maxy < 0)
        return;
        
    if (v1->z < 0 && v2->z < 0 && v3->z < 0)
        printf("cull?\n");    
    //printf( "minx: %d, miny: %d, maxx: %d, maxy: %d\n", minx, miny, maxx, maxy );
    //printf( "z1: %f, z2: %f, z3: %f\n", v1->z, v2->z, v3->z );
    float a01 = y1 - y2, b01 = x2 - x1;
    float a12 = y2 - y3, b12 = x3 - x2;
    float a20 = y3 - y1, b20 = x1 - x3;

    // Correct for filling convention. FIXME: Not sure if this is correct!
    int bias0 = a01 < 0 ? 0 : -1;
    int bias1 = a12 < 0 ? 0 : -1;
    int bias2 = a20 < 0 ? 0 : -1;
    //if (a01 < 0 || (a01 == 0 && b01 < 0)) bias0 = 0;
    //if (a12 < 0 || (a12 == 0 && b12 < 0)) bias1 = 0;
    //if (a20 < 0 || (a20 == 0 && b20 < 0)) bias2 = 0;

    float w0row = orient2D( x2, y2, x3, y3, minx, miny ) + bias0;
    float w1row = orient2D( x3, y3, x1, y1, minx, miny ) + bias1;
    float w2row = orient2D( x1, y1, x2, y2, minx, miny ) + bias2;

    Uint32* target = (Uint32*)((Uint8*)outBuffer + miny * rowPitch);
    float* targetZ = (float*)((Uint8*)zBuffer + miny * rowPitch);

    for (int y = miny; y <= maxy; ++y)
    {
        float w0 = w0row;
        float w1 = w1row;
        float w2 = w2row;

        for (int x = minx; x <= maxx; ++x)
        {
            float z = 1.0f / (w0 * z1 + w1 * z2 + w2 * z3);

            if (z > targetZ[ x ] && w0 >= 0 && w1 >= 0 && w2 >= 0)
            {
                targetZ[ x ] = z;

                float s = w0 * s1 + w1 * s2 + w2 * s3;
                float t = w0 * t1 + w1 * t2 + w2 * t3;
                s *= z;
                t *= z;

                int ix = s * ((float)texDim - 1.0f) + 0.5f;
                int iy = t * ((float)texDim - 1.0f) + 0.5f;

                ix = maxi( 0, mini( ix, texDim - 1 ) );
                iy = maxi( 0, mini( iy, texDim - 1 ) );

                target[ x ] = texture[ iy * texDim + ix ];
            }

            w0 += a12;
            w1 += a20;
            w2 += a01;
        }

        w0row += b12;
        w1row += b20;
        w2row += b01;

        target += WIDTH;
        targetZ += WIDTH;
    }
}

// Vertices must be in CCW order! texture must be a 4-channel 32-bit format.
// Optimized version of drawTriangle2(). Block-based approach.
// texture dimension must be square (width == height)
void drawTriangle3( Vertex* v1, Vertex* v2, Vertex* v3, int rowPitch, int* texture, int texDim, float* zBuffer, int* outBuffer )
{
    float x1 = v1->x;
    float x2 = v2->x;
    float x3 = v3->x;
    float y1 = v1->y;
    float y2 = v2->y;
    float y3 = v3->y;

    float s1 = v1->u / v1->z;
    float s2 = v2->u / v2->z;
    float s3 = v3->u / v3->z;
    float t1 = v1->v / v1->z;
    float t2 = v2->v / v2->z;
    float t3 = v3->v / v3->z;

    float z1 = 1.0f / v1->z;
    float z2 = 1.0f / v2->z;
    float z3 = 1.0f / v3->z;

    int minx = round( fmin( x1, fmin( x2, x3 ) ) );
    int miny = round( fmin( y1, fmin( y2, y3 ) ) );
    int maxx = fmax( x1, fmax( x2, x3 ) );
    int maxy = fmax( y1, fmax( y2, y3 ) );

    // Clip against screen bounds
    minx = fmax( minx, 0 );
    miny = fmax( miny, 0 );
    maxx = fmin( maxx, WIDTH - 1 );
    maxy = fmin( maxy, HEIGHT - 1 );

    float ratio = getRatio( (Vec3){ minx, miny, 0 }, (Vec3){ maxx, maxy, 0 } );
    bool useBlock = ratio > 0.4f && ratio < 1.6f;

    const int blockDim = 8;
    
    // Start in corner of 8x8 block
    minx &= ~(blockDim - 1);
    miny &= ~(blockDim - 1);
    
    float a01 = y1 - y2, b01 = x2 - x1;
    float a12 = y2 - y3, b12 = x3 - x2;
    float a20 = y3 - y1, b20 = x1 - x3;

    // Correct for filling convention. FIXME: Not sure if this is correct!
    int bias0 = a01 < 0 ? 0 : -1;
    int bias1 = a12 < 0 ? 0 : -1;
    int bias2 = a20 < 0 ? 0 : -1;
    //if (a01 < 0 || (a01 == 0 && b01 < 0)) bias0 = 0;
    //if (a12 < 0 || (a12 == 0 && b12 < 0)) bias1 = 0;
    //if (a20 < 0 || (a20 == 0 && b20 < 0)) bias2 = 0;

    float w0row = orient2D( x2, y2, x3, y3, minx, miny ) + bias0;
    float w1row = orient2D( x3, y3, x1, y1, minx, miny ) + bias1;
    float w2row = orient2D( x1, y1, x2, y2, minx, miny ) + bias2;

    Uint32* target = (Uint32*)((Uint8*)outBuffer + miny * rowPitch);
    float* targetZ = (float*)((Uint8*)zBuffer + miny * rowPitch);

    for (int y = miny; y <= maxy; y += blockDim)
    {
        float w0 = w0row;
        float w1 = w1row;
        float w2 = w2row;

        for (int x = minx; x <= maxx; x += blockDim)
        {
            // Corners of block.
            int x0 = x;
            int x1 = x + blockDim - 1;
            int y0 = y;
            int y1 = y + blockDim - 1;
            
            // TODO stuff...
            
            float z = 1.0f / (w0 * z1 + w1 * z2 + w2 * z3);

            if (z > targetZ[ x ] && w0 >= 0 && w1 >= 0 && w2 >= 0)
            {
                targetZ[ x ] = z;

                float s = w0 * s1 + w1 * s2 + w2 * s3;
                float t = w0 * t1 + w1 * t2 + w2 * t3;
                s *= z;
                t *= z;

                int ix = s * ((float)texDim - 1.0f) + 0.5f;
                int iy = t * ((float)texDim - 1.0f) + 0.5f;

                ix = maxi( 0, mini( ix, texDim - 1 ) );
                iy = maxi( 0, mini( iy, texDim - 1 ) );

                target[ x ] = texture[ iy * texDim + ix ];
            }

            w0 += a12;
            w1 += a20;
            w2 += a01;
        }

        w0row += b12;
        w1row += b20;
        w2row += b01;

        target += WIDTH;
        targetZ += WIDTH;
    }
}

void renderMesh( Mesh* mesh, Matrix44* localToClip, int pitch, int* texture, int texDim, float* zBuffer, int* outBuffer )
{
    double accumTriangleTime = 0;
    int renderedTriangleCount = 0;
    uint64_t accumCycles = 0;
    
    //int positionCount[ 32 ] = { 0 };

    for (unsigned f = 0; f < mesh->faceCount; ++f)
    {
        Vertex cv0;
        Vertex cv1;
        Vertex cv2;

        //positionCount[ mesh->faces[ f ].a ]++;
        //positionCount[ mesh->faces[ f ].b ]++;
        //positionCount[ mesh->faces[ f ].c ]++;

        Vec3 v = localToRaster( mesh->positions[ mesh->faces[ f ].a ], localToClip );
        cv0.x = v.x;
        cv0.y = v.y;
        cv0.z = v.z;
        cv0.u = mesh->uvs[ mesh->faces[ f ].a ].u;
        cv0.v = mesh->uvs[ mesh->faces[ f ].a ].v;

        v = localToRaster( mesh->positions[ mesh->faces[ f ].b ], localToClip );
        cv1.x = v.x;
        cv1.y = v.y;
        cv1.z = v.z;
        cv1.u = mesh->uvs[ mesh->faces[ f ].b ].u;
        cv1.v = mesh->uvs[ mesh->faces[ f ].b ].v;

        v = localToRaster( mesh->positions[ mesh->faces[ f ].c ], localToClip );
        cv2.x = v.x;
        cv2.y = v.y;
        cv2.z = v.z;
        cv2.u = mesh->uvs[ mesh->faces[ f ].c ].u;
        cv2.v = mesh->uvs[ mesh->faces[ f ].c ].v;

        uint64_t startTime = SDL_GetPerformanceCounter();
        uint64_t startCycles = __rdtsc();
        
        // Unoptimized:
        /*if (isBackface( cv0.x, cv0.y, cv1.x, cv1.y, cv2.x, cv2.y))
        {
            drawTriangle( &cv0, &cv1, &cv2, pitch, texture, texDim, zBuffer, outBuffer );
            ++renderedTriangleCount;
        }*/

        // Optimized:
        if (!isBackface( cv0.x, cv0.y, cv2.x, cv2.y, cv1.x, cv1.y))
        {
            drawTriangle2( &cv0, &cv2, &cv1, pitch, texture, texDim, zBuffer, outBuffer );
            ++renderedTriangleCount;
        }

        uint64_t currTime = SDL_GetPerformanceCounter();
        double elapsedTime = (double)((currTime - startTime) / (double)(SDL_GetPerformanceFrequency()));

        accumTriangleTime += elapsedTime;
        accumCycles += __rdtsc() - startCycles;
    }

    /*for (int i = 0; i < mesh->faceCount; ++i)
    {
        printf( "position %d hit rate: %d\n", i, positionCount[ i ] );
    }*/

    //printf( "One mesh's triangle rendering time: %f seconds, %llu cycles. Rendered triangle count: %d\n", accumTriangleTime, accumCycles, renderedTriangleCount );
}
