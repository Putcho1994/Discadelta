# Discadelta: RectSegmentContext — 2D Extension with Templates and Cross-Axis Calculations 🦊 (Chapter 6)

## Overview

After implementing nested 1D linear layouts in Chapter 5, we now extend Discadelta into full 2D rectangular space using RectSegmentContext.

This version supports:
- FlexDirection::Row and FlexDirection::Column flows
- Independent main-axis and cross-axis sizing
- Percentage, fixed, and aspect-ratio based cross-axis behavior
- Templated helpers shared between LinearSegmentContext and RectSegmentContext
- Bottom-up metric accumulation + top-down placing (same philosophy as 1D)

The design is heavily inspired by modern layout engines (CSS Flexbox, Yoga, Flutter) but optimized for real-time game UI usage (Vulkan/SDL3/GLFW resize events → fast dirty subtree updates).

## Goals for the 2D Version

1. Reuse as much 1D logic as possible via C++23 templates
2. Support meaningful cross-axis sizing rules (auto, %, fixed, aspect-ratio locked)
3. Keep pre-computation fast and incremental (O(N) per structural change)
4. Guarantee exact sum matching (no float drift)
5. Allow nested hierarchies with different flow directions per level

## Core Data Structures
```c++
    enum class FlexDirection {
        Column,
        Row
    };
    
    struct RectSegment {
        std::string name{"none"};
        float widthBase           {0.0f};
        float heightBase          {0.0f};
        float widthExpandDelta    {0.0f};
        float heightExpandDelta   {0.0f};
        float width               {0.0f};
        float height              {0.0f};
        float x                   {0.0f};
        float y                   {0.0f};
        size_t order              {0};
    };
    
    struct RectSegmentCreateInfo {
        std::string name{"none"};
        float width{0.0f};
        float widthMin{0.0f};
        float widthMax{0.0f};
        float height{0.0f};
        float heightMin{0.0f};
        float heightMax{0.0f};
        FlexDirection direction{FlexDirection::Column};
        float flexCompress{0.0f};
        float flexExpand{0.0f};
        size_t order{0};
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
        float accumulatedCompressSolidify = 0.0f;
        float accumulatedExpandRatio = 0.0f;
        float compressRatio = 0.0f;
        float widthCompressCapacity = 0.0f;
        float widthCompressSolidify = 0.0f;
        float heightCompressCapacity = 0.0f;
        float heightCompressSolidify = 0.0f;
        float expandRatio = 0.0f;
        size_t order{0};
    
        explicit RectSegmentContext(RectSegmentCreateInfo  config) : config(std::move(config)) {}
    };
```
## Templated Helpers for Reuse
To share code between 1D (`LinearSegmentContext`) and 2D (`RectSegmentContext`), we use templates:

```c++
    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    [[nodiscard]] constexpr std::vector<size_t> GetOrderedIndices(const ContextT& ctx) noexcept {
        std::vector<size_t> indices(ctx.children.size());
        std::iota(indices.begin(), indices.end(), size_t{0});
    
        std::ranges::sort(indices,[ctx](const size_t a, const size_t b) noexcept {return ctx.children[a]->order < ctx.children[b]->order;});
    
        return indices;
    }
```
```c++
    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    [[nodiscard]] constexpr auto GetChildSegmentContext(const ContextT& parentCtx, const std::string& name) noexcept ->ContextT* {
        const auto it = parentCtx.childrenIndies.find(name);
        return it == parentCtx.childrenIndies.end()? nullptr : parentCtx.children[it->second];
    }
```
```c++
    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    [[nodiscard]] constexpr auto GetChildSegmentContext(const ContextT& parentCtx, const size_t index) noexcept ->ContextT*{
        return index < parentCtx.children.size() ? parentCtx.children[index] : nullptr;
    }

```

```c++
    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    constexpr void UpdatePriorityLists(ContextT& ctx) noexcept
    {
        ctx.compressCascadePriorities.clear();
        ctx.expandCascadePriorities.clear();

        if (ctx.children.empty()) {
            return;
        }

        ctx.compressCascadePriorities.reserve(ctx.children.size());
        ctx.expandCascadePriorities.reserve(ctx.children.size());

        std::vector<std::pair<float, size_t>> compressPriorities;
        std::vector<std::pair<float, size_t>> expandPriorities;

        compressPriorities.reserve(ctx.children.size());
        expandPriorities.reserve(ctx.children.size());

        for (size_t i = 0; i < ctx.children.size(); ++i){
            const auto& child = ctx.children[i];

            float compressRoom{0.0f};
            float expandRoom{0.0f};

            if constexpr (std::same_as<ContextT, LinearSegmentContext>) {
                compressRoom = ChooseGreaterDistance(0.0f,child->validatedBase - child->validatedMin);
                expandRoom   = ChooseGreaterDistance(0.0f,child->validatedMax  - child->validatedBase);
            }
            else if (ctx.config.direction == FlexDirection::Row) {
                compressRoom = ChooseGreaterDistance(0.0f,child->validatedWidthBase - child->validatedWidthMin);
                expandRoom   = ChooseGreaterDistance(0.0f,child->validatedWidthMax - child->validatedWidthBase);
            } else {
                compressRoom = ChooseGreaterDistance(0.0f,child->validatedHeightBase - child->validatedHeightMin);
                expandRoom   = ChooseGreaterDistance(0.0f,child->validatedHeightMax - child->validatedHeightBase);
            }

            compressPriorities.emplace_back(compressRoom, i);
            expandPriorities.emplace_back(expandRoom,   i);
        }

        std::ranges::sort(compressPriorities, [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        for (const auto &val: compressPriorities | std::views::values) {
            ctx.compressCascadePriorities.push_back(val);
        }

        std::ranges::sort(expandPriorities, [](const auto& a, const auto& b) {
            return a.first > b.first;
        });

        for (const auto &val: expandPriorities | std::views::values) {
            ctx.expandCascadePriorities.push_back(val);
        }
    }
```
```c++
    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    void Unlink(ContextT& child) noexcept {
        if (child.parent == nullptr) return;

        auto& parent = *child.parent;
        const auto it = std::ranges::find(parent.children, &child);

        if (it == parent.children.end()) {
            child.parent = nullptr;
            return;
        }

        parent.children.erase(it);
        child.parent = nullptr;

        if (!parent.children.empty()) {
            UpdateContextMetrics(parent);
        }
    }

```
```c++
    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    void Link(ContextT& parent, ContextT& child) noexcept {
        if (&child == &parent) return;
        if (child.parent == &parent) return;
        if (child.parent != nullptr) Unlink(child);

        child.parent = &parent;
        parent.children.push_back(&child);

        UpdateContextMetrics(parent);
    }

```
```c++
    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    constexpr void DestroySegmentContext(ContextT* ptr) noexcept {
        if (!ptr) return;

        // Unlink children (no ownership — do NOT destroy them)
        while (!ptr->children.empty()) {
            ContextT* child = ptr->children.front();
            Unlink(*child);
        }

        ptr->children.clear();

        if (ptr->parent != nullptr) {
            Unlink(*ptr);
        }

        ptr->childrenIndies.clear();
        ptr->compressCascadePriorities.clear();
        ptr->expandCascadePriorities.clear();

        delete ptr;
    }
```
```c++
    template<typename ContextT,typename ConfigT>
    requires ((std::same_as<ContextT, LinearSegmentContext> && std::same_as<ConfigT, LinearSegmentCreateInfo>) ||(std::same_as<ContextT, RectSegmentContext>   && std::same_as<ConfigT, RectSegmentCreateInfo>))
    constexpr auto CreateSegmentContext(const ConfigT& config) noexcept -> std::unique_ptr<ContextT, decltype(&DestroySegmentContext<ContextT>)> {
        auto* ctx = new ContextT{config};
        UpdateContextMetrics(*ctx);

        return std::unique_ptr<ContextT, decltype(&DestroySegmentContext<ContextT>)>{ctx,&DestroySegmentContext<ContextT>
        };
    }
```

These helpers work seamlessly for both types, enabling polymorphic tree traversal without runtime overhead.

## Accumulation and Update Metrics (Pre-Compute)
Bottom-up: Sum child metrics per axis.
```c++
    void UpdateAccumulatedMetrics(RectSegmentContext& ctx) noexcept {
        ctx.accumulatedWidthBase = 0.0f;
        ctx.accumulatedHeightBase = 0.0f;
        ctx.accumulatedWidthMin = 0.0f;
        ctx.accumulatedHeightMin = 0.0f;
        ctx.accumulatedExpandRatio = 0.0f;
        ctx.accumulatedCompressSolidify = 0.0f;
        ctx.childrenIndies.clear();
        if (ctx.children.empty()) return;

        ctx.childrenIndies.reserve(ctx.children.size());

        for (size_t i = 0; i < ctx.children.size(); ++i) {
            const auto& child = ctx.children[i];
            ctx.childrenIndies[child->config.name] = i;


            float compressSolidify{0.0f};

            if (ctx.config.direction == FlexDirection::Row) {
                ctx.accumulatedWidthBase += child->validatedWidthBase;
                ctx.accumulatedHeightBase = ChooseGreaterDistance(ctx.accumulatedHeightBase, child->validatedHeightBase);
                compressSolidify = child->widthCompressSolidify;
                const float& childWidthMin = ChooseGreaterDistance(child->validatedWidthMin, compressSolidify);
                const float& childHeightMin = ChooseGreaterDistance(child->validatedHeightMin, child->accumulatedHeightMin);
                ctx.accumulatedWidthMin += ChooseGreaterDistance(childWidthMin, child->accumulatedWidthMin);
                ctx.accumulatedHeightMin = ChooseGreaterDistance(ctx.accumulatedHeightMin, childHeightMin);
            }
            else {
                ctx.accumulatedWidthBase = ChooseGreaterDistance(ctx.accumulatedWidthBase, child->validatedWidthBase);
                ctx.accumulatedHeightBase += child->validatedHeightBase;
                compressSolidify = child->heightCompressSolidify;
                const float& childHeightMin = ChooseGreaterDistance(child->validatedHeightMin, compressSolidify);
                const float& childWidthMin = ChooseGreaterDistance(child->validatedWidthMin, child->accumulatedWidthMin);
                ctx.accumulatedWidthMin = ChooseGreaterDistance(ctx.accumulatedWidthMin, childWidthMin);
                ctx.accumulatedHeightMin += ChooseGreaterDistance(child->accumulatedHeightMin, childHeightMin);
            }

            ctx.accumulatedCompressSolidify += compressSolidify;
            ctx.accumulatedExpandRatio += child->expandRatio;
        }

    }
```

validate metrics by choosing the greater of the two values (self and child) per axis. 

```c++
    void UpdateContextMetrics(RectSegmentContext& ctx) noexcept {
        UpdateAccumulatedMetrics(ctx);

        const RectSegmentCreateInfo& config = ctx.config;

        ctx.validatedWidthMin = ChooseGreaterDistance(0.0f, config.widthMin, ctx.accumulatedWidthMin);
        ctx.validatedWidthMax = ChooseGreaterDistance(0.0f, ctx.validatedWidthMin, config.widthMax);
        ctx.validatedHeightMin = ChooseGreaterDistance(0.0f, config.heightMin, ctx.accumulatedHeightMin);
        ctx.validatedHeightMax = ChooseGreaterDistance(0.0f, ctx.validatedHeightMin, config.heightMax);
        ctx.validatedWidthBase = std::clamp(ChooseGreaterDistance(0.0f, config.width, ctx.accumulatedWidthBase), ctx.validatedWidthMin, ctx.validatedWidthMax);
        ctx.validatedHeightBase = std::clamp(ChooseGreaterDistance(0.0f, config.height, ctx.accumulatedHeightBase), ctx.validatedHeightMin, ctx.validatedHeightMax);
        ctx.compressRatio = ChooseGreaterDistance(0.0f,config.flexCompress);
        ctx.widthCompressCapacity = ctx.validatedWidthBase * ctx.compressRatio;
        ctx.widthCompressSolidify = ChooseGreaterDistance(0.0f, ctx.validatedWidthBase  - ctx.widthCompressCapacity);
        ctx.heightCompressCapacity = ctx.validatedHeightBase * ctx.compressRatio;
        ctx.heightCompressSolidify = ChooseGreaterDistance(0.0f, ctx.validatedHeightBase - ctx.heightCompressCapacity);
        ctx.expandRatio = ChooseGreaterDistance(0.0f, config.flexExpand);

        UpdatePriorityLists(ctx);

        if (ctx.parent != nullptr && ctx.parent != &ctx) {
            UpdateContextMetrics(*ctx.parent);
        }
    }
```

## Sizing & Placing

Prepare sizing mode metrics, validating input and catching which is the main axis.
```c++
[[nodiscard]] constexpr std::tuple<float, float, bool, bool> MakeSizeMetrics(const float& targetWidth, const float& targetHeight, const RectSegmentContext& ctx, const bool& round) noexcept {
        const float validatedWidthInput = ChooseGreaterDistance(ctx.accumulatedWidthMin,ctx.validatedWidthMin, targetWidth );
        const float validatedHeightInput = ChooseGreaterDistance(ctx.accumulatedHeightMin, ctx.validatedHeightMin, targetHeight);
        const bool isRow = ctx.config.direction == FlexDirection::Row;
        const float directionAccumulatedBase = isRow ? ctx.accumulatedWidthBase : ctx.accumulatedHeightBase;
        const float roundedBase = round? std::lroundf(directionAccumulatedBase) : directionAccumulatedBase;
        const float directionInput =  isRow ? validatedWidthInput : validatedHeightInput;
        const bool isCompressionMode = directionInput < roundedBase;

        return std::make_tuple(validatedWidthInput, validatedHeightInput, isRow, isCompressionMode);
    }
```


Prepare the cascade metrics, `isRow` is to determine which direction to use for the cascade.

```c++
    [[nodiscard]] constexpr std::tuple<float,float,float, std::span<const size_t>>
    MakeCompressCascadeMetrics(const float& widthInput, const float& heightInput, const RectSegmentContext& ctx, const bool& isRow, const bool& round ) noexcept {
        const float& value = isRow ? widthInput : heightInput;
        const float& directionAccumulatedBase = isRow ? ctx.accumulatedWidthBase : ctx.accumulatedHeightBase;
        const float accumulatedBase = round? std::lroundf(directionAccumulatedBase) : directionAccumulatedBase;

        return std::make_tuple(
            value,
            accumulatedBase,
            ctx.accumulatedCompressSolidify,
            std::span(ctx.compressCascadePriorities));
    }
```

```c++
    [[nodiscard]] constexpr std::tuple<const bool,float,float, std::span<const size_t>>
    MakeExpandCascadeMetrics(const float& widthInput, const float& heightInput, const RectSegmentContext& ctx, const bool& isRow, const bool& round) {
        const float& value = isRow ? widthInput : heightInput;
        const float& directionAccumulatedBase = isRow? ctx.accumulatedWidthBase: ctx.accumulatedHeightBase;
        const float accumulatedBase = round? std::lroundf(directionAccumulatedBase) : directionAccumulatedBase;
        const float cascadeExpandDelta = std::max(value - accumulatedBase, 0.0f);

        return std::make_tuple(cascadeExpandDelta > 0.0f,cascadeExpandDelta, ctx.accumulatedExpandRatio, std::span(ctx.expandCascadePriorities));
    }
```

Prepare tuple of metrics for child sizing using `isRow` to determine which direction to use for the child. 
```c++
    [[nodiscard]] constexpr std::tuple<const float,const float,const float,const float,const float,const float>
    MakeCompressSizeMetrics(const float& cascadeCompressDistance, const float&cascadeBaseDistance, const float& cascadeCompressSolidify, const RectSegmentContext& ctx, const bool& isRow, const bool& round) noexcept {
        const float childEffectiveBase = isRow ?ctx.validatedWidthBase : ctx.validatedHeightBase;
        const float roundedBase = round? std::lroundf(childEffectiveBase) : childEffectiveBase;

        return std::make_tuple(
            cascadeCompressDistance - cascadeCompressSolidify,
            cascadeBaseDistance - cascadeCompressSolidify,
            isRow? ctx.widthCompressSolidify: ctx.heightCompressSolidify,
            isRow? ctx.widthCompressCapacity: ctx.heightCompressCapacity,
            roundedBase,
            isRow? ctx.validatedWidthMin : ctx.validatedHeightMin);
    }
```

```c++
    [[nodiscard]] constexpr std::tuple<const float, const float, const float>
    MakeExpandSizeMetrics(const RectSegmentContext& ctx, const bool& isRow, const bool& round) {
        const float& directionBase = isRow ?ctx.validatedWidthBase :ctx.validatedHeightBase;
        const float roundedBase = round? std::lroundf(directionBase) : directionBase;
        const float& directionMax = isRow ? ctx.validatedWidthMax : ctx.validatedHeightMax;

        const float expandCapacity = std::max(0.0f, directionMax - roundedBase);

        return std::make_tuple(
            roundedBase,
            ctx.expandRatio,
            expandCapacity);
    }
```

Compute Cross size for the child using `isRow` to determine which direction to use for the child.

Select the lowest distance between the child's base, the cross-input, and max constraint.
```c++
    constexpr float ComputeCrossSize(const RectSegmentContext& ctx,const float mainInput,const float crossInput,const bool isRow) noexcept {
        const float& input = isRow ? crossInput : mainInput;
        const float& crossValidatedBase = isRow ? ctx.validatedHeightBase : ctx.validatedWidthBase;
        const float& crossMin = isRow ? ctx.validatedHeightMin : ctx.validatedWidthMin;
        const float& crossMax = isRow ? ctx.validatedHeightMax : ctx.validatedWidthMax;
        const float lowestCrossDistance = ChooseLowestDistance(crossMax, input, crossValidatedBase);

        return ChooseGreaterDistance(0.0f, lowestCrossDistance, crossMin);
    }
```
Compressing and Expanding with the logic to solve which direction is the main axis and which is the cross axis.
```c++
    void Compressing(const RectSegmentContext& ctx, const float& mainInput, const float& crossInput, const bool& isRow, const bool& round) noexcept {
        auto [cascadeCompressDistance, cascadeBaseDistance, cascadeCompressSolidify, priorityList] = MakeCompressCascadeMetrics(mainInput, crossInput, ctx, isRow, round);

        for (const auto index : priorityList) {
            auto* childCtx = GetChildSegmentContext(ctx,index);
            if (childCtx == nullptr) continue;
            auto [remainDist, remainCap, solidify, capacity, validatedBase, validatedMin] =
                MakeCompressSizeMetrics(cascadeCompressDistance, cascadeBaseDistance, cascadeCompressSolidify, *childCtx, isRow, round);
            const float compressBaseDistance = Scaler(remainDist, remainCap, capacity) + solidify;
            const float clampedDist = ChooseGreaterDistance(compressBaseDistance, validatedMin);
            const float roundedDist = round? std::lroundf(clampedDist) : clampedDist;

            const float oppositeBase = ComputeCrossSize(*childCtx, mainInput, crossInput, isRow);
            const float roundedOppositeBase = round? std::lroundf(oppositeBase) : oppositeBase;
            const float& widthDist = isRow ? roundedDist : roundedOppositeBase;
            const float& heightDist = isRow ? roundedOppositeBase : roundedDist;

            Sizing(*childCtx, widthDist, heightDist, 0.0f, 0.0f, round);

            cascadeCompressDistance -= roundedDist;
            cascadeCompressSolidify -= solidify;
            cascadeBaseDistance -= validatedBase;
        }
    }
```

```c++
void Expanding(const RectSegmentContext& ctx, const float& mainInput, const float& crossInput, const bool& isRow, const bool& round) {
        auto [processingExpansion,cascadeExpandDelta, cascadeExpandRatio, priorityList] = MakeExpandCascadeMetrics(mainInput, crossInput, ctx, isRow, round);

        if (!processingExpansion) return;

        for (const auto index : priorityList) {
            auto* childCtx = GetChildSegmentContext(ctx,index);
            if (childCtx == nullptr) continue;

            auto [validatedBase, expandRatio, maxDelta] = MakeExpandSizeMetrics(*childCtx, isRow, round);
            const float expandDelta = Scaler(cascadeExpandDelta, cascadeExpandRatio, expandRatio);
            const float clampedDelta = ChooseLowestDistance(expandDelta, maxDelta);
            const float roundedDelta = round? std::lroundf(clampedDelta) : clampedDelta;

            const float& widthDelta = isRow ? roundedDelta : 0;
            const float& heightDelta = isRow ? 0 : roundedDelta;

            const float oppositeBase = ComputeCrossSize(*childCtx, mainInput, crossInput, isRow);
            const float roundedOppositeBase = round? std::lroundf(oppositeBase) : oppositeBase;
            const float& widthDist = isRow ? validatedBase : roundedOppositeBase;
            const float& heightDist = isRow ? roundedOppositeBase : validatedBase;

            Sizing(*childCtx, widthDist, heightDist, widthDelta, heightDelta , round);

            cascadeExpandDelta -= roundedDelta;
            cascadeExpandRatio -= expandRatio;
        }
    }
```

```c++
    void Sizing(RectSegmentContext& ctx, const float& width, const float& height, const float& widthDelta, const float& heightDelta, const bool& round) {
        const auto [validatedWidthInput, validatedHeightInput, isRow, processingCompression] = MakeSizeMetrics(width, height, ctx, round);

        ctx.content.widthBase  = validatedWidthInput;
        ctx.content.heightBase = validatedHeightInput;
        ctx.content.widthExpandDelta = widthDelta;
        ctx.content.heightExpandDelta = heightDelta;
        ctx.content.width = validatedWidthInput + widthDelta;
        ctx.content.height = validatedHeightInput + heightDelta;

        if (processingCompression) {
            Compressing(ctx, ctx.content.width, ctx.content.height, isRow, round);
        }
        else {
            Expanding(ctx, ctx.content.width, ctx.content.height, isRow, round);
        }
    }
```

Top-down, 2D offsets:

```c++
    constexpr void Placing(RectSegmentContext& ctx, const float relativeX = 0.0f, const float relativeY = 0.0f) noexcept
    {
        ctx.content.x = relativeX;
        ctx.content.y = relativeY;

        if (ctx.children.empty()) return;

        const auto orderedIndices = GetOrderedIndices(ctx);
        const bool isRow = ctx.config.direction == FlexDirection::Row;
        float currentMainOffset = 0.0f;

        for (const size_t idx : orderedIndices){
            RectSegmentContext* child = GetChildSegmentContext(ctx, idx);
            if (child == nullptr) continue;

            const float childX = relativeX + (isRow ? currentMainOffset : 0.0f);
            const float childY = relativeY + (isRow ? 0.0f : currentMainOffset);

            Placing(*child, childX, childY);

            currentMainOffset += isRow ? child->content.width : child->content.height;
        }
    }
```

## Example: Nested 2D Layout (version 1.2)
```c++
import ufox_discadelta_lib;
import ufox_discadelta_core;

#include <cmath>
#include <iostream>
#include <thread>


using namespace ufox::geometry::discadelta;

// ─────────────────────────────────────────────────────────────────────────────
// Debug print for RectSegmentContext tree (2D version)
// Prints name, width/height, x/y, and indents children
// ─────────────────────────────────────────────────────────────────────────────
void PrintTreeDebugWithOffset(const RectSegmentContext& ctx, int indent = 0) noexcept {
    std::string pad(indent * 4, ' ');
    std::cout << pad << ctx.config.name
              << " | w: " << ctx.content.width
              << " | h: " << ctx.content.height
              << " | x: " << ctx.content.x
              << " | y: " << ctx.content.y
              << "\n";

    for (const auto* child : ctx.children) {
        PrintTreeDebugWithOffset(*child, indent + 1);
    }
}

int main() {
    std::cout << "Nester Rect Tree Debugger Test\n\n";

    const std::string title = "Rect Tree Debug";

    auto root = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>({
            .name          = "Root",
            .width         = 0.0f,
            .widthMin      = 0.0f,
            .widthMax      = std::numeric_limits<float>::max(),
            .height        = 0.0f,
            .heightMin     = 0.0f,
            .heightMax     = std::numeric_limits<float>::max(),
            .direction     = FlexDirection::Row,
            .flexCompress  = 1.0f,
            .flexExpand    = 1.0f,
            .order         = 0
            });

    auto rect1 = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>({
            .name          = "Rect01",
            .width         = 0.0f,
            .widthMin      = 50.0f,
            .widthMax      = std::numeric_limits<float>::max(),
            .height        = 0.0f,
            .heightMin     = 50.0f,
            .heightMax     = std::numeric_limits<float>::max(),
            .direction     = FlexDirection::Row,
            .flexCompress  = 1.0f,
            .flexExpand    = 1.0f,
            .order         = 0
            });

    auto rect2 = CreateSegmentContext<RectSegmentContext, RectSegmentCreateInfo>({
            .name          = "Rect02",
            .width         = 0.0f,
            .widthMin      = 50.0f,
            .widthMax      = std::numeric_limits<float>::max(),
            .height        = std::numeric_limits<float>::max(),
            .heightMin     = 00.0f,
            .heightMax     = std::numeric_limits<float>::max(),
            .direction     = FlexDirection::Row,
            .flexCompress  = 1.0f,
            .flexExpand    = 1.0f,
            .order         = 0
            });

    Link(*root.get(), *rect1.get());
    Link(*root.get(), *rect2.get());

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // ────────────────────────────────────────────────────────────────
    // First test: small size (400×600) → should compress
    // ────────────────────────────────────────────────────────────────
    Sizing(*root.get(), 400.0f, 600.0f, 0.0f, 0.0f, false);
    Placing(*root.get());

    std::cout << "=== " << title << " (size 400x600) ===" << std::endl;
    PrintTreeDebugWithOffset(*root.get());

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // ────────────────────────────────────────────────────────────────
    // Second test: larger size (800×600) → should expand (with rounding)
    // ────────────────────────────────────────────────────────────────
    Sizing(*root.get(), 800.0f, 600.0f, 0.0f, 0.0f, true);
    Placing(*root.get());

    std::cout << std::endl;
    std::cout << "=== " << title << " (size 800x600, rounded) ===" << std::endl;
    PrintTreeDebugWithOffset(*root.get());

    std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}
```

## Summary
RectSegmentContext brings Discadelta to 2D with:

* Dual-axis metrics and cascades.
* Templated reuse for 1D/2D sharing.
* Cross-length calculations for flexible perpendicular sizing.
* Efficient tree management for nested UIs.

