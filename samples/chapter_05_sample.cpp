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

    // // ────────────────────────────────────────────────────────────────
    // // Create root (Row direction)
    // // ────────────────────────────────────────────────────────────────
    // auto root = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>(
    //     RectSegmentCreateInfo{
    //         .name          = "Root",
    //         .width         = Length{LengthUnitType::Flat, 0.0f},
    //         .widthMin      = 0.0f,
    //         .widthMax      = std::numeric_limits<float>::max(),
    //         .height        = Length{LengthUnitType::Flat, 0.0f},
    //         .heightMin     = 0.0f,
    //         .heightMax     = std::numeric_limits<float>::max(),
    //         .direction     = FlexDirection::Row,
    //         .flexCompress  = 1.0f,
    //         .flexExpand    = 1.0f,
    //         .order         = 0
    //     }
    // );
    //
    // // ────────────────────────────────────────────────────────────────
    // // Create panels (similar ratios/min/max as your original test)
    // // ────────────────────────────────────────────────────────────────
    // auto panelA = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>(
    //     RectSegmentCreateInfo{
    //         .name          = "PanelA",
    //         .width         = Length{LengthUnitType::Flat, 200.0f},
    //         .widthMin      = 10.0f,
    //         .widthMax      = std::numeric_limits<float>::max(),
    //         .height        = Length{LengthUnitType::Auto, 50.0f},
    //         .heightMin     = 0.0f,
    //         .heightMax     = std::numeric_limits<float>::max(),
    //         .direction     = FlexDirection::Row,
    //         .flexCompress  = 1.0f,
    //         .flexExpand    = 1.0f,
    //         .order         = 0
    //     }
    // );
    //
    // auto panelB = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>(
    //     RectSegmentCreateInfo{
    //         .name          = "PanelB",
    //         .width         = Length{LengthUnitType::Auto, 0.0f},
    //         .widthMin      = 20.0f,
    //         .widthMax      = std::numeric_limits<float>::max(),
    //         .height        = Length{LengthUnitType::Auto, 0.0f},
    //         .heightMin     = 0.0f,
    //         .heightMax     = std::numeric_limits<float>::max(),
    //         .direction     = FlexDirection::Column,
    //         .flexCompress  = 1.0f,
    //         .flexExpand    = 1.0f,
    //         .order         = 1
    //     }
    // );
    //
    // auto panelC = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>(
    //     RectSegmentCreateInfo{
    //         .name          = "PanelC",
    //         .width         = Length{LengthUnitType::Flat, 200.0f},
    //         .widthMin      = 30.0f,
    //         .widthMax      = std::numeric_limits<float>::max(),
    //         .height        = Length{LengthUnitType::Auto, 0.0f},
    //         .heightMin     = 0.0f,
    //         .heightMax     = std::numeric_limits<float>::max(),
    //         .direction     = FlexDirection::Row,
    //         .flexCompress  = 1.0f,
    //         .flexExpand    = 1.0f,
    //         .order         = 2
    //     }
    // );
    //
    // auto panelB1 = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>(
    //     RectSegmentCreateInfo{
    //         .name          = "PanelB1",
    //         .width         = Length{LengthUnitType::Flat, 100.0f},
    //         .widthMin      = 0.0f,
    //         .widthMax      = std::numeric_limits<float>::max(),
    //         .height        = Length{LengthUnitType::Flat, 100.0f},
    //         .heightMin     = 110.0f,
    //         .heightMax     = std::numeric_limits<float>::max(),
    //         .direction     = FlexDirection::Column,
    //         .flexCompress  = 1.0f,
    //         .flexExpand    = 1.0f,
    //         .order         = 0
    //     }
    // );
    //
    // auto panelB2 = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>(
    //     RectSegmentCreateInfo{
    //         .name          = "PanelB2",
    //         .width         = Length{LengthUnitType::Flat, 100.0f},
    //         .widthMin      = 0.0f,
    //         .widthMax      = std::numeric_limits<float>::max(),
    //         .height        = Length{LengthUnitType::Flat, 100.0f},
    //         .heightMin     = 50.0f,
    //         .heightMax     = std::numeric_limits<float>::max(),
    //         .direction     = FlexDirection::Column,
    //         .flexCompress  = 1.0f,
    //         .flexExpand    = 1.0f,
    //         .order         = 1
    //     }
    // );
    //
    // auto panelB3 = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>(
    //     RectSegmentCreateInfo{
    //         .name          = "PanelB3",
    //         .width         = Length{LengthUnitType::Flat, 50.0f},
    //         .widthMin      = 0.0f,
    //         .widthMax      = std::numeric_limits<float>::max(),
    //         .height        = Length{LengthUnitType::Auto, 50.0f},
    //         .heightMin     = 0.0f,
    //         .heightMax     = std::numeric_limits<float>::max(),
    //         .direction     = FlexDirection::Column,
    //         .flexCompress  = 1.0f,
    //         .flexExpand    = 1.0f,
    //         .order         = 2
    //     }
    // );
    //
    // auto panelB1_1 = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>(
    //     RectSegmentCreateInfo{
    //         .name          = "PanelB1_1",
    //         .width         = Length{LengthUnitType::Auto, 100.0f},
    //         .widthMin      = 0.0f,
    //         .widthMax      = std::numeric_limits<float>::max(),
    //         .height        = Length{LengthUnitType::Auto, 50.0f},
    //         .heightMin     = 0.0f,
    //         .heightMax     = std::numeric_limits<float>::max(),
    //         .direction     = FlexDirection::Column,
    //         .flexCompress  = 1.0f,
    //         .flexExpand    = 1.0f,
    //         .order         = 2
    //     }
    // );
    //
    // // ────────────────────────────────────────────────────────────────
    // // Build hierarchy (same nesting as your original Rect test)
    // // ────────────────────────────────────────────────────────────────
    // Link(*root.get(), *panelA.get());
    // Link(*root.get(), *panelB.get());
    // Link(*root.get(), *panelC.get());
    //
    // Link(*panelB.get(), *panelB1.get());
    // Link(*panelB.get(), *panelB2.get());
    // Link(*panelB.get(), *panelB3.get());
    //
    // Link(*panelB1.get(), *panelB1_1.get());

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

    std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}
