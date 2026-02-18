# Discadelta: Nested Pre-Compute and Management via Link/Unlink/Create/Destroy 🦊 (Chapter 5)
## Overview
In the previous chapter, we explored the dynamic placing pass, which computes final offsets and positions for segments after sizing. This chapter shifts focus to the **pre-compute phase** and **tree management** mechanisms that enable efficient layout updates in a game engine context. We introduce how Discadelta handles **nested hierarchies** through parent-child relationships, and how pre-computation is triggered by structural operations like linking, unlinking, creation, and destruction.

This chapter emphasizes the **LinearSegmentContext** (1D layout) version, as it serves as the foundation for the more complex Rect (2D) version. Pre-computation ensures that metrics (validated sizes, accumulated mins/maxes, priorities) are always up-to-date, allowing fast Vulkan buffer updates or SDL3/GLFW resize handling without redundant recalculations. The system is designed for modern C++23, with RAII safety (`unique_ptr` with custom deleters) and noexcept guarantees for hot paths.

## Key Concepts in Pre-Compute
* **Nested Hierarchy**: Discadelta uses a tree structure where each node (context) has a `parent` pointer and a `children` vector of raw pointers. This allows recursive propagation (bottom-up for metrics, top-down for sizing/placing).


* **Pre-Compute Triggers**: Structural changes (create/link/unlink/destroy) automatically trigger `UpdateContextMetrics` to recompute validated/accumulated values and priorities.


* **Free Helper Methods**: All management is done via free functions (e.g. `Link`, `Unlink`, `CreateSegmentContext`, `DestroySegmentContext`) — keeps code modular and easy to integrate with Vulkan/SD L3/GLFW event loops.


* **Performance in Engine**: Pre-compute is O(N) per change — fine for UI trees. In Vulkan, use SDL3 resize after to update only dirty subtrees for vertex buffers.

## LinearSegmentContext — Nested Pre-Compute and Management
The `LinearSegmentContext` (1D layout) is the core building block for linear flex-like arrangements (e.g., horizontal toolbars or vertical menus in a game UI). It manages a single axis (distance/offset) but supports nested hierarchies for complex layouts.

### Nested Hierarchy Structure
The nesting is achieved through:

* `LinearSegmentContext* parent = nullptr;` — back-pointer to parent for upward propagation


* `std::vector<LinearSegmentContext*> children;` — non-owning raw pointers to direct children


* `std::unordered_map<std::string, size_t> childrenIndies;` — fast name → index lookup for children

This forms a tree:

Root has `parent = nullptr`
Leaves have `children.empty()`
Nesting allows sub-layouts (e.g. root bar → nested submenu)

Struct definition:
```c++
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
        float flexCompress{0.0f};
        float flexExpand{0.0f};
        float min{};
        float max{};
        size_t order;
    };

struct LinearSegmentContext {
        LinearSegmentCreateInfo             config{};
        LinearSegment                       content{};
        LinearSegmentContext* parent = nullptr;
        std::vector<LinearSegmentContext*> children;
        std::unordered_map<std::string, size_t> childrenIndies;
        std::vector<size_t> compressCascadePriorities;
        std::vector<size_t> expandCascadePriorities;
        size_t order{0};

        float validatedBase = 0.0f;
        float validatedMin = 0.0f;
        float validatedMax = 0.0f;
        float accumulatedBase = 0.0f;
        float accumulatedMin = 0.0f;
        float accumulatedCompressSolidify = 0.0f;
        float accumulatedExpandRatio = 0.0f;
        float compressRatio = 0.0f;
        float expandRatio = 0.0f;
        float compressCapacity = 0.0f;
        float compressSolidify = 0.0f;

        explicit LinearSegmentContext(LinearSegmentCreateInfo config) : config(std::move(config)) {}
    };
```

### Pre-Compute by Create/Link/Unlink/Destroy
Pre-compute is triggered on structural changes to keep metrics (validated base/min/max, accumulated mins, priorities) fresh. This is crucial for efficient engine integration (e.g., only update Vulkan buffers if metrics are changed).

#### Create (Factory with RAII)
**CreateSegmentContext** allocates a new node and runs initial pre-compute:
```c++
auto CreateSegmentContext<LinearSegmentContext, LinearSegmentCreateInfo>(config, mainInput = 0.0f) noexcept
    -> std::unique_ptr<LinearSegmentContext, decltype(&DestroySegmentContext<LinearSegmentContext>)>
{
    auto* ctx = new LinearSegmentContext{config};
    UpdateContextMetrics(*ctx);  // Initial pre-compute: validate, accumulate (empty), priorities (empty)
    // Content setup: base = max(accumulatedMin, validatedMin, mainInput), delta = 0
    return unique_ptr with deleter;
}
```
* Pre-compute: it calls **UpdateContextMetrics** — sets validated values, but accumulated = 0 (no children yet)


* RAII: **unique_ptr** with custom **DestroyLinearSegmentContext** ensures unlink + delete on scope exit


* Engine note: Call in Vulkan init / SDL3 setup — pre-computes root with initial size

#### Link (Attach child to parent)
**Link** sets a parent-child link and triggers pre-compute on parent:
```c++
void Link(LinearSegmentContext& parent, LinearSegmentContext& child) noexcept {
    // Safety checks
    if (&child == &parent) return;
    if (child.parent == &parent) return;
    if (child.parent != nullptr) Unlink(child);

    child.parent = &parent;
    parent.children.push_back(&child);

    // Pre-compute trigger: full metrics refresh on parent (propagates up)
    UpdateContextMetrics(parent);
}
```
* Nesting: adds a child pointer to parent's **children**, sets child's **parent**


* Pre-compute: **UpdateContextMetrics**(parent) — recomputes parent's accumulated (now includes child's validated), priorities, validated — propagates to grandparents


* Engine note: Call in GLFW/SDL3 event (e.g., add widget) — auto-refreshes affected subtree


#### Unlink (Detach child from parent)
**Unlink** removes the link and triggers pre-compute on parent:
```c++
void Unlink(LinearSegmentContext& child) noexcept {
    if (child.parent == nullptr) return;

    auto& parent = *child.parent;
    const auto it = std::ranges::find(parent.children, &child);
    if (it == parent.children.end()) {
        child.parent = nullptr;
        return;
    }

    parent.children.erase(it);
    child.parent = nullptr;

    // Pre-compute trigger: only if parent still has children
    if (!parent.children.empty()) {
        UpdateContextMetrics(parent);
    }
}
```
* Nesting: it removes a child from a parent's `children`, clears a child's `parent`


* Pre-compute: `UpdateContextMetrics`(parent) if parent not empty — recomputes parent's accumulated (now excludes child), priorities


* Engine note: Call on UI removal (SDL3/GLFW input) — refreshes only the detached branch

#### Destroy (Cleanup node)
`DestroySegmentContext` unlinks children/parent and deletes:
```c++
constexpr void DestroySegmentContext<LinearSegmentContext>(LinearSegmentContext* ptr) noexcept {
    if (!ptr) return;

    // Unlink all children (no delete — non-owning)
    while (!ptr->children.empty()) {
        LinearSegmentContext* child = ptr->children.front();
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
* Nesting: unlinks all children + self from parent


* Pre-compute: `Unlink` calls already trigger `UpdateContextMetrics` on parent


* Engine note: RAII via `unique_ptr` deleter — auto-cleanup on scope exit (e.g., Vulkan shutdown)

### Helper functions

* `ChooseGreaterDistance` / `ChooseLowestDistance`: Safe min/max helpers that treat negative values as 0. Prevents invalid (negative) sizes/rooms in layout math.


* `GetOrderedIndices`: Returns child indices sorted by `.order` field. Used in Placing pass to respect visual z-order / layout sequence.


* `GetChildSegmentContext` (name overload): Fast lookup via childrenIndies map → O(1) child access by string name. Beneficial when UI code refers to widgets by name ("close-button," "health-bar").


* `GetChildSegmentContext` (index overload): Direct vector access — used internally by cascade loops.

Core purpose: clean, fast, safe access + ordering + math helpers that avoid NaN/negative bugs.

```c++
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
```
```c++
[[nodiscard]] constexpr std::vector<size_t> GetOrderedIndices(const LinearSegmentContext& ctx) noexcept {
        std::vector<size_t> indices(ctx.children.size());
        std::iota(indices.begin(), indices.end(), size_t{0});

        std::ranges::sort(indices,[ctx](const size_t a, const size_t b) noexcept {return ctx.children[a]->order < ctx.children[b]->order;});

        return indices;
}
```
```c++
[[nodiscard]] constexpr auto GetChildSegmentContext(const LinearSegmentContext& parentCtx, const std::string& name) noexcept ->ContextT* {
        const auto it = parentCtx.childrenIndies.find(name);
        return it == parentCtx.childrenIndies.end()? nullptr : parentCtx.children[it->second];
}


[[nodiscard]] constexpr auto GetChildSegmentContext(const LinearSegmentContext& parentCtx, const size_t index) noexcept ->ContextT*{
        return index < parentCtx.children.size() ? parentCtx.children[index] : nullptr;
}
```

### Pre-Compute Pipeline Details
When triggered by link/unlink/create/destroy:

1. `UpdateAccumulatedMetrics` — reset + sum/min/max child validated/accumulated/solidify/ratios

2. `UpdatePriorityLists` — sort compress (low room first) / expand (high room first)

3. `UpdateContextMetrics` — validate/clamp + ratios/capacity/solidify + call 1-2 + recurse parent

This ensures the tree is always pre-computed — O(N) per change, but local (only affected subtree).

```c++
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
```
```c++
    constexpr void UpdatePriorityLists(LinearSegmentContext& ctx) noexcept
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

            float compressRoom = ChooseGreaterDistance(0.0f,child->validatedBase - child->validatedMin);
            float expandRoom = ChooseGreaterDistance(0.0f,child->validatedMax  - child->validatedBase);

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
```
### Sizing & Placing
#### Sizing pipeline (main entry: Sizing(ctx, value, delta, round))

1. `MakeSizeMetrics`
→ decides target distance + detects compress vs expand mode
2. Writes final `.base`, `.expandDelta`, `.distance` on context
3. Branches:
   * Compression mode → `Compressing`
   Cascade from the lowest remaining room first (solidify protects important widgets)
   * Expansion mode → `Expanding`
   Cascade from the highest remaining room first (greedy largest-first growth)


Both cascades use priority lists computed earlier → rapid O(n log n) sort once per structural change.
#### Key math helpers inside cascades

* `Scaler(remain, total, weight)` → proportional allocation
Classic flex-grow / flex-shrink math
* Repeated subtraction from cascade remainders
Ensures exact fit, no over- or under-shoot


```c++
[[nodiscard]] constexpr float Scaler(float distance, float accumulateFactor, float factor) noexcept {
        if (distance <= 0.0f || accumulateFactor <= 0.0f || factor <= 0.0f) {
            return 0.0f;
        }
        return distance <= 0.0f || accumulateFactor <= 0.0f || factor <= 0.0f ? 0.0f : distance / accumulateFactor * factor;
    }
```

```c++
[[nodiscard]] constexpr std::pair<float, bool> MakeSizeMetrics(const float& targetDistance, const LinearSegmentContext& ctx, const bool& round) noexcept {
        const float validatedInputDistance = ChooseGreaterDistance(ctx.accumulatedMin, ctx.validatedMin, targetDistance);
        const float roundedBase = round? std::lroundf(ctx.accumulatedBase) : ctx.accumulatedBase;
        const bool isCompressionMode = validatedInputDistance < roundedBase;

        return std::make_tuple(
            validatedInputDistance,
            isCompressionMode
        );
    }
```
```c++
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
```

```c++
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
```

```c++
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
```

```c++
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
```

```c++
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
```

```c++
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
```

```c++
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
```
#### Placing

* Pure top-down pass


* Respects child `.order` via `GetOrderedIndices`


* Accumulates `.offset` sequentially


* No metric changes — only writes the final screen position


```c++
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
```

# Chapter 5 Summary
Discadelta LinearSegmentContext implements efficient nested 1D layout with:

* Tree structure (parent and raw children pointers)
* Automatic metric pre-computation on create/link/unlink/destroy
* Bottom-up validation and accumulation
* Priority-based compress/expand cascades (lowest-room / highest-room first)
* Clean RAII factory and free Link/Unlink/Destroy helpers
* Separation: pre-compute (structural), sizing (distance-driven), placing (offset accumulation)

Result: rapid incremental updates suitable for real-time UI in games (Vulkan/SDL3 resize → only dirty subtrees).
Next Chapter we will extend the same ideas → 2D RectSegmentContext (width + height, row/column flows, wrap, alignment).