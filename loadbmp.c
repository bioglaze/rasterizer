// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#pragma pack(push)
#pragma pack( 1 )
typedef struct
{
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;    
} BMPHeader;
#pragma pack( pop )

typedef struct
{
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits;
    uint32_t compression;
    uint32_t imageSize;
    int32_t xResolution;
    int32_t yResolution;
    uint32_t colors;
    uint32_t importantColors;
} BMPInfo;

// Only supports uncompressed images.
// Allocates memory for outPixels. Caller should free() it.
// Returns pixel data that will contain RGBA data, regardless of the input file.
int* loadBMP( const char* path, int* outWidth, int* outHeight )
{
    FILE* file = fopen( path, "rb" );

    if (!file)
    {
        printf( "Could not open %s\n", path );
        exit( 1 );
    }

    BMPHeader header = { 0 };
    fread( &header, 1, sizeof( header ), file );

    BMPInfo info = { 0 };
    fread( &info, 1, sizeof( info ), file );
    printf( "texture %s width: %d, height: %d, bpp: %d, bits / 8: %d\n", path, info.width, info.height, info.bits, info.bits / 8 );

    if (info.height < 0)
    {
        printf( "%s has negative height, loadBMP() doesn't handle flipping the image upside-down yet.\n", path );
        info.height = -info.height;    
    }
    
    if (info.compression != 0)
    {
        printf( "%s must be uncompressed!\n", path );
    }

    *outWidth = info.width;
    *outHeight = info.height;

    int* outPixels = malloc( info.width * info.height * 4 );
    char* imageData = malloc( info.width * info.height * info.bits / 8 );
    fread( imageData, 1, info.width * info.height * info.bits / 8, file );

    int j = 0;
    for (int i = 0; i < info.width * info.height; ++i)
    {
        outPixels[ j + 0 ] = imageData[ i * 3 ];
        ++j;
    }

    /*for (int y = 0; y < info.height; ++y)
    {
        for (int x = 0; x < info.width * 4; ++x)
        {
            outPixels[ y * info.width * 4 + x ] = outPixels[ (info.height - y - 1) * info.width * 4 + x ];
            //outPixels[ y * info.width * 4 + x * 4 + 1 ] = outPixels[ (info.height - y - 1) * info.width * 4 + x * 4 + 1];
            //outPixels[ y * info.width * 4 + x * 4 + 2 ] = outPixels[ (info.height - y - 1) * info.width * 4 + x * 4 + 2];
            //outPixels[ y * info.width * 4 + x * 4 + 3 ] = outPixels[ (info.height - y - 1) * info.width * 4 + x * 4 + 3];
        }
        }*/
    
    fclose( file );
    free( imageData );
    
    return outPixels;
}
