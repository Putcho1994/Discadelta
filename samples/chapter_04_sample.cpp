#include <iomanip>
#include <iostream>
#include <thread>
#include <algorithm>

import ufox_discadelta_lib;
import ufox_discadelta_core;

using namespace ufox::geometry::discadelta;

// ─────────────────────────────────────────────────────────────────────────────
// Debug print for LinearSegmentContext tree (1D version)
// Prints name, distance, offset, and indents children
// ─────────────────────────────────────────────────────────────────────────────
void PrintTreeDebugWithOffset(const LinearSegmentContext& ctx, int indent = 0) noexcept {
    std::string pad(indent * 4, ' ');
    std::cout << pad << ctx.config.name
              << " | distance: " << ctx.content.distance
              << " | offset: "   << ctx.content.offset
              << " | base: "     << ctx.content.base
              << " | expandDelta: " << ctx.content.expandDelta
              << "\n";

    for (const auto* child : ctx.children) {
        PrintTreeDebugWithOffset(*child, indent + 1);
    }
}

int main() {
    std::cout << "Nester Linear Tree Debugger Test\n\n";

    const std::string title = "Linear Tree Debug";

    // ────────────────────────────────────────────────────────────────
    // Create root (horizontal layout by default)
    // ────────────────────────────────────────────────────────────────
    auto root = CreateSegmentContext<LinearSegmentContext, LinearSegmentCreateInfo>(
        LinearSegmentCreateInfo{
            .name          = "Root",
            .base          = Length{LengthUnitType::Flat, 0.0f},
            .flexCompress  = 1.0f,
            .flexExpand    = 1.0f,
            .min           = 0.0f,
            .max           = std::numeric_limits<float>::max(),
            .order         = 0
        });

    // ────────────────────────────────────────────────────────────────
    // Create panels (some auto, some fixed, some nested)
    // ────────────────────────────────────────────────────────────────
    auto panelA = CreateSegmentContext<LinearSegmentContext, LinearSegmentCreateInfo>(
        LinearSegmentCreateInfo{
            .name         = "PanelA",
            .base         = Length{LengthUnitType::Flat, 200.0f},
            .flexCompress = 0.5f,
            .flexExpand   = 1.0f,
            .min          = 100.0f,
            .max          = std::numeric_limits<float>::max(),
            .order        = 0
        }
    );

    auto panelB = CreateSegmentContext<LinearSegmentContext, LinearSegmentCreateInfo>(
        LinearSegmentCreateInfo{
            .name         = "PanelB",
            .base         = Length{LengthUnitType::Auto, 0.0f},
            .flexCompress = 1.0f,
            .flexExpand   = 2.0f,
            .min          = 150.0f,
            .max          = std::numeric_limits<float>::max(),
            .order        = 1
        }
    );

    auto panelC = CreateSegmentContext<LinearSegmentContext, LinearSegmentCreateInfo>(
        LinearSegmentCreateInfo{
            .name         = "PanelC",
            .base         = Length{LengthUnitType::Flat, 200.0f},
            .flexCompress = 0.8f,
            .flexExpand   = 0.5f,
            .min          = 120.0f,
            .max          = std::numeric_limits<float>::max(),
            .order        = 2
        }
    );

    auto panelB1 = CreateSegmentContext<LinearSegmentContext, LinearSegmentCreateInfo>(
        LinearSegmentCreateInfo{
            .name         = "PanelB1",
            .base         = Length{LengthUnitType::Flat, 100.0f},
            .flexCompress = 0.0f,
            .flexExpand   = 1.0f,
            .min          = 80.0f,
            .max          = std::numeric_limits<float>::max(),
            .order        = 0
        }
    );

    auto panelB2 = CreateSegmentContext<LinearSegmentContext, LinearSegmentCreateInfo>(
        LinearSegmentCreateInfo{
            .name         = "PanelB2",
            .base         = Length{LengthUnitType::Flat, 100.0f},
            .flexCompress = 0.0f,
            .flexExpand   = 1.0f,
            .min          = 80.0f,
            .max          = std::numeric_limits<float>::max(),
            .order        = 1
        }
    );

    auto panelB3 = CreateSegmentContext<LinearSegmentContext, LinearSegmentCreateInfo>(
        LinearSegmentCreateInfo{
            .name         = "PanelB3",
            .base         = Length{LengthUnitType::Auto, 0.0f},
            .flexCompress = 1.0f,
            .flexExpand   = 0.5f,
            .min          = 50.0f,
            .max          = std::numeric_limits<float>::max(),
            .order        = 2
        }
    );

    // ────────────────────────────────────────────────────────────────
    // Build hierarchy (same nesting as your Rect example)
    // ────────────────────────────────────────────────────────────────
    Link(*root.get(), *panelA.get());
    Link(*root.get(), *panelB.get());
    Link(*root.get(), *panelC.get());

    Link(*panelB.get(), *panelB1.get());
    Link(*panelB.get(), *panelB2.get());
    Link(*panelB.get(), *panelB3.get());

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // ────────────────────────────────────────────────────────────────
    // First test: small size (400) → should compress
    // ────────────────────────────────────────────────────────────────
    Sizing(*root.get(), 400.0f, 0.0f, false);
    Placing(*root.get());

    std::cout << "=== " << title << " (size 400) ===" << std::endl;
    PrintTreeDebugWithOffset(*root.get());

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // ────────────────────────────────────────────────────────────────
    // Second test: larger size (800) → should expand
    // ────────────────────────────────────────────────────────────────
    Sizing(*root.get(), 800.0f, 0.0f, true);  // with rounding
    Placing(*root.get());

    std::cout << std::endl;
    std::cout << "=== " << title << " (size 800, rounded) ===" << std::endl;
    PrintTreeDebugWithOffset(*root.get());

    std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}
