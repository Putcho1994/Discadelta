//
// Created by Puwiwad B on 02.01.2026.
//
module;

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <limits>
#include <numeric>
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

    struct NestedSegmentContext {
        Configuration config{};
        Segment content{};

        NestedSegmentContext() = default;
        ~NestedSegmentContext(){
            Clear();
            Unlink();
        }

        explicit NestedSegmentContext(Configuration  config) : config(std::move(config)) { UpdateContext();}

        [[nodiscard]] std::string GetName() const noexcept { return config.name; }

        [[nodiscard]] constexpr  float GetValidatedBase() const noexcept { return validatedBase; }

        [[nodiscard]] constexpr  float GetValidatedMin() const noexcept { return validatedMin; }

        [[nodiscard]] constexpr  float GetValidateMax() const noexcept { return validateMax; }

        [[nodiscard]] constexpr  float GetCompressRatio() const noexcept { return compressRatio; }

        [[nodiscard]] constexpr  float GetExpandRatio() const noexcept { return expandRatio; }

        [[nodiscard]] constexpr  float GetCompressCapacity() const noexcept { return compressCapacity; }

        [[nodiscard]] constexpr  float GetCompressSolidify() const noexcept { return compressSolidify; }

        [[nodiscard]] constexpr  float GetGreaterBase() const noexcept {return std::max(accumulateBaseDistance, config.base);}

        [[nodiscard]] constexpr  float GetGreaterMin() const noexcept {
            const float minVal = std::max(0.0f, config.min);
            return std::max(accumulateMin, minVal);
        }

        [[nodiscard]] constexpr  size_t GetDepth() const noexcept { return depth; }

        [[nodiscard]] constexpr  size_t GetOrder() const noexcept { return order; }

        [[nodiscard]] constexpr  size_t GetChildCount() const { return children.size(); }

        [[nodiscard]] constexpr std::span<const NestedSegmentContext* const> GetChildren() const noexcept { return children;}

        [[nodiscard]] constexpr std::span<const size_t> GetCompressCascadePriorities() const noexcept { return compressCascadePriorities; }

        [[nodiscard]] constexpr std::span<const size_t> GetExpandCascadePriorities() const noexcept { return expandCascadePriorities; }

        [[nodiscard]] std::vector<size_t> GetOrderedChildrenIndices() const noexcept {
            std::vector<size_t> indices(children.size());
            std::iota(indices.begin(), indices.end(), size_t{0});

            std::ranges::sort(indices,[this](const size_t a, const size_t b) noexcept {return children[a]->GetOrder() < children[b]->GetOrder();});

            return indices;
        }

        [[nodiscard]] NestedSegmentContext* GetChildByName(const std::string& name) noexcept {
            const auto it = childrenIndies.find(name);
            return it == childrenIndies.end()? nullptr : children[it->second];
        }

        [[nodiscard]] constexpr NestedSegmentContext* GetChildByIndex(const size_t index) const noexcept { return index < children.size() ? children[index] : nullptr; }

        [[nodiscard]] constexpr  float GetAccumulateBase() const noexcept { return accumulateBaseDistance; }

        [[nodiscard]] constexpr  float GetAccumulateMin() const noexcept { return accumulateMin; }

        [[nodiscard]] constexpr  float GetAccumulateCompressSolidify() const noexcept { return accumulateCompressSolidify; }

        [[nodiscard]] constexpr  float GetAccumulateExpandRatio() const noexcept { return accumulateExpandRatio; }


        [[nodiscard]] const NestedSegmentContext* GetRoot() const noexcept {
            return parent == nullptr || depth == 0 ? this : parent->GetRoot();
        }

        void UpdateContext() noexcept {
            validatedMin = GetGreaterMin();
            validateMax = std::max(validatedMin, config.max);
            validatedBase = std::clamp(GetGreaterBase(), validatedMin, validateMax);
            order = config.order;

            compressRatio = std::max(0.0f, config.compressRatio);
            expandRatio   = std::max(0.0f, config.expandRatio);

            compressCapacity = validatedBase * compressRatio;
            compressSolidify = std::max (0.0f, validatedBase - compressCapacity);

            validatedMin = std::max(compressSolidify, validatedMin);

            compressCascadePriorities.clear();
            expandCascadePriorities.clear();
            compressCascadePriorities.reserve(children.size());
            expandCascadePriorities.reserve(children.size());

            childrenIndies.clear();
            accumulateCompressSolidify = 0.0f;
            accumulateExpandRatio = 0.0f;

            float compressPriorityLowestValue = std::numeric_limits<float>::max();
            float expandPriorityLowestValue{0.0f};

            for (size_t i = 0; i < children.size(); ++i) {
                auto& child = children[i];
                const float cMin = child->GetValidatedMin();
                const float cBase = child->GetValidatedBase();
                const float compressPriorityValue = std::max(0.0f, cBase - cMin);

                if (compressPriorityValue <= compressPriorityLowestValue) {
                    compressCascadePriorities.insert(compressCascadePriorities.begin(), i);
                    compressPriorityLowestValue = compressPriorityValue;
                } else {
                    compressCascadePriorities.push_back(i);
                }

                const float expandPriorityValue = std::max(0.0f, child->GetValidateMax() - cBase);
                if (expandPriorityValue >= expandPriorityLowestValue) {
                    expandCascadePriorities.push_back(i);
                    expandPriorityLowestValue = expandPriorityValue;
                }
                else {
                    expandCascadePriorities.insert(expandCascadePriorities.begin(), i);
                }

                childrenIndies[child->GetName()] = i;
                accumulateCompressSolidify += child->GetCompressSolidify();
                accumulateExpandRatio += child->GetExpandRatio();
            }

            content.name = config.name;
            content.order = order;
            content.base = validatedBase;
            content.distance = validatedBase;
            content.expandDelta = 0.0f;
        }

        void Link(NestedSegmentContext* new_parent) noexcept {
            if (!new_parent || new_parent == this) return;
            Unlink();
            new_parent->children.push_back(this);
            parent = new_parent;

            UpdateDepth();
            RefreshAccumulateBaseDistance(depth, validatedBase, validatedMin, true);
            parent->UpdateContext();
        }

        void Unlink() noexcept {
            if (parent == nullptr) return;

            const auto it = std::ranges::find(parent->children, this);
            if (it == parent->children.end()) return;

            parent->children.erase(it);

            RefreshAccumulateBaseDistance(depth, validatedBase, validatedMin, false);
            parent->UpdateContext();
            parent = nullptr;

            UpdateDepth();
        }

        void Clear() noexcept {
            while (!children.empty()) {
                auto& it = *children.begin();
                it->Unlink();
            }
            Unlink();
            childrenIndies.clear();
            compressCascadePriorities.clear();
            expandCascadePriorities.clear();
        }

    private:
        NestedSegmentContext* parent{nullptr};
        std::vector<NestedSegmentContext*> children;
        std::unordered_map<std::string, size_t> childrenIndies;
        size_t depth{0};
        size_t order{0};

        float validatedMin{0.0f};
        float validateMax{0.0f};
        float validatedBase{0.0f};
        float compressRatio{0.0f};
        float expandRatio{0.0f};
        float compressCapacity{0.0f};
        float compressSolidify{0.0f};

        float accumulateBaseDistance{0.0f};
        float accumulateMin{0.0f};
        float accumulateCompressSolidify{0.0f};
        float accumulateExpandRatio{0.0f};

        std::vector<size_t> compressCascadePriorities;
        std::vector<size_t> expandCascadePriorities;

        void UpdateDepth() noexcept {
            depth = parent ? parent->depth + 1 : 0;

            for (const auto &val: children ) {
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

        friend NestedSegmentContext;
    };

    using SegmentsPtrHandler = std::vector<std::unique_ptr<Segment>>;
}
