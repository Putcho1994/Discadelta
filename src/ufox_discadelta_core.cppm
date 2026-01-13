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
    /**
     * Creates a context for processing segment configurations and input distances, constructing
     * associated metrics and determining the compression state.
     *
     * @param configs A constant reference to a vector of `SegmentConfig` structures, each containing
     *        the configuration data for a segment, including names, base values, compression/expansion
     *        ratios, min/max allowed distances and processing order.
     * @param inputDistance A floating-point value representing the input distance to be validated
     *        and used in the context computation.
     *
     * @return A tuple containing:
     *         - `SegmentsPtrHandler`: A collection of dynamically allocated and initialized `Segment`
     *           objects based on the provided configurations.
     *         - `PreComputeMetrics`: A structure containing precomputed metrics such as base distances,
     *           compression capacities, solidified compression values, minimum and maximum constraints,
     *           prioritized indices for compression and expansion, and accumulated values (e.g., total
     *           base distance, accumulated compression solidification and expansion ratios).
     *         - `bool`: A boolean value indicating if the process involves compression, determined by
     *           comparing the validated input distance with the accumulated base distance.
     *
     * Iterates through each segment configuration, validates and clamps the provided values to respect
     * constraints and computes metrics such as compression/expansion priorities, capacities and limits.
     * Dynamically creates segments, assigns computed attributes and organizes them into a handler.
     * Also determines the order of segments based on their compression and expansion priority, ensuring
     * that the resulting metrics and structure are ready for the next phase of processing.
     */
    auto MakeContext = [](const std::vector<Configuration>& configs, const float inputDistance) -> std::tuple<SegmentsPtrHandler, PreComputeMetrics, bool>{
        const float validatedInputDistance = std::max(0.0f, inputDistance);
        const size_t segmentCount = configs.size();

        SegmentsPtrHandler segments;
        segments.reserve(segmentCount);

        PreComputeMetrics preComputeMetrics(segmentCount, validatedInputDistance);

        float compressPriorityLowestValue = std::numeric_limits<float>::max();
        float expandPriorityLowestValue{0.0f};

        for (size_t i = 0; i < segmentCount; ++i) {
            const auto& [name, rawBase, rawCompressRatio, rawExpandRatio, rawMin, rawMax, rawOrder] = configs[i];

            const float minVal = std::max(0.0f, rawMin);
            const float maxVal = std::max(minVal, rawMax);
            const float baseVal = std::clamp(rawBase, minVal, maxVal);

            const float compressRatio = std::max(0.0f, rawCompressRatio);
            const float expandRatio   = std::max(0.0f, rawExpandRatio);

            const float compressCapacity = baseVal * compressRatio;
            const float compressSolidify = std::max (0.0f, baseVal - compressCapacity);

            preComputeMetrics.compressCapacities.push_back(compressCapacity);
            preComputeMetrics.compressSolidifies.push_back(compressSolidify);
            preComputeMetrics.baseDistances.push_back(baseVal);
            preComputeMetrics.expandRatios.push_back(expandRatio);
            preComputeMetrics.minDistances.push_back(minVal);
            preComputeMetrics.maxDistances.push_back(maxVal);

            preComputeMetrics.accumulateBaseDistance += baseVal;
            preComputeMetrics.accumulateCompressSolidify += compressSolidify;
            preComputeMetrics.accumulateExpandRatio += expandRatio;

            auto seg = std::make_unique<Segment>();
            seg->name = name;
            seg->order = rawOrder;
            seg->base = baseVal;
            seg->distance = baseVal;
            seg->expandDelta = 0.0f;

            preComputeMetrics.segments.push_back(seg.get());
            segments.push_back(std::move(seg));

            const float greaterMin = std::max(compressSolidify, minVal);
            const float compressPriorityValue = std::max(0.0f, baseVal - greaterMin);

            if (compressPriorityValue <= compressPriorityLowestValue) {
                preComputeMetrics.compressPriorityIndies.insert(preComputeMetrics.compressPriorityIndies.begin(), i);
                compressPriorityLowestValue = compressPriorityValue;
            }
            else {
                preComputeMetrics.compressPriorityIndies.push_back(i);
            }

            const float expandPriorityValue = std::max(0.0f, maxVal - baseVal);
            if (expandPriorityValue >= expandPriorityLowestValue) {
                preComputeMetrics.expandPriorityIndies.push_back(i);
                expandPriorityLowestValue = expandPriorityValue;
            }
            else {
                preComputeMetrics.expandPriorityIndies.insert(preComputeMetrics.expandPriorityIndies.begin(), i);
            }
        }

        const bool processingCompression = validatedInputDistance < preComputeMetrics.accumulateBaseDistance;

        return { std::move(segments), std::move(preComputeMetrics), processingCompression };
    };

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

    /**
     * A method for performing discadelta compression on the given pre-computed metrics.
     * This method iteratively adjusts the base and distance values of segments based on
     * cascading compression parameters and clamped limits.
     *
     * @param preComputeMetrics A constant reference to a `DiscadeltaPreComputeMetrics` structure,
     *        containing precomputed quantities and configuration details such as input distances,
     *        accumulated distances, compression priorities, segment references, and individual
     *        constraints (e.g., min distances, base distances and compress capacities).
     *
     * This method ensures that the segments' base and distance parameters are recalculated based
     * on the remaining distances and capacities in a cascading manner. It also enforces segment
     * constraints such as the minimum allowed distances, and updates cascading parameters
     * post-segment adjustment to maintain consistency across the following computations.
     */
    void Compressing(const PreComputeMetrics& preComputeMetrics) {
        float cascadeCompressDistance = preComputeMetrics.inputDistance;
        float cascadeBaseDistance = preComputeMetrics.accumulateBaseDistance;
        float cascadeCompressSolidify = preComputeMetrics.accumulateCompressSolidify;

        for (size_t i = 0; i < preComputeMetrics.segments.size(); ++i) {
            const size_t index = preComputeMetrics.compressPriorityIndies[i];
            Segment* seg = preComputeMetrics.segments[index];
            if (seg == nullptr) break;

            const float remainDist = cascadeCompressDistance - cascadeCompressSolidify;
            const float remainCap = cascadeBaseDistance - cascadeCompressSolidify;
            const float& solidify = preComputeMetrics.compressSolidifies[index];

            const float compressBaseDistance = Scaler(remainDist, remainCap, preComputeMetrics.compressCapacities[index]) + solidify;

            const float& min = preComputeMetrics.minDistances[index];
            const float clampedDist = std::max(compressBaseDistance, min);

            seg->base = seg->distance = clampedDist;
            cascadeCompressDistance -= clampedDist;
            cascadeCompressSolidify -= solidify;
            cascadeBaseDistance -= preComputeMetrics.baseDistances[index];
        }
    }

    /**
     * Expands the distances of all segments according to the provided pre-computed metrics.
     *
     * @param preComputeMetrics A reference to a `DiscadeltaPreComputeMetrics` structure
     *        that contains aggregated pre-computed data for the Discadelta processing, including
     *        input distance, accumulated base distance, expand ratios, priority indices, base distances,
     *        maximum distances and segment references.
     *
     * This function evaluates the remaining expandable distance (`cascadeExpandDelta`) by subtracting
     * the accumulated base distance from the input distance, ensuring a non-negative value. It also
     * initializes the remaining cumulative expand-ratios (`cascadeExpandRatio`).
     *
     * The loop iterates over all priority indices, processing each segment by:
     * - Calculating the potential expand-delta using the `DiscadeltaScaler`.
     * - Clamping the expand-delta within permissible limits defined by the difference between maximum
     *   and base distances of each segment.
     * - Updating the segment's expand-delta and recalculating its expanded distance.
     * - Reducing the remaining expandable delta and cumulative expand ratios for the following segments.
     *
     * If the expandable delta (`cascadeExpandDelta`) is initially non-positive, the function exits
     * early without applying any changes.
     */
    void Expanding(const PreComputeMetrics& preComputeMetrics) {
        float cascadeExpandDelta = std::max(preComputeMetrics.inputDistance - preComputeMetrics.accumulateBaseDistance, 0.0f);
        float cascadeExpandRatio = preComputeMetrics.accumulateExpandRatio;

        if (cascadeExpandDelta <= 0.0f) return;

        for (size_t i = 0; i < preComputeMetrics.segments.size(); ++i) {
            const size_t index = preComputeMetrics.expandPriorityIndies[i];
            Segment* seg = preComputeMetrics.segments[index];
            const float& base = preComputeMetrics.baseDistances[index];
            const float& ratio = preComputeMetrics.expandRatios[index];

            const float expandDelta = Scaler(cascadeExpandDelta, cascadeExpandRatio, ratio);

            const float maxDelta = std::max(0.0f, preComputeMetrics.maxDistances[index] - base);
            const float clampedDelta = std::min(expandDelta, maxDelta);

            seg->expandDelta = clampedDelta;
            seg->distance = base + clampedDelta;
            cascadeExpandDelta -= clampedDelta;
            cascadeExpandRatio -= ratio;
        }
    }

    constexpr std::pair<float, bool> ValidateInputDistance(const float& value, const float& validatedMin, const float& accumulateBase) noexcept {
        const float validatedInputDistance = std::max(validatedMin, value);
        return std::make_pair(validatedInputDistance, validatedInputDistance < accumulateBase);
    }

    constexpr std::tuple<float,float,float, std::span<const size_t>>
    MakeCompressCascadeMetrics(const float& input,const NestedSegmentContext& ctx ) noexcept {
        return std::make_tuple(
            input, ctx.GetAccumulateBase(),
            ctx.GetAccumulateCompressSolidify(),
            ctx.GetCompressCascadePriorities());
    }

    constexpr std::tuple<const float,const float,const float,const float,const float,const float>
    MakeCompressSizeMetrics(const float& cascadeCompressDistance, const float&cascadeBaseDistance, const float& cascadeCompressSolidify, const NestedSegmentContext& ctx) noexcept {
        return std::make_tuple(
            cascadeCompressDistance - cascadeCompressSolidify,
            cascadeBaseDistance - cascadeCompressSolidify,
            ctx.GetCompressSolidify(),
            ctx.GetCompressCapacity(),
            ctx.GetValidatedMin(),
            ctx.GetValidatedBase());
    }

    constexpr std::tuple<const bool,float,float, std::span<const size_t>>
    MakeExpandCascadeMetrics(const float& value, const NestedSegmentContext& ctx) {
        const float cascadeExpandDelta = std::max(value - ctx.GetAccumulateBase(), 0.0f);

        return std::make_tuple(cascadeExpandDelta > 0.0f,cascadeExpandDelta, ctx.GetAccumulateExpandRatio(), ctx.GetExpandCascadePriorities());
    }

    constexpr std::tuple<const float, const float,const float>
    MakeExpandSizeMetrics(const NestedSegmentContext& ctx) {
        const float base = ctx.GetValidatedBase();
        return std::make_tuple(base, ctx.GetExpandRatio(), std::max(0.0f, ctx.GetValidateMax() - base));
    }

    void Sizing(NestedSegmentContext& ctx, const float& value, const float& delta = 0.0f);

    void Compressing(const NestedSegmentContext& ctx, const float& inputDistance) noexcept {
        auto [cascadeCompressDistance, cascadeBaseDistance, cascadeCompressSolidify, priorityList] = MakeCompressCascadeMetrics(inputDistance, ctx);

        for (const auto index : priorityList) {
            auto* childCtx = ctx.GetChildByIndex(index);
            if (childCtx == nullptr) continue;
            auto [remainDist, remainCap, solidify, capacity, validatedMin, greaterBase] =
                MakeCompressSizeMetrics(cascadeCompressDistance, cascadeBaseDistance, cascadeCompressSolidify, *childCtx);
            const float compressBaseDistance = Scaler(remainDist, remainCap, capacity) + solidify;
            const float clampedDist = std::max(compressBaseDistance, validatedMin);

            Sizing(*childCtx, clampedDist);

            cascadeCompressDistance -= clampedDist;
            cascadeCompressSolidify -= solidify;
            cascadeBaseDistance -= greaterBase;
        }
    }

    void Expanding(const NestedSegmentContext& ctx, const float& inputDistance) {
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

    void Sizing(NestedSegmentContext& ctx, const float& value, const float& delta) {
        const auto [validatedInputDistance, processingCompression] = ValidateInputDistance(value, ctx.GetValidatedMin(), ctx.GetAccumulateBase());

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

    void Placing(NestedSegmentContext& ctx, const float& parentOffset = 0.0f) noexcept {
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


    /**
     * Adjusts the placement of segments by sorting them based on their processing order
     * and calculates their respective offsets cumulatively.
     *
     * @param preComputeMetrics A reference to a `PreComputeMetrics` structure containing the segments
     *        to be processed, their configuration details and precomputed metrics such as distance
     *        and order. The `segments` vector is modified in-place, sorting the segments by their
     *        `order` property and updating the `offset` for each segment based on their calculated
     *        position.
     *
     * Sorts the segments in ascending order based on the `order` property and iteratively sets each
     * segment's `offset` as the accumulated value of preceding segment distances. Segments with invalid
     * or null pointers are skipped. Updates the cumulative offset after processing each segment's distance.
     */
    void Placing(PreComputeMetrics& preComputeMetrics) {
        std::ranges::sort(preComputeMetrics.segments, [](const auto& a, const auto& b) {
            return a->order < b->order;
        });

        float currentOffset = 0.0f;
        for (auto* seg : preComputeMetrics.segments) {
            if (!seg) continue;

            seg->offset = currentOffset;

            currentOffset += seg->distance;
        }
    }


    /**
     * Sets the order of a specific segment within the precomputed metrics based on its name.
     *
     * @param preComputeMetrics A reference to a `PreComputeMetrics` structure containing the list
     *        of segments and their associated precomputed attributes.
     * @param name A constant string view representing the name of the segment whose order is to
     *        be updated.
     * @param order An unsigned integer specifying the new order to be assigned to the segment.
     */
    void SetSegmentOrder(PreComputeMetrics& preComputeMetrics, std::string_view name, const size_t order) {
        const auto it = std::ranges::find_if(
            preComputeMetrics.segments, [&](const auto& seg) { return seg->name == name; });
        if (it != preComputeMetrics.segments.end()) (*it)->order = order;
    }


    /**
     * Constructs a `SegmentConfig` object with validated and clamped parameters.
     *
     * @param name A string representing the name of the segment. It is moved into the configuration.
     * @param rawBase A floating-point value representing the base value for the segment.
     *                It is clamped to the range [rawMin, rawMax].
     * @param rawCompressRatio A floating-point value representing the compression ratio.
     *                         It is clamped to be >= 0. Default is 1.0.
     * @param rawExpandRatio A floating-point value representing the expansion ratio.
     *                       It is clamped to be >= 0. Default is 1.0.
     * @param rawMin A floating-point value representing the minimum allowed value for the base.
     *               It is clamped to be >= 0. Default is 0.0.
     * @param rawMax A floating-point value representing the maximum allowed value for the base.
     *               It is clamped to be >= rawMin. Default is the maximum valid float.
     * @param rawOrder An unsigned integer representing the order of the segment. Default is 0.
     * @return A `SegmentConfig` object with all fields properly initialized and values validated
     *         or clamped based on the provided inputs.
     *
     * This function ensures:
     * - Minimum value is non-negative.
     * - Maximum value is at least equal to the minimum.
     * - Base value is clamped between the minimum and maximum.
     * - Compression and expansion ratios are non-negative.
     */
    constexpr Configuration MakeConfig(
        std::string name,
        float rawBase,
        float rawCompressRatio = 1.0f,
        float rawExpandRatio = 1.0f,
        float rawMin = 0.0f,
        float rawMax = std::numeric_limits<float>::max(),
        size_t rawOrder = 0
    ) noexcept {
        Configuration config;

        config.name = std::move(name);
        config.order = rawOrder;

        const float minVal = std::max(0.0f, rawMin);
        const float maxVal = std::max(minVal, rawMax);
        const float baseVal = std::clamp(rawBase, minVal, maxVal);
        const float compressRatio = std::max(0.0f, rawCompressRatio);
        const float expandRatio   = std::max(0.0f, rawExpandRatio);

        config.base = baseVal;
        config.compressRatio = compressRatio;
        config.expandRatio = expandRatio;
        config.min = minVal;
        config.max = maxVal;

        return config;
    }


    /**
     * A utility function that creates and initializes a `SegmentConfig` using raw parameters.
     *
     * @param name A null-terminated string representing the name of the segment.
     * @param rawBase A floating-point value specifying the base value for the segment.
     * @param rawCompressRatio An optional floating-point value representing the compression ratio
     *        applied to the segment. Defaults to 1.0.
     * @param rawExpandRatio An optional floating-point value representing the expansion ratio
     *        applied to the segment. Defaults to 1.0.
     * @param rawMin An optional floating-point value specifying the minimum range for the
     *        segment. Defaults to 0.0.
     * @param rawMax An optional floating-point value specifying the maximum range for the
     *        segment. Defaults to the maximum representable float value.
     * @param rawOrder An optional integer value defining the order or priority of the segment.
     *        Defaults to 0.
     * @return A `SegmentConfig` object constructed using the provided parameters. The function
     *         internally delegates to an overloaded version accepting a `std::string` for the name.
     *
     * This function provides a convenient way to initialize a `SegmentConfig` with lightweight
     * input arguments, ensuring valid parameter conversion and delegation.
     */
    Configuration MakeConfig(
        const char* name,
        float rawBase,
        float rawCompressRatio = 1.0f,
        float rawExpandRatio = 1.0f,
        float rawMin = 0.0f,
        float rawMax = std::numeric_limits<float>::max(),
        size_t rawOrder = 0
    ) noexcept {
        return MakeConfig(std::string(name), rawBase, rawCompressRatio, rawExpandRatio, rawMin, rawMax, rawOrder);
    }
}
