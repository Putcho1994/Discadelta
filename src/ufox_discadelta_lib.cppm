//
// Created by Puwiwad B on 02.01.2026.
//
module;

#include <memory>
#include <string>
#include <vector>

export module ufox_discadelta_lib;


export namespace ufox::geometry::discadelta {
    struct DiscadeltaSegment {
        std::string name{"none"};
        float base{0.0f};
        float expandDelta{0.0f};
        float distance{0.0f};
        float offset;
        size_t order;
    };

    struct DiscadeltaSegmentConfig {
        std::string name{"none"};
        float base{0.0f};
        float compressRatio{0.0f};
        float expandRatio{0.0f};
        float min{0.0f};
        float max{0.0f};
        size_t order;
    };

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

        // NEW: Non-owning pointers — safe because ownedSegments outlives metrics
        std::vector<DiscadeltaSegment*> segments;

        std::vector<size_t> compressPriorityIndies;
        std::vector<size_t> expandPriorityIndies;

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

            compressPriorityIndies.reserve(segmentCount);
            expandPriorityIndies.reserve(segmentCount);
        }
    };

    using DiscadeltaSegmentsHandler = std::vector<std::unique_ptr<DiscadeltaSegment>>;
}
