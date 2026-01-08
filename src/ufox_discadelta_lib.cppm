//
// Created by Puwiwad B on 02.01.2026.
//
module;

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <ranges>
#include <unordered_map>

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

    struct Configuration {
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

    struct NestedSegment {
        Configuration config{};
        Segment content{};
        PreComputeMetrics preComputeMetrics{};
        NestedSegment() = default;
        ~NestedSegment() = default;

        explicit NestedSegment(Configuration  config) : config(std::move(config)) { ValidatingConfig();}

        [[nodiscard]] float GetBase() const noexcept { return base; }

        [[nodiscard]] float GetMin() const noexcept { return min; }

        [[nodiscard]] float GetGreaterBase() const noexcept { return std::max(accumulateBaseDistance, base); }

        [[nodiscard]] float GetGreaterMin() const noexcept { return std::max(accumulateMin, min); }

        [[nodiscard]] uint32_t GetDepth() const noexcept { return depth; }

        void ValidatingConfig() noexcept {
            const float minVal = std::max(0.0f, config.min);
            max = std::max(minVal, config.max);
            base = std::clamp(config.base, minVal, max);

            compressRatio = std::max(0.0f, config.compressRatio);
            expandRatio   = std::max(0.0f, config.expandRatio);
            compressCapacity = base * compressRatio;
            compressSolidify = std::max (0.0f, base - compressCapacity);

            min = std::max(compressSolidify, minVal);
        }


        void Link(NestedSegment* new_parent) noexcept {
            if (!new_parent || new_parent == this) return;

            Unlink();

            new_parent->children[config.name] = this;
            parent = new_parent;
            UpdateDepth();


            RefreshAccumulateBaseDistance(depth, GetGreaterBase(), GetAccumulateMin(), true);

            root = parent->GetRoot();
        }

        void Unlink() noexcept {
            if (parent == nullptr) return;

            parent->children.erase(config.name);

            const float minVal = std::max(0.0f, config.min);
            const float maxVal = std::max(minVal, config.max);
            const float baseVal = std::clamp(config.base, minVal, maxVal);

            const float greaterBase = std::max(accumulateBaseDistance, baseVal);
            const float greaterMin = std::max(accumulateMin, minVal);

            RefreshAccumulateBaseDistance(depth, greaterBase, greaterMin, false);
            parent = nullptr;
            UpdateDepth();
            root = this;
        }

        void Clear() noexcept {
            while (!children.empty()) {
                auto it = children.begin();
                it->second->Unlink();  // Safe: Unlink removes itself from parent map
            }
        }

        [[nodiscard]] std::vector<NestedSegment*> GetChildren() const noexcept {
            std::vector<NestedSegment*> result;
            result.reserve(children.size());
            for (const auto &val: children | std::views::values) {
                result.push_back(val);
            }
            return result;
        }

        [[nodiscard]] NestedSegment* GetChildByName(const std::string& name) noexcept {
            const auto it = children.find(name);
            return it != children.end() ? it->second : nullptr;
        }

        [[nodiscard]] float GetAccumulateBase() const noexcept { return accumulateBaseDistance; }

        [[nodiscard]] float GetAccumulateMin() const noexcept { return accumulateMin; }

        [[nodiscard]] const NestedSegment* GetRoot() const noexcept { return root; }

    private:
        NestedSegment* parent{nullptr};
        std::unordered_map<std::string, NestedSegment*> children;
        uint32_t depth{0};

        float min{0.0f};
        float max{0.0f};
        float base{0.0f};
        float compressRatio{0.0f};
        float expandRatio{0.0f};
        float compressCapacity{0.0f};
        float compressSolidify{0.0f};

        float accumulateBaseDistance{0.0f};
        float accumulateMin{0.0f};
        float accumulateExpandRatio{0.0f};

        mutable const NestedSegment* root{this};


        void UpdateDepth() noexcept {
            depth = parent ? parent->depth + 1 : 0;

            for (const auto &val: children | std::views::values) {
                val->UpdateDepth();
            }

        }

        void RefreshAccumulateBaseDistance(const uint32_t& currentDepth, const float& base, const float& min, const bool& isLink) const noexcept {
            if (parent == nullptr || currentDepth <= 0) return;
            //Parent may have a different greater base, so we withdraw old greater base from parent's parent
            parent->RefreshAccumulateBaseDistance(parent->depth, parent->GetGreaterBase(), parent->GetGreaterMin(), false);
            //new accumulate mean Greater Base can be different
            if (isLink) {
                parent->accumulateBaseDistance += base;
                parent->accumulateMin += min;
            } else {
                parent->accumulateBaseDistance -= base;
                parent->accumulateMin -= min;
            }

            //early return, because in all case in current depth 1, the parent has no parent.
            if (currentDepth <= 1) return;
            parent->RefreshAccumulateBaseDistance(parent->depth, parent->GetGreaterBase(), parent->GetGreaterMin(), true); //keep go 1 level down till it reaches 1, b-b-but current is over 9000, don't care.
        }



        friend NestedSegment;
    };

    using SegmentsPtrHandler = std::vector<std::unique_ptr<Segment>>;
}
