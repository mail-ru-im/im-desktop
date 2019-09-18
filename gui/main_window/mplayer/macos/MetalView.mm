#include "stdafx.h"

#include "MetalView.h"

#import "shader_types.h"

@implementation MetalView
{
    id<MTLTexture> texture_;

    id<MTLBuffer> vertices_;

    id<MTLRenderPipelineState> pipelineState_;

    id<MTLCommandQueue> commandQueue_;

    NSUInteger numVertices_;

    vector_uint2 viewportSize_;

    QSize frameSize_;
    QSize scaledSize_;
}

- (instancetype)initWithFrame:(CGRect)frameRect device : (id<MTLDevice>) device
{
    if (self = [super initWithFrame : frameRect
                             device : device])
    {
        [self initMetal];
    }

    return self;
}

- (void) initMetal
{
    self.device = MTLCreateSystemDefaultDevice();
    self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.autoResizeDrawable = true;

    NSError *error = NULL;

    QFile shadersFile(qsl(":/videoplayer/shaders"));
    shadersFile.open(QIODevice::ReadOnly);
    QByteArray shaders = shadersFile.readAll();
    dispatch_data_t data = dispatch_data_create(shaders.constData(), shaders.size(),
                                                dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT);

    id <MTLLibrary> shaderLibrary = [self.device newLibraryWithData : data
                                                              error : &error];
    dispatch_release(data);
    if (error)
        qWarning() << "Shader Error" << error;

    // create command queue for usage during drawing
    commandQueue_ = [self.device newCommandQueue];

    // load shaders
    id <MTLFunction> fragmentProgram = [shaderLibrary newFunctionWithName : @"samplingShader"];
    id <MTLFunction> vertexProgram = [shaderLibrary newFunctionWithName : @"vertexShader"];

    //  create a pipeline state
    MTLRenderPipelineDescriptor *pQuadPipelineStateDescriptor = [MTLRenderPipelineDescriptor new];
    pQuadPipelineStateDescriptor.colorAttachments[0].pixelFormat = self.colorPixelFormat;
    pQuadPipelineStateDescriptor.vertexFunction   = vertexProgram;
    pQuadPipelineStateDescriptor.fragmentFunction = fragmentProgram;
    NSError *pipErr = nil;
    pipelineState_ = [self.device newRenderPipelineStateWithDescriptor : pQuadPipelineStateDescriptor
                                                                 error : &pipErr];

    if (pipErr)
        NSLog(@"newRenderPipelineStateWithDescriptor err: %@", pipErr);
}

- (void) updateVerticesWithSize : (NSSize) size
{
    const Vertex quadVertices[] =
    {
            // Pixel positions, Texture coordinates
            { {  static_cast<float>(size.width/2.f),  static_cast<float>(-size.height/2.f) },  { 1.f, 1.f } },
            { { static_cast<float>(-size.width/2.f),  static_cast<float>(-size.height/2.f) },  { 0.f, 1.f } },
            { { static_cast<float>(-size.width/2.f),   static_cast<float>(size.height/2.f) },  { 0.f, 0.f } },

            { {  static_cast<float>(size.width/2.f),  static_cast<float>(-size.height/2.f) },  { 1.f, 1.f } },
            { { static_cast<float>(-size.width/2.f),   static_cast<float>(size.height/2.f) },  { 0.f, 0.f } },
            { {  static_cast<float>(size.width/2.f),   static_cast<float>(size.height/2.f) },  { 1.f, 0.f } },
    };
    
    vertices_ = [self.device newBufferWithBytes : quadVertices
                                         length : sizeof(quadVertices)
                                        options : MTLResourceStorageModeShared];

    numVertices_ = sizeof(quadVertices) / sizeof(Vertex);
}

- (void) setFrameImage : (CGImageRef) image
{
    [texture_ release];

    MTKTextureLoader *loader = [[MTKTextureLoader alloc] initWithDevice : self.device];
    NSError *err = nil;
    texture_ = [loader newTextureWithCGImage : image
                                     options : nil
                                       error : &err];
    [loader release];
}

- (void) setFrameWithSize : (CGImageRef) image size : (NSSize) size
{
    [self setFrameImage : image];

    frameSize_ = QSize(size.width, size.height);

    QSize scaledSize = frameSize_.scaled(QSize(viewportSize_.x, viewportSize_.y), Qt::KeepAspectRatio);

    if (scaledSize != scaledSize_)
        [self updateVerticesWithSize : NSMakeSize(scaledSize.width(), scaledSize.height())];

    scaledSize_ = scaledSize;
}

- (void)setFrameSize: (NSSize) newSize;
{
    viewportSize_.x = newSize.width;
    viewportSize_.y = newSize.height;

    auto scaledSize = frameSize_.scaled(QSize(newSize.width, newSize.height), Qt::KeepAspectRatio);
    [self updateVerticesWithSize : NSMakeSize(scaledSize.width(), scaledSize.height())];

    [super setFrameSize : newSize];
}

- (void)drawRect:(CGRect)rect
{
    id <MTLCommandBuffer> commandBuffer = [commandQueue_ commandBuffer];
    MTLRenderPassDescriptor *renderPassDescriptor = self.currentRenderPassDescriptor;

    id <MTLRenderCommandEncoder>  renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor : renderPassDescriptor];

    [renderEncoder setRenderPipelineState : pipelineState_];

    [renderEncoder setVertexBuffer : vertices_
                            offset : 0
                           atIndex : VerticesIndex];

    [renderEncoder setVertexBytes : &viewportSize_
                           length : sizeof(viewportSize_)
                          atIndex : ViewportSizeIndex];

    [renderEncoder setFragmentTexture : texture_
                              atIndex : 0];
    
    [renderEncoder drawPrimitives : MTLPrimitiveTypeTriangle
                      vertexStart : 0
                      vertexCount : numVertices_];

    [renderEncoder endEncoding];
    [commandBuffer presentDrawable : self.currentDrawable];
    [commandBuffer commit];
}

@end
