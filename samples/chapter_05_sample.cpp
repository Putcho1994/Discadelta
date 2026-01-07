import ufox_discadelta_lib;
import ufox_discadelta_core;
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

using namespace ufox::geometry::discadelta;

void DebugPrintNesterTree(const Nester& root, const std::string& title = "Nester Tree Debug") {
    std::cout << "=== " << title << " ===\n";
    std::cout << std::format("Root: \"{}\" | Depth: {} | OwnBase: {:.1f} | OwnMin: {:.1f} | "
                             "AccumBase: {:.1f} | AccumMin: {:.1f} | "
                             "GreaterBase: {:.1f} | GreaterMin: {:.1f}\n\n",
                             root.content ? root.content->name : "no_content",
                             root.GetDepth(),
                             root.GetBase(),
                             root.GetMin(),
                             root.GetAccumulateBase(),
                             root.GetAccumulateMin(),  // direct access since private, but debugger is friend or external
                             root.GetGreaterBase(),
                             root.GetGreaterMin());

    // Recursive lambda to print subtree
    std::function<void(const Nester&, int)> printSubtree = [&](const Nester& node, int indentLevel) {
        std::string indent(indentLevel * 2, ' ');

        for (const Nester* child : node.GetChildren()) {
            std::cout << std::format("{} | Depth: {} | OwnBase: {:.1f} | OwnMin: {:.1f} | "
                                     "AccumBase: {:.1f} | AccumMin: {:.1f} | "
                                     "GreaterBase: {:.1f} | GreaterMin: {:.1f}\n",
                                     child->content ? child->content->name : "no_content",
                                     child->GetDepth(),
                                     child->GetBase(),
                                     child->GetMin(),
                                     child->GetAccumulateBase(),
                                     child->GetAccumulateMin(),
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

    auto config0     = MakeConfig("Root",     0.0f, 1.0f, 1.0f, 0.0f, 2000.0f);
    auto config1     = MakeConfig("PanelA",   100.0f, 0.5f, 0.5f, 50.0f, 300.0f);
    auto config1_0   = MakeConfig("SubA1",    80.0f,  0.2f, 0.4f, 40.0f, 150.0f);
    auto config1_0_0 = MakeConfig("SubA1-1",  90.0f,  0.1f, 0.2f, 60.0f, 120.0f);  // min > base test
    auto config1_1   = MakeConfig("SubA2",    90.0f,  0.2f, 0.4f, 100.0f, 200.0f); // high min
    auto config2     = MakeConfig("PanelB",   200.0f, 0.3f, 0.8f, 150.0f, 400.0f);

    Nester root(&config0);
    Nester panelA(&config1);
    Nester subA1(&config1_0);
    Nester subA1_1(&config1_0_0);
    Nester subA2(&config1_1);
    Nester panelB(&config2);

    panelA.Link(&root);
    subA1.Link(&panelA);
    subA1_1.Link(&subA1);
    subA2.Link(&panelA);
    panelB.Link(&root);

    // Full tree debug
    DebugPrintNesterTree(root, "Final Tree State");

    std::this_thread::sleep_for(std::chrono::seconds(2));
    // Test reparenting
    std::cout << "Reparenting SubA1 to Root...\n";
    subA1.Link(&root);

    DebugPrintNesterTree(root, "After Reparenting SubA1 to Root");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    // Test unlink
    std::cout << "Unlinking PanelB...\n";
    panelB.Unlink();

    DebugPrintNesterTree(root, "After Unlinking PanelB");
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "subA1_1 root: " << subA1_1.GetRoot()->content->name << "\n";

    subA1_1.Unlink();

    std::cout << "subA1_1 root: " << subA1_1.GetRoot()->content->name << "\n";

    std::cout << "Done!\n";
    return 0;
}
