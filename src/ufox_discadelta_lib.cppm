//
// Created by Puwiwad B on 02.01.2026.
//
module;

#include <memory>
#include <string>
#include <vector>
#include <algorithm>

export module ufox_discadelta_lib;


export namespace ufox::geometry::discadelta {
    struct Segment {
        std::string name{"none"};
        float base{0.0f};
        float expandDelta{0.0f};
        float distance{0.0f};
        float offset;
        size_t order;
    };

    struct SegmentConfig {
        std::string name{"none"};
        float base{0.0f};
        float compressRatio{0.0f};
        float expandRatio{0.0f};
        float min{0.0f};
        float max{0.0f};
        size_t order;
    };

    struct PreComputeMetrics {
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
        std::vector<Segment*> segments;

        std::vector<size_t> compressPriorityIndies;
        std::vector<size_t> expandPriorityIndies;

        PreComputeMetrics() = default;
        ~PreComputeMetrics() = default;

        explicit PreComputeMetrics(const size_t segmentCount, const float& rootBase) : inputDistance(rootBase) {
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

    struct Nester {
    Nester* parent{nullptr};
    std::vector<Nester*> children;

    std::unique_ptr<const SegmentConfig> ownedContent;


    float accumulateBaseDistance{0.0f};

    explicit Nester(SegmentConfig config_, Nester* parent_ = nullptr)
        : parent(parent_)
        , ownedContent(std::make_unique<const SegmentConfig>(std::move(config_)))
    {}

    [[nodiscard]] constexpr  float GetOwnBase() const noexcept {
        return ownedContent ? ownedContent->base : 0.0f;
    }

    [[nodiscard]] constexpr  float GetGreaterBaseDistance() const noexcept {
        return std::max(accumulateBaseDistance, GetOwnBase());
    }

    void Add(Nester* child) noexcept {
        if (!child || child == this) return;  // Safety

        children.push_back(child);
        if (child->parent != this) {
            if (child->parent) {
                child->parent->Remove(child);  // Detach from old parent
            }
            child->parent = this;
        }

        // Update accumulation: add child's full subtree demand
        accumulateBaseDistance += child->GetGreaterBaseDistance();
    }

    void Remove(Nester* child) noexcept {
        if (!child) return;

        auto it = std::ranges::find(children, child);
        if (it != children.end()) {
            accumulateBaseDistance -= child->GetGreaterBaseDistance();
            children.erase(it);
            child->parent = nullptr;
        }
    }

    // Optional: clear all children recursively
    void Clear() noexcept {
        for (Nester* child : children) {
            child->parent = nullptr;
        }
        children.clear();
        accumulateBaseDistance = 0.0f;
    }

    ~Nester() {
        Clear();  // Break cycles safely
    }
};

    using SegmentsPtrHandler = std::vector<std::unique_ptr<Segment>>;

}
