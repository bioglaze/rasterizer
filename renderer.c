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

// Vertices must be in CCW order! texture must be a 64x64 4-channel 32-bit format.
void drawTriangle( Vertex* v1, Vertex* v2, Vertex* v3, int rowPitch, int* texture, float* zBuffer, int* outBuffer )
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

                if (z < targetZ[ x ] )
                {
                    continue;
                }

                targetZ[ x ] = z;
                
                s *= z;
                t *= z;
                
                int ix = s * 63.0f + 0.5f;
                int iy = t * 63.0f + 0.5f;

                target[ x ] = texture[ (int)( iy * 64.0f + ix ) ];
            }
        }

        target += WIDTH;
        targetZ += WIDTH;
    }
}

// Vertices must be in CCW order! texture must be a 64x64 4-channel 32-bit format.
// Optimized version of drawTriangle().
void drawTriangle2( Vertex* v1, Vertex* v2, Vertex* v3, int rowPitch, int* texture, float* zBuffer, int* outBuffer )
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

    float invArea = 1.0f / edgeFunction( x1, y1, x2, y2, x3, y3 );

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

                w0 *= invArea;
                w1 *= invArea;
                w2 *= invArea;

                float s = w0 * s1 + w1 * s2 + w2 * s3;
                float t = w0 * t1 + w1 * t2 + w2 * t3;

                float z = 1.0f / (w0 * z1 + w1 * z2 + w2 * z3);

                if (z < targetZ[ x ] )
                {
                    continue;
                }

                targetZ[ x ] = z;
                
                s *= z;
                t *= z;
                
                int ix = s * 63.0f + 0.5f;
                int iy = t * 63.0f + 0.5f;

                target[ x ] = texture[ (int)( iy * 64.0f + ix ) ];
            }
        }

        target += WIDTH;
        targetZ += WIDTH;
    }
}

