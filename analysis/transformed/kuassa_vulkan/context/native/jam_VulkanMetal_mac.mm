/**
 * @file jam_VulkanMetal_mac.mm
 * @brief macOS Metal layer creation and scale factor utilities for Vulkan surface setup.
 */

void* jam::createMetalLayerForView (void* nsView)
{
    NSView* view = (__bridge NSView*) nsView;

    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    metalLayer.device = MTLCreateSystemDefaultDevice();
    metalLayer.frame = view.bounds;
    metalLayer.opaque = NO;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.zPosition = 1;
    metalLayer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
    metalLayer.maximumDrawableCount = 3;

    const CGFloat scale = (view.window != nil) ? view.window.backingScaleFactor
                                                : [NSScreen mainScreen].backingScaleFactor;
    metalLayer.contentsScale = scale;
    metalLayer.drawableSize = CGSizeMake (view.bounds.size.width * scale,
                                           view.bounds.size.height * scale);

    [view.layer addSublayer: metalLayer];

    return (__bridge void*) metalLayer;
}

float jam::getScaleFactor (void* nsView)
{
    NSView* view = (__bridge NSView*) nsView;

    if (view.window != nil)
        return static_cast<float> (view.window.backingScaleFactor);

    return static_cast<float> ([NSScreen mainScreen].backingScaleFactor);
}

void jam::updateMetalLayerFrame (void* layer, int width, int height, float scale)
{
    CAMetalLayer* metalLayer = (__bridge CAMetalLayer*) layer;
    metalLayer.frame = CGRectMake (0, 0, width, height);
    metalLayer.contentsScale = scale;
    metalLayer.drawableSize = CGSizeMake (width * scale, height * scale);
}

jam::Size<int> jam::getMetalLayerSize (void* layer)
{
    CAMetalLayer* metalLayer = (__bridge CAMetalLayer*) layer;

    return jam::Size<int> (static_cast<int> (metalLayer.frame.size.width),
                           static_cast<int> (metalLayer.frame.size.height));
}
