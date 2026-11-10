#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cmath>
#include <cstdio>

static void set_rgb(CGContextRef ctx, uint32_t hex, float a = 1.0f) {
    float r = ((hex >> 16) & 0xFF) / 255.0f;
    float g = ((hex >> 8) & 0xFF) / 255.0f;
    float b = (hex & 0xFF) / 255.0f;
    CGContextSetRGBFillColor(ctx, r, g, b, a);
    CGContextSetRGBStrokeColor(ctx, r, g, b, a);
}

int main(int argc, char** argv) {
    const char* out_path = (argc > 1) ? argv[1] : "AppIcon.png";
    const int W = 1024;
    const int H = 1024;

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(nullptr, W, H, 8, W * 4, cs,
                                             kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    if (!ctx) return 1;

    // Flip coordinate system so (0,0) is top-left
    CGContextTranslateCTM(ctx, 0, H);
    CGContextScaleCTM(ctx, 1.0, -1.0);

    // 1. macOS Squircle Clip (80px inset, 200px corner radius)
    CGRect card = CGRectMake(52, 52, W - 104, H - 104);
    CGPathRef clip_path = CGPathCreateWithRoundedRect(card, 200, 200, nullptr);
    CGContextSaveGState(ctx);
    CGContextAddPath(ctx, clip_path);
    CGContextClip(ctx);

    // 2. Deep Midnight Radio Sky Gradient (Dark Sleek Studio Aesthetic: #050811 -> #0B1222 -> #131E35)
    CGFloat bg_Colors[] = {
        0.02f, 0.04f, 0.08f, 1.0f,
        0.05f, 0.08f, 0.15f, 1.0f,
        0.08f, 0.13f, 0.23f, 1.0f,
        0.04f, 0.06f, 0.11f, 1.0f
    };
    CGFloat bg_Locs[] = {0.0f, 0.38f, 0.76f, 1.0f};
    CGGradientRef bg_grad = CGGradientCreateWithColorComponents(cs, bg_Colors, bg_Locs, 4);
    CGContextDrawLinearGradient(ctx, bg_grad, CGPointMake(512, 52), CGPointMake(512, 972), 0);
    CGGradientRelease(bg_grad);

    // 3. Radial RF Aura behind the antenna emitter
    const float ant_x = 512.0f;
    const float ant_y = 230.0f;

    CGFloat aura_Colors[] = {
        0.00f, 0.90f, 1.00f, 0.40f,
        0.06f, 0.72f, 0.51f, 0.18f,
        0.08f, 0.14f, 0.28f, 0.06f,
        0.00f, 0.00f, 0.00f, 0.00f
    };
    CGFloat aura_Locs[] = {0.0f, 0.35f, 0.70f, 1.0f};
    CGGradientRef aura_grad = CGGradientCreateWithColorComponents(cs, aura_Colors, aura_Locs, 4);
    CGContextDrawRadialGradient(ctx, aura_grad,
                                CGPointMake(ant_x, ant_y), 15.0f,
                                CGPointMake(ant_x, ant_y + 40.0f), 520.0f,
                                kCGGradientDrawsAfterEndLocation);
    CGGradientRelease(aura_grad);

    // 4. Concentric Stylized RF Propagation Waves (Pure Antenna Radiation Pattern)
    // 5 progressive expanding arcs radiating outward from the tip
    for (int i = 1; i <= 5; ++i) {
        float rad = 78.0f * i + 14.0f;
        float alpha = 0.90f - i * 0.15f;
        uint32_t col = (i % 2 == 1) ? 0x00E5FF : 0x10B981;
        set_rgb(ctx, col, alpha);
        CGContextSetLineWidth(ctx, 16.0f - i * 1.8f);
        CGContextSetLineCap(ctx, kCGLineCapRound);

        // Left RF wave arc
        CGContextBeginPath(ctx);
        CGContextAddArc(ctx, ant_x, ant_y, rad, 3.14159265f - 0.72f, 3.14159265f + 0.72f, 0);
        CGContextStrokePath(ctx);

        // Right RF wave arc
        CGContextBeginPath(ctx);
        CGContextAddArc(ctx, ant_x, ant_y, rad, -0.72f, 0.72f, 0);
        CGContextStrokePath(ctx);
    }

    // 5. Stylized Guy Wires supporting the Antenna Mast
    set_rgb(ctx, 0x38BDF8, 0.35f);
    CGContextSetLineWidth(ctx, 4.5f);
    CGContextBeginPath(ctx);
    // Upper guy wires from (512, 380) out to base anchors
    CGContextMoveToPoint(ctx, 512, 380);
    CGContextAddLineToPoint(ctx, 160, 890);
    CGContextMoveToPoint(ctx, 512, 380);
    CGContextAddLineToPoint(ctx, 864, 890);
    // Mid guy wires from (512, 540)
    CGContextMoveToPoint(ctx, 512, 540);
    CGContextAddLineToPoint(ctx, 230, 890);
    CGContextMoveToPoint(ctx, 512, 540);
    CGContextAddLineToPoint(ctx, 794, 890);
    CGContextStrokePath(ctx);

    // 6. Antenna Concrete Base & Mounting Pedestal (Clean Geometric Platform)
    // Foundation shadow / rim
    set_rgb(ctx, 0x0F172A, 0.95f);
    CGRect base_rect = CGRectMake(310, 875, 404, 35);
    CGPathRef base_path = CGPathCreateWithRoundedRect(base_rect, 10, 10, nullptr);
    CGContextAddPath(ctx, base_path);
    CGContextFillPath(ctx);
    set_rgb(ctx, 0x334155, 1.0f);
    CGContextSetLineWidth(ctx, 4.0f);
    CGContextAddPath(ctx, base_path);
    CGContextStrokePath(ctx);
    CGPathRelease(base_path);

    // Foundation upper bevel
    set_rgb(ctx, 0x1E293B, 1.0f);
    CGRect top_base = CGRectMake(360, 855, 304, 24);
    CGPathRef top_base_path = CGPathCreateWithRoundedRect(top_base, 8, 8, nullptr);
    CGContextAddPath(ctx, top_base_path);
    CGContextFillPath(ctx);
    set_rgb(ctx, 0x475569, 1.0f);
    CGContextSetLineWidth(ctx, 3.0f);
    CGContextAddPath(ctx, top_base_path);
    CGContextStrokePath(ctx);
    CGPathRelease(top_base_path);

    // 7. Heavy-Duty Stylized Antenna Lattice Tower (tapering from 855 down to 230)
    // Left & right main tubular uprights
    const float base_left_x = 420.0f;
    const float base_right_x = 604.0f;
    const float base_y = 855.0f;
    const float top_left_x = 496.0f;
    const float top_right_x = 528.0f;
    const float top_y = 285.0f;

    // Glowing subtle neon backing along the tower core
    set_rgb(ctx, 0x00E5FF, 0.22f);
    CGContextSetLineWidth(ctx, 28.0f);
    CGContextBeginPath(ctx);
    CGContextMoveToPoint(ctx, 512, base_y);
    CGContextAddLineToPoint(ctx, 512, ant_y);
    CGContextStrokePath(ctx);

    // Tower Lattice X-Braces (8 sturdy bays)
    const int kNumBays = 8;
    set_rgb(ctx, 0x94A3B8, 0.95f);
    CGContextSetLineWidth(ctx, 6.0f);
    CGContextSetLineCap(ctx, kCGLineCapRound);

    for (int i = 0; i < kNumBays; ++i) {
        float t0 = static_cast<float>(i) / kNumBays;
        float t1 = static_cast<float>(i + 1) / kNumBays;
        float y0 = base_y - t0 * (base_y - top_y);
        float y1 = base_y - t1 * (base_y - top_y);
        float xl0 = base_left_x + t0 * (top_left_x - base_left_x);
        float xr0 = base_right_x - t0 * (base_right_x - top_right_x);
        float xl1 = base_left_x + t1 * (top_left_x - base_left_x);
        float xr1 = base_right_x - t1 * (base_right_x - top_right_x);

        CGContextBeginPath(ctx);
        // Horizontal tier strut
        CGContextMoveToPoint(ctx, xl0, y0);
        CGContextAddLineToPoint(ctx, xr0, y0);
        // Diagonal cross-braces
        CGContextMoveToPoint(ctx, xl0, y0);
        CGContextAddLineToPoint(ctx, xr1, y1);
        CGContextMoveToPoint(ctx, xr0, y0);
        CGContextAddLineToPoint(ctx, xl1, y1);
        CGContextStrokePath(ctx);
    }

    // Top tier strut
    CGContextBeginPath(ctx);
    CGContextMoveToPoint(ctx, top_left_x, top_y);
    CGContextAddLineToPoint(ctx, top_right_x, top_y);
    CGContextStrokePath(ctx);

    // Main structural uprights (Left and Right tower legs)
    set_rgb(ctx, 0xF1F5F9, 1.0f);
    CGContextSetLineWidth(ctx, 14.0f);
    CGContextSetLineCap(ctx, kCGLineCapRound);
    CGContextSetLineJoin(ctx, kCGLineJoinRound);

    CGContextBeginPath(ctx);
    CGContextMoveToPoint(ctx, base_left_x, base_y);
    CGContextAddLineToPoint(ctx, top_left_x, top_y);
    CGContextMoveToPoint(ctx, base_right_x, base_y);
    CGContextAddLineToPoint(ctx, top_right_x, top_y);
    CGContextStrokePath(ctx);

    // 8. Vertical Antenna Mast (extends straight up from tower apex through beacon)
    set_rgb(ctx, 0xE2E8F0, 1.0f);
    CGContextSetLineWidth(ctx, 11.0f);
    CGContextBeginPath(ctx);
    CGContextMoveToPoint(ctx, 512, top_y + 10.0f);
    CGContextAddLineToPoint(ctx, 512, 185);
    CGContextStrokePath(ctx);

    // 9. Multi-Band HF / VHF Yagi Beam Elements (Sleek Horizontal Dipole Crossbars)
    // Element 1 (Reflector - Lower)
    set_rgb(ctx, 0x00E5FF, 1.0f);
    CGContextSetLineWidth(ctx, 11.0f);
    CGContextBeginPath(ctx);
    CGContextMoveToPoint(ctx, 360, 365);
    CGContextAddLineToPoint(ctx, 664, 365);
    CGContextStrokePath(ctx);

    // Tip caps for Reflector
    set_rgb(ctx, 0x38BDF8, 1.0f);
    CGContextFillEllipseInRect(ctx, CGRectMake(360 - 9, 365 - 9, 18, 18));
    CGContextFillEllipseInRect(ctx, CGRectMake(664 - 9, 365 - 9, 18, 18));

    // Element 2 (Driven element - Mid)
    set_rgb(ctx, 0x00E5FF, 1.0f);
    CGContextSetLineWidth(ctx, 9.5f);
    CGContextBeginPath(ctx);
    CGContextMoveToPoint(ctx, 400, 305);
    CGContextAddLineToPoint(ctx, 624, 305);
    CGContextStrokePath(ctx);

    // Tip caps for Driven element
    set_rgb(ctx, 0x38BDF8, 1.0f);
    CGContextFillEllipseInRect(ctx, CGRectMake(400 - 8, 305 - 8, 16, 16));
    CGContextFillEllipseInRect(ctx, CGRectMake(624 - 8, 305 - 8, 16, 16));

    // Element 3 (Director - High)
    set_rgb(ctx, 0x38BDF8, 1.0f);
    CGContextSetLineWidth(ctx, 8.0f);
    CGContextBeginPath(ctx);
    CGContextMoveToPoint(ctx, 442, 250);
    CGContextAddLineToPoint(ctx, 582, 250);
    CGContextStrokePath(ctx);

    // Tip caps for Director
    set_rgb(ctx, 0x7DD3FC, 1.0f);
    CGContextFillEllipseInRect(ctx, CGRectMake(442 - 7, 250 - 7, 14, 14));
    CGContextFillEllipseInRect(ctx, CGRectMake(582 - 7, 250 - 7, 14, 14));

    // Element 4 (Upper VHF stub)
    set_rgb(ctx, 0x7DD3FC, 1.0f);
    CGContextSetLineWidth(ctx, 6.5f);
    CGContextBeginPath(ctx);
    CGContextMoveToPoint(ctx, 474, 212);
    CGContextAddLineToPoint(ctx, 550, 212);
    CGContextStrokePath(ctx);

    // 10. Glowing Antenna Apex Beacon (RF Feedpoint Beacon Orb)
    // Outermost pulse ring
    set_rgb(ctx, 0x00E5FF, 0.28f);
    CGContextFillEllipseInRect(ctx, CGRectMake(ant_x - 48, ant_y - 48, 96, 96));
    // Mid glow
    set_rgb(ctx, 0x00E5FF, 0.55f);
    CGContextFillEllipseInRect(ctx, CGRectMake(ant_x - 30, ant_y - 30, 60, 60));
    // Core golden warm glow
    set_rgb(ctx, 0xFBBF24, 0.95f);
    CGContextFillEllipseInRect(ctx, CGRectMake(ant_x - 18, ant_y - 18, 36, 36));
    // Intense hot white filament center
    set_rgb(ctx, 0xFFFFFF, 1.0f);
    CGContextFillEllipseInRect(ctx, CGRectMake(ant_x - 9, ant_y - 9, 18, 18));

    // Tip terminal beacon at very top of mast (512, 185)
    set_rgb(ctx, 0xEF4444, 0.90f);
    CGContextFillEllipseInRect(ctx, CGRectMake(ant_x - 7, 185 - 7, 14, 14));
    set_rgb(ctx, 0xFFFFFF, 1.0f);
    CGContextFillEllipseInRect(ctx, CGRectMake(ant_x - 3, 185 - 3, 6, 6));

    // 11. Outer Squircle Neon Rim Highlight
    CGContextRestoreGState(ctx);
    set_rgb(ctx, 0x38BDF8, 0.85f);
    CGContextSetLineWidth(ctx, 10.0f);
    CGContextAddPath(ctx, clip_path);
    CGContextStrokePath(ctx);
    CGPathRelease(clip_path);

    // Export PNG
    CGImageRef img = CGBitmapContextCreateImage(ctx);
    CFStringRef cf_path = CFStringCreateWithCString(nullptr, out_path, kCFStringEncodingUTF8);
    CFURLRef url = CFURLCreateWithFileSystemPath(nullptr, cf_path, kCFURLPOSIXPathStyle, false);
    CGImageDestinationRef dest = CGImageDestinationCreateWithURL(url, CFSTR("public.png"), 1, nullptr);
    if (dest && img) {
        CGImageDestinationAddImage(dest, img, nullptr);
        CGImageDestinationFinalize(dest);
    }
    if (dest) CFRelease(dest);
    if (url) CFRelease(url);
    if (cf_path) CFRelease(cf_path);
    if (img) CGImageRelease(img);
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
    return 0;
}
