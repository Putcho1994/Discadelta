#include <iomanip>
#include <iostream>
#include <thread>
#include <algorithm>

import ufox_discadelta_lib;
import ufox_discadelta_core;

constexpr void Debugger(const ufox::geometry::discadelta::DiscadeltaSegmentsHandler& segmentDistances, const ufox::geometry::discadelta::DiscadeltaPreComputeMetrics &preComputeMetrics) {
    std::cout <<"=== Discadelta Layout: Metrics & Final Distribution ===" << std::endl;
    std::cout << std::format("Input distance: {}", preComputeMetrics.inputDistance)<< std::endl;

    // Table header
    std::cout << std::left
              << "|"
              << std::setw(10) << "Segment"
              << "|"
              << std::setw(20) << "Base"
              << "|"
              << std::setw(15) << "Delta"
              << "|"
              << std::setw(15) << "Distance"
              << "|"
              << std::setw(15) << "Order"
              << "|"
              << std::setw(15) << "Offset"
              << "|"
              << std::endl;

    std::cout << std::left
                   << "|"
                   << std::string(10, '-')
                   << "|"
                   << std::string(20, '-')
                   << "|"
                   << std::string(15, '-')
                   << "|"
                   << std::string(15, '-')
                   << "|"
                   << std::string(15, '-')
                   << "|"
                   << std::string(15, '-')
                   << "|"
                   << std::endl;

    float total{0.0f};
    for (const auto & res : segmentDistances) {
        total += res->distance;

        std::cout << std::left<< "|"<< std::setw(10) << res->name<< "|"
                  << std::setw(20) << std::format("{:.3f}",res->base)
                  << "|"
                  << std::setw(15) << std::format("{:.3f}",res->expandDelta)
                  << "|"
                  << std::setw(15) << std::format("{:.3f}",res->distance)
                  << "|"
                  << std::setw(15) << std::format("{}",res->order)
                  << "|"
                  << std::setw(15) << std::format("{:.3f}",res->offset)
                  << "|"
                  << std::endl;
    }
}

int main() {

    std::vector<ufox::geometry::discadelta::DiscadeltaSegmentConfig> segmentConfigs{
              {"Segment_1", 200.0f, 0.7f, 0.1f, 0.0f, 100.0f, 2},
              {"Segment_2", 200.0f, 1.0f, 1.0f, 300.0f, 800.0f, 1},
              {"Segment_3", 150.0f, 0.0f, 2.0f, 0.0f, 200.0f, 3},
              {"Segment_4", 350.0f, 0.3f, 0.5f, 50.0f, 300.0f, 0}};

    constexpr float rootDistance = 900.0f;
    auto [segmentDistances, preComputeMetrics, processingCompression] = ufox::geometry::discadelta::MakeContext(segmentConfigs, rootDistance);

    if (processingCompression) {
       ufox::geometry::discadelta::Compressing(preComputeMetrics);
    }
    else {
        ufox::geometry::discadelta::Expanding(preComputeMetrics);
    }

    ufox::geometry::discadelta::Placing(preComputeMetrics);

    Debugger(segmentDistances, preComputeMetrics);

    //the debugger is slow we sleep for 2 seconds before trigger another log
    std::this_thread::sleep_for(std::chrono::seconds(2));

    ufox::geometry::discadelta::SetSegmentOrder(preComputeMetrics, "Segment_1", 3);
    ufox::geometry::discadelta::SetSegmentOrder(preComputeMetrics, "Segment_3", 2);

    ufox::geometry::discadelta::Placing(preComputeMetrics);

    Debugger(segmentDistances, preComputeMetrics);

    return 0;
}
