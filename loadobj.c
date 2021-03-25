typedef struct
{
    unsigned posInd[ 3 ];
    unsigned uvInd[ 3 ];
    unsigned normInd[ 3 ];
} OBJFace;

typedef struct
{
    Vec3* positions;
    unsigned positionCount;
    unsigned positionOffset;
    UV* uvs;
    unsigned uvCount;
    unsigned uvOffset;
    Vec3* normals;
    unsigned normalCount;
    unsigned normalOffset;
    OBJFace* faces;
    unsigned faceCount;
} OBJMesh;

// Returns an array of meshes. Caller must deallocate them.
OBJMesh* initMeshArrays( FILE* file )
{
    char line[ 255 ];

    OBJMesh* meshes = malloc( sizeof( OBJMesh ) * 10 );
    unsigned meshCount = 0;

    unsigned positionCount = 0;
    unsigned uvCount = 0;
    unsigned normalCount = 0;
    unsigned faceCount = 0;
    unsigned totalPositionCount = 0;
    unsigned totalUVCount = 0;
    unsigned totalNormalCount = 0;

    while (fgets( line, 255, file ) != NULL)
    {
        char input[ 255 ];
        sscanf( line, "%254s", input );

        if (strstr( input, "vn" ))
        {
            ++normalCount;
            ++totalNormalCount;
        }
        else if (strstr( input, "vt" ))
        {
            ++uvCount;
            ++totalUVCount;
        }
        else if (strstr( input, "f" ))
        {
            ++faceCount;
        }
        else if (strstr( input, "s" ))
        {
            char str1[ 128 ];
            char smoothName[ 128 ];
            sscanf( line, "%127s %127s", str1, smoothName );

            if (strstr( smoothName, "off" ) == NULL)
            {
                printf( "Warning: The file contains smoothing groups. They are not supported by the converter.\n" );
            }
        }
        else if (strstr( input, "v" ))
        {
            ++positionCount;
            ++totalPositionCount;
        }

        if (strcmp( input, "o" ) == 0 || strcmp( input, "g" ) == 0)
        {
            if (meshCount != 0)
            {
                meshes[ meshCount - 1 ].faceCount = faceCount;
                meshes[ meshCount - 1 ].positionCount = positionCount;
                meshes[ meshCount - 1 ].uvCount = uvCount;
                meshes[ meshCount - 1 ].normalCount = normalCount;

                meshes[ meshCount - 1 ].faces = malloc( sizeof( OBJFace ) * faceCount );
                meshes[ meshCount - 1 ].positions = malloc( sizeof( Vec3 ) * positionCount );
                meshes[ meshCount - 1 ].uvs = malloc( sizeof( UV ) * uvCount );
                meshes[ meshCount - 1 ].normals = malloc( sizeof( Vec3 ) * normalCount );

                meshes[ meshCount ].positionOffset = totalPositionCount;
                meshes[ meshCount ].uvOffset = totalUVCount;
                meshes[ meshCount ].normalOffset = totalNormalCount;

                faceCount = 0;
                positionCount = 0;
                uvCount = 0;
                normalCount = 0;
            }

            ++meshCount;
        }
    }

    // Last mesh
    meshes[ meshCount - 1 ].positionOffset = 0;
    meshes[ meshCount - 1 ].uvOffset = 0;
    meshes[ meshCount - 1 ].normalOffset = 0;
    meshes[ meshCount - 1 ].faceCount = faceCount;
    meshes[ meshCount - 1 ].positionCount = positionCount;
    meshes[ meshCount - 1 ].uvCount = uvCount;
    meshes[ meshCount - 1 ].normalCount = normalCount;
    meshes[ meshCount - 1 ].faces = malloc( sizeof( OBJFace ) * faceCount );
    meshes[ meshCount - 1 ].positions = malloc( sizeof( Vec3 ) * positionCount );
    meshes[ meshCount - 1 ].uvs = malloc( sizeof( UV ) * uvCount );
    meshes[ meshCount - 1 ].normals = malloc( sizeof( Vec3 ) * normalCount );

    fseek( file, 0, SEEK_SET );

    unsigned positionIndex = 0;
    unsigned uvIndex = 0;
    unsigned normalIndex = 0;
    int meshIndex = -1;

    while (fgets( line, 255, file ) != NULL)
    {
        char input[ 255 ];
        sscanf( line, "%254s", input );

        if (strstr( input, "vn" ))
        {
            Vec3* normal = &meshes[ meshIndex ].normals[ normalIndex ];
            sscanf( line, "%254s %f %f %f", input, &normal->x, &normal->y, &normal->z );
            ++normalIndex;
        }
        else if (strstr( input, "vt" ))
        {
            float u, v;
            sscanf( line, "%254s %f %f", input, &u, &v );
            v = 1 - v;
            meshes[ meshIndex ].uvs[ uvIndex ].u = u;
            meshes[ meshIndex ].uvs[ uvIndex ].v = v;
            ++uvIndex;
        }
        else if (strstr( input, "v" ))
        {
            Vec3* pos = &meshes[ meshIndex ].positions[ positionIndex ];
            sscanf( line, "%254s %f %f %f", input, &pos->x, &pos->y, &pos->z );
            ++positionIndex;
        }
        if (strcmp( input, "o" ) == 0 || strcmp( input, "g" ) == 0)
        {
            ++meshIndex;
            positionIndex = 0;
            uvIndex = 0;
            normalIndex = 0;
        }
    }

    return meshes;
}

bool AlmostEqualsVec( const Vec3* v1, const Vec3* v2 )
{
    return fabs( v1->x - v2->x ) < 0.0001f &&
           fabs( v1->y - v2->y ) < 0.0001f &&
           fabs( v1->z - v2->z ) < 0.0001f;
}

bool AlmostEqualsUV( const UV* uv1, const UV* uv2 )
{
    return fabs( uv1->u - uv2->u ) < 0.0001f && fabs( uv1->v - uv2->v ) < 0.0001f;
}

void createFinalGeometry( const OBJMesh* objMesh, Mesh* mesh )
{
    VertexInd newFace = { 0 };

    mesh->positions = malloc( sizeof( Vec3 ) * 153636 );
    mesh->uvs = malloc( sizeof( UV ) * 153636 );
    mesh->normals = malloc( sizeof( Vec3 ) * 153636 );
    mesh->faces = malloc( sizeof( VertexInd ) * 128343 );
    mesh->faceCount = 0;
    mesh->vertexCount = 0;
    
    for (unsigned f = 0; f < objMesh->faceCount; ++f)
    {
        Vec3 pos = objMesh->positions[ objMesh->faces[ f ].posInd[ 0 ] ];
        Vec3 norm = objMesh->normals[ objMesh->faces[ f ].normInd[ 0 ] ];
        UV uv = objMesh->uvs[ objMesh->faces[ f ].uvInd[ 0 ] ];

        bool found = false;

        for (unsigned i = 0; i < mesh->faceCount; ++i)
        {
            if (AlmostEqualsVec( &pos, &mesh->positions[ mesh->faces[ i ].a ] ) &&
                AlmostEqualsUV( &uv, &mesh->uvs[ mesh->faces[ i ].a ] ) &&
                AlmostEqualsVec( &norm, &mesh->normals[ mesh->faces[ i ].a ] ))
            {
                found = true;
                newFace.a = mesh->faces[ i ].a;
                break;
            }
        }

        if (!found)
        {
            mesh->positions[ mesh->vertexCount ] = pos;
            mesh->uvs[ mesh->vertexCount ] = uv;
            mesh->normals[ mesh->vertexCount ] = norm;

            ++mesh->vertexCount;
            assert( mesh->vertexCount < 153636 );

            newFace.a = (unsigned short)(mesh->vertexCount - 1);
        }

        pos = objMesh->positions[ objMesh->faces[ f ].posInd[ 1 ] ];
        norm = objMesh->normals[ objMesh->faces[ f ].normInd[ 1 ] ];
        uv = objMesh->uvs[ objMesh->faces[ f ].uvInd[ 1 ] ];

        found = false;

        for (unsigned i = 0; i < mesh->faceCount; ++i)
        {
            if (AlmostEqualsVec( &pos, &mesh->positions[ mesh->faces[ i ].b ] ) &&
                AlmostEqualsUV( &uv, &mesh->uvs[ mesh->faces[ i ].b ] ) &&
                AlmostEqualsVec( &norm, &mesh->normals[ mesh->faces[ i ].b ] ))
            {
                found = true;
                newFace.b = mesh->faces[ i ].b;
                break;
            }
        }

        if (!found)
        {
            mesh->positions[ mesh->vertexCount ] = pos;
            mesh->uvs[ mesh->vertexCount ] = uv;
            mesh->normals[ mesh->vertexCount ] = norm;

            ++mesh->vertexCount;
            assert( mesh->vertexCount < 153636 );

            newFace.b = (unsigned short)(mesh->vertexCount - 1);
        }

        pos = objMesh->positions[ objMesh->faces[ f ].posInd[ 2 ] ];
        norm = objMesh->normals[ objMesh->faces[ f ].normInd[ 2 ] ];
        uv = objMesh->uvs[ objMesh->faces[ f ].uvInd[ 2 ] ];

        found = false;

        for (unsigned i = 0; i < mesh->faceCount; ++i)
        {
            if (AlmostEqualsVec( &pos, &mesh->positions[ mesh->faces[ i ].c ] ) &&
                AlmostEqualsUV( &uv, &mesh->uvs[ mesh->faces[ i ].c ] ) &&
                AlmostEqualsVec( &norm, &mesh->normals[ mesh->faces[ i ].c ] ))
            {
                found = true;
                newFace.c = mesh->faces[ i ].c;
                break;
            }
        }

        if (!found)
        {
            mesh->positions[ mesh->vertexCount ] = pos;
            mesh->uvs[ mesh->vertexCount ] = uv;
            mesh->normals[ mesh->vertexCount ] = norm;

            ++mesh->vertexCount;
            assert( mesh->vertexCount < 153636 );

            newFace.c = (unsigned short)(mesh->vertexCount - 1);
        }

        mesh->faces[ f ] = newFace;
        ++mesh->faceCount;
        assert( mesh->faceCount < 128343 );
    }

    for (unsigned i = 0; i < mesh->vertexCount; ++i)
    {
        if (mesh->positions[ i ].x < mesh->aabbMin.x)
        {
            mesh->aabbMin.x = mesh->positions[ i ].x;
        }

        if (mesh->positions[ i ].y < mesh->aabbMin.y)
        {
            mesh->aabbMin.y = mesh->positions[ i ].y;
        }

        if (mesh->positions[ i ].z < mesh->aabbMin.z)
        {
            mesh->aabbMin.z = mesh->positions[ i ].z;
        }

        if (mesh->positions[ i ].x > mesh->aabbMax.x)
        {
            mesh->aabbMax.x = mesh->positions[ i ].x;
        }

        if (mesh->positions[ i ].y > mesh->aabbMax.y)
        {
            mesh->aabbMax.y = mesh->positions[ i ].y;
        }

        if (mesh->positions[ i ].z > mesh->aabbMax.z)
        {
            mesh->aabbMax.z = mesh->positions[ i ].z;
        }
    }
}

void loadObj( const char* path, Mesh* outMesh )
{
    FILE* file = fopen( path, "rb" );

    if (!file)
    {
        printf( "Could not open %s\n", path );
        return;
    }

    OBJMesh* meshes = initMeshArrays( file );

    fseek( file, 0, SEEK_SET );

    char line[ 255 ];
    unsigned faceIndex = 0;
    int meshCount = 0;

    while (fgets( line, 255, file ) != NULL)
    {
        char input[ 255 ] = { 0 };
        sscanf( line, "%254s", input );

        if (strcmp( input, "o" ) == 0 || strcmp( input, "g" ) == 0)
        {
            ++meshCount;
            faceIndex = 0;
        }
        else if (strstr( input, "f" ))
        {
            OBJFace* face = &meshes[ meshCount - 1 ].faces[ faceIndex ];
            sscanf( line, "%254s %u/%u/%u %u/%u/%u %u/%u/%u", input, &face->posInd[ 0 ], &face->uvInd[ 0 ], &face->normInd[ 0 ],
                    &face->posInd[ 1 ], &face->uvInd[ 1 ], &face->normInd[ 1 ], &face->posInd[ 2 ], &face->uvInd[ 2 ], &face->normInd[ 2 ] );

            face->posInd[ 0 ] -= meshes[ meshCount - 1 ].positionOffset + 1;
            face->posInd[ 1 ] -= meshes[ meshCount - 1 ].positionOffset + 1;
            face->posInd[ 2 ] -= meshes[ meshCount - 1 ].positionOffset + 1;

            face->uvInd[ 0 ] -= meshes[ meshCount - 1 ].uvOffset + 1;
            face->uvInd[ 1 ] -= meshes[ meshCount - 1 ].uvOffset + 1;
            face->uvInd[ 2 ] -= meshes[ meshCount - 1 ].uvOffset + 1;

            face->normInd[ 0 ] -= meshes[ meshCount - 1 ].normalOffset + 1;
            face->normInd[ 1 ] -= meshes[ meshCount - 1 ].normalOffset + 1;
            face->normInd[ 2 ] -= meshes[ meshCount - 1 ].normalOffset + 1;

            ++faceIndex;
        }
    }

    fclose( file );

    for (int m = 0; m < meshCount; ++m)
    {
        createFinalGeometry( &meshes[ m ], outMesh );
    }
}
