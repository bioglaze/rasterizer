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

void calculateNormal( Plane* plane )
{
    const Vec3 v1 = sub( plane->a, plane->b );
    const Vec3 v2 = sub( plane->c, plane->b );
    plane->normal = cross( v2, v1 );
    normalize( &plane->normal );
    plane->d = -( dot( plane->normal, plane->b ) );
}

void frustumSetProjection( Frustum* frustum, float fovDegrees, float aAspect, float aNear, float aFar )
{
    frustum->zNear = aNear;
    frustum->zFar  = aFar;

    // Computes width and height of the near and far plane sections.
    const float deg2rad = 3.14159265358979f / 180.0f;
    const float tang = tanf( deg2rad * fovDegrees * 0.5f );
    frustum->nearHeight = aNear * tang;
    frustum->nearWidth  = frustum->nearHeight * aAspect;
    frustum->farHeight  = frustum->zFar * tang;
    frustum->farWidth   = frustum->farHeight * aAspect;
}

void updateCornersAndCenters( Frustum* frustum, Vec3 cameraPosition, Vec3 zAxis )
{
    const Vec3 up = { 0, 1, 0 };
    Vec3 xAxis = cross( up, zAxis );
    normalize( &xAxis );
    Vec3 yAxis = cross( zAxis, xAxis );
    normalize( &yAxis );

    // Computes the centers of near and far planes.
    frustum->nearCenter = sub( cameraPosition, mulf( zAxis, frustum->zNear ) );
    frustum->farCenter  = sub( cameraPosition, mulf( zAxis, frustum->zFar ) );

    // Computes the near plane corners.
    frustum->nearTopLeft     = sub( add( frustum->nearCenter, mulf( yAxis, frustum->nearHeight ) ), mulf( xAxis, frustum->nearWidth ) );
    frustum->nearTopRight    = add( add( frustum->nearCenter, mulf( yAxis, frustum->nearHeight ) ), mulf( xAxis, frustum->nearWidth ) );
    frustum->nearBottomLeft  = sub( sub( frustum->nearCenter, mulf( yAxis, frustum->nearHeight ) ), mulf( xAxis, frustum->nearWidth ) );
    frustum->nearBottomRight = add( sub( frustum->nearCenter, mulf( yAxis, frustum->nearHeight ) ), mulf( xAxis, frustum->nearWidth ) );

    // Computes the far plane corners.
    frustum->farTopLeft     = sub( add( frustum->farCenter, mulf( yAxis, frustum->farHeight ) ), mulf( xAxis, frustum->farWidth ) );
    frustum->farTopRight    = add( add( frustum->farCenter, mulf( yAxis, frustum->farHeight ) ), mulf( xAxis, frustum->farWidth ) );
    frustum->farBottomLeft  = sub( sub( frustum->farCenter, mulf( yAxis, frustum->farHeight ) ), mulf( xAxis, frustum->farWidth ) );
    frustum->farBottomRight = add( sub( frustum->farCenter, mulf( yAxis, frustum->farHeight ) ), mulf( xAxis, frustum->farWidth ) );
}

void updateFrustum( Frustum* frustum, Vec3 cameraPosition, Vec3 cameraDirection )
{
    const Vec3 zAxis = cameraDirection;
    updateCornersAndCenters( frustum, cameraPosition, zAxis );

    enum FrustumPlane
    {
        Far = 0,
        Near,
        Bottom,
        Top,
        Left,
        Right
    };

    frustum->planes[ Top ].a = frustum->nearTopRight;
    frustum->planes[ Top ].b = frustum->nearTopLeft;
    frustum->planes[ Top ].c = frustum->farTopLeft;
    calculateNormal( &frustum->planes[ Top ] );

    frustum->planes[ Bottom ].a = frustum->nearBottomLeft;
    frustum->planes[ Bottom ].b = frustum->nearBottomRight;
    frustum->planes[ Bottom ].c = frustum->farBottomRight;
    calculateNormal( &frustum->planes[ Bottom ] );

    frustum->planes[ Left ].a = frustum->nearTopLeft;
    frustum->planes[ Left ].b = frustum->nearBottomLeft;
    frustum->planes[ Left ].c = frustum->farBottomLeft;
    calculateNormal( &frustum->planes[ Left ] );

    frustum->planes[ Right ].a = frustum->nearBottomRight;
    frustum->planes[ Right ].b = frustum->nearTopRight;
    frustum->planes[ Right ].c = frustum->farBottomRight;
    calculateNormal( &frustum->planes[ Right ] );

    planeSetNormalAndPoint( &frustum->planes[ Near ], neg( zAxis ), frustum->nearCenter );
    planeSetNormalAndPoint( &frustum->planes[ Far  ],  zAxis, frustum->farCenter  );
}

bool boxInFrustum( Frustum* frustum, Vec3 vMin, Vec3 vMax )
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
