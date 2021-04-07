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

Vec3 normalized( Vec3 v )
{
    const float invLen = 1.0f / sqrtf( v.x * v.x + v.y * v.y + v.z * v.z );
    v.x *= invLen;
    v.y *= invLen;
    v.z *= invLen;

    return v;
}

Vec3 neg( Vec3 v )
{
    return (Vec3){ -v.x, -v.y, -v.z };
}

Vec3 sub( Vec3 a, Vec3 b )
{
    return (Vec3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec3 add( Vec3 a, Vec3 b )
{
    return (Vec3){ a.x + b.x, a.y + b.y, a.z + b.z };
}

Vec3 mulf( Vec3 v, float f )
{
    return (Vec3){ v.x * f, v.y * f, v.z * f };
}
