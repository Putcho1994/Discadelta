#include <iostream>
#include <vector>
#include <format>
#include <iomanip>
#include <thread>
#include <algorithm>

struct DiscadeltaSegment {
    float base;
    float expandDelta;
    float distance;
};

struct DiscadeltaSegmentConfig {
    float base;
    float compressRatio;
    float expandRatio;
};

int main() {
    std::vector<DiscadeltaSegmentConfig> segmentConfigs{
            {200.0f, 0.7f, 0.1f},
            {300.0f, 1.0f, 1.0f},
            {150.0f, 1.0f, 2.0f},
            {250.0f, 0.3f, 0.5f}
    };

    // std::vector<DiscadeltaSegmentConfig> segmentConfigs{
    //         {100.0f, 1.0f, 0.3f},
    //         {150.0f, 1.0f, 1.0f},
    //         {70.0f, 1.0f, 1.0f},
    //         {50.0f, 1.0f, 0.8f}
    // };

    const size_t segmentCount = segmentConfigs.size();

    std::vector<DiscadeltaSegment> segmentDistances(segmentCount);


#pragma region // Prepare Compute Context
    constexpr float rootBase = 800.0f;

    // Internal containers for validated data
    std::vector<float> compressCapacities{};
    compressCapacities.reserve(segmentCount);
    std::vector<float> compressSolidifies{};
    compressSolidifies.reserve(segmentCount);
    std::vector<float> baseDistances{};
    baseDistances.reserve(segmentCount);
    std::vector<float> expandRatios{};
    expandRatios.reserve(segmentCount);

    float accumulateBaseDistance{0.0f};
    float accumulateCompressSolidify{0.0f};
    float accumulateExpandRatio{0.0f};

    for (size_t i = 0; i < segmentCount; ++i) {
        const auto &[rawBase, rawCompressRatio, rawExpandRatio] = segmentConfigs[i];

        // --- VALIDATION STEP ---
        // Clamp all configurations to 0.0 to prevent negative distance logic errors
        const float base = std::max(rawBase, 0.0f);
        const float compressRatio = std::max(rawCompressRatio, 0.0f);
        const float expandRatio = std::max(rawExpandRatio, 0.0f);

        // Calculate base proportion metrics
        const float compressCapacity = base * compressRatio;
        const float compressSolidify = base - compressCapacity;

        compressCapacities.push_back(compressCapacity);
        compressSolidifies.push_back(compressSolidify);
        baseDistances.push_back(base);

        // Accumulate metrics
        accumulateBaseDistance += base;
        accumulateCompressSolidify += compressSolidify;

        DiscadeltaSegment& segment = segmentDistances[i];
        segment.base = base;
        segment.distance = base;

        expandRatios.push_back(expandRatio);
        accumulateExpandRatio += expandRatio;
    }


#pragma endregion //Prepare Compute Context

#pragma region //Compute Segment Base Distance

    if (rootBase < accumulateBaseDistance) {
        //compressing
        float cascadeCompressDistance = rootBase;
        float cascadeBaseDistance = accumulateBaseDistance;
        float cascadeCompressSolidify = accumulateCompressSolidify;

        for (size_t i = 0; i < segmentCount; ++i) {
            const float remainCompressDistance = cascadeCompressDistance - cascadeCompressSolidify;
            const float remainCompressCapacity = cascadeBaseDistance - cascadeCompressSolidify;
            const float& compressCapacity = compressCapacities[i];
            const float& compressSolidify = compressSolidifies[i];
            const float compressBaseDistance = (remainCompressDistance <= 0 || remainCompressCapacity <= 0 || compressCapacity <= 0? 0.0f:
            remainCompressDistance / remainCompressCapacity * compressCapacity) + compressSolidify;

            DiscadeltaSegment& segment = segmentDistances[i];
            segment.base = compressBaseDistance;
            segment.distance = compressBaseDistance; //overwrite pre compute

            cascadeCompressDistance -= compressBaseDistance;
            cascadeCompressSolidify -= compressSolidify;
            cascadeBaseDistance -= baseDistances[i];
        }
    }
    else {
        //Expanding
        float cascadeExpandDistance = std::max(rootBase - accumulateBaseDistance, 0.0f);
        float cascadeExpandRatio = accumulateExpandRatio;

        if (cascadeExpandDistance > 0.0f) {
            for (size_t i = 0; i < segmentCount; ++i) {
                const float& expandRatio = expandRatios[i];
                const float expandDelta = cascadeExpandRatio <= 0.0f || expandRatio <= 0.0f? 0.0f :
                cascadeExpandDistance / cascadeExpandRatio * expandRatio;

                segmentDistances[i].expandDelta = expandDelta;
                segmentDistances[i].distance += expandDelta; //add to precompute

                cascadeExpandDistance -= expandDelta;
                cascadeExpandRatio -= expandRatio;
            }
        }
    }

#pragma endregion //Compute Segment Base Distance


#pragma region //Print Result
    std::cout << "=== Dynamic Base Segment (Underflow Handling) ===" << std::endl;
    std::cout << std::format("Input distance: {}", rootBase)<< std::endl;

    // Table header
    std::cout << std::left
              << "|"
              << std::setw(10) << "Segment"
              << "|"
              << std::setw(20) << "Compress Solidify"
              << "|"
              << std::setw(20) << "Compress Capacity"
              << "|"
              << std::setw(20) << "Compress Distance"
              << "|"
              << std::setw(15) << "Expand Delta"
              << "|"
              << std::setw(20) << "Scaled Distance"
              << "|"
              << std::endl;

    std::cout << std::left
                   << "|"
                   << std::string(10, '-')
                   << "|"
                   << std::string(20, '-')
                   << "|"
                   << std::string(20, '-')
                   << "|"
                   << std::string(20, '-')
                   << "|"
                   << std::string(15, '-')
                   << "|"
                   << std::string(20, '-')
                   << "|"
                   << std::endl;


    float total{0.0f};
    for (size_t i = 0; i < segmentCount; ++i) {
        const auto& res = segmentDistances[i];

        total += res.distance;

        std::cout << std::fixed << std::setprecision(3)
                  << "|"
                  << std::setw(10) << (i + 1)
                  << "|"
                  << std::setw(20) << std::format("Total: {:.4f}",compressSolidifies[i])
                  << "|"
                  << std::setw(20) << std::format("Total: {:.4f}",compressCapacities[i])
                  << "|"
                  << std::setw(20) << std::format("Total: {:.4f}",res.base)
                  << "|"
                  << std::setw(15) << std::format("Total: {:.4f}",res.expandDelta)
                  << "|"
                  << std::setw(20) << std::format("Total: {:.4f}",res.distance)
                  << "|"
                  << std::endl;
    }


    std::cout << std::format("Total: {:.4f} (expected 800.0)\n", total);

    std::this_thread::sleep_for(std::chrono::seconds(2));
#pragma endregion //Print Result

    return 0;
}
