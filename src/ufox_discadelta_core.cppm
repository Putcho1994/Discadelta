//
// Created by Puwiwad B on 02.01.2026.
//
module;

#include <algorithm>
#include <cmath>
#include <string>
#include <memory>
#include <vector>
#include <ranges>
#include <format>
#include <iomanip>
#include <numeric>

export module ufox_discadelta_core;

import ufox_discadelta_lib;

export namespace ufox::geometry::discadelta {

    void Sizing(LinearSegmentContext& ctx, const float& value, const float& delta, const bool& round);
    void Sizing(RectSegmentContext& ctx, const float& width, const float& height, const float& widthDelta, const float& heightDelta, const bool& round);

    [[nodiscard]] constexpr float ChooseGreaterDistance(const float& a, const float& b) noexcept {
        return std::max(a,b);
    }

    [[nodiscard]] constexpr float ChooseGreaterDistance(const float& a, const float& b, const float& c) noexcept {
        return std::max({a,b,c});
    }

    [[nodiscard]] constexpr float ChooseLowestDistance(const float& a, const float& b) noexcept {
        return std::min({a,b});
    }

    [[nodiscard]] constexpr float ChooseLowestDistance(const float& a, const float& b, const float& c) noexcept {
        return std::min({a,b,c});
    }

    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    [[nodiscard]] constexpr std::vector<size_t> GetOrderedIndices(const ContextT& ctx) noexcept {
        std::vector<size_t> indices(ctx.children.size());
        std::iota(indices.begin(), indices.end(), size_t{0});

        std::ranges::sort(indices,[ctx](const size_t a, const size_t b) noexcept {return ctx.children[a]->order < ctx.children[b]->order;});

        return indices;
    }

    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    [[nodiscard]] constexpr auto GetChildSegmentContext(const ContextT& parentCtx, const std::string& name) noexcept ->ContextT* {
        const auto it = parentCtx.childrenIndies.find(name);
        return it == parentCtx.childrenIndies.end()? nullptr : parentCtx.children[it->second];
    }

    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    [[nodiscard]] constexpr auto GetChildSegmentContext(const ContextT& parentCtx, const size_t index) noexcept ->ContextT*{
        return index < parentCtx.children.size() ? parentCtx.children[index] : nullptr;
    }

    void UpdateAccumulatedMetrics(LinearSegmentContext& ctx) noexcept {
        ctx.accumulatedBase               = 0.0f;
        ctx.accumulatedMin                = 0.0f;
        ctx.accumulatedExpandRatio        = 0.0f;
        ctx.accumulatedCompressSolidify   = 0.0f;
        ctx.childrenIndies.clear();
        if (ctx.children.empty()) return;

        ctx.childrenIndies.reserve(ctx.children.size());

        for (size_t i = 0; i < ctx.children.size(); ++i) {
            const auto& child = ctx.children[i];
            ctx.childrenIndies[child->config.name] = i;
            ctx.accumulatedBase += child->validatedBase;
            ctx.accumulatedMin += ChooseGreaterDistance(child->validatedMin, child->compressSolidify);
            ctx.accumulatedCompressSolidify += child->compressSolidify;
            ctx.accumulatedExpandRatio += child->expandRatio;
        }
    }

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


    void UpdateContextMetrics(LinearSegmentContext& ctx) noexcept{
        UpdateAccumulatedMetrics(ctx);

        const LinearSegmentCreateInfo& config = ctx.config;

        ctx.validatedMin = ChooseGreaterDistance(0.0f, config.min, ctx.accumulatedMin);
        ctx.validatedMax = ChooseGreaterDistance(0.0f, ctx.validatedMin, config.max);
        ctx.validatedBase = std::clamp(ChooseGreaterDistance(0.0f, config.base, ctx.accumulatedBase), ctx.validatedMin, ctx.validatedMax);
        ctx.compressRatio = ChooseGreaterDistance(0.0f, config.flexCompress);
        ctx.compressCapacity = ctx.validatedBase * ctx.compressRatio;
        ctx.compressSolidify = ChooseGreaterDistance(0.0f, ctx.validatedBase - ctx.compressCapacity);
        ctx.expandRatio = ChooseGreaterDistance(0.0f, config.flexExpand);

        UpdatePriorityLists(ctx);

        if (ctx.parent != nullptr && ctx.parent != &ctx){
            UpdateContextMetrics(*ctx.parent);
        }
    }


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
        parent.branchCount = std::max(size_t{1}, parent.branchCount - child.branchCount);
        child.parent = nullptr;

        if (!parent.children.empty()) {
            UpdateContextMetrics(parent);
        }
    }


    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    void Link(ContextT& parent, ContextT& child) noexcept {
        if (&child == &parent) return;
        if (child.parent == &parent) return;
        if (child.parent != nullptr) Unlink(child);

        child.parent = &parent;
        parent.children.push_back(&child);
        parent.branchCount += child.branchCount;

        UpdateContextMetrics(parent);
    }


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


    template<typename ContextT,typename ConfigT>
    requires ((std::same_as<ContextT, LinearSegmentContext> && std::same_as<ConfigT, LinearSegmentCreateInfo>) ||(std::same_as<ContextT, RectSegmentContext>   && std::same_as<ConfigT, RectSegmentCreateInfo>))
    constexpr auto CreateSegmentContext(const ConfigT& config) noexcept -> std::unique_ptr<ContextT, decltype(&DestroySegmentContext<ContextT>)> {
        auto* ctx = new ContextT{config};
        ctx->branchCount = 1;
        UpdateContextMetrics(*ctx);

        return std::unique_ptr<ContextT, decltype(&DestroySegmentContext<ContextT>)>{ctx,&DestroySegmentContext<ContextT>
        };
    }


    [[nodiscard]] constexpr std::pair<float, bool> MakeSizeMetrics(const float& targetDistance, const LinearSegmentContext& ctx, const bool& round) noexcept {
        const float validatedInputDistance = ChooseGreaterDistance(ctx.accumulatedMin, ctx.validatedMin, targetDistance);
        const float roundedBase = round? std::lroundf(ctx.accumulatedBase) : ctx.accumulatedBase;
        const bool isCompressionMode = validatedInputDistance < roundedBase;

        return std::make_tuple(
            validatedInputDistance,
            isCompressionMode
        );
    }


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

    [[nodiscard]] constexpr float Scaler(float distance, float accumulateFactor, float factor) noexcept {
        if (distance <= 0.0f || accumulateFactor <= 0.0f || factor <= 0.0f) {
            return 0.0f;
        }
        return distance <= 0.0f || accumulateFactor <= 0.0f || factor <= 0.0f ? 0.0f : distance / accumulateFactor * factor;
    }

    [[nodiscard]] constexpr std::tuple<float, float, float, std::span<const size_t>>
    MakeCompressCascadeMetrics(const float& inputDistance, const LinearSegmentContext& ctx, const bool& round) noexcept {
        const float accumulatedBase = round? std::lroundf(ctx.accumulatedBase) : ctx.accumulatedBase;

        return std::make_tuple(
            inputDistance,
            accumulatedBase,
            ctx.accumulatedCompressSolidify,
            std::span(ctx.compressCascadePriorities)
        );
    }

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

    [[nodiscard]] constexpr std::tuple<float, float, float, float, float, float>
    MakeCompressSizeMetrics(const float& cascadeCompressDistance, const float& cascadeBaseDistance, const float& cascadeCompressSolidify, const LinearSegmentContext& ctx, const bool& round) noexcept {
        const float childEffectiveBase = ctx.validatedBase;
        const float roundedBase = round? std::lroundf(childEffectiveBase) : childEffectiveBase;

        return std::make_tuple(
            cascadeCompressDistance - cascadeCompressSolidify,
            cascadeBaseDistance - cascadeCompressSolidify,
            ctx.compressSolidify,
            ctx.compressCapacity,
            roundedBase,
            ctx.validatedMin
        );
    }

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

    [[nodiscard]] constexpr std::tuple<bool, float, float, std::span<const size_t>>
    MakeExpandCascadeMetrics(const float& inputDistance, const LinearSegmentContext& ctx, const bool& round) noexcept {
        const float accumulatedBase = round? std::lroundf(ctx.accumulatedBase) : ctx.accumulatedBase;
        const float cascadeExpandDelta = std::max(inputDistance - accumulatedBase, 0.0f);

        return std::make_tuple(
            cascadeExpandDelta > 0.0f,
            cascadeExpandDelta,
            ctx.accumulatedExpandRatio,
            std::span(ctx.expandCascadePriorities)
        );
    }

    [[nodiscard]] constexpr std::tuple<const bool,float,float, std::span<const size_t>>
    MakeExpandCascadeMetrics(const float& widthInput, const float& heightInput, const RectSegmentContext& ctx, const bool& isRow, const bool& round) {
        const float& value = isRow ? widthInput : heightInput;
        const float& directionAccumulatedBase = isRow? ctx.accumulatedWidthBase: ctx.accumulatedHeightBase;
        const float accumulatedBase = round? std::lroundf(directionAccumulatedBase) : directionAccumulatedBase;
        const float cascadeExpandDelta = std::max(value - accumulatedBase, 0.0f);

        return std::make_tuple(cascadeExpandDelta > 0.0f,cascadeExpandDelta, ctx.accumulatedExpandRatio, std::span(ctx.expandCascadePriorities));
    }

    [[nodiscard]] constexpr std::tuple<float, float, float>
    MakeExpandSizeMetrics(const LinearSegmentContext& ctx, const bool& round) noexcept {
        const float childEffectiveBase = ctx.validatedBase;
        const float roundedBase = round? std::lroundf(childEffectiveBase) : childEffectiveBase;
        const float childMax = ctx.validatedMax;
        const float expandCapacity = std::max(0.0f, childMax - roundedBase);
        return std::make_tuple(
            roundedBase,
            ctx.expandRatio,
            expandCapacity
        );
    }

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

    constexpr float ComputeCrossSize(const RectSegmentContext& ctx,const float mainInput,const float crossInput,const bool isRow) noexcept {
        const float& input = isRow ? crossInput : mainInput;
        const float& crossValidatedBase = isRow ? ctx.validatedHeightBase : ctx.validatedWidthBase;
        const float& crossMin = isRow ? ctx.validatedHeightMin : ctx.validatedWidthMin;
        const float& crossMax = isRow ? ctx.validatedHeightMax : ctx.validatedWidthMax;
        const float lowestCrossDistance = ChooseLowestDistance(crossMax, input, crossValidatedBase);

        return ChooseGreaterDistance(0.0f, lowestCrossDistance, crossMin);
    }


    void Compressing(const LinearSegmentContext& ctx, const float& inputDistance, const bool& round) noexcept {
        auto [cascadeCompressDistance, cascadeBaseDistance, cascadeCompressSolidify, priorityList] = MakeCompressCascadeMetrics(inputDistance, ctx, round);

        for (const auto index : priorityList) {
            auto* childCtx = GetChildSegmentContext<LinearSegmentContext>(ctx,index);
            if (childCtx == nullptr) continue;
            auto [remainDist, remainCap, solidify, capacity, greaterBase, validatedMin] =
                MakeCompressSizeMetrics(cascadeCompressDistance, cascadeBaseDistance, cascadeCompressSolidify, *childCtx, round);
            const float compressBaseDistance = Scaler(remainDist, remainCap, capacity) + solidify;
            const float clampedDist = ChooseGreaterDistance(compressBaseDistance, validatedMin);
            const float roundedDist = round? std::lroundf(clampedDist) : clampedDist;

            Sizing(*childCtx, roundedDist, 0.0f, round);

            cascadeCompressDistance -= roundedDist;
            cascadeCompressSolidify -= solidify;
            cascadeBaseDistance -= greaterBase;
        }
    }


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


    void Expanding(const LinearSegmentContext& ctx, const float& inputDistance, const bool& round) {
        auto [processingExpansion,cascadeExpandDelta, cascadeExpandRatio, priorityList] = MakeExpandCascadeMetrics(inputDistance, ctx, round);
        if (!processingExpansion) return;

        for (const auto index : priorityList) {
            auto* childCtx = GetChildSegmentContext(ctx,index);
            if (childCtx == nullptr) continue;

            auto [validateBase, expandRatio, maxDelta] = MakeExpandSizeMetrics(*childCtx, round);
            const float expandDelta = Scaler(cascadeExpandDelta, cascadeExpandRatio, expandRatio);
            const float clampedDelta = ChooseLowestDistance(expandDelta, maxDelta);
            const float roundedDelta = round? std::lroundf(clampedDelta) : clampedDelta;

            Sizing(*childCtx, validateBase, roundedDelta, round);

            cascadeExpandDelta -= roundedDelta;
            cascadeExpandRatio -= expandRatio;
        }
    }

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


    void Sizing(LinearSegmentContext& ctx, const float& value, const float& delta, const bool& round) {
        const auto [validatedInputDistance, processingCompression] = MakeSizeMetrics(value, ctx, round);

        ctx.content.base  = validatedInputDistance;
        ctx.content.expandDelta = delta;
        ctx.content.distance = validatedInputDistance + delta;

        if (processingCompression) {
            Compressing(ctx, ctx.content.distance, round);
        }
        else {
            Expanding(ctx, ctx.content.distance, round);
        }
    }

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


    constexpr void Placing(LinearSegmentContext& ctx, const float& parentOffset = 0.0f) noexcept {
        ctx.content.offset = parentOffset;
        if (ctx.children.empty()) return;

        const auto indices = GetOrderedIndices(ctx);

        float currentOffset = parentOffset;
        for (const size_t idx : indices) {
            auto* childCtx = GetChildSegmentContext(ctx,idx);
            if (childCtx == nullptr) continue;
            Placing(*childCtx, currentOffset);
            currentOffset += childCtx->content.distance;
        }
    }

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

    using RectSegmentContextHandler = std::unique_ptr<RectSegmentContext, decltype(&DestroySegmentContext<RectSegmentContext>)>;
    using LinearSegmentContextHandler = std::unique_ptr<LinearSegmentContext, decltype(&DestroySegmentContext<LinearSegmentContext>)>;

    void UpdateSegments(LinearSegmentContext& rootCtx, const float inputDistance, const bool round) {
        Sizing(rootCtx, inputDistance, 0.0f, round);
        Placing(rootCtx);
    }

    void UpdateSegments(RectSegmentContext& rootCtx, const float mainInput, const float crossInput, const bool round ) {
        Sizing(rootCtx, mainInput, crossInput, 0.0f,0.0f, round);
        Placing(rootCtx);
    }
}
