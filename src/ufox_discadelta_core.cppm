//
// Created by Puwiwad B on 02.01.2026.
//
module;

#include <algorithm>
#include <string>
#include <limits>
#include <memory>
#include <vector>
#include <ranges>
#include <format>
#include <iomanip>     // if needed
#include <iostream>

export module ufox_discadelta_core;

import ufox_discadelta_lib;

export namespace ufox::geometry::discadelta {

    void Sizing(LinearSegmentContext& ctx, const float& value, const float& delta = 0.0f);
    void Sizing(RectSegmentContext& ctx, const float& width, const float& height, const float& widthDelta = 0.0f, const float& heightDelta = 0.0f);

    constexpr void UpdateAccumulatedMetrics(RectSegmentContext& ctx) noexcept {
            ctx.accumulatedWidthBase = 0.0f;
            ctx.accumulatedHeightBase = 0.0f;

            ctx.accumulatedWidthMin = 0.0f;
            ctx.accumulatedHeightMin = 0.0f;

            ctx.accumulatedExpandRatio = 0.0f;
            ctx.accumulatedWidthCompressSolidify = 0.0f;
            ctx.accumulatedHeightCompressSolidify = 0.0f;

            ctx.childrenIndies.clear();
            ctx.childrenIndies.reserve(ctx.children.size());

            ctx.compressCascadePriorities.clear();
            ctx.expandCascadePriorities.clear();
            ctx.compressCascadePriorities.reserve(ctx.children.size());
            ctx.expandCascadePriorities.reserve(ctx.children.size());

            for (size_t i = 0; i < ctx.children.size(); ++i) {
                const auto& child = ctx.children[i];
                ctx.childrenIndies[child->getName()] = i;

                ctx.accumulatedWidthBase += child->config.width.type == LengthUnitType::Auto? child->accumulatedWidthBase : child->validatedWidthBase;
                ctx.accumulatedHeightBase += child->config.height.type == LengthUnitType::Auto? child->accumulatedHeightBase : child->validatedHeightBase;

                if (ctx.config.direction == FlexDirection::Row) {
                    const float& childWidthMin = std::max(child->validatedWidthMin, child->widthCompressSolidify);
                    const float& childHeightMin = std::max(child->validatedHeightMin, child->accumulatedHeightMin);
                    ctx.accumulatedWidthMin += std::max(childWidthMin, child->accumulatedWidthMin);
                    ctx.accumulatedHeightMin = std::max(ctx.accumulatedHeightMin, childHeightMin);
                }
                else {
                    const float& childHeightMin = std::max(child->validatedHeightMin, child->heightCompressSolidify);
                    const float& childWidthMin = std::max(child->validatedWidthMin, child->accumulatedWidthMin);
                    ctx.accumulatedWidthMin = std::max(ctx.accumulatedWidthMin, childWidthMin);
                    ctx.accumulatedHeightMin += std::max(childHeightMin, child->accumulatedHeightMin);
                }

                ctx.accumulatedExpandRatio += child->expandRatio;
                ctx.accumulatedWidthCompressSolidify += child->widthCompressSolidify;
                ctx.accumulatedHeightCompressSolidify += child->heightCompressSolidify;
            }
    }

    constexpr void UpdatePriorityLists(RectSegmentContext& ctx) noexcept {
            ctx.compressCascadePriorities.clear();
            ctx.expandCascadePriorities.clear();
            ctx.compressCascadePriorities.reserve(ctx.children.size());
            ctx.expandCascadePriorities.reserve(ctx.children.size());

            std::vector<std::pair<float, size_t>> compressPriorities;
            std::vector<std::pair<float, size_t>> expandPriorities;
            compressPriorities.reserve(ctx.children.size());
            expandPriorities.reserve(ctx.children.size());

            for (size_t i = 0; i < ctx.children.size(); ++i) {
                const auto& child = ctx.children[i];
                if (ctx.config.direction == FlexDirection::Row) {
                    compressPriorities.emplace_back(std::max(0.0f, child->validatedWidthBase - child->validatedWidthMin), i);
                    expandPriorities.emplace_back(std::max(0.0f, child->validatedWidthMax - child->validatedWidthBase), i);
                } else {
                    compressPriorities.emplace_back(std::max(0.0f, child->validatedHeightBase - child->validatedHeightMin), i);
                    expandPriorities.emplace_back(std::max(0.0f, child->validatedHeightMax - child->validatedHeightBase), i);
                }
            }

            // Sort compress priorities ascending (lowest first)
            std::ranges::sort(compressPriorities, [](const auto& a, const auto& b) {
                return a.first < b.first;
            });
            for (const auto &val: compressPriorities | std::views::values) {
                ctx.compressCascadePriorities.push_back(val);
            }

            // Sort expand priorities descending (highest first)
            std::ranges::sort(expandPriorities, [](const auto& a, const auto& b) {
                return a.first > b.first;
            });
            for (const auto &val: expandPriorities | std::views::values) {
                ctx.expandCascadePriorities.push_back(val);
            }
        }

    constexpr void UpdateContextMetrics(RectSegmentContext& ctx) noexcept {
        float parentWidth{0.0f};
        float parentHeight{0.0f};

        if (ctx.parent != nullptr) {
            parentWidth = ctx.parent->content.width;
            parentHeight = ctx.parent->content.height;
        }

        const RectSegmentCreateInfo& config = ctx.config;

        ctx.validatedWidthMin = std::max(0.0f, config.widthMin);
        ctx.validatedWidthMax = std::max(ctx.validatedWidthMin, config.widthMax);
        ctx.validatedHeightMin = std::max(0.0f, config.heightMin);
        ctx.validatedHeightMax = std::max(ctx.validatedHeightMin, config.heightMax);

        ctx.validatedWidthBase = std::clamp(config.width.resolve(parentWidth), ctx.validatedWidthMin, ctx.validatedWidthMax);
        ctx.validatedHeightBase = std::clamp(config.height.resolve(parentHeight), ctx.validatedHeightMin, ctx.validatedHeightMax);
        ctx.compressRatio = std::max(0.0f,config.flexCompress);
        ctx.widthCompressCapacity = ctx.validatedWidthBase * ctx.compressRatio;
        ctx.widthCompressSolidify = std::max(0.0f, ctx.validatedWidthBase  - ctx.widthCompressCapacity);
        ctx.heightCompressCapacity = ctx.validatedHeightBase * ctx.compressRatio;
        ctx.heightCompressSolidify = std::max(0.0f, ctx.validatedHeightBase - ctx.heightCompressCapacity);
        ctx.expandRatio = std::max(0.0f, config.flexExpand);
        if (ctx.children.empty()) return;

        UpdateAccumulatedMetrics(ctx);
        UpdatePriorityLists(ctx);

        if (ctx.parent != nullptr && ctx.parent != &ctx) {
            UpdateContextMetrics(*ctx.parent);
        }
    }

    void Unlink(RectSegmentContext& parent, RectSegmentContext& child) noexcept {
        const auto it = std::ranges::find(parent.children, &child);
        if (it == parent.children.end()) return;
        parent.children.erase(it);
        child.parent = nullptr;
        UpdateContextMetrics(parent);
    }

    void Link(RectSegmentContext& parent, RectSegmentContext& child) noexcept {
        if (&child == &parent) return;
        if (child.parent == &parent) return;
        if (child.parent != nullptr)  Unlink(*child.parent,child);
        child.parent = &parent;
        parent.children.push_back(&child);

        UpdateContextMetrics(parent);
    }


    constexpr void DestroyRectSegmentContext(RectSegmentContext* ptr) noexcept
    {
        if (ptr)
        {
            while (!ptr->children.empty()) {
                const auto& it = *ptr->children.begin();
                Unlink(*ptr,*it);
            }
            ptr->children.clear();
            if (ptr->parent != nullptr) Unlink(*ptr->parent,*ptr);
            ptr->childrenIndies.clear();
            ptr->compressCascadePriorities.clear();
            ptr->expandCascadePriorities.clear();

            delete ptr;
        }
    }

    constexpr std::pair<float, bool> MakeSizeMetrics(const float& value, const float& validatedMin, const float& accumulateBase) noexcept {
        const float validatedInputDistance = std::max(validatedMin, value);
        return std::make_pair(validatedInputDistance, validatedInputDistance < accumulateBase);
    }

    constexpr std::tuple<float, float, bool, bool> MakeSizeMetrics(const float& targetWidth, const float& targetHeight,
       const RectSegmentContext& ctx) noexcept {
        const float validatedWidthInput = std::max(std::max(ctx.accumulatedWidthMin, ctx.validatedWidthMin), targetWidth);
        const float validatedHeightInput = std::max(std::max(ctx.accumulatedHeightMin, ctx.validatedHeightMin), targetHeight);
        const bool isRow = ctx.config.direction == FlexDirection::Row;
        const float directionAccumulatedBase = isRow ? ctx.accumulatedWidthBase : ctx.accumulatedHeightBase;
        const float directionInput =  isRow ? validatedWidthInput : validatedHeightInput;

        return std::make_tuple(validatedWidthInput, validatedHeightInput, isRow, directionInput < directionAccumulatedBase);
    }

   std::unique_ptr<RectSegmentContext, decltype(&DestroyRectSegmentContext)> CreateRectSegmentContext(const RectSegmentCreateInfo& config, const float& mainInput =0, const float& crossInput=0) noexcept {
        auto* ctx = new RectSegmentContext{config};
        UpdateContextMetrics(*ctx);

        const auto [validatedWidthInput, validatedHeightInput, isRow, processingCompression] = MakeSizeMetrics(mainInput, crossInput, *ctx);

        ctx->content.widthBase  = validatedWidthInput;
        ctx->content.heightBase = validatedHeightInput;
        ctx->content.width = validatedWidthInput;
        ctx->content.height = validatedHeightInput;

        return std::unique_ptr<RectSegmentContext, decltype(&DestroyRectSegmentContext)>(std::move(ctx), &DestroyRectSegmentContext);
    }


    /**
     * Computes a scaled value based on a given distance, accumulation factor and scaling factor,
     * ensuring all inputs are positive and valid.
     *
     * @param distance A floating-point value representing the input distance to be scaled.
     *        Must be greater than 0.0f to produce a valid result.
     * @param accumulateFactor A floating-point value representing the accumulation factor used
     *        in scaling. Must be greater than 0.0f to avoid division by zero.
     * @param factor A floating-point scaling factor applied to the result after dividing the
     *        distance by the accumulateFactor. Must be greater than 0.0f.
     *
     * @return The computed scaled value as a floating-point result. If any input parameter
     *         is less than or equal to 0.0f, it returns 0.0f.
     */
    [[nodiscard]] constexpr float Scaler(float distance, float accumulateFactor, float factor) noexcept {
        if (distance <= 0.0f || accumulateFactor <= 0.0f || factor <= 0.0f) {
            return 0.0f;
        }
        return distance <= 0.0f || accumulateFactor <= 0.0f || factor <= 0.0f ? 0.0f : distance / accumulateFactor * factor;
    }



    constexpr std::tuple<float,float,float, std::span<const size_t>>
    MakeCompressCascadeMetrics(const float& input,const LinearSegmentContext& ctx ) noexcept {
        return std::make_tuple(
            input, ctx.GetAccumulateBase(),
            ctx.GetAccumulateCompressSolidify(),
            ctx.GetCompressCascadePriorities());
    }

    constexpr std::tuple<float,float,float, std::span<const size_t>>
    MakeCompressCascadeMetrics(const float& widthInput, const float& heightInput, const RectSegmentContext& ctx, const bool& isRow ) noexcept {
        const float& value = isRow ? widthInput : heightInput;

        return std::make_tuple(
            value,
            isRow ? ctx.accumulatedWidthBase : ctx.accumulatedHeightBase,
            isRow ? ctx.accumulatedWidthCompressSolidify: ctx.accumulatedHeightCompressSolidify,
            std::span(ctx.compressCascadePriorities));
    }

    constexpr std::tuple<const float,const float,const float,const float,const float,const float>
    MakeCompressSizeMetrics(const float& cascadeCompressDistance, const float&cascadeBaseDistance, const float& cascadeCompressSolidify, const LinearSegmentContext& ctx) noexcept {

        return std::make_tuple(
            cascadeCompressDistance - cascadeCompressSolidify,
            cascadeBaseDistance - cascadeCompressSolidify,
            ctx.GetCompressSolidify(),
            ctx.GetCompressCapacity(),
            ctx.GetValidatedBase(),
            ctx.GetValidatedMin());
    }

    constexpr float ComputeCrossSize(const RectSegmentContext& ctx,const float mainInput,const float crossInput,const bool isRow) noexcept {
        const Length& crossLength   = isRow ? ctx.config.height : ctx.config.width;
        const float&   input    = isRow ? crossInput : mainInput;
        const float&   crossValidatedBase = isRow ? ctx.validatedHeightBase : ctx.validatedWidthBase;
        const float&   crossMin      = isRow ? ctx.validatedHeightMin : ctx.validatedWidthMin;
        const float&   crossMax      = isRow ? ctx.validatedHeightMax : ctx.validatedWidthMax;

        const float oppositeAutoBase = crossLength.type != LengthUnitType::Flat? crossLength.resolve(input, input)  : crossValidatedBase;

        return std::clamp(oppositeAutoBase, crossMin, crossMax);
    }

    constexpr std::tuple<const float,const float,const float,const float,const float,const float>
    MakeCompressSizeMetrics(const float& cascadeCompressDistance, const float&cascadeBaseDistance, const float& cascadeCompressSolidify, const RectSegmentContext& ctx, const bool& isRow) noexcept {

        return std::make_tuple(
            cascadeCompressDistance - cascadeCompressSolidify,
            cascadeBaseDistance - cascadeCompressSolidify,
            isRow? ctx.widthCompressSolidify: ctx.heightCompressSolidify,
            isRow? ctx.widthCompressCapacity: ctx.heightCompressCapacity,
            isRow ?
            ctx.config.width.type == LengthUnitType::Auto ? ctx.accumulatedWidthBase : ctx.validatedWidthBase :
            ctx.config.height.type == LengthUnitType::Auto ? ctx.accumulatedHeightBase : ctx.validatedHeightBase,
            isRow? ctx.validatedWidthMin : ctx.validatedHeightMin);
    }

    constexpr std::tuple<const bool,float,float, std::span<const size_t>>
    MakeExpandCascadeMetrics(const float& value, const LinearSegmentContext& ctx) {
        const float cascadeExpandDelta = std::max(value - ctx.GetAccumulateBase(), 0.0f);

        return std::make_tuple(cascadeExpandDelta > 0.0f,cascadeExpandDelta, ctx.GetAccumulateExpandRatio(), ctx.GetExpandCascadePriorities());
    }

    constexpr std::tuple<const bool,float,float, std::span<const size_t>>
    MakeExpandCascadeMetrics(const float& widthInput, const float& heightInput, const RectSegmentContext& ctx, const bool& isRow) {
        const float& value = isRow ? widthInput : heightInput;
        const float& directionAccumulatedBase = isRow? ctx.accumulatedWidthBase: ctx.accumulatedHeightBase;
        const float cascadeExpandDelta = std::max(value - directionAccumulatedBase, 0.0f);

        return std::make_tuple(cascadeExpandDelta > 0.0f,cascadeExpandDelta, ctx.accumulatedExpandRatio, std::span(ctx.expandCascadePriorities));
    }

    constexpr std::tuple<const float, const float,const float>
    MakeExpandSizeMetrics(const LinearSegmentContext& ctx) {
        const float base = ctx.GetValidatedBase();
        return std::make_tuple(base, ctx.GetExpandRatio(), std::max(0.0f, ctx.GetValidateMax() - base));
    }

    constexpr std::tuple<const float, const float, const float>
    MakeExpandSizeMetrics(const RectSegmentContext& ctx, const bool& isRow) {

        const float& directionBase = isRow ?
        ctx.config.width.type == LengthUnitType::Auto ? ctx.accumulatedWidthBase : ctx.validatedWidthBase :
        ctx.config.height.type == LengthUnitType::Auto ? ctx.accumulatedHeightBase : ctx.validatedHeightBase;
        const float& directionMax = isRow ? ctx.validatedWidthMax : ctx.validatedHeightMax;

        const float expandCapacity = std::max(0.0f, directionMax - directionBase);

        return std::make_tuple(
            directionBase,
            ctx.expandRatio,
            expandCapacity);
    }


    void Compressing(const LinearSegmentContext& ctx, const float& inputDistance) noexcept {
        auto [cascadeCompressDistance, cascadeBaseDistance, cascadeCompressSolidify, priorityList] = MakeCompressCascadeMetrics(inputDistance, ctx);

        for (const auto index : priorityList) {
            auto* childCtx = ctx.GetChildByIndex(index);
            if (childCtx == nullptr) continue;
            auto [remainDist, remainCap, solidify, capacity, greaterBase, validatedMin] =
                MakeCompressSizeMetrics(cascadeCompressDistance, cascadeBaseDistance, cascadeCompressSolidify, *childCtx);
            const float compressBaseDistance = Scaler(remainDist, remainCap, capacity) + solidify;
            const float clampedDist = std::max(compressBaseDistance, validatedMin);

            Sizing(*childCtx, clampedDist);

            cascadeCompressDistance -= clampedDist;
            cascadeCompressSolidify -= solidify;
            cascadeBaseDistance -= greaterBase;
        }
    }

    void Compressing(const RectSegmentContext& ctx, const float& mainInput, const float& crossInput, const bool& isRow) noexcept {
        auto [cascadeCompressDistance, cascadeBaseDistance, cascadeCompressSolidify, priorityList] = MakeCompressCascadeMetrics(mainInput, crossInput, ctx, isRow);

        for (const auto index : priorityList) {
            auto* childCtx = ctx.getChildByIndex(index);
            if (childCtx == nullptr) continue;
            auto [remainDist, remainCap, solidify, capacity, validatedBase, validatedMin] =
                MakeCompressSizeMetrics(cascadeCompressDistance, cascadeBaseDistance, cascadeCompressSolidify, *childCtx, isRow);
            const float compressBaseDistance = Scaler(remainDist, remainCap, capacity) + solidify;
            const float clampedDist = std::max(compressBaseDistance, validatedMin);

            const float oppositeBase = ComputeCrossSize(*childCtx, mainInput, crossInput, isRow);
            const float& widthDist = isRow ? clampedDist : oppositeBase;
            const float& heightDist = isRow ? oppositeBase : clampedDist;

            Sizing(*childCtx, widthDist, heightDist);

            cascadeCompressDistance -= clampedDist;
            cascadeCompressSolidify -= solidify;
            cascadeBaseDistance -= validatedBase;
        }
    }

    void Expanding(const LinearSegmentContext& ctx, const float& inputDistance) {
        auto [processingExpansion,cascadeExpandDelta, cascadeExpandRatio, priorityList] = MakeExpandCascadeMetrics(inputDistance, ctx);
        if (!processingExpansion) return;


        for (const auto index : priorityList) {
            auto* childCtx = ctx.GetChildByIndex(index);
            if (childCtx == nullptr) continue;

            auto [validateBase, expandRatio, maxDelta] = MakeExpandSizeMetrics(*childCtx);
            const float expandDelta = Scaler(cascadeExpandDelta, cascadeExpandRatio, expandRatio);
            const float clampedDelta = std::min(expandDelta, maxDelta);

            Sizing(*childCtx, validateBase, clampedDelta );

            cascadeExpandDelta -= clampedDelta;
            cascadeExpandRatio -= expandRatio;
        }
    }

    void Expanding(const RectSegmentContext& ctx, const float& mainInput, const float& crossInput, const bool& isRow) {
        auto [processingExpansion,cascadeExpandDelta, cascadeExpandRatio, priorityList] = MakeExpandCascadeMetrics(mainInput, crossInput, ctx, isRow);

        if (!processingExpansion) return;

        for (const auto index : priorityList) {
            auto* childCtx = ctx.getChildByIndex(index);
            if (childCtx == nullptr) continue;

            auto [validatedBase, expandRatio, maxDelta] = MakeExpandSizeMetrics(*childCtx, isRow);
            const float expandDelta = Scaler(cascadeExpandDelta, cascadeExpandRatio, expandRatio);
            const float clampedDelta = std::min(expandDelta, maxDelta);

            const float& widthDelta = isRow ? clampedDelta : 0;
            const float& heightDelta = isRow ? 0 : clampedDelta;

            const float oppositeBase = ComputeCrossSize(*childCtx, mainInput, crossInput, isRow);
            const float& widthDist = isRow ? validatedBase : oppositeBase;
            const float& heightDist = isRow ? oppositeBase : validatedBase;

            Sizing(*childCtx, widthDist, heightDist, widthDelta, heightDelta );

            cascadeExpandDelta -= clampedDelta;
            cascadeExpandRatio -= expandRatio;
        }
    }

    void Sizing(LinearSegmentContext& ctx, const float& value, const float& delta) {
        const auto [validatedInputDistance, processingCompression] = MakeSizeMetrics(value, ctx.GetValidatedMin(), ctx.GetAccumulateBase());

        ctx.content.base  = validatedInputDistance;
        ctx.content.expandDelta = delta;
        ctx.content.distance = validatedInputDistance + delta;

        if (processingCompression) {
            Compressing(ctx, ctx.content.distance);
        }
        else {
            Expanding(ctx, ctx.content.distance);
        }
    }

    void Sizing(RectSegmentContext& ctx, const float& width, const float& height, const float& widthDelta, const float& heightDelta) {
        const auto [validatedWidthInput, validatedHeightInput, isRow, processingCompression] = MakeSizeMetrics(width, height, ctx);

        ctx.content.widthBase  = validatedWidthInput;
        ctx.content.heightBase = validatedHeightInput;
        ctx.content.widthExpandDelta = widthDelta;
        ctx.content.heightExpandDelta = heightDelta;
        ctx.content.width = validatedWidthInput + widthDelta;
        ctx.content.height = validatedHeightInput + heightDelta;

        if (processingCompression) {
            Compressing(ctx, ctx.content.width, ctx.content.height, isRow);
        }
        else {
            Expanding(ctx, ctx.content.width, ctx.content.height, isRow);
        }
    }

    void Placing(LinearSegmentContext& ctx, const float& parentOffset = 0.0f) noexcept {
        ctx.content.offset = parentOffset;
        if (ctx.GetChildCount() == 0) return;

        auto indices = ctx.GetOrderedChildrenIndices();

        float currentOffset = parentOffset;
        for (size_t idx : indices) {
            auto* childCtx = ctx.GetChildByIndex(idx);
            if (childCtx == nullptr) continue;
            Placing(*childCtx, currentOffset);
            currentOffset += childCtx->content.distance;
        }
    }
}
