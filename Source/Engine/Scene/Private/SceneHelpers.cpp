#include "Engine/Scene/SceneHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/Components/TransformComponent.hpp"
#include "Engine/Scene/Scene.hpp"

#include "Utils/Assert.hpp"

AABBox SceneHelpers::ComputeSceneBBox(const Scene& scene)
{
    AABBox bbox;

    const auto& geometryStorageComponent = scene.ctx().get<GeometryStorageComponent>();

    for (auto&& [entity, tc, rc] : scene.view<TransformComponent, RenderComponent>().each())
    {
        for (const auto& ro : rc.renderObjects)
        {
            const Primitive& primitive = geometryStorageComponent.primitives[ro.primitive];

            bbox.Add(primitive.GetBBox().GetTransformed(tc.GetWorldTransform().GetMatrix()));
        }
    }

    return bbox;
}

vk::AccelerationStructureInstanceKHR SceneHelpers::GetTlasInstance(const Scene& scene,
        const TransformComponent& tc, const RenderObject& ro)
{
    const auto& geometryComponent = scene.ctx().get<GeometryStorageComponent>();
    const auto& materialComponent = scene.ctx().get<MaterialStorageComponent>();

    vk::TransformMatrixKHR transformMatrix;

    const glm::mat4 transposedTransform = glm::transpose(tc.GetWorldTransform().GetMatrix());

    std::memcpy(&transformMatrix.matrix, &transposedTransform, sizeof(vk::TransformMatrixKHR));

    Assert(ro.primitive <= static_cast<uint32_t>(std::numeric_limits<uint16_t>::max()));
    Assert(ro.material <= static_cast<uint32_t>(std::numeric_limits<uint8_t>::max()));

    const uint32_t customIndex = ro.primitive | (ro.material << 16);

    const Material& material = materialComponent.materials[ro.material];

    const vk::GeometryInstanceFlagsKHR flags = MaterialHelpers::GetTlasInstanceFlags(material.flags);

    const vk::AccelerationStructureKHR blas = geometryComponent.primitives[ro.primitive].GetBlas();

    return vk::AccelerationStructureInstanceKHR(transformMatrix,
            customIndex, 0xFF, 0, flags, VulkanContext::device->GetAddress(blas));
}
