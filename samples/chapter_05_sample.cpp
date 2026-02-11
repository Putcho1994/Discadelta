import ufox_discadelta_lib;
import ufox_discadelta_core;

#include <iostream>
#include <thread>


using namespace ufox::geometry::discadelta;


void PrintTreeDebugWithOffset(const RectSegmentContext& ctx, int indent = 0) noexcept {

    std::string pad(indent * 4, ' ');
    std::cout << pad << ctx.config.name
              << " | w: " << ctx.content.width
              << " | h: " << ctx.content.height
              << "\n";

    for (const auto* child : ctx.children) {
        PrintTreeDebugWithOffset(*child, indent + 1);
    }
}

int main() {
    std::cout << "Nester Tree Debugger Test\n\n";

    const std::string title = "Nester Tree Debug";

    // LinearSegmentContext root{{"Root",     0.0f, 1.0f, 1.0f, 0.0f, std::numeric_limits<float>::max()}};
    // LinearSegmentContext panelA{{"PanelA",    200.0f, 0.7f, 0.1f, 0.0f, std::numeric_limits<float>::max(),1}};
    // LinearSegmentContext subA1{{"SubA1",     80.0f,   0.2f, 0.4f, 40.0f, 150.0f}};
    // LinearSegmentContext subA1_1{{"SubA1-1",    90.0f,    0.1f, 0.2f, 60.0f, 120.0f}};
    // LinearSegmentContext subA2{{"SubA2",     90.0f,    0.2f, 0.4f, 100.0f, 200.0f}};
    // LinearSegmentContext panelB{{"PanelB",    350.0f,    0.3f, 0.5f, 0.0f, 255.0f,0}};




    const auto root = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>({"Root",{LengthUnitType::Flat, 0.0f}, 0.0f, std::numeric_limits<float>::max(),
        {LengthUnitType::Flat, 0.0f}, 0.0f, std::numeric_limits<float>::max(), FlexDirection::Row, 1.0f, 1.0f,0});
    const auto panelA = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>({"PanelA", {LengthUnitType::Flat, 200.0f}, 10.0f, std::numeric_limits<float>::max(),
        {LengthUnitType::Auto, 50.0f}, 0.0f, std::numeric_limits<float>::max(), FlexDirection::Row, 1.0f, 1.0f,0});
    const auto panelB = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>({"PanelB", {LengthUnitType::Auto, 000.0f}, 20.0f, std::numeric_limits<float>::max(),
        {LengthUnitType::Auto, 0.0f}, 0.0f, std::numeric_limits<float>::max(), FlexDirection::Row, 1.0f, 1.0f,0});
    const auto panelC = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>({"PanelC", {LengthUnitType::Flat, 200.0f}, 30.0f, std::numeric_limits<float>::max(),
        {LengthUnitType::Auto, 0.0f}, 0.0f, std::numeric_limits<float>::max(), FlexDirection::Row, 1.0f, 1.0f,0});
    const auto panelB1= CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>({"PanelB1", {LengthUnitType::Flat, 100.0f}, 0.0f, std::numeric_limits<float>::max(),
        {LengthUnitType::Flat, 100.0f}, 110.0f, std::numeric_limits<float>::max(), FlexDirection::Column, 1.0f, 1.0f,0});
    const auto panelB2= CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>({"PanelB2", {LengthUnitType::Flat, 100.0f}, 0.0f, std::numeric_limits<float>::max(),
        {LengthUnitType::Flat, 100.0f}, 50.0f, std::numeric_limits<float>::max(), FlexDirection::Column, 1.0f, 1.0f,0});
    const auto panelB3= CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>({"PanelB3", {LengthUnitType::Flat, 50.0f}, 0.0f, std::numeric_limits<float>::max(),
        {LengthUnitType::Auto, 50.0f}, 0.0f, std::numeric_limits<float>::max(), FlexDirection::Column, 1.0f, 1.0f,0});

    Link(*root.get(),*panelA.get());
    Link(*root.get(),*panelB.get());
    Link(*root.get(),*panelC.get());

    Link(*panelB.get(),*panelB1.get());
    Link(*panelB.get(),*panelB2.get());
    Link(*panelB.get(),*panelB3.get());

std::this_thread::sleep_for(std::chrono::seconds(2));

    Sizing(*root.get(), 400, 600);
    //
    // Placing(root);

    std::cout << "=== " << title << " ===" << std::endl;
    PrintTreeDebugWithOffset(*root.get());



    // Sizing(root, 600);
    //
    // Placing(root);
    //
    // std::cout << "=== " << title << " ===" << std::endl;
    // PrintTreeDebugWithOffset(root);
    //
    // std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}
// Nester Tree Debugger Test
//
// Root check, w: 800 h: 600
// Root processingExpansion: 1
// Root cascadeExpandDelta: 300
// Root cascadeExpandRatio: 3
// PanelA check, w: 200 h: 0
// PanelA processingExpansion: 1
// PanelA cascadeExpandDelta: 300
// PanelA cascadeExpandRatio: 0
// PanelB check, w: 100 h: 160
// PanelB cascadeCompressDistance: 200
// PanelB cascadeBaseDistance: 250
// PanelB cascadeCompressSolidify: 0
// PanelB3 remainDist: 200
// PanelB3 remainCap: 250
// PanelB3 solidify: 0
// PanelB3 capacity: 50
// PanelB3 validatedBase: 50
// PanelB3 validatedMin: 0
// PanelB3 oppositeBase: 50
// PanelB3 oppositeMin: 0
// PanelB3 oppositeMax: 70
// PanelB3 check, w: 40 h: 50
// PanelB3 processingExpansion: 1
// PanelB3 cascadeExpandDelta: 50
// PanelB3 cascadeExpandRatio: 0
// PanelB1 remainDist: 160
// PanelB1 remainCap: 200
// PanelB1 solidify: 0
// PanelB1 capacity: 110
// PanelB1 validatedBase: 100
// PanelB1 validatedMin: 0
// PanelB1 oppositeBase: 110
// PanelB1 oppositeMin: 110
// PanelB1 oppositeMax: 3.40282e+38
// PanelB1 check, w: 88 h: 110
// PanelB1 processingExpansion: 1
// PanelB1 cascadeExpandDelta: 110
// PanelB1 cascadeExpandRatio: 0
// PanelB2 remainDist: 72
// PanelB2 remainCap: 100
// PanelB2 solidify: 0
// PanelB2 capacity: 100
// PanelB2 validatedBase: 100
// PanelB2 validatedMin: 0
// PanelB2 oppositeBase: 100
// PanelB2 oppositeMin: 50
// PanelB2 oppositeMax: 3.40282e+38
// PanelB2 check, w: 72 h: 100
// PanelB2 processingExpansion: 1
// PanelB2 cascadeExpandDelta: 100
// PanelB2 cascadeExpandRatio: 0
// PanelC check, w: 200 h: 0
// PanelC processingExpansion: 1
// PanelC cascadeExpandDelta: 300
// PanelC cascadeExpandRatio: 0
// === Nester Tree Debug ===
// Root | w: 800 | h: 600
//     PanelA | w: 300 | h: 0
//     PanelB | w: 200 | h: 160
//         PanelB1 | w: 88 | h: 110
//         PanelB2 | w: 72 | h: 100
//         PanelB3 | w: 40 | h: 50
//     PanelC | w: 300 | h: 0
//
// Process finished with exit code 0
