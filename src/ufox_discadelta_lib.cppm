//
// Created by Puwiwad B on 02.01.2026.
//
module;

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <unordered_map>
#ifdef HAS_VULKAN
#include <vulkan/vulkan_raii.hpp>
#endif

export module ufox_discadelta_lib;

export namespace ufox::geometry::discadelta {

    using Hash = size_t;

    enum class FlexDirection {
        Column,
        Row,
    };

    struct LinearSegment {
        std::string name{"none"};
        float base{0.0f};
        float expandDelta{0.0f};
        float distance{0.0f};
        float offset;
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

    struct LinearSegmentContext {
        LinearSegmentCreateInfo             config{};
        LinearSegment                       content{};
        LinearSegmentContext* parent = nullptr;
        std::vector<LinearSegmentContext*> children;
        std::unordered_map<std::string, size_t> childrenIndies;
        std::vector<size_t> compressCascadePriorities;
        std::vector<size_t> expandCascadePriorities;

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
        size_t order{0};
        size_t branchCount = 1;
        Hash hash{0};

        explicit LinearSegmentContext(LinearSegmentCreateInfo config) : config(std::move(config)) {}
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
        size_t branchCount = 1;
        Hash hash{0};

        explicit RectSegmentContext(RectSegmentCreateInfo  config) : config(std::move(config)) {}
    };

}
