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

        const SegmentConfig* content{nullptr};

        ~Nester() = default;

        explicit Nester(const SegmentConfig* config_) : content(config_) {}

        [[nodiscard]] float GetBase() const noexcept {
            return content ? content->base : 0.0f;
        }

        [[nodiscard]] float GetMin() const noexcept {
            return content ? content->min : 0.0f;
        }

        [[nodiscard]] float GetGreaterBase() const noexcept {
            return std::max(accumulateBaseDistance, GetBase());
        }

        [[nodiscard]] float GetGreaterMin() const noexcept {
            return std::max(accumulateMin, GetMin());
        }

        [[nodiscard]] uint32_t GetDepth() const noexcept {
            return depth;
        }


        void Link(Nester* new_parent) noexcept {
            if (!new_parent || new_parent == this) return;

            Unlink();

            new_parent->children.push_back(this);
            parent = new_parent;
            UpdateDepth();
            RefreshAccumulateBaseDistance(depth, GetGreaterBase(), GetGreaterMin(), true);

            root = parent->GetRoot();
        }

        void Unlink() noexcept {
            if (parent == nullptr) return;

            parent->UnlinkChild(this);
            RefreshAccumulateBaseDistance(depth, GetGreaterBase(), GetGreaterMin(), false);
            parent = nullptr;
            UpdateDepth();
            root = this;
        }

        void Clear() const noexcept {
            while (!children.empty()) {
                children.back()->Unlink();
            }
        }

        [[nodiscard]] std::vector<Nester*> GetChildren() const noexcept { return children; }

        [[nodiscard]] float GetAccumulateBase() const noexcept { return accumulateBaseDistance; }

        [[nodiscard]] float GetAccumulateMin() const noexcept { return accumulateMin; }

        [[nodiscard]] const Nester* GetRoot() const noexcept { return root; }

    private:
        Nester* parent{nullptr};
        std::vector<Nester*> children{};
        uint32_t depth{0};

        float accumulateBaseDistance{0.0f};
        float accumulateMin{0.0f};

        mutable const Nester* root{this};

        void UnlinkChild(Nester* child) noexcept {
            auto it = std::ranges::find(children, child);
            if (it != children.end()) {
                children.erase(it);
            }
        }

        void UpdateDepth() noexcept {
            depth = parent ? parent->depth + 1 : 0;

            for (Nester* child : children) {
                child->UpdateDepth();
            }
        }

        void RefreshAccumulateBaseDistance(const uint32_t currentDepth, const float base, const float min, const bool isLink) const noexcept {
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



        friend Nester;
    };

    using SegmentsPtrHandler = std::vector<std::unique_ptr<Segment>>;
}
