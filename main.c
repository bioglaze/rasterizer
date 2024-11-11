// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// Author: Timo Wiren
// Modified: 2023-02-17
//
// Tested and profiled on MacBook Pro 2010, Intel Core 2 Duo P8600, 2.4 GHz, 3 MiB L2 cache, 1066 MHz FSB. Compiled using GCC 9.3.0.
//
// References:
// https://web.archive.org/web/20120212184251/http://devmaster.net/forums/topic/1145-advanced-rasterization/
// Barycentric interpolation from https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage
// https://www.scratchapixel.com/code.php?id=26&origin=/lessons/3d-basic-rendering/rasterization-practical-implementation
// https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
// https://kristoffer-dyrkorn.github.io/triangle-rasterizer/
// View disassembly: objdump -drwC -Mintel main
// Benchmarking: https://easyperf.net/notes/
//
// TODO:
// 4x3 matrices for non-projective stuff or mul(float4(v.xyz,1.0f),m) -> v.x*m[0]+(v.y*m[1]+(v.z*m[2]+m[3]));
// Frustum culling
// Verify that min() and max() are branchless
// vectorcall
// MAD
// Post-transform vertex cache
// Mipmaps
// SIMD triangle rendering: https://t0rakka.silvrback.com/software-rasterizer
// Hi-Z
// -march=x86_64-v2 (for MacBook Pro 2010)
// Optimize triangle test with (w0 | w1 | w2) >= 0 and check its disassembly.
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pmmintrin.h>
#if _MSC_VER
#include "SDL.h"
#include <intrin.h>
#else
#include <stdalign.h>
#include <SDL2/SDL.h>
#include <x86intrin.h>
#endif

const int WIDTH = 1920 / 2;
const int HEIGHT = 1080 / 2;

#include "vec3.c"
#include "mymath.c"
#include "frustum.c"
#include "renderer.c"
#include "loadobj.c"
#include "loadbmp.c"

typedef struct GameObject
{
    Vec3 position;
    Vec3 rotation;
} GameObject;

int main( int argc, char** argv )
{
    (void)argc;
    (void)argv;
    int texWidth = 0;
    int texHeight = 0;
    int* checkerTex = loadBMP( "checker.bmp", &texWidth, &texHeight );
    assert( texWidth == texHeight && "drawTriangle assumes square texture dimension!" );
    
    SDL_Init( SDL_INIT_VIDEO );
    const unsigned createFlags = SDL_WINDOW_SHOWN;
    SDL_Window* win = SDL_CreateWindow( "Software Rasterizer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, createFlags );
    SDL_Renderer* renderer = SDL_CreateRenderer( win, -1, SDL_RENDERER_PRESENTVSYNC );
    if (!renderer)
    {
        printf( "Unable to create renderer\n" );
    }

    float* zBuf = malloc( WIDTH * HEIGHT * 4 );

    SDL_Texture* renderTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT );
    void* pixels = NULL;
    int* backBuf = malloc( WIDTH * HEIGHT * 4 );
    int pitch = 0;

    Mesh cube[ 2 ];
    int cubeMeshCount = 1;
    loadObj( "cube.obj", &cube[ 0 ], &cubeMeshCount );
    
    Frustum cameraFrustum;

    Matrix44 projMat;
    makeProjection( 45.0f, WIDTH / (float)HEIGHT, 0.1f, 100.0f, &projMat );

    frustumSetProjection( &cameraFrustum, 45.0f, WIDTH / (float)HEIGHT, 0.1f, 100.0f );
    
    SDL_SetWindowGrab( win, SDL_TRUE );
    SDL_SetRelativeMouseMode( SDL_TRUE );

    float angleDeg = 0;

    Vec3 cameraPos = { 0, 0, 0 };
    Vec3 cameraFront;
    Vec3 cameraDir = { 0, 0, 0 };
    float yaw = 90;
    float cameraPitch = 0;

    GameObject scene[ 2 ];
    scene[ 0 ].position = (Vec3){ -2, 0, -5 };
    scene[ 1 ].position = (Vec3){ 2, 0, -5 };

    uint32_t startTime = SDL_GetTicks();
    double deltaTime = 0.0;
    
    while (1)
    {
        startTime = SDL_GetTicks();
        
        SDL_Event e;

        while (SDL_PollEvent( &e ))
        {
            if (e.type == SDL_QUIT)
            {
                free( checkerTex );
                free( zBuf );
                free( backBuf );
                return 0;
            }

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
            {
                free( cube[ 0 ].positions );
                free( cube[ 0 ].normals );
                free( cube[ 0 ].uvs );
                free( cube[ 0 ].faces );

                free( checkerTex );
                free( zBuf );
                free( backBuf );
                return 0;
            }

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_UP)
            {
                ++cameraPitch;
            }

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_DOWN)
            {
                --cameraPitch;
            }

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_LEFT)
            {
                ++yaw;
            }

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RIGHT)
            {
                --yaw;
            }

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_w)
            {
                cameraPos = add( cameraPos, mulf( cameraDir, 10.0f * (float)deltaTime ) );
            }

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_s)
            {
                cameraPos = sub( cameraPos, mulf( cameraDir, 10.0f * (float)deltaTime ) );
            }

            if (e.type == SDL_MOUSEMOTION)
            {
                float dx = (float)e.motion.xrel;
                float dy = (float)e.motion.yrel;
                dx *= 0.02f;
                dy *= 0.02f;
                yaw -= dx;
                cameraPitch -= dy;
            }
        }

        float yawRad = yaw * 3.14159265f / 180.0f;
        float pitchRad = cameraPitch * 3.14159265f / 180.0f;
        
        cameraDir.x = cosf( yawRad ) * cosf( pitchRad );
        cameraDir.y = sinf( pitchRad );
        cameraDir.z = sinf( yawRad ) * cosf( pitchRad );
        cameraFront = normalized( cameraDir );

        SDL_RenderClear( renderer );

        if (SDL_LockTexture( renderTexture, NULL, &pixels, &pitch ) < 0)
        {
            printf( "SDL_LockTexture failed!\n" );
        }

        memset( pixels, 0, WIDTH * HEIGHT * 4 );
        memset( zBuf, 0, WIDTH * HEIGHT * 4 );

        Matrix44 worldToView;
        makeLookat( cameraPos, add( cameraPos, cameraFront ), &worldToView );

        //printf( "cameraDir: %f, %f, %f, cameraFront: %f, %f, %f\n", cameraDir.x, cameraDir.y, cameraDir.z, cameraFront.x, cameraFront.y, cameraFront.z );
        updateFrustum( &cameraFrustum, cameraPos, cameraFront );

        for (int i = 0; i < 1; ++i)
        {
            Matrix44 meshLocalToWorld;
            makeIdentity( &meshLocalToWorld );
            meshLocalToWorld.m[ 12 ] = scene[ i ].position.x;
            meshLocalToWorld.m[ 13 ] = scene[ i ].position.y;
            meshLocalToWorld.m[ 14 ] = scene[ i ].position.z;

            Matrix44 rotation;
            makeRotationXYZ( angleDeg, angleDeg, angleDeg, &rotation );

            multiplySSE( &rotation, &meshLocalToWorld, &meshLocalToWorld );
        
            Matrix44 localToView;
            multiplySSE( &meshLocalToWorld, &worldToView, &localToView );

            Matrix44 localToClip;
            multiplySSE( &localToView, &projMat, &localToClip );

            angleDeg += 0.5f;

            Vec3 meshAabbWorld[ 8 ];
            Vec3 meshAabbMinWorld = cube[ 0 ].aabbMin;
            Vec3 meshAabbMaxWorld = cube[ 0 ].aabbMax;
            //printf( "aabMin: %f, %f, %f, aabMax: %f, %f, %f\n", cube.aabbMin.x, cube.aabbMin.y, cube.aabbMin.z, cube.aabbMax.x, cube.aabbMax.y, cube.aabbMax.z );
            getCorners( meshAabbMinWorld, meshAabbMaxWorld, meshAabbWorld );

            for (unsigned v = 0; v < 8; ++v)
            {
                Vec3 res;
                transformPoint( meshAabbWorld[ v ], &meshLocalToWorld, &res );
                meshAabbWorld[ v ] = res;
            }

            getMinMax( meshAabbWorld, 8, &meshAabbMinWorld, &meshAabbMaxWorld );

            if (cameraPos.x > meshAabbMinWorld.x && cameraPos.x < meshAabbMaxWorld.x &&
                cameraPos.y > meshAabbMinWorld.y && cameraPos.y < meshAabbMaxWorld.y &&
                cameraPos.z > meshAabbMinWorld.z && cameraPos.z < meshAabbMaxWorld.z)
            {
                printf("camera inside AABB\n");
            }

            if (boxInFrustum( &cameraFrustum, meshAabbMinWorld, meshAabbMaxWorld ))
            {           
                for (int subMesh = 0; subMesh < cubeMeshCount; ++subMesh)
                {
                    //printf( "minAABBWorld: %f, %f, %f, maxAABBWorld: %f, %f, %f\n", meshAabbMinWorld.x, meshAabbMinWorld.y, meshAabbMinWorld.z, meshAabbMaxWorld.x, meshAabbMaxWorld.y, meshAabbMaxWorld.z );
                    renderMesh( &cube[ subMesh ], &localToClip, pitch, checkerTex, texWidth, zBuf, pixels );
                }
            }
        }
        
        for (int y = 0; y < mini( texHeight, HEIGHT ); ++y)
        {
            for (int x = 0; x < mini( texWidth, WIDTH ); ++x)
            {
                backBuf[ y * WIDTH + x ] = checkerTex[ y * texWidth + x ];
            }
        }
        //memcpy( pixels, backBuf, WIDTH * HEIGHT * 4 );

        SDL_UnlockTexture( renderTexture );

        SDL_RenderCopy( renderer, renderTexture, NULL, NULL );
        SDL_RenderPresent( renderer );

        uint32_t endTime = SDL_GetTicks();
        deltaTime = (endTime - startTime) / 1000.0;
    }

    free( backBuf );
    free( checkerTex );

    return 0;
}
