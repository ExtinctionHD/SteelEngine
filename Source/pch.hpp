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

#pragma warning(push, 0)

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VK_ENABLE_BETA_EXTENSIONS
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#pragma warning(pop)

#undef CreateSemaphore
#undef GetCurrentDirectory
