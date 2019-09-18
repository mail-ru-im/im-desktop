#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

#import "shader_types.h"

struct RasterizerData
{
    float4 position [[position]];
    float2 textureCoordinate;
};

// Vertex Function
vertex RasterizerData
vertexShader(uint vertexID [[ vertex_id ]],
             constant Vertex *vertexArray [[ buffer(VerticesIndex) ]],
             constant vector_uint2 *viewportSizePointer  [[ buffer(ViewportSizeIndex) ]])

{

    RasterizerData out;

    float2 pixelSpacePosition = vertexArray[vertexID].position.xy;


    float2 viewportSize = float2(*viewportSizePointer);

    out.position = vector_float4(0.0, 0.0, 0.0, 1.0);
    out.position.xy = pixelSpacePosition / (viewportSize / 2.0);

    out.textureCoordinate = vertexArray[vertexID].textureCoordinate;

    return out;
}

// Fragment function
fragment float4
samplingShader(RasterizerData in [[stage_in]],
               texture2d<half> colorTexture [[ texture(TextureIndexBaseColor) ]])
{
    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);

    const half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);

    return float4(colorSample);
}

