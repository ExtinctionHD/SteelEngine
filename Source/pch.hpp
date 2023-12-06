#pragma once

#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <any>
#include <memory>
#include <optional>
#include <variant>
#include <iostream>
#include <cassert>
#include <functional>

#include "Utils/Defines.hpp"

PRAGMA_DISABLE_WARNINGS

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VK_ENABLE_BETA_EXTENSIONS
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_XYZW_ONLY
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <entt/entity/registry.hpp>

#include <easy/profiler.h>

PRAGMA_ENABLE_WARNINGS

#undef CreateSemaphore
#undef GetCurrentDirectory
