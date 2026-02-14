#include "Editor/Panels/InspectorPanel.h"

#include <array>
#include <cstring>

#include "Engine/Scene/Components.h"

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
#include <imgui.h>
#endif

namespace rg::editor
{
namespace
{
void EditVector3(const char* label, Vector3& value)
{
    float raw[3] {value.x, value.y, value.z};
    if (ImGui::DragFloat3(label, raw, 0.05f))
    {
        value.x = raw[0];
        value.y = raw[1];
        value.z = raw[2];
    }
}

std::array<char, 256> ToBuffer(const std::string& value)
{
    std::array<char, 256> buffer {};
    const std::size_t count = (value.size() < (buffer.size() - 1U)) ? value.size() : (buffer.size() - 1U);
    std::memcpy(buffer.data(), value.data(), count);
    buffer[count] = '\0';
    return buffer;
}
} // namespace

const char* InspectorPanel::Name() const
{
    return "Inspector";
}

void InspectorPanel::Draw(EditorContext& context)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    Entity entity = context.world.FindEntity(context.state.selectedEntity);
    if (!entity.IsValid())
    {
        ImGui::TextUnformatted("Select an entity in Scene Objects.");
        return;
    }

    ImGui::Text("Entity #%u", entity.GetId());

    auto nameBuffer = ToBuffer(entity.GetName());
    if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size()))
    {
        entity.SetName(nameBuffer.data());
    }

    ImGui::SeparatorText("Transform");
    TransformComponent& transform = entity.Transform();
    EditVector3("Position", transform.position);
    EditVector3("Rotation", transform.rotation);
    EditVector3("Scale", transform.scale);

    if (entity.HasScript())
    {
        ImGui::SeparatorText("Script");
        ScriptComponent& script = entity.Script();
        ImGui::Checkbox("Enabled", &script.enabled);

        auto scriptBuffer = ToBuffer(script.className);
        if (ImGui::InputText("Class", scriptBuffer.data(), scriptBuffer.size()))
        {
            script.className = scriptBuffer.data();
        }
    }

    if (entity.HasMesh())
    {
        ImGui::SeparatorText("Mesh");
        MeshComponent& mesh = entity.Mesh();
        ImGui::Checkbox("Visible", &mesh.visible);

        auto meshBuffer = ToBuffer(mesh.meshAsset);
        if (ImGui::InputText("Mesh Asset", meshBuffer.data(), meshBuffer.size()))
        {
            mesh.meshAsset = meshBuffer.data();
        }

        auto materialBuffer = ToBuffer(mesh.materialAsset);
        if (ImGui::InputText("Material Asset", materialBuffer.data(), materialBuffer.size()))
        {
            mesh.materialAsset = materialBuffer.data();
        }
    }

    if (entity.HasComponent<RigidbodyComponent>())
    {
        ImGui::SeparatorText("Rigidbody");
        RigidbodyComponent& rigidbody = entity.GetComponent<RigidbodyComponent>();
        ImGui::Checkbox("Kinematic", &rigidbody.isKinematic);
        ImGui::Checkbox("Use Gravity", &rigidbody.useGravity);
        ImGui::DragFloat("Mass", &rigidbody.mass, 0.1f, 0.01f, 1000.0f);
        EditVector3("Velocity", rigidbody.velocity);
    }

    if (entity.HasComponent<VoxelPlayerComponent>())
    {
        ImGui::SeparatorText("Voxel Player");
        VoxelPlayerComponent& player = entity.GetComponent<VoxelPlayerComponent>();
        ImGui::DragFloat("Walk Speed", &player.walkSpeed, 0.1f, 0.1f, 20.0f);
        ImGui::DragFloat("Jump Speed", &player.jumpSpeed, 0.1f, 0.1f, 25.0f);
        ImGui::DragFloat("Reach", &player.reachDistance, 0.1f, 1.0f, 12.0f);
        ImGui::Text("Selected Block Id: %u", player.selectedBlockId);
        ImGui::Text("Grounded: %s", player.grounded ? "yes" : "no");
    }

    ImGui::Separator();
    if (!entity.HasComponent<RigidbodyComponent>() && ImGui::Button("Add Rigidbody"))
    {
        entity.AddComponent<RigidbodyComponent>();
    }

    ImGui::SameLine();
    if (!entity.HasComponent<VoxelPlayerComponent>() && ImGui::Button("Add VoxelPlayer"))
    {
        entity.AddComponent<VoxelPlayerComponent>();
    }

    ImGui::SameLine();
    if (ImGui::Button("Delete Entity"))
    {
        context.world.DestroyEntity(entity);
        context.state.selectedEntity = ecs::InvalidEntity;
    }
#else
    (void)context;
#endif
}
} // namespace rg::editor
