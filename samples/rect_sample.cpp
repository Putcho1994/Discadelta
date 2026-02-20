import ufox_discadelta_lib;
import ufox_discadelta_core;

#include <cmath>
#include <iostream>
#include <thread>


using namespace ufox::geometry::discadelta;

// ─────────────────────────────────────────────────────────────────────────────
// Debug print for RectSegmentContext tree (2D version)
// Prints name, width/height, x/y, and indents children
// ─────────────────────────────────────────────────────────────────────────────
void PrintTreeDebugWithOffset(const RectSegmentContext& ctx, int indent = 0) noexcept {
    std::string pad(indent * 4, ' ');
    std::cout << pad << ctx.config.name
              << " | w: " << ctx.content.width
              << " | h: " << ctx.content.height
              << " | x: " << ctx.content.x
              << " | y: " << ctx.content.y
              << "\n";

    for (const auto* child : ctx.children) {
        PrintTreeDebugWithOffset(*child, indent + 1);
    }
}

int main() {
    std::cout << "Nester Rect Tree Debugger Test\n\n";

    const std::string title = "Rect Tree Debug";

    auto root = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>({
            .name          = "Root",
            .width         = 0.0f,
            .widthMin      = 0.0f,
            .widthMax      = std::numeric_limits<float>::max(),
            .height        = 0.0f,
            .heightMin     = 0.0f,
            .heightMax     = std::numeric_limits<float>::max(),
            .direction     = FlexDirection::Row,
            .flexCompress  = 1.0f,
            .flexExpand    = 1.0f,
            .order         = 0
            });

    auto rect1 = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>({
            .name          = "Rect01",
            .width         = 0.0f,
            .widthMin      = 50.0f,
            .widthMax      = std::numeric_limits<float>::max(),
            .height        = 0.0f,
            .heightMin     = 50.0f,
            .heightMax     = std::numeric_limits<float>::max(),
            .direction     = FlexDirection::Row,
            .flexCompress  = 1.0f,
            .flexExpand    = 1.0f,
            .order         = 0
            });

    auto rect2 = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>({
            .name          = "Rect02",
            .width         = 0.0f,
            .widthMin      = 50.0f,
            .widthMax      = std::numeric_limits<float>::max(),
            .height        = std::numeric_limits<float>::max(),
            .heightMin     = 00.0f,
            .heightMax     = std::numeric_limits<float>::max(),
            .direction     = FlexDirection::Row,
            .flexCompress  = 1.0f,
            .flexExpand    = 1.0f,
            .order         = 0
            });

    Link(*root.get(), *rect1.get());
    Link(*root.get(), *rect2.get());

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // ────────────────────────────────────────────────────────────────
    // First test: small size (400×600) → should compress
    // ────────────────────────────────────────────────────────────────
    Sizing(*root.get(), 400.0f, 600.0f, 0.0f, 0.0f, false);
    Placing(*root.get());

    std::cout << "=== " << title << " (size 400x600) ===" << std::endl;
    PrintTreeDebugWithOffset(*root.get());

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // ────────────────────────────────────────────────────────────────
    // Second test: larger size (800×600) → should expand (with rounding)
    // ────────────────────────────────────────────────────────────────
    Sizing(*root.get(), 800.0f, 600.0f, 0.0f, 0.0f, true);
    Placing(*root.get());

    std::cout << std::endl;
    std::cout << "=== " << title << " (size 800x600, rounded) ===" << std::endl;
    PrintTreeDebugWithOffset(*root.get());

    std::cout << "branchCount: " << root->branchCount;

    std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}
