#pragma once

#include <vulkan/vulkan.h>
#include <type_traits>

namespace vknc {

template <typename T>
struct StructureType
{
    static_assert(sizeof(T) == 0, "StructureType not implemented for given type!");
};

#include "structuretype-gen.inl"

} // namespace vknc
