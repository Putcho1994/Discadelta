#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <algorithm>

struct DiscadeltaSegment {
    std::string name{"none"};//optional
    float base{0.0f};
    float expandDelta{0.0f};
    float distance{0.0f};
};

struct DiscadeltaSegmentConfig {
    std::string name{"none"};//optional
    float base{0.0f};
    float compressRatio{0.0f};
    float expandRatio{0.0f};
    float min{0.0f};
    float max{0.0f};
};

using DiscadeltaSegmentsHandler = std::vector<std::unique_ptr<DiscadeltaSegment>>;

struct DiscadeltaPreComputeMetrics {
    float inputDistance{};
    std::vector<float> compressCapacities{};
    std::vector<float> compressSolidifies{};
    std::vector<float> baseDistances{};
    std::vector<float> expandRatios{};
    std::vector<float> minDistances{};
    std::vector<float> maxDistances{};
    float accumulateBaseDistance{0.0f};
    float accumulateCompressSolidify{0.0f};
    float accumulateExpandRatio{0.0f};

    // NEW: Non-owning pointers
    std::vector<DiscadeltaSegment*> segments;

    DiscadeltaPreComputeMetrics() = default;
    ~DiscadeltaPreComputeMetrics() = default;

    explicit DiscadeltaPreComputeMetrics(const size_t segmentCount, const float& rootBase) : inputDistance(rootBase) {
        compressCapacities.reserve(segmentCount);
        compressSolidifies.reserve(segmentCount);
        baseDistances.reserve(segmentCount);
        expandRatios.reserve(segmentCount);
        minDistances.reserve(segmentCount);
        maxDistances.reserve(segmentCount);
        segments.reserve(segmentCount);
    }
};

constexpr auto MakeDiscadeltaContext = [](const std::vector<DiscadeltaSegmentConfig>& configs, const float inputDistance) -> std::tuple<DiscadeltaSegmentsHandler, DiscadeltaPreComputeMetrics, bool>
{
    const float validatedInputDistance = std::max(0.0f, inputDistance);
    const size_t segmentCount = configs.size();

    DiscadeltaSegmentsHandler segments;
    segments.reserve(segmentCount);

    DiscadeltaPreComputeMetrics preComputeMetrics(segmentCount, validatedInputDistance);

    for (const auto& cfg : configs) {
        const auto& [name, rawBase, rawCompressRatio, rawExpandRatio, rawMin, rawMax] = cfg;

        // --- INPUT VALIDATION ---
        const float minVal = std::max(0.0f, rawMin);
        const float maxVal = std::max(minVal, rawMax);
        const float baseVal = std::clamp(rawBase, minVal, maxVal);

        const float compressRatio = std::max(0.0f, rawCompressRatio);
        const float expandRatio   = std::max(0.0f, rawExpandRatio);

        // --- COMPRESS METRICS ---
        const float compressCapacity = baseVal * compressRatio;
        const float compressSolidify = std::max (0.0f, baseVal - compressCapacity);

        // --- STORE PRE-COMPUTE ---
        preComputeMetrics.compressCapacities.push_back(compressCapacity);
        preComputeMetrics.compressSolidifies.push_back(compressSolidify);
        preComputeMetrics.baseDistances.push_back(baseVal);
        preComputeMetrics.expandRatios.push_back(expandRatio);
        preComputeMetrics.minDistances.push_back(minVal);
        preComputeMetrics.maxDistances.push_back(maxVal);

        preComputeMetrics.accumulateBaseDistance += baseVal;
        preComputeMetrics.accumulateCompressSolidify += compressSolidify;
        preComputeMetrics.accumulateExpandRatio += expandRatio;

        // --- CREATE OWNED SEGMENT ---
        auto seg = std::make_unique<DiscadeltaSegment>();
        seg->name = name;
        seg->base = baseVal;
        seg->distance = baseVal;
        seg->expandDelta = 0.0f;

        preComputeMetrics.segments.push_back(seg.get());

        segments.push_back(std::move(seg));
    }

    const bool processingCompression = validatedInputDistance < preComputeMetrics.accumulateBaseDistance;

    return { std::move(segments), std::move(preComputeMetrics), processingCompression };
};

void RedistributeDiscadeltaCompressDistance(const DiscadeltaPreComputeMetrics& preComputeMetrics) {
    float cascadeCompressDistance = preComputeMetrics.inputDistance;
    float cascadeBaseDistance = preComputeMetrics.accumulateBaseDistance;
    float cascadeCompressSolidify = preComputeMetrics.accumulateCompressSolidify;

    DiscadeltaPreComputeMetrics nextLineMetrics(preComputeMetrics.segments.size(), cascadeCompressDistance);

    int recursiveSafeguardValue{0}; // Tracks flexible segments
    int recursiveSafeguardLimit{0}; // Tracks total segments in this pass

    for (size_t i = 0; i < preComputeMetrics.segments.size(); ++i) {
        DiscadeltaSegment* seg = preComputeMetrics.segments[i];
        if (seg == nullptr) break;

        const float remainDist = cascadeCompressDistance - cascadeCompressSolidify;
        const float remainCap = cascadeBaseDistance - cascadeCompressSolidify;
        const float& cap = preComputeMetrics.compressCapacities[i];
        const float& solidify = preComputeMetrics.compressSolidifies[i];
        const float& base = preComputeMetrics.baseDistances[i];

        // Proportional math with safety checks
        const float compressBaseDistance = (remainDist <= 0 || remainCap <= 0 || cap <= 0 ? 0.0f :
                                      remainDist / remainCap * cap) + solidify;

        // Apply MIN Constraint
        const float& min = preComputeMetrics.minDistances[i];
        const float clampedDist = std::max(compressBaseDistance, min);

        // If Clamped: Reduce the budget for the next pass
        // If Flexible: Add to the pool for the next pass
        if (compressBaseDistance != clampedDist || cap <= 0.0f) {
            nextLineMetrics.inputDistance -= clampedDist;
        }
        else {
            nextLineMetrics.accumulateBaseDistance += base;
            nextLineMetrics.accumulateCompressSolidify += solidify;
            nextLineMetrics.compressCapacities.push_back(cap);
            nextLineMetrics.compressSolidifies.push_back(solidify);
            nextLineMetrics.baseDistances.push_back(base);
            nextLineMetrics.minDistances.push_back(min);
            nextLineMetrics.segments.push_back(seg);
            recursiveSafeguardValue++;
        }

        seg->base = seg->distance = clampedDist;
        cascadeCompressDistance -= clampedDist;
        cascadeCompressSolidify -= solidify;
        cascadeBaseDistance -= base;
        recursiveSafeguardLimit++;
    }

    // Recursion ends when Value == Limit (No new segments hit constraints)
    if (recursiveSafeguardValue < recursiveSafeguardLimit) {
        RedistributeDiscadeltaCompressDistance(nextLineMetrics);
    }
}

void RedistributeDiscadeltaExpandDistance(const DiscadeltaPreComputeMetrics& preComputeMetrics) {
    float cascadeExpandDelta = std::max(preComputeMetrics.inputDistance - preComputeMetrics.accumulateBaseDistance, 0.0f);
    float cascadeExpandRatio = preComputeMetrics.accumulateExpandRatio;
    if (cascadeExpandDelta <= 0.0f) return;

    DiscadeltaPreComputeMetrics nextLineMetrics(preComputeMetrics.segments.size(), cascadeExpandDelta);
    int recursiveSafeguardValue{0}, recursiveSafeguardLimit{0};

    for (size_t i = 0; i < preComputeMetrics.segments.size(); ++i) {
        DiscadeltaSegment* seg = preComputeMetrics.segments[i];
        const float& base = preComputeMetrics.baseDistances[i];
        const float& ratio = preComputeMetrics.expandRatios[i];

        const float expandDelta = (cascadeExpandRatio <= 0.0f || ratio <= 0.0f) ? 0.0f :
                                      cascadeExpandDelta / cascadeExpandRatio * ratio;

        // Apply MAX Constraint
        const float maxDelta = std::max(0.0f, preComputeMetrics.maxDistances[i] - base);
        const float clampedDelta = std::min(expandDelta, maxDelta);

        if (clampedDelta != expandDelta || ratio <= 0.0f) {
            nextLineMetrics.inputDistance -= clampedDelta;
        } else {
            nextLineMetrics.accumulateBaseDistance += base;
            nextLineMetrics.accumulateExpandRatio += ratio;
            nextLineMetrics.expandRatios.push_back(ratio);
            nextLineMetrics.baseDistances.push_back(base);
            nextLineMetrics.maxDistances.push_back(preComputeMetrics.maxDistances[i]);
            nextLineMetrics.segments.push_back(seg);
            recursiveSafeguardValue++;
        }

        seg->expandDelta = clampedDelta;
        seg->distance = base + clampedDelta;
        cascadeExpandDelta -= clampedDelta;
        cascadeExpandRatio -= ratio;
        recursiveSafeguardLimit++;
    }

    // Add back base distance for absolute distance input in recursion
    nextLineMetrics.inputDistance += nextLineMetrics.accumulateBaseDistance;

    if (recursiveSafeguardValue < recursiveSafeguardLimit) {
        RedistributeDiscadeltaExpandDistance(nextLineMetrics);
    }
}


int main() {
    std::vector<DiscadeltaSegmentConfig> segmentConfigs{
          {"1", 200.0f, 0.7f, 0.1f, 0.0f, 100.0f},
          {"2", 200.0f, 1.0f, 1.0f, 300.0f, 800.0f},
          {"3", 150.0f, 0.0f, 2.0f, 0.0f, 200.0f},
          {"4", 350.0f, 0.3f, 0.5f, 50.0f, 300.0f}};

    constexpr float rootDistance = 800.0f;
    auto [segmentDistances, preComputeMetrics, processingCompression] = MakeDiscadeltaContext(segmentConfigs, rootDistance);


    if (processingCompression) {

        RedistributeDiscadeltaCompressDistance(preComputeMetrics);
    }
    else {
        RedistributeDiscadeltaExpandDistance(preComputeMetrics);
    }

#pragma region //Print Result
    std::cout << "=== Dynamic Base Segment (Underflow Handling) ===" << std::endl;
    std::cout << std::format("Input distance: {}", rootDistance)<< std::endl;

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
    for (size_t i = 0; i < segmentDistances.size(); ++i) {
        const auto& res = segmentDistances[i];

        total += res->distance;

        std::cout << std::fixed << std::setprecision(3)
                  << "|"
                  << std::setw(10) << (i + 1)
                  << "|"
                  << std::setw(20) << std::format("Total: {:.4f}",preComputeMetrics.compressSolidifies[i])
                  << "|"
                  << std::setw(20) << std::format("Total: {:.4f}",preComputeMetrics.compressCapacities[i])
                  << "|"
                  << std::setw(20) << std::format("Total: {:.4f}",res->base)
                  << "|"
                  << std::setw(15) << std::format("Total: {:.4f}",res->expandDelta)
                  << "|"
                  << std::setw(20) << std::format("Total: {:.4f}",res->distance)
                  << "|"
                  << std::endl;
    }


    std::cout << std::format("Total: {:.4f} (expected 800.0)\n", total);

    std::this_thread::sleep_for(std::chrono::seconds(2));
#pragma endregion //Print Result

    return 0;
}