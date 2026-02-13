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
#include <iostream>
#include <numeric>

export module ufox_discadelta_core;

import ufox_discadelta_lib;

export namespace ufox::geometry::discadelta {

    void Sizing(LinearSegmentContext& ctx, const float& value, const float& delta, const bool& round);
    void Sizing(RectSegmentContext& ctx, const float& width, const float& height, const float& widthDelta, const float& heightDelta, const bool& round);

    /**
     * Returns the greater of two float values.
     *
     * This function compares two float inputs and returns the value that is greater.
     * The function is marked as `[[nodiscard]]` to indicate that the return value should not
     * be ignored. It is also `constexpr`, allowing it to be evaluated at compile time when possible.
     *
     * @param a The first float value to compare.
     * @param b The second float value to compare.
     * @return The greater of the two float values.
     * @throws None. The function does not throw exceptions.
     */
    [[nodiscard]] constexpr float ChooseGreaterDistance(const float& a, const float& b) noexcept {
        return std::max(a,b);
    }

    /**
     * Returns the greatest of three float values.
     *
     * This function compares three float inputs and returns the value that is the greatest.
     * The function is marked as `[[nodiscard]]` to indicate that the return value should not be ignored.
     * It is also `constexpr`, allowing it to be evaluated at compile time when possible.
     *
     * @param a The first float value to compare.
     * @param b The second float value to compare.
     * @param c The third float value to compare.
     * @return The greatest of the three float values.
     * @throws None. The function does not throw exceptions.
     */
    [[nodiscard]] constexpr float ChooseGreaterDistance(const float& a, const float& b, const float& c) noexcept {
        return std::max({a,b,c});
    }

    /**
     * Reads the numeric value of a Length instance, with an optional fallback for 'auto' values.
     *
     * This function evaluates a `Length` object. If the `Length` type is `LengthUnitType::Auto`,
     * the supplied fallback value `autoFallback` is returned. Otherwise, the numeric value of the `Length`
     * is returned. It is marked as `[[nodiscard]]` to ensure the return value is not ignored and is
     * `constexpr` to allow compile-time evaluation when possible.
     *
     * @param length The `Length` object containing a value and its unit type.
     * @param autoFallback A default value returned if the length type is `LengthUnitType::Auto`. Defaults to 0.0f.
     * @return The numeric value of the `Length`, or the fallback value if it is of type `Auto`.
     * @throws None. The function does not throw exceptions.
     */
    [[nodiscard]] constexpr float ReadLengthValue(const Length& length, const float autoFallback = 0.0f) noexcept {
        return length.type == LengthUnitType::Auto? autoFallback : length.value;
    }


    /**
     * Retrieves a vector of indices ordered based on the `order` field of the children in the given context.
     *
     * This function generates a vector of indices corresponding to the children in the provided context
     * and sorts them according to the `order` attribute of the children. It is marked as `[[nodiscard]]`
     * to indicate that the return value should not be ignored and is `constexpr`, enabling potential
     * compile-time evaluation.
     *
     * @tparam ContextT The type of the context, which must be either `LinearSegmentContext` or `RectSegmentContext`.
     * @param ctx The context containing the children whose indices need to be ordered.
     * @return A vector of indices, sorted based on the `order` field of the context's children.
     * @throws None. The function does not throw exceptions.
     */
    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    [[nodiscard]] constexpr std::vector<size_t> GetOrderedIndices(const ContextT& ctx) noexcept {
        std::vector<size_t> indices(ctx.children.size());
        std::iota(indices.begin(), indices.end(), size_t{0});

        std::ranges::sort(indices,[ctx](const size_t a, const size_t b) noexcept {return ctx.children[a]->order < ctx.children[b]->order;});

        return indices;
    }

    /**
     * Retrieves a child segment context by name from the parent context.
     *
     * This function searches for a child context within the provided parent context using
     * the specified name. If found, it returns a pointer to the child context; otherwise,
     * it returns nullptr. The function is marked as `[[nodiscard]]` to emphasize that the
     * return value should not be ignored and is declared `constexpr` for potential compile-time evaluation.
     *
     * @param parentCtx The parent context containing child segments.
     * @param name The name of the child segment to retrieve.
     * @return A pointer to the child context if it exists, nullptr otherwise.
     * @throws None. The function does not throw exceptions.
     */
    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    [[nodiscard]] constexpr auto GetChildSegmentContext(const ContextT& parentCtx, const std::string& name) noexcept ->ContextT* {
        const auto it = parentCtx.childrenIndies.find(name);
        return it == parentCtx.childrenIndies.end()? nullptr : parentCtx.children[it->second];
    }

    /**
     * Retrieves a pointer to a child context from the parent context at the specified index.
     *
     * This function checks if the given index is within the bounds of the parent context's
     * children container. If the index is valid, it returns a pointer to the child context
     * at that index. Otherwise, it returns a nullptr. The function is marked as `[[nodiscard]]`,
     * indicating that the return value should not be ignored, and `constexpr`, allowing it to
     * be evaluated at compile time when possible.
     *
     * @tparam ContextT The type of context being used.
     * @param parentCtx A reference to the parent context from which the child context is to be retrieved.
     * @param index The index of the child context to retrieve.
     * @return A pointer to the child context at the specified index, or nullptr if the index is out of bounds.
     * @throws None. The function does not throw exceptions.
     */
    template<typename ContextT>
    requires std::same_as<ContextT, LinearSegmentContext> || std::same_as<ContextT, RectSegmentContext>
    [[nodiscard]] constexpr auto GetChildSegmentContext(const ContextT& parentCtx, const size_t index) noexcept ->ContextT*{
        return index < parentCtx.children.size() ? parentCtx.children[index] : nullptr;
    }


    /**
     * Updates the accumulated metrics of a `LinearSegmentContext` object.
     *
     * This function recalculates and updates several accumulated values in the given context object
     * based on its children elements. These metrics include base, minimum, expand ratio,
     * and compress solidify values. The indices of the children are also cleared and repopulated.
     *
     * The function ensures that if the children list is empty, no processing occurs. If children are present,
     * it iterates through each child and calculates cumulative metrics while associating indices with child names.
     * Specific computations involve comparing distances and handling various attributes of child configurations.
     *
     * @param ctx A reference to the `LinearSegmentContext` object whose metrics are to be updated.
     * @return None.
     * @throws None. The function does not throw exceptions.
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
            ctx.accumulatedBase += child->config.base.type == LengthUnitType::Auto? child->accumulatedBase: child->validatedBase;
            float childMin = ChooseGreaterDistance(child->validatedMin, child->compressSolidify);
            ctx.accumulatedMin += ChooseGreaterDistance(child->accumulatedMin, childMin);
            ctx.accumulatedCompressSolidify += child->compressSolidify;
            ctx.accumulatedExpandRatio += child->expandRatio;
        }
    }


    /**
     * Updates the accumulated metrics for a given `RectSegmentContext`.
     *
     * This function clears and recalculates the accumulated metrics of a `RectSegmentContext` object.
     * It iterates through the children of the context, checks their properties, and updates
     * multiple accumulated values such as base dimensions, minimum dimensions, expand ratios, and
     * compress solidify factors. The updates depend on the direction of the layout (row or column)
     * specified in the context's configuration.
     *
     * @param ctx Reference to the `RectSegmentContext` object whose accumulated metrics will be updated.
     * @throws None. The function is declared as `noexcept` and guarantees no exceptions will be thrown.
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
                ctx.accumulatedWidthBase += child->config.width.type == LengthUnitType::Auto? child->accumulatedWidthBase : child->validatedWidthBase;
                ctx.accumulatedHeightBase = ChooseGreaterDistance(ctx.accumulatedHeightBase, child->config.height.type == LengthUnitType::Auto? child->accumulatedHeightBase : child->validatedHeightBase);
                compressSolidify = child->widthCompressSolidify;
                const float& childWidthMin = ChooseGreaterDistance(child->validatedWidthMin, compressSolidify);
                const float& childHeightMin = ChooseGreaterDistance(child->validatedHeightMin, child->accumulatedHeightMin);
                ctx.accumulatedWidthMin += ChooseGreaterDistance(childWidthMin, child->accumulatedWidthMin);
                ctx.accumulatedHeightMin = ChooseGreaterDistance(ctx.accumulatedHeightMin, childHeightMin);
            }
            else {
                ctx.accumulatedWidthBase = ChooseGreaterDistance(ctx.accumulatedWidthBase, child->config.width.type == LengthUnitType::Auto? child->accumulatedWidthBase : child->validatedWidthBase);
                ctx.accumulatedHeightBase += child->config.height.type == LengthUnitType::Auto? child->accumulatedHeightBase : child->validatedHeightBase;
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

    /**
     * Updates the compression and expansion priority lists in the given context.
     *
     * This function clears existing compression and expansion priority lists in the
     * provided context and recalculates them based on the child elements' defined constraints
     * (e.g., size, min, max). It evaluates the "room" available for compression and expansion
     * for each child and orders the priorities accordingly for compression (ascending order)
     * and expansion (descending order).
     *
     * @tparam ContextT The type of the context, which determines specific handling logic.
     * @param ctx Reference to the context object containing child elements, priorities,
     *            and configuration settings.
     * @return None. The updates are applied directly to the existing context object.
     * @throws None. The function is marked as `noexcept`.
     */
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

    /**
     * Updates the metrics and derived values in a given linear segment context.
     *
     * This function performs multiple operations to validate, calculate,
     * and update the metrics of the provided linear segment context. It
     * processes compression and expansion ratios, and ensures derived values
     * remain within valid boundaries. If the context has a parent, the metrics
     * for the parent context are also updated recursively. The function is
     * marked as `noexcept`, indicating that it does not throw exceptions.
     *
     * @param ctx The linear segment context to update. The context contains
     *            configuration values and accumulated metrics that are modified
     *            by this function.
     */
    void UpdateContextMetrics(LinearSegmentContext& ctx) noexcept{
        UpdateAccumulatedMetrics(ctx);

        const LinearSegmentCreateInfo& config = ctx.config;

        ctx.validatedMin = ChooseGreaterDistance(0.0f, config.min);
        ctx.validatedMax = ChooseGreaterDistance(ctx.validatedMin, config.max);
        ctx.validatedBase = std::clamp(ReadLengthValue(config.base, ctx.accumulatedBase), ctx.validatedMin, ctx.validatedMax);
        ctx.compressRatio = ChooseGreaterDistance(0.0f, config.flexCompress);
        ctx.compressCapacity = ctx.validatedBase * ctx.compressRatio;
        ctx.compressSolidify = ChooseGreaterDistance(0.0f, ctx.validatedBase - ctx.compressCapacity);
        ctx.expandRatio = ChooseGreaterDistance(0.0f, config.flexExpand);

        UpdatePriorityLists(ctx);

        if (ctx.parent != nullptr && ctx.parent != &ctx)
        {
            UpdateContextMetrics(*ctx.parent);
        }
    }

    /**
     * Updates the metrics of a given rectangular segment context.
     *
     * This function recalculates various validated dimensions, compression and expansion
     * properties, and priority lists for the given rectangular segment context. If the
     * context has a parent, metrics are also updated recursively for the parent context.
     * The function is marked as `noexcept`, indicating that it does not throw exceptions.
     *
     * @param ctx The rectangular segment context whose metrics are to be updated.
     */
    void UpdateContextMetrics(RectSegmentContext& ctx) noexcept {
        UpdateAccumulatedMetrics(ctx);

        const RectSegmentCreateInfo& config = ctx.config;

        ctx.validatedWidthMin = ChooseGreaterDistance(0.0f, config.widthMin);
        ctx.validatedWidthMax = ChooseGreaterDistance(ctx.validatedWidthMin, config.widthMax);
        ctx.validatedHeightMin = ChooseGreaterDistance(0.0f, config.heightMin);
        ctx.validatedHeightMax = ChooseGreaterDistance(ctx.validatedHeightMin, config.heightMax);
        ctx.validatedWidthBase = std::clamp(ReadLengthValue(config.width, ctx.accumulatedWidthBase), ctx.validatedWidthMin, ctx.validatedWidthMax);
        ctx.validatedHeightBase = std::clamp(ReadLengthValue(config.height, ctx.accumulatedHeightBase), ctx.validatedHeightMin, ctx.validatedHeightMax);
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

    /**
     * Unlinks a child context from its parent context.
     *
     * This function removes the specified child context from its parent's list of children,
     * updates the child's parent to null, and, if applicable, updates the parent's context metrics.
     * The function is `noexcept` to indicate it does not throw exceptions.
     *
     * @param child A reference to the child context to be unlinked. Must not be null.
     * @return None. The function does not return a value.
     * @throws None. The function does not throw exceptions.
     */
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

    /**
     * Establishes a parent-child relationship between two context objects.
     *
     * This function links a given child context to a parent context. If the child is already linked
     * to a different parent, it is first unlinked. The relationship is updated by setting the child's
     * parent pointer to the provided parent context and adding the child to the parent's collection
     * of children. The parent's context metrics are then updated accordingly.
     *
     * @param parent The parent context that the child will be linked to.
     * @param child The child context to be linked to the parent.
     * @throws None. This function is marked as `noexcept` and is guaranteed not to throw exceptions.
     */
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

    /**
     * Destroys the context of a segment and deallocates its memory.
     *
     * This function ensures proper cleanup of a segment context by unlinking its children without
     * destroying them, clearing any associated data structures, and finally deallocating the
     * memory for the context object. The function is marked as `constexpr` to allow compile-time
     * invocation and `noexcept` to signal it does not throw exceptions.
     *
     * @param ptr A pointer to the segment context to be destroyed.
     *            If the pointer is null, the function returns immediately without performing any action.
     */
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

    /**
     * Creates and initializes a segment context and returns a managed pointer to it.
     *
     * This function dynamically allocates a new segment context, updates its metrics,
     * and wraps it in a `std::unique_ptr` with a custom deleter for automatic cleanup.
     *
     * @param config The configuration object used to initialize the segment context.
     * @return A `std::unique_ptr` managing the lifecycle of the created segment context.
     *         The custom deleter ensures proper cleanup using `DestroySegmentContext`.
     * @throws None. The function is noexcept.
     */
    template<typename ContextT,typename ConfigT>
    requires ((std::same_as<ContextT, LinearSegmentContext> && std::same_as<ConfigT, LinearSegmentCreateInfo>) ||(std::same_as<ContextT, RectSegmentContext>   && std::same_as<ConfigT, RectSegmentCreateInfo>))
    constexpr auto CreateSegmentContext(const ConfigT& config) noexcept -> std::unique_ptr<ContextT, decltype(&DestroySegmentContext<ContextT>)> {
        auto* ctx = new ContextT{config};
        UpdateContextMetrics(*ctx);

        return std::unique_ptr<ContextT, decltype(&DestroySegmentContext<ContextT>)>{ctx,&DestroySegmentContext<ContextT>
        };
    }

    /**
     * Computes size metrics based on a target distance and a given context.
     *
     * This function validates the provided `targetDistance` against contextual
     * minimum values and determines whether the calculated state represents a
     * compression mode. It optionally rounds a base value based on the `round` parameter.
     * The function is marked as `[[nodiscard]]` to indicate that its return value should not be ignored.
     *
     * @param targetDistance The target distance to be validated and measured.
     * @param ctx The context containing segments and accumulated dimensional values.
     * @param round A boolean indicating whether rounding should be applied to the base value.
     * @return A pair consisting of:
     *         - The validated distance (float).
     *         - A boolean indicating whether the context is in compression mode.
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
     * Computes size metrics based on the input dimensions and context settings.
     *
     * This function calculates validated width and height values, determines an operational
     * layout direction (row or column), and identifies if the current state is in a compression mode
     * based on the direction-aligned dimensions and associated thresholds.
     *
     * The function uses contextual configuration and optionally applies rounding to the accumulated base sizes.
     * It returns calculated metrics packed into a tuple.
     *
     * @param targetWidth The target width input to be validated and used in calculations.
     * @param targetHeight The target height input to be validated and used in calculations.
     * @param ctx The context containing accumulated metrics and configuration settings.
     * @param round A boolean flag indicating whether the accumulated base dimensions should be rounded.
     * @return A tuple containing:
     *         (1) Validated width input (`float`),
     *         (2) Validated height input (`float`),
     *         (3) A boolean indicating if the layout direction is row (`true`) or column (`false`),
     *         (4) A boolean indicating if the computed metrics represent a compression mode.
     * @throws None. This function is marked `noexcept` and does not throw exceptions.
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
     * Computes a scaled value based on given input factors.
     *
     * This function calculates a scaled value by dividing the distance by the accumulate factor
     * and multiplying the result by the given factor. If any of the inputs are less than or equal
     * to zero, the function returns 0.0f. The function is marked as `[[nodiscard]]` to emphasize
     * that its return value should not be ignored, and `constexpr` to enable compile-time evaluation
     * when possible. It is also marked `noexcept`, indicating that it does not throw exceptions.
     *
     * @param distance The distance value used for the calculation. Must be greater than 0.0f.
     * @param accumulateFactor The accumulate factor used in the calculation. Must be greater than 0.0f.
     * @param factor The multiplier factor used in the computation. Must be greater than 0.0f.
     * @return The scaled value if all inputs are greater than 0.0f; otherwise, returns 0.0f.
     * @throws None. The function does not throw exceptions.
     */
    [[nodiscard]] constexpr float Scaler(float distance, float accumulateFactor, float factor) noexcept {
        if (distance <= 0.0f || accumulateFactor <= 0.0f || factor <= 0.0f) {
            return 0.0f;
        }
        return distance <= 0.0f || accumulateFactor <= 0.0f || factor <= 0.0f ? 0.0f : distance / accumulateFactor * factor;
    }

    /**
     * Generates a tuple containing metrics for compress cascade calculations.
     *
     * This function computes and returns a collection of cascade-related metrics based on
     * the provided input distance, a linear segment context, and an optional rounding option.
     * It utilizes the context's internal values to produce the necessary metrics while
     * supporting optional rounding for the accumulated base value.
     *
     * The function is marked as `[[nodiscard]]` to indicate that the return value should not be ignored.
     * It is also `constexpr`, allowing for potential compile-time evaluation, and guarantees
     * noexcept operation.
     *
     * @param inputDistance The input float value representing the distance for calculations.
     * @param ctx A reference to the LinearSegmentContext containing relevant cascade data.
     * @param round A boolean reference indicating whether to apply rounding to the accumulated base.
     *              If true, the base is rounded using `std::lroundf`.
     * @return A tuple containing:
     *         - The input distance as a float.
     *         - The accumulated base as a float, optionally rounded.
     *         - The accumulated compress solidify as a float.
     *         - A span of size_t values representing the cascade priorities from the context.
     * @throws None. The function does not throw exceptions.
     */
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

    /**
     * Computes and returns a tuple containing cascade compression metrics for a given input.
     *
     * This function evaluates specified cascading compression-related metrics based on the provided
     * context data and parameters. It processes a specific direction (row or column) and factors in
     * optional rounding behavior. The result is returned as a tuple comprising key metrics and a span
     * representing priority data.
     *
     * @param widthInput Input measurement for width.
     * @param heightInput Input measurement for height.
     * @param ctx A `RectSegmentContext` instance providing context-based data such as accumulated
     *        dimensions and compression priorities.
     * @param isRow A boolean flag indicating whether the operation should apply row-related calculations
     *        (true) or column-related calculations (false).
     * @param round A boolean determining whether accumulated base values should be rounded.
     * @return A tuple containing:
     *         - Selected input dimension (either `widthInput` or `heightInput`).
     *         - Direction-specific accumulated base value, optionally rounded.
     *         - Accumulated solidified compression metric from the context.
     *         - Span of priorities for the compression cascade.
     * @throws None. This function is noexcept.
     */
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

    /**
     * Computes a set of metrics related to size compression for cascading contexts.
     *
     * This function calculates six float values based on the inputs provided, which are useful
     * for determining size compression metrics in the context of linear segments. The function
     * ensures calculations for adjusted compression distances, compression parameters, and a possibly
     * rounded effective base value, depending on the context configuration.
     *
     * The function is marked as `[[nodiscard]]` to ensure its result is used and `constexpr`,
     * allowing compile-time evaluation when possible. It does not throw exceptions.
     *
     * @param cascadeCompressDistance The compression distance for the cascade.
     * @param cascadeBaseDistance The base distance for the cascade.
     * @param cascadeCompressSolidify The solidify value influencing compression behavior.
     * @param ctx The linear segment context providing additional configuration and values.
     * @param round A flag indicating whether to apply rounding to the effective base.
     * @return A tuple containing six float values representing the computed metrics:
     *         1. Adjusted compress distance.
     *         2. Adjusted base distance.
     *         3. Compression solidify from the context.
     *         4. Compression capacity from the context.
     *         5. Possibly rounded base value.
     *         6. Minimum validated value from the context.
     * @throws None. The function is marked as `noexcept`.
     */
    [[nodiscard]] constexpr std::tuple<float, float, float, float, float, float>
    MakeCompressSizeMetrics(const float& cascadeCompressDistance, const float& cascadeBaseDistance, const float& cascadeCompressSolidify, const LinearSegmentContext& ctx, const bool& round) noexcept {
        const float childEffectiveBase = ctx.config.base.type == LengthUnitType::Auto ? ctx.accumulatedBase : ctx.validatedBase;
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

    /**
     * Constructs and returns a tuple containing size and compression-related metrics.
     *
     * This function calculates specific metrics based on input parameters and the context
     * of a rectangular segment, with considerations for compression distances, solidification,
     * and optional rounding. It separates behaviors for row-wise and column-wise configurations.
     * The result is a tuple containing six float values that represent calculated metrics.
     *
     * @param cascadeCompressDistance The initial compression distance for the cascade.
     * @param cascadeBaseDistance The base distance of the cascade.
     * @param cascadeCompressSolidify The compression solidification factor for the cascade.
     * @param ctx The contextual information for the rectangular segment, including dimensions
     *            and configuration settings.
     * @param isRow A boolean flag that determines if values are calculated for rows or columns.
     * @param round A boolean flag that determines whether certain base values should be rounded.
     * @return A tuple containing:
     *         - Adjusted compression distance after solidification for the cascade.
     *         - Adjusted base distance after solidification for the cascade.
     *         - Solidification factor for the width or height, based on `isRow`.
     *         - Compression capacity for the width or height, based on `isRow`.
     *         - Effective base size, optionally rounded.
     *         - Validated minimum size for the width or height, based on `isRow`.
     * @throws None. This function does not throw exceptions.
     */
    [[nodiscard]] constexpr std::tuple<const float,const float,const float,const float,const float,const float>
    MakeCompressSizeMetrics(const float& cascadeCompressDistance, const float&cascadeBaseDistance, const float& cascadeCompressSolidify, const RectSegmentContext& ctx, const bool& isRow, const bool& round) noexcept {
        const float childEffectiveBase = isRow ?
        ctx.config.width.type == LengthUnitType::Auto ? ctx.accumulatedWidthBase : ctx.validatedWidthBase :
        ctx.config.height.type == LengthUnitType::Auto ? ctx.accumulatedHeightBase : ctx.validatedHeightBase;
        const float roundedBase = round? std::lroundf(childEffectiveBase) : childEffectiveBase;

        return std::make_tuple(
            cascadeCompressDistance - cascadeCompressSolidify,
            cascadeBaseDistance - cascadeCompressSolidify,
            isRow? ctx.widthCompressSolidify: ctx.heightCompressSolidify,
            isRow? ctx.widthCompressCapacity: ctx.heightCompressCapacity,
            roundedBase,
            isRow? ctx.validatedWidthMin : ctx.validatedHeightMin);
    }

    /**
     * Computes metrics for expanding a cascade, based on input distance and context data.
     *
     * This function calculates whether a cascade expansion is required, the expansion delta,
     * and additional context-based parameters. The result is returned as a tuple containing
     * a boolean flag, two floating-point values, and a span of size_t values. The calculations
     * factor in optional rounding of the accumulated base value.
     *
     * The function is marked as `[[nodiscard]]` to indicate that its return value should not
     * be ignored and is `constexpr`, allowing for potential compile-time evaluation. It is also
     * declared `noexcept`, ensuring no exceptions are thrown during execution.
     *
     * @param inputDistance A constant reference to a float representing the input distance
     *                       used for the cascade expansion computation.
     * @param ctx            A constant reference to a LinearSegmentContext object, containing
     *                       the necessary contextual data for the calculations, such as
     *                       accumulatedBase and expandCascadePriorities.
     * @param round          A constant reference to a boolean indicating whether the context's
     *                       accumulatedBase value should be rounded during calculations.
     * @return A tuple containing:
     *         - A boolean indicating whether the cascade expansion delta is greater than zero.
     *         - A float value representing the computed cascade expansion delta.
     *         - A float value representing the context's accumulated expand ratio.
     *         - A span of size_t values corresponding to the expand cascade priorities.
     * @throws None. The function does not throw exceptions.
     */
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

    /**
     * Computes and returns metrics related to cascade expansion based on the provided parameters.
     *
     * This function calculates the cascade expand delta and other metrics, returning them in a tuple.
     * The metrics include whether expansion occurred, the calculated delta, an expansion ratio,
     * and a span of priority values. It is marked as `[[nodiscard]]` and `constexpr`, recommending
     * the return value to be used and allowing compile-time evaluation when possible.
     *
     * @param widthInput The input value representing the width in the context of the layout.
     * @param heightInput The input value representing the height in the context of the layout.
     * @param ctx A constant reference to a `RectSegmentContext` object holding contextual information for the calculation.
     * @param isRow A boolean indicating whether the computation is row-based (`true`) or column-based (`false`).
     * @param round A boolean indicating whether to round the accumulated base value during the calculation.
     * @return A tuple containing the following:
     *         - A boolean indicating if the cascade expansion delta is greater than zero.
     *         - A float representing the calculated cascade expand delta.
     *         - A float representing the accumulated expand ratio value from the context.
     *         - A span containing size_t priority values related to expand cascade priorities.
     * @throws None. The function does not throw exceptions.
     */
    [[nodiscard]] constexpr std::tuple<const bool,float,float, std::span<const size_t>>
    MakeExpandCascadeMetrics(const float& widthInput, const float& heightInput, const RectSegmentContext& ctx, const bool& isRow, const bool& round) {
        const float& value = isRow ? widthInput : heightInput;
        const float& directionAccumulatedBase = isRow? ctx.accumulatedWidthBase: ctx.accumulatedHeightBase;
        const float accumulatedBase = round? std::lroundf(directionAccumulatedBase) : directionAccumulatedBase;
        const float cascadeExpandDelta = std::max(value - accumulatedBase, 0.0f);

        return std::make_tuple(cascadeExpandDelta > 0.0f,cascadeExpandDelta, ctx.accumulatedExpandRatio, std::span(ctx.expandCascadePriorities));
    }

    /**
     * Computes key metrics for expanding size calculations based on the given context.
     *
     * This function evaluates the base size, expansion ratio, and available expansion capacity
     * of a child element based on the provided context. It optionally rounds the base size
     * and calculates the expandable capacity while ensuring no negative values.
     *
     * @param ctx The LinearSegmentContext containing configuration and sizing information.
     * @param round A boolean flag indicating whether the base size should be rounded.
     * @return A tuple containing:
     *         - The effective base size (rounded if specified).
     *         - The expansion ratio.
     *         - The calculated expansion capacity.
     * @throws None. This function is noexcept.
     */
    [[nodiscard]] constexpr std::tuple<float, float, float>
    MakeExpandSizeMetrics(const LinearSegmentContext& ctx, const bool& round) noexcept {
        const float childEffectiveBase = ctx.config.base.type == LengthUnitType::Auto ? ctx.accumulatedBase : ctx.validatedBase;
        const float roundedBase = round? std::lroundf(childEffectiveBase) : childEffectiveBase;
        const float childMax = ctx.validatedMax;
        const float expandCapacity = std::max(0.0f, childMax - roundedBase);
        return std::make_tuple(
            roundedBase,
            ctx.expandRatio,
            expandCapacity
        );
    }

    /**
     * Calculates metrics required to determine expansion size.
     *
     * This function computes and returns a tuple containing the base direction size,
     * an expansion ratio, and the maximum expandable capacity for a given rectangle segment context.
     * The result is influenced by the context settings, orientation, and whether rounding is applied.
     * It is marked as `[[nodiscard]]` to indicate that the returned value must not be ignored
     * and as `constexpr` to allow compile-time evaluation.
     *
     * @param ctx The rectangle segment context containing configuration and size details.
     * @param isRow A boolean flag indicating the orientation.
     *              If true, it considers the row-related dimensions; otherwise, considers column-related ones.
     * @param round A boolean flag for enabling or disabling rounding on the base dimension.
     * @return A tuple containing:
     *         - The base direction size (rounded if specified).
     *         - The expansion ratio from the context.
     *         - The computed capacity available for expansion.
     * @throws None. The function does not throw exceptions.
     */
    [[nodiscard]] constexpr std::tuple<const float, const float, const float>
    MakeExpandSizeMetrics(const RectSegmentContext& ctx, const bool& isRow, const bool& round) {
        const float& directionBase = isRow ?
        ctx.config.width.type == LengthUnitType::Auto ? ctx.accumulatedWidthBase : ctx.validatedWidthBase :
        ctx.config.height.type == LengthUnitType::Auto ? ctx.accumulatedHeightBase : ctx.validatedHeightBase;
        const float roundedBase = round? std::lroundf(directionBase) : directionBase;
        const float& directionMax = isRow ? ctx.validatedWidthMax : ctx.validatedHeightMax;

        const float expandCapacity = std::max(0.0f, directionMax - roundedBase);

        return std::make_tuple(
            roundedBase,
            ctx.expandRatio,
            expandCapacity);
    }


    /**
     * Computes the cross size within specified boundaries.
     *
     * This function calculates the cross size for a given layout context by considering input constraints,
     * validation ranges, and configuration-specific measurements. It uses conditional logic to differentiate
     * between row and column orientations, ensuring that the cross size adheres to the provided boundaries.
     *
     * @param ctx The context containing configuration and validation data for segment computations.
     * @param mainInput The main size input value, relative to the primary (main) dimension.
     * @param crossInput The cross size input value, relative to the secondary (cross) dimension.
     * @param isRow A boolean value indicating if the primary axis is a row (true) or a column (false).
     * @return The computed cross size within defined minimum and maximum limits.
     * @throws None. This function does not throw exceptions.
     */
    constexpr float ComputeCrossSize(const RectSegmentContext& ctx,const float mainInput,const float crossInput,const bool isRow) noexcept {
        const Length& crossLength   = isRow ? ctx.config.height : ctx.config.width;
        const float&   input    = isRow ? crossInput : mainInput;
        const float&   crossValidatedBase = isRow ? ctx.validatedHeightBase : ctx.validatedWidthBase;
        const float&   crossMin      = isRow ? ctx.validatedHeightMin : ctx.validatedWidthMin;
        const float&   crossMax      = isRow ? ctx.validatedHeightMax : ctx.validatedWidthMax;
        const float   lowestCrossMax = std::min(crossMax, input);

        const float oppositeAutoBase = crossLength.type != LengthUnitType::Flat? ReadLengthValue(crossLength, input)  : crossValidatedBase;

        return std::clamp(oppositeAutoBase, crossMin, lowestCrossMax);
    }

    /**
     * Executes a compression process for linear segments based on cascade metrics.
     *
     * This function performs a series of calculations to compress specified
     * linear segments using context and distance parameters. It utilizes various
     * helper functions to compute the cascade compression metrics and processes
     * the segments iteratively based on priority.
     *
     * @param ctx The context of the linear segment, providing relevant information for compression.
     * @param inputDistance The distance value used to initiate the compression calculations.
     * @param round A boolean flag indicating whether the output distances should be rounded.
     * @throws None. This function is marked `noexcept` and does not throw exceptions.
     */
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

    /**
     * Performs compression calculations and applies sizing adjustments to child segments
     * based on the provided context and input parameters.
     *
     * This function computes cascade compression metrics, processes child contexts in a
     * prioritized order, and adjusts their sizes using the calculated compression distances
     * and base values.
     *
     * @param ctx The context of the rectangle segment that is being compressed.
     * @param mainInput The primary input value used for compression calculations.
     * @param crossInput The secondary input value used for cross-size computations.
     * @param isRow A boolean flag indicating whether the operation is row-based.
     * @param round A boolean flag indicating whether to round the computed distances.
     * @throws None. The function does not throw exceptions.
     */
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

    /**
     * Dynamically expands linear segments according to the given distance and cascade expansion metrics.
     *
     * This function computes and applies expansion metrics to child linear segments in a hierarchical manner.
     * Each child segment's expansion is determined by calculating metrics such as expansion deltas, ratios,
     * and maximum allowable deltas. Adjustments are performed iteratively over a priority list. The function
     * also supports rounding of expansion values for precise scaling when required.
     *
     * @param ctx The context of the current linear segment, providing metadata and state information
     *            required for expansion.
     * @param inputDistance The base distance used as a reference to calculate the cascade expansion metrics.
     * @param round A boolean flag indicating whether expansion values should be rounded to the nearest integer.
     * @throws None. The function does not throw exceptions.
     */
    void Expanding(const LinearSegmentContext& ctx, const float& inputDistance, const bool& round) {
        auto [processingExpansion,cascadeExpandDelta, cascadeExpandRatio, priorityList] = MakeExpandCascadeMetrics(inputDistance, ctx, round);
        if (!processingExpansion) return;

        for (const auto index : priorityList) {
            auto* childCtx = GetChildSegmentContext(ctx,index);
            if (childCtx == nullptr) continue;

            auto [validateBase, expandRatio, maxDelta] = MakeExpandSizeMetrics(*childCtx, round);
            const float expandDelta = Scaler(cascadeExpandDelta, cascadeExpandRatio, expandRatio);
            const float clampedDelta = std::min(expandDelta, maxDelta);
            const float roundedDelta = round? std::lroundf(clampedDelta) : clampedDelta;

            Sizing(*childCtx, validateBase, roundedDelta, round);

            cascadeExpandDelta -= roundedDelta;
            cascadeExpandRatio -= expandRatio;
        }
    }

    /**
     * Handles the expansion of child segments within a given context.
     *
     * This function processes the expansion cascade for a segment and adjusts its child segments
     * based on the provided parameters. It calculates cascading expansion metrics and applies
     * size and position adjustments to child segments accordingly.
     *
     * @param ctx The context of the current segment being expanded.
     * @param mainInput The primary input dimension used for expansion calculations.
     * @param crossInput The cross dimension input used for expansion calculations.
     * @param isRow A boolean flag that determines if the expansion is applied in the row direction.
     * @param round A boolean flag indicating whether to round the calculated values.
     * @return None.
     * @throws None. The function does not throw exceptions.
     */
    void Expanding(const RectSegmentContext& ctx, const float& mainInput, const float& crossInput, const bool& isRow, const bool& round) {
        auto [processingExpansion,cascadeExpandDelta, cascadeExpandRatio, priorityList] = MakeExpandCascadeMetrics(mainInput, crossInput, ctx, isRow, round);

        if (!processingExpansion) return;

        for (const auto index : priorityList) {
            auto* childCtx = GetChildSegmentContext(ctx,index);
            if (childCtx == nullptr) continue;

            auto [validatedBase, expandRatio, maxDelta] = MakeExpandSizeMetrics(*childCtx, isRow, round);
            const float expandDelta = Scaler(cascadeExpandDelta, cascadeExpandRatio, expandRatio);
            const float clampedDelta = std::min(expandDelta, maxDelta);
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
     * Adjusts the size metrics and applies compression or expansion operations to a given context.
     *
     * This function validates and processes the input size values, updates the contextual metrics,
     * and determines whether to apply compression or expansion based on the specified parameters.
     *
     * @param ctx A reference to the LinearSegmentContext instance, which holds size and operational data.
     * @param value The input size value to validate and process.
     * @param delta The delta value to calculate the expanded distance.
     * @param round A boolean flag specifying whether the operations should be rounded.
     * @return None. The function modifies the context provided via reference.
     * @throws None. The function does not throw exceptions.
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
     * Adjusts the size properties of a rectangular segment context by applying width and height deltas.
     *
     * This function updates the dimensions of the rectangular segment context based on the provided width,
     * height, widthDelta, and heightDelta inputs. It validates the input dimensions and determines whether
     * to apply a compression or expansion operation based on the provided parameters.
     *
     * @param ctx The RectSegmentContext object whose size properties are to be modified.
     * @param width The input base width value for the rectangular segment.
     * @param height The input base height value for the rectangular segment.
     * @param widthDelta The additional width value to be added to the validated base width.
     * @param heightDelta The additional height value to be added to the validated base height.
     * @param round A boolean indicating if the resulting dimensions should be rounded.
     *
     * @return None. Changes are applied directly to the RectSegmentContext object.
     *
     * @throws None. The function does not throw exceptions.
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
     * Recursively calculates and sets the offsets for a linear segment context and its child contexts.
     *
     * This function computes and assigns the offset for the provided `LinearSegmentContext`
     * and recursively processes its child contexts in an ordered manner. If the context
     * has no children, the function returns immediately. The function is marked as `constexpr`,
     * allowing compile-time evaluation when possible, and `noexcept`, ensuring it does not throw exceptions.
     *
     * @param ctx Reference to the `LinearSegmentContext` to process.
     * @param parentOffset Optional parameter specifying the offset of the parent. Defaults to 0.0f.
     * @return None. This function modifies the provided context in place.
     * @throws None. The function does not throw exceptions.
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
     * Recursively positions a rectangular segment and its child segments relative to their parent's coordinates.
     *
     * This function assigns the positional offsets of the provided rectangle and its nested child segments
     * within the parent rectangle, considering the specified relative x and y coordinates. The placement
     * of child segments is determined based on the main-axis direction (row or column).
     *
     * @param ctx The context of the rectangular segment being positioned. Contains configuration, child data, and size information.
     * @param relativeX The relative x-coordinate to position the rectangle. Defaults to 0.0f.
     * @param relativeY The relative y-coordinate to position the rectangle. Defaults to 0.0f.
     * @throws None. The function does not throw exceptions.
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
}
