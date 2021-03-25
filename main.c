// Author: Timo Wiren
// Modified: 2021-03-25
//
// Tested and profiled on MacBook Pro 2010, Intel Core 2 Duo P8600, 2.4 GHz, 3 MiB L2 cache, 1066 MHz FSB. Compiled using GCC 9.3.0.
//
// References:
// https://web.archive.org/web/20120212184251/http://devmaster.net/forums/topic/1145-advanced-rasterization/
// Barycentric interpolation from https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage
// https://www.scratchapixel.com/code.php?id=26&origin=/lessons/3d-basic-rendering/rasterization-practical-implementation
// https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
// View disassembly: objdump -drwC -Mintel main
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdalign.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <SDL2/SDL.h>

const unsigned WIDTH = 1920 / 2;
const unsigned HEIGHT = 1080 / 2;

#include "mymath.c"
#include "renderer.c"
#include "loadobj.c"
#include "loadbmp.c"

void renderMesh( Mesh* mesh, Matrix44* localToClip, int pitch, int* texture, float* zBuffer, int* outBuffer )
{
    clock_t clo;
    double accumTriangleTime = 0;
    
    for (unsigned f = 0; f < mesh->faceCount; ++f)
    {
        Vertex cv0;
        Vertex cv1;
        Vertex cv2;

        Vec3 v = localToRaster( &mesh->positions[ mesh->faces[ f ].a ], localToClip );
        cv0.x = v.x;
        cv0.y = v.y;
        cv0.z = v.z;
        cv0.u = mesh->uvs[ mesh->faces[ f ].a ].u;
        cv0.v = mesh->uvs[ mesh->faces[ f ].a ].v;

        v = localToRaster( &mesh->positions[ mesh->faces[ f ].b ], localToClip );
        cv1.x = v.x;
        cv1.y = v.y;
        cv1.z = v.z;
        cv1.u = mesh->uvs[ mesh->faces[ f ].b ].u;
        cv1.v = mesh->uvs[ mesh->faces[ f ].b ].v;

        v = localToRaster( &mesh->positions[ mesh->faces[ f ].c ], localToClip );
        cv2.x = v.x;
        cv2.y = v.y;
        cv2.z = v.z;
        cv2.u = mesh->uvs[ mesh->faces[ f ].c ].u;
        cv2.v = mesh->uvs[ mesh->faces[ f ].c ].v;

        clo = clock();
        drawTriangle( &cv0, &cv1, &cv2, pitch, texture, zBuffer, outBuffer );
        clo = clock() - clo;
        accumTriangleTime += ((double)clo) / CLOCKS_PER_SEC;
    }

    printf( "One mesh's triangle rendering time: %f seconds\n", accumTriangleTime );
}

int main()
{
    int texWidth = 0;
    int texHeight = 0;
    int* checkerTex = loadBMP( "checker.bmp", &texWidth, &texHeight );

    SDL_Init( SDL_INIT_VIDEO );
    const unsigned createFlags = SDL_WINDOW_SHOWN;
    SDL_Window* win = SDL_CreateWindow( "Software Rasterizer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, createFlags );
    SDL_Renderer* renderer = SDL_CreateRenderer( win, -1, 0 );
    if (!renderer)
    {
        printf( "Unable to create renderer\n" );
    }

    float* zBuf = malloc( WIDTH * HEIGHT * 4 );

    SDL_Texture* texture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT );
    void* pixels = NULL;
    int* backBuf = malloc( WIDTH * HEIGHT * 4 );
    int pitch = 0;

    Mesh cube;
    loadObj( "cube.obj", &cube );

    Matrix44 projMat;
    makeProjection( 45.0f, WIDTH / (float)HEIGHT, 0.1f, 100.0f, &projMat );
    
    float angleDeg = 0;
    
    while (1)
    {
        SDL_Event e;
        
        while (SDL_PollEvent( &e ))
        {
            if (e.type == SDL_QUIT)
            {
                free( checkerTex );
                return 0;
            }

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
            {
                free( backBuf );
                free( checkerTex );
                return 0;
            }
        }

        SDL_RenderClear( renderer );

        if (SDL_LockTexture( texture, NULL, &pixels, &pitch ) < 0)
        {
            printf( "SDL_LockTexture failed!\n" );
        }

        memset( pixels, 0, WIDTH * HEIGHT * 4 );
        memset( zBuf, 0, WIDTH * HEIGHT * 4 );
        
        Matrix44 meshLocalToWorld;
        makeIdentity( &meshLocalToWorld );
        meshLocalToWorld.m[ 12 ] = -2;
        meshLocalToWorld.m[ 13 ] = 0;
        meshLocalToWorld.m[ 14 ] = -5;

        Matrix44 rotation;
        makeRotationXYZ( angleDeg, angleDeg, angleDeg, &rotation );

        multiply( &rotation, &meshLocalToWorld, &meshLocalToWorld );

        Matrix44 localToClip;
        multiply( &meshLocalToWorld, &projMat, &localToClip );

        Matrix44 meshLocalToWorld2;
        makeIdentity( &meshLocalToWorld2 );
        meshLocalToWorld2.m[ 12 ] = 2;
        meshLocalToWorld2.m[ 13 ] = 0;
        meshLocalToWorld2.m[ 14 ] = -5;

        multiply( &rotation, &meshLocalToWorld2, &meshLocalToWorld2 );

        Matrix44 localToClip2;
        multiply( &meshLocalToWorld2, &projMat, &localToClip2 );

        angleDeg += 0.5f;

        renderMesh( &cube, &localToClip, pitch, checkerTex, zBuf, pixels );
        renderMesh( &cube, &localToClip2, pitch, checkerTex, zBuf, pixels );

        for (int y = 0; y < mini( texHeight, HEIGHT ); ++y)
        {
            for (int x = 0; x < mini( texWidth, WIDTH ); ++x)
            {
                backBuf[ y * WIDTH + x ] = checkerTex[ y * texWidth + x ];
            }
        }
        //memcpy( pixels, backBuf, WIDTH * HEIGHT * 4 );

        SDL_UnlockTexture( texture );

        SDL_RenderCopy( renderer, texture, NULL, NULL );
        SDL_RenderPresent( renderer );
    }

    free( backBuf );
    free( checkerTex );

    return 0;
}
