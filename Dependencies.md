# Dependencies

Libraries under `External/` are Git submodules. Exact revisions are recorded in
the parent repository.

Inspect the checked-out revisions with `git submodule status --recursive`.

| Library | Role |
| --- | --- |
| [GLFW](https://github.com/glfw/glfw) | Window creation and input |
| [GLM](https://github.com/g-truc/glm) | Vector, matrix, and quaternion math |
| [Dear ImGui](https://github.com/ocornut/imgui) | Debug UI |
| [glslang](https://github.com/KhronosGroup/glslang) | Runtime GLSL-to-SPIR-V compilation |
| [stb](https://github.com/nothings/stb) | Image loading and writing |
| [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) | Vulkan device memory allocation |
| [TinyGLTF](https://github.com/syoyo/tinygltf) | glTF scene loading |
| [portable-file-dialogs](https://github.com/samhocevar/portable-file-dialogs) | Native file dialogs |
| [TetGen](https://github.com/ufz/tetgen) | Light-volume tetrahedralization |
| [EnTT](https://github.com/skypjack/entt) | Entity and component storage |
| [easy_profiler](https://github.com/yse/easy_profiler) | CPU profiling |
| [SPIRV-Reflect](https://github.com/KhronosGroup/SPIRV-Reflect) | Shader resource reflection |
| [inifile-cpp](https://github.com/Rookfighter/inifile-cpp) | INI configuration loading and saving |
| [cyCodeBase](https://github.com/cemyuksel/cyCodeBase) | Weighted sample elimination |

## Notes

- **TinyGLTF** is pinned to **v2.9.7**, which provides the C++ API used by `SceneLoader`.
- **GLM** GTX headers require `GLM_ENABLE_EXPERIMENTAL`, defined in `Source/pch.hpp`.
- **glslang** builds without the optional SPIRV-Tools optimizer.
