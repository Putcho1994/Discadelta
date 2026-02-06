//
// Created by Puwiwad B on 02.01.2026.
//
module;

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <ranges>
#include <unordered_map>

export module ufox_discadelta_lib;

export namespace ufox::geometry::discadelta {
    enum class FlexDirection {
        Column,
        Row,
    };

    enum class LengthUnitType : uint8_t {
        Auto,
        Flat,
        Percent
    };

    struct Length {
        LengthUnitType type{LengthUnitType::Flat};
        float value{0.0f};

        constexpr Length() noexcept = default;
        constexpr Length(const LengthUnitType t, const float v) noexcept : type(t), value(v) {}

        friend constexpr Length operator""_flat(unsigned long long v) noexcept { return {LengthUnitType::Flat, static_cast<float>(v)}; }
        friend constexpr Length operator""_flat(long double v) noexcept { return {LengthUnitType::Flat, static_cast<float>(v)}; }
        friend constexpr Length operator""_pct(unsigned long long v) noexcept { return {LengthUnitType::Percent, static_cast<float>(v)}; }
        friend constexpr Length operator""_pct(long double v) noexcept { return {LengthUnitType::Percent, static_cast<float>(v)}; }
        friend constexpr Length operator""_auto(unsigned long long) noexcept { return {LengthUnitType::Auto, 0.0f}; }

        [[nodiscard]] constexpr float resolve(const float parentSize, const float autoFallback = 0.0f) const noexcept {
            switch (type) {
                case LengthUnitType::Auto:    return autoFallback;
                case LengthUnitType::Flat:    return value;
                case LengthUnitType::Percent: return parentSize * (value / 100.0f);
            }
            return 0.0f;
        }
    };

    struct LinearSegment {
        std::string name{"none"};
        float base{0.0f};
        float expandDelta{0.0f};
        float distance{0.0f};
        float offset;
        size_t order;
    };

    struct LinearSegmentCreateInfo {
        std::string name{"none"};
        float base{0.0f};
        float compressRatio{0.0f};
        float expandRatio{0.0f};
        float min{0.0f};
        float max{0.0f};
        size_t order;
    };

    struct RectSegment {
        std::string name{"none"};
        float widthBase{0.0f};
        float heightBase{0.0f};
        float widthExpandDelta{0.0f};
        float heightExpandDelta{0.0f};
        float width{0.0f};
        float height{0.0f};
        float x{0.0f};
        float y{0.0f};
        size_t order{0};

        RectSegment() = default;
        ~RectSegment() = default;

        [[nodiscard]] constexpr std::pair<float,float> getSize() const noexcept { return std::make_pair(width, height); }
        [[nodiscard]] constexpr std::pair<float,float> getOffset() const noexcept { return std::make_pair(x, y); }
        [[nodiscard]] constexpr std::pair<uint32_t,uint32_t> getPixelSize() const noexcept {
            return std::make_pair(static_cast<uint32_t>(std::lround(width)), static_cast<uint32_t>(std::lround(height)));
        }
        [[nodiscard]] constexpr std::pair<int,int> getPixelOffset() const noexcept {
            return std::make_pair(static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)));
        }
        [[nodiscard]] constexpr std::tuple<float,float,float,float> getRect() const noexcept { return std::make_tuple(x, y, width, height); }
        [[nodiscard]] constexpr std::tuple<int,int,uint32_t,uint32_t> getPixelRect() const noexcept {
            return std::make_tuple(static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)), static_cast<uint32_t>(std::lround(width)), static_cast<uint32_t>(std::lround(height)));
        }
    };

    struct RectSegmentCreateInfo {
        std::string name{"none"};
        Length width{LengthUnitType::Auto, 0.0f};
        float widthMin{0.0f};
        float widthMax{0.0f};
        Length height{LengthUnitType::Auto, 0.0f};
        float heightMin{0.0f};
        float heightMax{0.0f};
        FlexDirection direction{FlexDirection::Column};
        float flexCompress{0.0f};
        float flexExpand{0.0f};
        size_t order{0};

    };

    struct LinearSegmentContext {
        LinearSegmentCreateInfo config{};
        LinearSegment content{};

        LinearSegmentContext() = default;
        ~LinearSegmentContext(){
            Clear();
            Unlink();
        }

        explicit LinearSegmentContext(LinearSegmentCreateInfo  config) : config(std::move(config)) { UpdateContext();}

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

        [[nodiscard]] constexpr std::span<const LinearSegmentContext* const> GetChildren() const noexcept { return children;}

        [[nodiscard]] constexpr std::span<const size_t> GetCompressCascadePriorities() const noexcept { return compressCascadePriorities; }

        [[nodiscard]] constexpr std::span<const size_t> GetExpandCascadePriorities() const noexcept { return expandCascadePriorities; }

        [[nodiscard]] std::vector<size_t> GetOrderedChildrenIndices() const noexcept {
            std::vector<size_t> indices(children.size());
            std::iota(indices.begin(), indices.end(), size_t{0});

            std::ranges::sort(indices,[this](const size_t a, const size_t b) noexcept {return children[a]->GetOrder() < children[b]->GetOrder();});

            return indices;
        }

        [[nodiscard]] LinearSegmentContext* GetChildByName(const std::string& name) noexcept {
            const auto it = childrenIndies.find(name);
            return it == childrenIndies.end()? nullptr : children[it->second];
        }

        [[nodiscard]] constexpr LinearSegmentContext* GetChildByIndex(const size_t index) const noexcept { return index < children.size() ? children[index] : nullptr; }

        [[nodiscard]] constexpr  float GetAccumulateBase() const noexcept { return accumulateBaseDistance; }

        [[nodiscard]] constexpr  float GetAccumulateMin() const noexcept { return accumulateMin; }

        [[nodiscard]] constexpr  float GetAccumulateCompressSolidify() const noexcept { return accumulateCompressSolidify; }

        [[nodiscard]] constexpr  float GetAccumulateExpandRatio() const noexcept { return accumulateExpandRatio; }


        [[nodiscard]] const LinearSegmentContext* GetRoot() const noexcept {
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

        void Link(LinearSegmentContext* new_parent) noexcept {
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
                const auto& it = *children.begin();
                it->Unlink();
            }
            Unlink();
            childrenIndies.clear();
            compressCascadePriorities.clear();
            expandCascadePriorities.clear();
        }

    private:
        LinearSegmentContext* parent{nullptr};
        std::vector<LinearSegmentContext*> children;
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

        friend LinearSegmentContext;
    };

    struct RectSegmentContext {
        RectSegmentCreateInfo               config{};
        RectSegment                         content{};
        RectSegmentContext* parent = nullptr;
        std::vector<RectSegmentContext*> children;
        std::unordered_map<std::string, size_t> childrenIndies;
        std::vector<size_t> compressCascadePriorities;
        std::vector<size_t> expandCascadePriorities;
        float validatedWidthBase = 0.0f;
        float validatedHeightBase = 0.0f;
        float validatedWidthMin = 0.0f;
        float validatedHeightMin = 0.0f;
        float validatedWidthMax = 0.0f;
        float validatedHeightMax = 0.0f;
        float accumulatedWidthBase = 0.0f;
        float accumulatedHeightBase = 0.0f;
        float accumulatedWidthMin = 0.0f;
        float accumulatedHeightMin = 0.0f;
        float accumulatedWidthCompressSolidify = 0.0f;
        float accumulatedHeightCompressSolidify = 0.0f;
        float accumulatedExpandRatio = 0.0f;
        float compressRatio = 0.0f;
        float widthCompressCapacity = 0.0f;
        float widthCompressSolidify = 0.0f;
        float heightCompressCapacity = 0.0f;
        float heightCompressSolidify = 0.0f;
        float expandRatio = 0.0f;

        explicit RectSegmentContext(RectSegmentCreateInfo  config) : config(std::move(config)) {}
        [[nodiscard]] std::string getName() const noexcept { return config.name; }

        // [[nodiscard]] constexpr float getGreaterWidthMin() const noexcept { return std::max(accumulatedWidthMin, validatedWidthMin); }
        // [[nodiscard]] constexpr float getGreaterWidthBase() const noexcept { return std::max(accumulatedWidthBase, validatedWidthBase); }
        // [[nodiscard]] constexpr float getGreaterHeightMin() const noexcept { return std::max(accumulatedHeightMin, validatedHeightMin); }
        // [[nodiscard]] constexpr float getGreaterHeightBase() const noexcept { return std::max(accumulatedHeightBase, validatedHeightBase); }


        [[nodiscard]] RectSegmentContext* getChildByName(const std::string& name) noexcept {
            const auto it = childrenIndies.find(name);
            return it == childrenIndies.end()? nullptr : children[it->second];
        }
        [[nodiscard]] constexpr RectSegmentContext* getChildByIndex(const size_t index) const noexcept { return index < children.size() ? children[index] : nullptr; }

    };
}
