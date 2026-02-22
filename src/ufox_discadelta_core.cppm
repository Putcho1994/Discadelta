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
#ifdef HAS_VULKAN
#include <vulkan/vulkan_raii.hpp>
#endif

export module ufox_discadelta_core;

import ufox_discadelta_lib;

export namespace ufox::geometry::discadelta {

    void Sizing(LinearSegmentContext& ctx, const float& value, const float& delta, const bool& round);
    void Sizing(RectSegmentContext& ctx, const float& width, const float& height, const float& widthDelta, const float& heightDelta, const bool& round);

    /**
     * Selects the greater of two distances.
     *
     * This method compares two floating-point values representing distances
     * and returns the greater of the two. The function is marked as `constexpr`
     * to allow compile-time evaluation, and `[[nodiscard]]` to encourage the caller
     * to handle the returned value.
     *
     * @param a The first distance to compare.
     * @param b The second distance to compare.
     * @return The greater of the two input distances.
     */
    [[nodiscard]] constexpr float ChooseGreaterDistance(const float& a, const float& b) noexcept {
        return std::max(a,b);
    }

    /**
     * Selects the greatest of three distances.
     *
     * This method compares three floating-point values representing distances
     * and returns the greatest of the three. The function is marked as `constexpr`
     * to allow compile-time evaluation, `[[nodiscard]]` to encourage the caller
     * to handle the returned value, and `noexcept` to indicate it will not throw exceptions.
     *
     * @param a The first distance to compare.
     * @param b The second distance to compare.
     * @param c The third distance to compare.
     * @return The greatest of the three input distances.
     */
    [[nodiscard]] constexpr float ChooseGreaterDistance(const float& a, const float& b, const float& c) noexcept {
        return std::max({a,b,c});
    }

    /**
     * Selects the lower of two distances.
     *
     * This method compares two floating-point values representing distances
     * and returns the lower of the two. The function is marked as `constexpr`
     * to allow compile-time evaluation, `[[nodiscard]]` to encourage the caller
     * to handle the returned value, and `noexcept` to indicate it will not throw exceptions.
     *
     * @param a The first distance to compare.
     * @param b The second distance to compare.
     * @return The lower of the two input distances.
     */
    [[nodiscard]] constexpr float ChooseLowestDistance(const float& a, const float& b) noexcept {
        return std::min({a,b});
    }

    /**
     * Selects the lowest of three distances.
     *
     * This method compares three floating-point values representing distances
     * and returns the lowest of the three. The function is marked as `constexpr`
     * to allow compile-time evaluation, `[[nodiscard]]` to encourage the caller
     * to handle the returned value, and `noexcept` to indicate it will not throw exceptions.
     *
     * @param a The first distance to compare.
     * @param b The second distance to compare.
     * @param c The third distance to compare.
     * @return The lowest of the three input distances.
     */
    [[nodiscard]] constexpr float ChooseLowestDistance(const float& a, const float& b, const float& c) noexcept {
        return std::min({a,b,c});
    }

    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    /**
     * Retrieves a list of indices ordered by the `order` property of child elements.
     *
     * This method generates a vector of indices corresponding to the children of the
     * provided context, then sorts these indices based on the `order` property of
     * the child elements in ascending order. The function is marked as `constexpr`
     * to enable compile-time evaluation and `[[nodiscard]]` to encourage the caller
     * to use the resulting value.
     *
     * @param ctx The context containing the children with the `order` property
     *            used for sorting.
     * @return A vector of indices representing the children, sorted by their
     *         `order` property.
     */
    [[nodiscard]] constexpr std::vector<size_t> GetOrderedIndices(const ContextT& ctx) noexcept {
        std::vector<size_t> indices(ctx.children.size());
        std::iota(indices.begin(), indices.end(), size_t{0});

        std::ranges::sort(indices,[ctx](const size_t a, const size_t b) noexcept {return ctx.children[a]->order < ctx.children[b]->order;});

        return indices;
    }

    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    /**
     * Retrieves the child segment context associated with a given name.
     *
     * This method searches for a child context within a parent context using
     * the specified name. If the name is found, it returns a pointer to the
     * corresponding child context; otherwise, it returns `nullptr`.
     *
     * @param parentCtx The parent context containing the child contexts.
     * @param name The name of the child context to retrieve.
     * @return A pointer to the child context if found, or `nullptr` if not.
     */
    [[nodiscard]] constexpr auto GetChildSegmentContext(const ContextT& parentCtx, const std::string& name) noexcept ->ContextT* {
        const auto it = parentCtx.childrenIndies.find(name);
        return it == parentCtx.childrenIndies.end()? nullptr : parentCtx.children[it->second];
    }

    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    /**
     * Retrieves a pointer to the child segment context at a specified index.
     *
     * This method attempts to access the child context from the parent's list of children
     * using the provided index. If the index is valid (within bounds of the children list),
     * the corresponding child context pointer is returned. Otherwise, a `nullptr` is returned.
     * The function is marked as `[[nodiscard]]` to encourage the caller to check the returned pointer.
     *
     * @param parentCtx The context of the parent containing the list of child contexts.
     * @param index The index of the desired child context within the parent's children list.
     * @return A pointer to the child context at the specified index, or `nullptr` if the index is out of bounds.
     */
    [[nodiscard]] constexpr auto GetChildSegmentContext(const ContextT& parentCtx, const size_t index) noexcept ->ContextT*{
        return index < parentCtx.children.size() ? parentCtx.children[index] : nullptr;
    }

    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    /**
     * Validates the parent of the given context.
     *
     * This method checks if the parent of the provided context is valid.
     * A parent is considered valid if it is not null and does not reference
     * the context itself.
     *
     * @param ctx The context whose parent is to be validated.
     * @return True if the context's parent is valid; otherwise, false.
     */
    [[nodiscard]] bool ValidateContextParent(const ContextT& ctx) {
        return ctx.parent != nullptr && ctx.parent != &ctx;
    }

    /**
     * Updates accumulated metrics in the provided LinearSegmentContext object.
     *
     * This method recalculates and updates several accumulated metrics, including
     * base, minimum value, expand ratio, and compress solidify, based on the context's children.
     * It also updates the children indices map with the corresponding names and positions
     * of the children. The method ensures proper handling of empty children by exiting early.
     *
     * @param ctx The LinearSegmentContext holding the metrics and children to update.
     */
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

    /**
     * Updates the accumulated metrics for a given rectangle segment context.
     *
     * This method recalculates various accumulated metrics, such as base dimensions,
     * minimum dimensions, expansion ratios, and compress-solidify values, for a given
     * context object. It iterates through the child segments of the given context and
     * updates these metrics based on the sizes and configurations of the child segments.
     *
     * @param ctx The rectangle segment context whose accumulated metrics are updated.
     */
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
    /**
     * Updates the priority lists for compression and expansion operations in a given context.
     *
     * This method evaluates the priority of compressing and expanding each child element
     * in the context based on available space. It populates the compression and expansion
     * cascade priority lists in order of importance, ensuring that priorities are sorted
     * appropriately for subsequent usage. The method is designed to handle both linear
     * segment contexts and contexts configured with flex directions.
     *
     * @param ctx The context object containing child elements and priority lists.
     *            The context must include configuration details and state-dependent
     *            properties such as children, compressCascadePriorities, and expandCascadePriorities.
     */
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

    /**
     * Validates and adjusts context metrics for a linear segment.
     *
     * This method processes the input context by validating and recalculating
     * various metrics based on configuration values and accumulated data, ensuring
     * they adhere to the specified constraints and relationships. Configured values
     * and calculated intermediate results are used to derive validated minimum,
     * maximum, base values, as well as compression and expansion parameters.
     *
     * @param ctx The context object containing configuration parameters, accumulated
     *            data, and fields to store validated metric outputs.
     */
    constexpr void ValidateContextMetrics(LinearSegmentContext& ctx) noexcept {
        const LinearSegmentCreateInfo& config = ctx.config;

        ctx.validatedMin = ChooseGreaterDistance(0.0f, config.min, ctx.accumulatedMin);
        ctx.validatedMax = ChooseGreaterDistance(0.0f, ctx.validatedMin, config.max);
        ctx.validatedBase = std::clamp(ChooseGreaterDistance(0.0f, config.base, ctx.accumulatedBase), ctx.validatedMin, ctx.validatedMax);
        ctx.compressRatio = ChooseGreaterDistance(0.0f, config.flexCompress);
        ctx.compressCapacity = ctx.validatedBase * ctx.compressRatio;
        ctx.compressSolidify = ChooseGreaterDistance(0.0f, ctx.validatedBase - ctx.compressCapacity);
        ctx.expandRatio = ChooseGreaterDistance(0.0f, config.flexExpand);
    }

    /**
     * Validates and updates the metrics of a rectangular segment context.
     *
     * This method evaluates and adjusts the given context's dimensional properties
     * (e.g., width, height, compress ratio) based on its configuration and accumulated
     * values. It ensures that the calculated values meet specific constraints, such
     * as being within defined minimum and maximum bounds. The method uses calls
     * to utility functions like `ChooseGreaterDistance` and `std::clamp` to perform
     * these validations and ensures robustness by handling edge cases (e.g., zero
     * and negative values).
     *
     * @param ctx The rectangular segment context to validate and update, passed by
     * reference. The context contains configuration data and accumulated metrics,
     * and this method modifies its validated parameters directly.
     */
    constexpr void ValidateContextMetrics(RectSegmentContext& ctx) noexcept {
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
    }


    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    /**
     * Updates various metrics associated with the provided context.
     *
     * This method performs a series of operations to update and validate the
     * metrics of the given context. It also propagates the update to the parent
     * context, if applicable. The function is marked as `noexcept` to indicate
     * that it does not throw exceptions.
     *
     * @param ctx The context object whose metrics need to be updated.
     */
    void UpdateContextMetrics(ContextT& ctx) noexcept{
        UpdateAccumulatedMetrics(ctx);

        ValidateContextMetrics(ctx);

        UpdatePriorityLists(ctx);

        if (ValidateContextParent(ctx)) UpdateContextMetrics(*ctx.parent);
    }

    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    /**
     * Detaches a child context from its parent context.
     *
     * This method removes the specified `child` context from its parent context's
     * list of children. The relationship between the parent and child is severed,
     * and the parent's branch count is adjusted accordingly. If the parent still
     * has remaining children after the operation, its metrics are updated.
     *
     * @param child The context to be unlinked from its parent.
     */
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
    /**
     * Links a child context to a parent context.
     *
     * This method establishes a parent-child relationship between two contexts.
     * If the child is already linked to the specified parent or if the parent and
     * child are the same, the function exits without changes. If the child is linked
     * to a different parent, it is first unlinked from its current parent. After linking,
     * the parent's branch count is updated and context metrics are recalculated.
     *
     * @param parent The context that will become the parent.
     * @param child The context to be linked as a child.
     */
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
    /**
     * Destroys a segment context and unlinks its relationships.
     *
     * This function safely finalizes and deallocates a given segment context.
     * It clears all child relationships, unlinks the context from its parent,
     * and resets any internal structures before deallocating the memory.
     * The function ensures no ownership over children, so they are unlinked
     * but not destroyed.
     *
     * @param ptr A pointer to the segment context to be destroyed. A null pointer
     *            input is safely handled without any operation.
     */
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
    /**
     * Creates a segment context using the provided configuration.
     *
     * This function initializes an instance of `ContextT` with the given configuration,
     * updates its metrics, and sets its `branchCount` to 1. The created context is returned
     * as a `std::unique_ptr` with a custom deleter to ensure proper resource management.
     *
     * @param config The configuration object used to initialize the segment context.
     * @return A `std::unique_ptr` managing the created segment context along with a custom deleter function.
     */
    constexpr auto CreateSegmentContext(const ConfigT& config) noexcept -> std::unique_ptr<ContextT, decltype(&DestroySegmentContext<ContextT>)> {
        auto* ctx = new ContextT{config};
        ctx->branchCount = 1;
        UpdateContextMetrics(*ctx);

        return std::unique_ptr<ContextT, decltype(&DestroySegmentContext<ContextT>)>{ctx,&DestroySegmentContext<ContextT>
        };
    }

    /**
     * Computes size metrics based on a target distance and context.
     *
     * This method validates the input distance in comparison to predefined
     * thresholds in the context, adjusts the base value based on the rounding
     * option, and determines whether the size metric is in compression mode.
     *
     * @param targetDistance The target distance to validate against context thresholds.
     * @param ctx The context containing accumulated and validated distance values.
     * @param round A flag indicating whether to round the accumulated base value.
     * @return A pair containing the validated input distance and a boolean
     *         indicating if the result is in compression mode.
     */
    [[nodiscard]] constexpr std::pair<float, bool> MakeSizeMetrics(const float& targetDistance, const LinearSegmentContext& ctx, const bool& round) noexcept {
        const float validatedInputDistance = ChooseGreaterDistance(ctx.accumulatedMin, ctx.validatedMin, targetDistance);
        const float roundedBase = round? std::lroundf(ctx.accumulatedBase) : ctx.accumulatedBase;
        const bool isCompressionMode = validatedInputDistance < roundedBase;

        return std::make_tuple(
            validatedInputDistance,
            isCompressionMode
        );
    }

    /**
     * Calculates size metrics based on the provided target dimensions, context, and rounding preference.
     *
     * This method computes validated width and height inputs while also determining the orientation
     * (row or column) and whether the input dimensions trigger compression mode. The calculations
     * consider minimum values from the context and optionally round a base value.
     *
     * @param targetWidth The desired target width for the computation.
     * @param targetHeight The desired target height for the computation.
     * @param ctx The layout context containing configuration and accumulated metrics.
     * @param round Indicates whether the accumulated base value should be rounded.
     * @return A tuple containing the validated width input, the validated height input,
     *         a boolean indicating if the direction is row-oriented,
     *         and a boolean indicating if compression mode is active.
     */
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

    /**
     * Computes a scaled value based on input distance, accumulation factor,
     * and scaling factor.
     *
     * This constexpr function calculates a scaled result using the given parameters.
     * If any of the input parameters are less than or equal to zero, the function
     * returns 0.0f to prevent invalid operations. The function is marked `[[nodiscard]]`
     * to ensure proper handling of the returned result by the caller, and `noexcept`
     * to guarantee that it does not throw exceptions.
     *
     * @param distance The base distance value to scale.
     * @param accumulateFactor The accumulation factor used in the scaling.
     * @param factor The scaling multiplier applied to the computation.
     * @return The computed scaled value or 0.0f if any input is non-positive.
     */
    [[nodiscard]] constexpr float Scaler(float distance, float accumulateFactor, float factor) noexcept {
        if (distance <= 0.0f || accumulateFactor <= 0.0f || factor <= 0.0f) {
            return 0.0f;
        }
        return distance <= 0.0f || accumulateFactor <= 0.0f || factor <= 0.0f ? 0.0f : distance / accumulateFactor * factor;
    }

    /**
     * Constructs a tuple containing compression cascade metrics.
     *
     * This method takes an input distance, a linear segment context, and a flag
     * indicating whether values should be rounded. It calculates and returns a
     * tuple consisting of the input distance, the accumulated base (optionally
     * rounded), and the accumulated compress solidify value from the context.
     *
     * @param inputDistance The input distance value.
     * @param ctx The linear segment context containing relevant parameters for the computation.
     * @param round A boolean flag indicating whether the accumulated base should be rounded to the nearest integer.
     * @return A tuple containing the input distance, the (optionally rounded) accumulated base,
     *         and the accumulated compress solidify value.
     */
    [[nodiscard]] constexpr std::tuple<float, float, float>
    MakeCompressCascadeMetrics(const float& inputDistance, const LinearSegmentContext& ctx, const bool& round) noexcept {
        const float accumulatedBase = round? std::lroundf(ctx.accumulatedBase) : ctx.accumulatedBase;

        return std::make_tuple(
            inputDistance,
            accumulatedBase,
            ctx.accumulatedCompressSolidify);
    }

    /**
     * Computes compressed cascade metrics based on input dimensions and context.
     *
     * This function calculates and returns a tuple containing the relevant metrics
     * used for handling compressed cascades. The metrics are derived from input
     * dimensions, a provided context, and additional parameters for controlling
     * the row/column orientation and rounding behavior.
     *
     * @param widthInput The width input value used for computation.
     * @param heightInput The height input value used for computation.
     * @param ctx A reference to the RectSegmentContext containing accumulated metrics.
     * @param isRow A boolean flag indicating whether the context applies to rows (true) or columns (false).
     * @param round A boolean flag indicating whether to round values during computation.
     * @return A tuple containing:
     *         - The selected input value (width or height based on isRow).
     *         - The accumulated base value, optionally rounded if round is true.
     *         - The accumulated compress solidification metric from the context.
     */
    [[nodiscard]] constexpr std::tuple<float,float,float>
    MakeCompressCascadeMetrics(const float& widthInput, const float& heightInput, const RectSegmentContext& ctx, const bool& isRow, const bool& round ) noexcept {
        const float& value = isRow ? widthInput : heightInput;
        const float& directionAccumulatedBase = isRow ? ctx.accumulatedWidthBase : ctx.accumulatedHeightBase;
        const float accumulatedBase = round? std::lroundf(directionAccumulatedBase) : directionAccumulatedBase;

        return std::make_tuple(
            value,
            accumulatedBase,
            ctx.accumulatedCompressSolidify);
    }

    /**
     * Calculates compression size metrics for cascaded segments.
     *
     * This method computes and returns a tuple containing adjusted values
     * for compression distance, base distance, and an optionally rounded
     * base value, based on provided parameters and context.
     *
     * @param cascadeCompressDistance The initial compression distance for the cascade.
     * @param cascadeBaseDistance The base distance for the cascade before adjustments.
     * @param cascadeCompressSolidify The solidification factor applied to compression adjustments.
     * @param ctx A LinearSegmentContext object containing contextual information, including the validated base.
     * @param round A flag indicating whether the validated base should be rounded to the nearest integer.
     * @return A tuple containing the adjusted compression distance, adjusted base distance, and the (optionally rounded) base value.
     */
    [[nodiscard]] constexpr std::tuple<const float, const float, const float>
    MakeCompressSizeMetrics(const float& cascadeCompressDistance, const float& cascadeBaseDistance, const float& cascadeCompressSolidify, const LinearSegmentContext& ctx, const bool& round) noexcept {
        const float childEffectiveBase = ctx.validatedBase;
        const float roundedBase = round? std::lroundf(childEffectiveBase) : childEffectiveBase;

        return std::make_tuple(cascadeCompressDistance - cascadeCompressSolidify, cascadeBaseDistance - cascadeCompressSolidify, roundedBase);
    }

    /**
     * Computes compression size metrics for a given cascade context.
     *
     * This function calculates and returns a tuple representing three values:
     * the adjusted cascade compression distance, the adjusted base distance,
     * and a rounded or unrounded effective base value depending on the parameters.
     *
     * @param cascadeCompressDistance The distance for compression to be adjusted.
     * @param cascadeBaseDistance The base distance for the compression to be adjusted.
     * @param cascadeCompressSolidify The amount by which to reduce compression.
     * @param ctx The context containing validated base dimensions for width and height.
     * @param isRow A flag indicating whether to compute based on row (true) or column (false).
     * @param round A flag indicating whether to round the effective base value.
     * @return A tuple containing the adjusted compression distance, adjusted base distance,
     *         and the final base value (rounded or unrounded based on the input).
     */
    [[nodiscard]] constexpr std::tuple<const float,const float,const float>
    MakeCompressSizeMetrics(const float& cascadeCompressDistance, const float&cascadeBaseDistance, const float& cascadeCompressSolidify, const RectSegmentContext& ctx, const bool& isRow, const bool& round) noexcept {
        const float childEffectiveBase = isRow ?ctx.validatedWidthBase : ctx.validatedHeightBase;
        const float roundedBase = round? std::lroundf(childEffectiveBase) : childEffectiveBase;

        return std::make_tuple(cascadeCompressDistance - cascadeCompressSolidify, cascadeBaseDistance - cascadeCompressSolidify, roundedBase);
    }

    /**
     * Computes metrics related to cascade expansion.
     *
     * This method calculates the cascade expansion delta and determines
     * if expansion is required. It also includes the accumulated expand
     * ratio from the provided context. The evaluation respects rounding
     * preferences for the base accumulation value.
     *
     * @param inputDistance The distance to evaluate for cascade expansion.
     * @param ctx The context object containing the necessary parameters,
     *            including accumulated base and expand ratio.
     * @param round A flag indicating whether the accumulated base value
     *              should be rounded to the nearest integer.
     * @return A tuple containing the following:
     *         - A boolean indicating if the cascade expansion delta is positive.
     *         - The evaluated cascade expansion delta.
     *         - The accumulated expand ratio obtained from the context.
     */
    [[nodiscard]] constexpr std::tuple<bool, float, float>
    MakeExpandCascadeMetrics(const float& inputDistance, const LinearSegmentContext& ctx, const bool& round) noexcept {
        const float accumulatedBase = round? std::lroundf(ctx.accumulatedBase) : ctx.accumulatedBase;
        const float cascadeExpandDelta = std::max(inputDistance - accumulatedBase, 0.0f);

        return std::make_tuple(
            cascadeExpandDelta > 0.0f,
            cascadeExpandDelta,
            ctx.accumulatedExpandRatio);
    }

    /**
     * Computes metrics for expanding cascading layouts.
     *
     * This method calculates metrics for determining if a layout needs expansion
     * in a cascading fashion based on input dimensions, context, orientation, and rounding preferences.
     * The metrics include a boolean indicating whether expansion occurs, the expansion delta,
     * and the accumulated expand ratio from the context.
     *
     * @param widthInput The provided width dimension for the layout.
     * @param heightInput The provided height dimension for the layout.
     * @param ctx The context containing accumulated dimensions and expand ratio data.
     * @param isRow A boolean indicating whether the layout is row-oriented (true) or column-oriented (false).
     * @param round A boolean indicating whether rounding should be applied to the accumulated base dimensions.
     * @return A tuple containing a boolean (indicating expansion state), the computed cascade expansion delta,
     *         and the accumulated expand ratio from the context.
     */
    [[nodiscard]] constexpr std::tuple<const bool,float,float>
    MakeExpandCascadeMetrics(const float& widthInput, const float& heightInput, const RectSegmentContext& ctx, const bool& isRow, const bool& round) {
        const float& value = isRow ? widthInput : heightInput;
        const float& directionAccumulatedBase = isRow? ctx.accumulatedWidthBase: ctx.accumulatedHeightBase;
        const float accumulatedBase = round? std::lroundf(directionAccumulatedBase) : directionAccumulatedBase;
        const float cascadeExpandDelta = std::max(value - accumulatedBase, 0.0f);

        return std::make_tuple(cascadeExpandDelta > 0.0f,cascadeExpandDelta, ctx.accumulatedExpandRatio);
    }

    /**
     * Computes size metrics for expansion based on the provided context and rounding option.
     *
     * This method calculates and returns a tuple containing the adjusted base value
     * and the expand capacity derived from the given context. The calculation takes
     * into account whether the base value should be rounded based on the input flag.
     *
     * @param ctx The context containing the validated base and maximum values.
     * @param round A flag indicating whether to round the base value.
     * @return A tuple where the first element is the adjusted base value and the
     *         second element is the computed expand capacity.
     */
    [[nodiscard]] constexpr std::tuple<const float,const float>
    MakeExpandSizeMetrics(const LinearSegmentContext& ctx, const bool& round) noexcept {
        const float childEffectiveBase = ctx.validatedBase;
        const float roundedBase = round? std::lroundf(childEffectiveBase) : childEffectiveBase;
        const float childMax = ctx.validatedMax;
        const float expandCapacity = std::max(0.0f, childMax - roundedBase);
        return std::make_tuple(roundedBase, expandCapacity);
    }

    /**
     * Computes size metrics for expansion based on input context and configuration.
     *
     * This method calculates metrics related to size expansion, taking into account
     * whether the operation is row-based or column-based, and whether rounding should
     * be applied. The returned tuple contains the adjusted base size and the calculated
     * expansion capacity.
     *
     * @param ctx The context containing size validation parameters.
     * @param isRow A boolean indicating whether the operation is row-based (true for row, false for column).
     * @param round A boolean indicating whether the base size should be rounded to the nearest integer.
     * @return A tuple where the first value is the adjusted base size and the second value is the expansion capacity.
     */
    [[nodiscard]] constexpr std::tuple<const float, const float>
    MakeExpandSizeMetrics(const RectSegmentContext& ctx, const bool& isRow, const bool& round) {
        const float& directionBase = isRow ?ctx.validatedWidthBase :ctx.validatedHeightBase;
        const float roundedBase = round? std::lroundf(directionBase) : directionBase;
        const float& directionMax = isRow ? ctx.validatedWidthMax : ctx.validatedHeightMax;

        const float expandCapacity = std::max(0.0f, directionMax - roundedBase);

        return std::make_tuple(roundedBase, expandCapacity);
    }

    /**
     * Calculates the cross-axis size based on input values and context parameters.
     *
     * This method determines the appropriate size along the cross-axis by evaluating
     * various constraints such as validated base size, minimum size, and maximum size,
     * and then selecting the optimal value that satisfies these constraints.
     *
     * @param ctx The context containing parameters for size validation along the axes.
     * @param mainInput The primary input size, used if the orientation is column-based.
     * @param crossInput The alternate input size, used if the orientation is row-based.
     * @param isRow A boolean indicating whether the calculation is for a row layout.
     * @return The computed cross-axis size after considering constraints.
     */
    constexpr float ComputeCrossSize(const RectSegmentContext& ctx,const float mainInput,const float crossInput,const bool isRow) noexcept {
        const float& input = isRow ? crossInput : mainInput;
        const float& crossValidatedBase = isRow ? ctx.validatedHeightBase : ctx.validatedWidthBase;
        const float& crossMin = isRow ? ctx.validatedHeightMin : ctx.validatedWidthMin;
        const float& crossMax = isRow ? ctx.validatedHeightMax : ctx.validatedWidthMax;
        const float lowestCrossDistance = ChooseLowestDistance(crossMax, input, crossValidatedBase);

        return ChooseGreaterDistance(0.0f, lowestCrossDistance, crossMin);
    }

    /**
     * Executes a compressing operation for a given linear segment context.
     *
     * This method calculates cascade compression metrics and iteratively adjusts
     * the properties of child segment contexts based on the input parameters and
     * compression priorities. It ensures that child contexts are properly sized
     * while accounting for compression-specific constraints such as solidification
     * and capacity bounds.
     *
     * @param ctx The linear segment context representing the configuration for the compression operation.
     * @param inputDistance The distance value used to initialize the compression calculations.
     * @param round A boolean flag indicating whether distances should be rounded during computation.
     */
    void Compressing(const LinearSegmentContext& ctx, const float& inputDistance, const bool& round) noexcept {
        auto [cascadeCompressDistance, cascadeBaseDistance, cascadeCompressSolidify] = MakeCompressCascadeMetrics(inputDistance, ctx, round);

        for (const auto index : ctx.compressCascadePriorities) {
            auto* childCtx = GetChildSegmentContext<LinearSegmentContext>(ctx,index);
            if (childCtx == nullptr) continue;
            auto [remainDist, remainCap, greaterBase] = MakeCompressSizeMetrics(cascadeCompressDistance, cascadeBaseDistance, cascadeCompressSolidify, *childCtx, round);
            const float& solidify = childCtx->compressSolidify;
            const float& capacity = childCtx->compressCapacity;
            const float& validatedMin = childCtx->validatedMin;
            const float compressBaseDistance = Scaler(remainDist, remainCap, capacity) + solidify;
            const float clampedDist = ChooseGreaterDistance(compressBaseDistance, validatedMin);
            const float roundedDist = round? std::lroundf(clampedDist) : clampedDist;

            Sizing(*childCtx, roundedDist, 0.0f, round);

            cascadeCompressDistance -= roundedDist;
            cascadeCompressSolidify -= solidify;
            cascadeBaseDistance -= greaterBase;
        }
    }

    /**
     * Computes and applies compression metrics for child segments within a rectangular segment context.
     *
     * This method calculates compression distances and sizes for the children of a given rectangular
     * segment context based on cascade priorities and applies sizing adjustments accordingly.
     * It processes each child context using provided main and cross directional input values,
     * optionally rounding the computed distances.
     *
     * @param ctx The parent rectangular segment context containing the child segments to process.
     * @param mainInput The primary input value used for computing compression distances.
     * @param crossInput The secondary input value used for cross-wise computations.
     * @param isRow A flag indicating whether the compression is performed row-wise (true) or column-wise (false).
     * @param round A flag indicating whether computed distances should be rounded to the nearest integer.
     */
    void Compressing(const RectSegmentContext& ctx, const float& mainInput, const float& crossInput, const bool& isRow, const bool& round) noexcept {
        auto [cascadeCompressDistance, cascadeBaseDistance, cascadeCompressSolidify] = MakeCompressCascadeMetrics(mainInput, crossInput, ctx, isRow, round);

        for (const auto index : ctx.compressCascadePriorities) {
            auto* childCtx = GetChildSegmentContext(ctx,index);
            if (childCtx == nullptr) continue;
            auto [remainDist, remainCap, validatedBase] = MakeCompressSizeMetrics(cascadeCompressDistance, cascadeBaseDistance, cascadeCompressSolidify, *childCtx, isRow, round);
            const float& solidify = isRow ? ctx.widthCompressSolidify : ctx.heightCompressSolidify;
            const float& capacity = isRow ? ctx.widthCompressCapacity : ctx.heightCompressCapacity;
            const float& validatedMin = isRow ? ctx.validatedWidthMin : ctx.validatedHeightMin;
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

    /**
     * Performs the expansion of linear segments within a hierarchical structure.
     *
     * This method applies an expansion process to child segments based on the
     * given input distance and contextual properties from the parent segment.
     * The expansion adjusts the sizes of child segments iteratively, considering
     * the maximum allowable delta and applying optional rounding to the values.
     *
     * @param ctx The context of the parent linear segment that contains
     *            properties and priorities for cascading the expansion.
     * @param inputDistance The initial distance that drives the expansion process.
     * @param round A boolean flag indicating whether the expansion delta values
     *              should be rounded to the nearest integer.
     */
    void Expanding(const LinearSegmentContext& ctx, const float& inputDistance, const bool& round) {
        auto [processingExpansion,cascadeExpandDelta, cascadeExpandRatio] = MakeExpandCascadeMetrics(inputDistance, ctx, round);
        if (!processingExpansion) return;

        for (const auto index : ctx.expandCascadePriorities) {
            auto* childCtx = GetChildSegmentContext(ctx,index);
            if (childCtx == nullptr) continue;

            auto [validateBase, maxDelta] = MakeExpandSizeMetrics(*childCtx, round);
            const float& expandRatio = childCtx->expandRatio;
            const float expandDelta = Scaler(cascadeExpandDelta, cascadeExpandRatio, expandRatio);
            const float clampedDelta = ChooseLowestDistance(expandDelta, maxDelta);
            const float roundedDelta = round? std::lroundf(clampedDelta) : clampedDelta;

            Sizing(*childCtx, validateBase, roundedDelta, round);

            cascadeExpandDelta -= roundedDelta;
            cascadeExpandRatio -= expandRatio;
        }
    }

    /**
     * Manages the expansion process for a rectangular segment context.
     *
     * This function applies a cascading expansion algorithm on a rectangular
     * segment context, distributing delta adjustments across child segments
     * based on their priorities, expand ratios, and size metrics. The expansion
     * process considers whether the operation is row-oriented and optionally
     * rounds the computed results.
     *
     * @param ctx The context of the rectangular segment to be expanded, containing
     *            all relevant data including child segments and priority information.
     * @param mainInput The primary size input (e.g., width or height) used in the expansion calculations.
     * @param crossInput The secondary size input (e.g., cross-axis dimension) used in the expansion calculations.
     * @param isRow Indicates whether the expansion is row-oriented (true for horizontal expansion,
     *              false for vertical expansion).
     * @param round Specifies whether computed floating-point values should be rounded to the nearest integer.
     */
    void Expanding(const RectSegmentContext& ctx, const float& mainInput, const float& crossInput, const bool& isRow, const bool& round) {
        auto [processingExpansion,cascadeExpandDelta, cascadeExpandRatio] = MakeExpandCascadeMetrics(mainInput, crossInput, ctx, isRow, round);

        if (!processingExpansion) return;

        for (const auto index : ctx.expandCascadePriorities) {
            auto* childCtx = GetChildSegmentContext(ctx,index);
            if (childCtx == nullptr) continue;

            auto [validatedBase, maxDelta] = MakeExpandSizeMetrics(*childCtx, isRow, round);
            const float& expandRatio = childCtx->expandRatio;
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

    /**
     * Adjusts the size metrics and applies either compression or expansion logic.
     *
     * This method calculates the size metrics based on the input value, delta, and
     * rounding preference. It updates the context's base distance, delta, and total
     * distance. Based on the processed compression status, it applies either
     * compression or expansion to the distance.
     *
     * @param ctx The context representing the linear segment to be adjusted.
     * @param value The input value used to compute the base distance.
     * @param delta The incremental change for the distance.
     * @param round A flag indicating whether rounding should be applied during processing.
     */
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

    /**
     * Adjusts the size and layout of a rectangular segment.
     *
     * This method calculates the final width and height of a rectangular
     * segment based on input parameters and applies either compression
     * or expansion logic depending on the configuration and metrics.
     *
     * @param ctx The context containing the properties and state of the rectangular segment.
     * @param width The initial width of the rectangular segment.
     * @param height The initial height of the rectangular segment.
     * @param widthDelta The additional width to expand or adjust the segment.
     * @param heightDelta The additional height to expand or adjust the segment.
     * @param round Indicates whether dimensions should be rounded during processing.
     */
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

    /**
     * Recursively updates the positional offset of a linear segment and its nested child segments.
     *
     * This function adjusts the `offset` property for the given context and its child contexts
     * based on their hierarchical structure and distances. It starts from a given `parentOffset`
     * and propagates updated offsets to all child segments. If no children exist, the function terminates early.
     *
     * The function ensures that child segments are ordered and processed sequentially, using
     * the ordering determined by `GetOrderedIndices`. For each child, it recursively applies
     * the same offset adjustment, taking into account the distance between segments.
     *
     * @param ctx A reference to the primary `LinearSegmentContext` whose positional offsets
     *            (and its children's offsets) need to be updated.
     * @param parentOffset An optional starting offset for the current segment. Defaults to 0.0f
     *                     if not specified.
     */
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

    /**
     * Places a rectangular segment and its children within a layout context.
     *
     * This method calculates and assigns the position of the given layout context
     * and its child segments based on the specified relative coordinates and layout
     * configuration. Child segments are arranged sequentially either in a row or
     * column, depending on the flex direction specified in the context configuration.
     *
     * @param ctx The rectangular segment's context containing the position, size,
     *            children, and layout configuration.
     * @param relativeX The x-coordinate offset relative to the parent context. Defaults to 0.0f.
     * @param relativeY The y-coordinate offset relative to the parent context. Defaults to 0.0f.
     */
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

    /**
     * Updates the layout of linear segments based on the input parameters.
     *
     * This method adjusts the sizing and placement of segments within a given
     * linear segment context. It utilizes the input distance, a rounding flag,
     * and a fixed start value to perform necessary updates.
     *
     * @param rootCtx The linear segment context to update.
     * @param inputDistance The distance value used to determine segment adjustments.
     * @param round A boolean flag indicating whether rounding should be applied.
     */
    void UpdateSegments(LinearSegmentContext& rootCtx, const float inputDistance, const bool round) {
        Sizing(rootCtx, inputDistance, 0.0f, round);
        Placing(rootCtx);
    }

    /**
     * Updates the segments based on the provided context and input parameters.
     *
     * This method adjusts the sizing and placement parameters of the segments
     * within the given RectSegmentContext. The inputs specify the primary and
     * secondary dimensions, along with an option to apply rounding during the update.
     *
     * @param rootCtx The context containing the root segment to be updated.
     * @param mainInput The primary input value influencing the update.
     * @param crossInput The secondary input value influencing the update.
     * @param round A boolean indicating whether rounding should be applied.
     */
    void UpdateSegments(RectSegmentContext& rootCtx, const float mainInput, const float crossInput, const bool round ) {
        Sizing(rootCtx, mainInput, crossInput, 0.0f,0.0f, round);
        Placing(rootCtx);
    }

#ifdef HAS_VULKAN
    /**
     * Converts a RectSegmentContext object to a Vulkan-compatible vk::Rect2D structure.
     *
     * This function takes a RectSegmentContext containing rectangle dimensions and position
     * and constructs a vk::Rect2D structure with Vulkan-specific types and constraints.
     * The conversion ensures proper casting between signed and unsigned integer types.
     *
     * @param ctx The RectSegmentContext containing the rectangle's position and size.
     * @return A Vulkan-compatible vk::Rect2D structure reflecting the input rectangle's properties.
     */
    constexpr vk::Rect2D MakeVKRect2D(const RectSegmentContext& ctx) noexcept {
        return vk::Rect2D{{static_cast<int32_t>(ctx.content.x), static_cast<int32_t>(ctx.content.y)}, {static_cast<uint32_t>(ctx.content.width), static_cast<uint32_t>(ctx.content.height)}};
    }
#endif

}
