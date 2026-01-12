import ufox_discadelta_lib;
import ufox_discadelta_core;
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

using namespace ufox::geometry::discadelta;

void DebugPrintNesterTree(const NestedSegmentContext& root, const std::string& title = "Nester Tree Debug") {
    std::cout << "=== " << title << " ===\n";
    std::cout << std::format("Root: \"{}\" | Depth: {} | OwnBase: {:.1f} | OwnMin: {:.1f} | "
                             "AccumBase: {:.1f} | AccumMin: {:.1f} | Accumulate Ratio: {:.4f} | Solidify: {:.4f} | Capacity: {:.4f} |"
                             "GreaterBase: {:.1f} | GreaterMin: {:.1f}\n\n",
                             root.config.name ,
                             root.GetDepth(),
                             root.GetValidatedBase(),
                             root.GetValidatedMin(),
                             root.GetAccumulateBase(),
                             root.GetAccumulateMin(),
                             root.GetAccumulateExpandRatio(),
                             root.GetCompressSolidify(),
                             root.GetCompressCapacity(),
                             root.GetGreaterBase(),
                             root.GetGreaterMin());

    // Recursive lambda to print subtree
    std::function<void(const NestedSegmentContext&, int)> printSubtree = [&](const NestedSegmentContext& node, int indentLevel) {
        std::string indent(indentLevel * 2, ' ');

        for (const NestedSegmentContext* child : node.GetChildren()) {
            std::cout << std::format("{} | Depth: {} | OwnBase: {:.4f} | OwnMin: {:.4f} | "
                                     "AccumBase: {:.4f} | AccumMin: {:.4f} | Accumulate Ratio: {:.4f} | Solidify: {:.4f} | Capacity: {:.4f} |"
                                     "GreaterBase: {:.4f} | GreaterMin: {:.4f}\n",
                                     child->config.name ,
                                     child->GetDepth(),
                                     child->GetValidatedBase(),
                                     child->GetValidatedMin(),
                                     child->GetAccumulateBase(),
                                     child->GetAccumulateMin(),
                                     child->GetAccumulateExpandRatio(),
                                     child->GetCompressSolidify(),
                                     child->GetCompressCapacity(),
                                     child->GetGreaterBase(),
                                     child->GetGreaterMin());

            printSubtree(*child, indentLevel + 1);
        }
    };

    printSubtree(root, 1);
    std::cout << "\n";
}

int main() {
    std::cout << "Nester Tree Debugger Test\n\n";

    NestedSegmentContext root{{"Root",     0.0f, 1.0f, 1.0f, 0.0f, std::numeric_limits<float>::max()}};
    NestedSegmentContext panelA{{"PanelA",    200.0f, 0.7f, 0.1f, 0.0f, std::numeric_limits<float>::max()}};
    NestedSegmentContext subA1{{"SubA1",     80.0f,   0.2f, 0.4f, 40.0f, 150.0f}};
    NestedSegmentContext subA1_1{{"SubA1-1",    90.0f,    0.1f, 0.2f, 60.0f, 120.0f}};
    NestedSegmentContext subA2{{"SubA2",     90.0f,    0.2f, 0.4f, 100.0f, 200.0f}};
    NestedSegmentContext panelB{{"PanelB",    350.0f,    0.3f, 0.5f, 0.0f, 255.0f}};

    panelA.Link(&root);
    subA1.Link(&panelA);
    //subA1_1.Link(&subA1);
    subA2.Link(&panelA);
    panelB.Link(&root);

    ComputeSegmentsSize(root, 400);

    std::cout << "=== Nester Tree Debug ===\n";
    std::cout << root.GetName() << " | " <<  root.content.distance << std::endl ;
    std::cout << panelA.GetName() << " | " <<  panelA.content.distance << std::endl ;
    std::cout << panelB.GetName() << " | " <<  panelB.content.distance << std::endl ;
    std::cout << subA1.GetName() << " | " <<  subA1.content.distance << std::endl ;
    std::cout << subA2.GetName() << " | " <<  subA2.content.distance << std::endl ;


    std::this_thread::sleep_for(std::chrono::seconds(2));

    ComputeSegmentsSize(root, 600);

    std::cout << "=== Nester Tree Debug ===\n";
    std::cout << root.GetName() << " | " <<  root.content.distance << std::endl ;
    std::cout << panelA.GetName() << " | " <<  panelA.content.distance << std::endl ;
    std::cout << panelB.GetName() << " | " <<  panelB.content.distance << std::endl ;
    std::cout << subA1.GetName() << " | " <<  subA1.content.distance << std::endl ;
    std::cout << subA2.GetName() << " | " <<  subA2.content.distance << std::endl ;


    std::this_thread::sleep_for(std::chrono::seconds(2));


    return 0;
}
