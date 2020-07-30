#ifndef shader_types_h
#define shader_types_h

#include <simd/simd.h>

enum VertexInputIndex
{
    VerticesIndex     = 0,
    ViewportSizeIndex = 1,
};

enum TextureIndex
{
    TextureIndexBaseColor = 0,
};

struct Vertex
{
    // Positions in pixel space.
    vector_float2 position;

    // 2D texture coordinate
    vector_float2 textureCoordinate;
};

#endif /* shader_types_h */
