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

export module ufox_discadelta_core;

import ufox_discadelta_lib;

export namespace ufox::geometry::discadelta {
    /**
     * A constexpr lambda function that generates and initializes the context for Discadelta processing.
     *
     * @param configs A vector of `DiscadeltaSegmentConfig` structures, each containing parameters
     *        for a segment such as name, base value, compression and expansion ratios,
     *        min/max distances, and order.
     * @param inputDistance A floating-point value representing the input distance used
     *        for processing. It is validated to be non-negative.
     * @return A tuple consisting of:
     *         - `DiscadeltaSegmentsHandler`: A container holding the initialized segments.
     *         - `DiscadeltaPreComputeMetrics`: Precomputed metrics for Discadelta processing,
     *           aggregating information such as compression capacities, base distances,
     *           and priority indices.
     *         - `bool`: A flag indicating whether the processing involves compression
     *           (true) or expansion (false) based on the input distance.
     *
     * This function initializes the segments with validated and clamped properties based on
     * the provided configuration. It calculates various pre-computation metrics, such as
     * compression capacity, base values, expand ratios, and priority indices for all segments.
     */
    constexpr auto MakeContext = [](const std::vector<SegmentConfig>& configs, const float inputDistance) -> std::tuple<SegmentsPtrHandler, PreComputeMetrics, bool>{
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
     * A constexpr function to compute a scaled distance value based on provided factors.
     *
     * @param distance The input distance value to scale. It is validated to be greater than zero.
     * @param accumulateFactor A scaling factor used to normalize the input distance. It is validated to be greater than zero.
     * @param factor A scaling multiplier to apply after normalization. It is validated to be greater than zero.
     * @return The scaled distance value, calculated as `(distance / accumulateFactor * factor)`.
     *         Returns `0.0f` if any of the input parameters is less than or equal to zero.
     *
     * This function ensures that invalid input parameters, such as non-positive values, result in a safe output of `0.0f`.
     */
    constexpr float Scaler(const float& distance, const float& accumulateFactor, const float& factor) {
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
     *        constraints (e.g., min distances, base distances, and compress capacities).
     *
     * This method ensures that the segments' base and distance parameters are recalculated based
     * on the remaining distances and capacities in a cascading manner. It also enforces segment
     * constraints such as the minimum allowed distances, and updates cascading parameters
     * post-segment adjustment to maintain consistency across subsequent computations.
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
     *        maximum distances, and segment references.
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

    /**
     * A constexpr function that arranges and adjusts segment offsets in a specified order
     * within the provided pre-computed metrics for Discadelta processing.
     *
     * @param preComputeMetrics A reference to a `DiscadeltaPreComputeMetrics` structure
     *        containing the segments to be sorted and updated. Each segment has properties
     *        such as order, distance, and offset, which are recalculated during the function.
     *
     * This function sorts all segments within `preComputeMetrics` based on their `order` value
     * in ascending order. After sorting, it calculates and assigns the offset for each segment,
     * iteratively accumulating their distances to determine the next offset.
     * Null pointers within the segments are safely skipped during processing.
     */
    constexpr void Placing(PreComputeMetrics& preComputeMetrics) {
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
     * Updates the order of a specific segment within the pre-computed metrics.
     *
     * @param preComputeMetrics A reference to `DiscadeltaPreComputeMetrics` that contains
     *        the collection of segments with pre-computed data for Discadelta processing.
     * @param name A string view representing the name of the segment to locate and update.
     * @param order A size_t value specifying the new order to assign to the target segment.
     *
     * The function searches for a segment by its name within the preComputeMetrics. If the
     * specified segment is found, its order is updated to the provided value.
     */
    constexpr void SetSegmentOrder(PreComputeMetrics& preComputeMetrics, std::string_view name, const size_t order) {
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
    constexpr SegmentConfig MakeConfig(
        std::string name,
        float rawBase,
        float rawCompressRatio = 1.0f,
        float rawExpandRatio = 1.0f,
        float rawMin = 0.0f,
        float rawMax = std::numeric_limits<float>::max(),
        size_t rawOrder = 0
    ) noexcept {
        SegmentConfig config;

        config.name = std::move(name);
        config.order = rawOrder;

        // 1. Min must be >= 0
        const float minVal = std::max(0.0f, rawMin);

        // 2. Max must be >= min
        const float maxVal = std::max(minVal, rawMax);

        // 3. Base clamped to [min, max]
        const float baseVal = std::clamp(rawBase, minVal, maxVal);

        // 4. Ratios clamped to >= 0
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
    SegmentConfig MakeConfig(
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
