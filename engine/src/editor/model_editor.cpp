/**
 * @file model_editor.cpp
 * @brief Runtime model editor implementation for the Ignis engine.
 */

// C++ standard includes
#include <cstdio>
#include <cmath>
#include <cstring>

#include "imgui.h"

extern "C" {
#include "defines.h"
#include "core/logger.h"
#include "core/kmemory.h"
#include "core/kstring.h"
#include "math/transform.h"
#include "math/kmath.h"
#include "systems/geometry_system.h"
#include "systems/resource_system.h"
#include "resources/resource_types.h"
}

#include "model_editor.h"

// Windows-specific includes for file scanning
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define PLATFORM_WINDOWS 1
#endif

// Maximum available models
#define MAX_AVAILABLE_MODELS 256
#define MODEL_NAME_MAX_LENGTH 256

// Editor state
typedef struct model_editor_state {
    b8 initialized;

    // Configuration pointers
    mesh* meshes;
    u32* mesh_count;
    u32 max_mesh_count;
    vec3* camera_position;
    vec3* camera_euler;
    vec4* background_color;

    // Available models list
    char available_models[MAX_AVAILABLE_MODELS][MODEL_NAME_MAX_LENGTH];
    u32 model_count;
    i32 selected_model_index;

    // Selected mesh for editing
    i32 editing_mesh_index;

    // Transform edit values (cached for smooth editing)
    vec3 edit_position;
    vec3 edit_rotation_euler;  // In degrees
    vec3 edit_scale;

    // UI state
    b8 show_demo_window;
    b8 models_panel_open;
    b8 transform_panel_open;
    b8 camera_panel_open;
} model_editor_state;

static model_editor_state state = {};

// Forward declarations
static void render_models_panel(void);
static void render_transform_panel(void);
static void render_camera_panel(void);
static void sync_transform_from_mesh(void);
static void apply_transform_to_mesh(void);

b8 model_editor_initialize(model_editor_config config) {
    if (state.initialized) {
        KWARN("Model editor already initialized.");
        return true;
    }

    state.meshes = config.meshes;
    state.mesh_count = config.mesh_count;
    state.max_mesh_count = config.max_mesh_count;
    state.camera_position = config.camera_position;
    state.camera_euler = config.camera_euler;
    state.background_color = config.background_color;

    state.model_count = 0;
    state.selected_model_index = -1;
    state.editing_mesh_index = -1;

    state.edit_position = vec3_zero();
    state.edit_rotation_euler = vec3_zero();
    state.edit_scale = vec3_one();

    state.show_demo_window = false;
    state.models_panel_open = true;
    state.transform_panel_open = true;
    state.camera_panel_open = true;

    // Scan for available models
    model_editor_scan_models();

    state.initialized = true;
    KINFO("Model editor initialized.");
    return true;
}

void model_editor_shutdown(void) {
    if (!state.initialized) {
        return;
    }

    kzero_memory(&state, sizeof(model_editor_state));
    KINFO("Model editor shut down.");
}

void model_editor_render(void) {
    if (!state.initialized) {
        return;
    }

    // Main menu bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Models Panel", nullptr, &state.models_panel_open);
            ImGui::MenuItem("Transform Panel", nullptr, &state.transform_panel_open);
            ImGui::MenuItem("Camera Panel", nullptr, &state.camera_panel_open);
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", nullptr, &state.show_demo_window);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // Show demo window if enabled
    if (state.show_demo_window) {
        ImGui::ShowDemoWindow(&state.show_demo_window);
    }

    // Render panels
    if (state.models_panel_open) {
        render_models_panel();
    }

    if (state.transform_panel_open) {
        render_transform_panel();
    }

    if (state.camera_panel_open) {
        render_camera_panel();
    }
}

void model_editor_scan_models(void) {
    state.model_count = 0;

#ifdef PLATFORM_WINDOWS
    // Supported extensions to scan for
    const char* extensions[] = { "*.ksm", "*.obj" };
    const int extension_count = 2;

    for (int ext = 0; ext < extension_count; ++ext) {
        char search_path[512];
        std::snprintf(search_path, sizeof(search_path), "..\\assets\\models\\%s", extensions[ext]);

        WIN32_FIND_DATAA find_data;
        HANDLE find_handle = FindFirstFileA(search_path, &find_data);

        if (find_handle == INVALID_HANDLE_VALUE) {
            continue;  // No files with this extension, try next
        }

        do {
            if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                // Extract name without extension
                char* dot = strrchr(find_data.cFileName, '.');
                if (dot) {
                    u64 name_len = dot - find_data.cFileName;
                    if (name_len < MODEL_NAME_MAX_LENGTH) {
                        // Check if model name already exists (avoid duplicates if both .ksm and .obj exist)
                        bool already_exists = false;
                        for (u32 i = 0; i < state.model_count; ++i) {
                            if (strncmp(state.available_models[i], find_data.cFileName, name_len) == 0 &&
                                state.available_models[i][name_len] == '\0') {
                                already_exists = true;
                                break;
                            }
                        }

                        if (!already_exists) {
                            kzero_memory(state.available_models[state.model_count], MODEL_NAME_MAX_LENGTH);
                            kcopy_memory(state.available_models[state.model_count], find_data.cFileName, name_len);
                            state.model_count++;

                            if (state.model_count >= MAX_AVAILABLE_MODELS) {
                                KWARN("Maximum model count reached during scan.");
                                break;
                            }
                        }
                    }
                }
            }
        } while (FindNextFileA(find_handle, &find_data));

        FindClose(find_handle);

        if (state.model_count >= MAX_AVAILABLE_MODELS) {
            break;
        }
    }
#endif

    KINFO("Found %d available models.", state.model_count);
}

b8 model_editor_load_model(const char* name) {
    if (!state.initialized || !state.meshes || !state.mesh_count) {
        KERROR("Model editor not properly initialized.");
        return false;
    }

    if (*state.mesh_count >= state.max_mesh_count) {
        KERROR("Maximum mesh count reached. Cannot load more models.");
        return false;
    }

    resource mesh_resource = {};
    if (!resource_system_load(name, RESOURCE_TYPE_MESH, &mesh_resource)) {
        KERROR("Failed to load mesh resource: %s", name);
        return false;
    }

    geometry_config* configs = (geometry_config*)mesh_resource.data;
    u32 geometry_count = (u32)mesh_resource.data_size;

    mesh* new_mesh = &state.meshes[*state.mesh_count];
    new_mesh->geometry_count = geometry_count;
    new_mesh->geometries = (geometry**)kallocate(sizeof(geometry*) * geometry_count, MEMORY_TAG_ARRAY);

    u32 valid_geometry_count = 0;
    for (u32 i = 0; i < geometry_count; ++i) {
        geometry* g = geometry_system_acquire_from_config(configs[i], true);
        if (g) {
            new_mesh->geometries[valid_geometry_count] = g;
            valid_geometry_count++;
        } else {
            KWARN("Failed to acquire geometry %d for model '%s'", i, name);
        }
    }

    // Update geometry count to only include valid geometries
    new_mesh->geometry_count = valid_geometry_count;

    if (valid_geometry_count == 0) {
        KERROR("No valid geometries loaded for model '%s'", name);
        kfree(new_mesh->geometries, sizeof(geometry*) * geometry_count, MEMORY_TAG_ARRAY);
        new_mesh->geometries = 0;
        return false;
    }

    new_mesh->transform = transform_create();
    resource_system_unload(&mesh_resource);

    (*state.mesh_count)++;

    KINFO("Loaded model: %s (%d geometries)", name, geometry_count);
    return true;
}

b8 model_editor_unload_model(u32 index) {
    if (!state.initialized || !state.meshes || !state.mesh_count) {
        KERROR("Model editor not properly initialized.");
        return false;
    }

    if (index >= *state.mesh_count) {
        KERROR("Invalid mesh index: %d", index);
        return false;
    }

    mesh* m = &state.meshes[index];

    // Release all geometries
    for (u32 i = 0; i < m->geometry_count; ++i) {
        if (m->geometries[i]) {
            geometry_system_release(m->geometries[i]);
        }
    }

    // Free geometries array
    if (m->geometries) {
        kfree(m->geometries, sizeof(geometry*) * m->geometry_count, MEMORY_TAG_ARRAY);
    }

    // Shift remaining meshes
    for (u32 i = index; i < *state.mesh_count - 1; ++i) {
        state.meshes[i] = state.meshes[i + 1];
    }

    (*state.mesh_count)--;

    // Update editing index
    if (state.editing_mesh_index == (i32)index) {
        state.editing_mesh_index = -1;
    } else if (state.editing_mesh_index > (i32)index) {
        state.editing_mesh_index--;
    }

    KINFO("Unloaded mesh at index %d", index);
    return true;
}

// Panel rendering implementations

static void render_models_panel(void) {
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Model Browser", &state.models_panel_open)) {
        // Scan button
        if (ImGui::Button("Refresh Model List")) {
            model_editor_scan_models();
        }

        ImGui::Separator();
        ImGui::Text("Available Models (%d):", state.model_count);

        // Available models list
        if (ImGui::BeginListBox("##available_models", ImVec2(-FLT_MIN, 150))) {
            for (u32 i = 0; i < state.model_count; ++i) {
                bool is_selected = (state.selected_model_index == (i32)i);
                if (ImGui::Selectable(state.available_models[i], is_selected)) {
                    state.selected_model_index = i;
                }
            }
            ImGui::EndListBox();
        }

        // Load button
        if (state.selected_model_index >= 0) {
            if (ImGui::Button("Load Selected Model")) {
                model_editor_load_model(state.available_models[state.selected_model_index]);
            }
        }

        ImGui::Separator();
        ImGui::Text("Loaded Meshes (%d):", *state.mesh_count);

        // Loaded meshes list
        if (ImGui::BeginListBox("##loaded_meshes", ImVec2(-FLT_MIN, 150))) {
            for (u32 i = 0; i < *state.mesh_count; ++i) {
                char label[64];
                std::snprintf(label, sizeof(label), "Mesh %d (%d geometries)", i, state.meshes[i].geometry_count);

                bool is_selected = (state.editing_mesh_index == (i32)i);
                if (ImGui::Selectable(label, is_selected)) {
                    state.editing_mesh_index = i;
                    sync_transform_from_mesh();
                }
            }
            ImGui::EndListBox();
        }

        // Unload button
        if (state.editing_mesh_index >= 0 && state.editing_mesh_index < (i32)*state.mesh_count) {
            if (ImGui::Button("Unload Selected Mesh")) {
                model_editor_unload_model(state.editing_mesh_index);
            }
        }
    }
    ImGui::End();
}

static void render_transform_panel(void) {
    ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Transform Editor", &state.transform_panel_open)) {
        if (state.editing_mesh_index < 0 || state.editing_mesh_index >= (i32)*state.mesh_count) {
            ImGui::TextDisabled("No mesh selected.");
            ImGui::End();
            return;
        }

        ImGui::Text("Editing: Mesh %d", state.editing_mesh_index);
        ImGui::Separator();

        bool changed = false;

        // Position
        ImGui::Text("Position");
        if (ImGui::DragFloat3("##position", state.edit_position.elements, 0.1f)) {
            changed = true;
        }

        // Rotation (in degrees)
        ImGui::Text("Rotation (degrees)");
        if (ImGui::DragFloat3("##rotation", state.edit_rotation_euler.elements, 1.0f, -360.0f, 360.0f)) {
            changed = true;
        }

        // Scale
        ImGui::Text("Scale");
        if (ImGui::DragFloat3("##scale", state.edit_scale.elements, 0.01f, 0.001f, 100.0f)) {
            changed = true;
        }

        // Uniform scale checkbox option
        static bool uniform_scale = false;
        ImGui::Checkbox("Uniform Scale", &uniform_scale);

        if (uniform_scale && changed) {
            // If any scale component changed, sync all to match
            static vec3 last_scale = {1, 1, 1};
            if (state.edit_scale.x != last_scale.x) {
                state.edit_scale.y = state.edit_scale.z = state.edit_scale.x;
            } else if (state.edit_scale.y != last_scale.y) {
                state.edit_scale.x = state.edit_scale.z = state.edit_scale.y;
            } else if (state.edit_scale.z != last_scale.z) {
                state.edit_scale.x = state.edit_scale.y = state.edit_scale.z;
            }
            last_scale = state.edit_scale;
        }

        if (changed) {
            apply_transform_to_mesh();
        }

        ImGui::Separator();

        // Reset buttons
        if (ImGui::Button("Reset Position")) {
            state.edit_position = vec3_zero();
            apply_transform_to_mesh();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Rotation")) {
            state.edit_rotation_euler = vec3_zero();
            apply_transform_to_mesh();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Scale")) {
            state.edit_scale = vec3_one();
            apply_transform_to_mesh();
        }
    }
    ImGui::End();
}

static void render_camera_panel(void) {
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Camera & Scene", &state.camera_panel_open)) {
        // Camera position
        if (state.camera_position) {
            ImGui::Text("Camera Position");
            ImGui::DragFloat3("##cam_pos", state.camera_position->elements, 0.1f);
        }

        // Camera rotation (euler)
        if (state.camera_euler) {
            ImGui::Text("Camera Rotation (pitch, yaw, roll)");
            ImGui::DragFloat3("##cam_rot", state.camera_euler->elements, 1.0f);
        }

        ImGui::Separator();

        // Background color
        if (state.background_color) {
            ImGui::Text("Background Color");
            ImGui::ColorEdit4("##bg_color", state.background_color->elements);
        }
    }
    ImGui::End();
}

static void sync_transform_from_mesh(void) {
    if (state.editing_mesh_index < 0 || state.editing_mesh_index >= (i32)*state.mesh_count) {
        return;
    }

    mesh* m = &state.meshes[state.editing_mesh_index];
    transform* t = &m->transform;

    // Get position
    state.edit_position = transform_get_position(t);

    // Get rotation and convert to euler (degrees)
    quat rot = transform_get_rotation(t);
    // Simple euler extraction (yaw, pitch, roll) - approximate
    // This is a simplified conversion, not handling gimbal lock
    f32 sinr_cosp = 2.0f * (rot.w * rot.x + rot.y * rot.z);
    f32 cosr_cosp = 1.0f - 2.0f * (rot.x * rot.x + rot.y * rot.y);
    state.edit_rotation_euler.x = std::atan2(sinr_cosp, cosr_cosp) * K_RAD2DEG_MULTIPLIER;

    f32 sinp = 2.0f * (rot.w * rot.y - rot.z * rot.x);
    if (kabs(sinp) >= 1.0f) {
        state.edit_rotation_euler.y = std::copysign(K_PI / 2.0f, sinp) * K_RAD2DEG_MULTIPLIER;
    } else {
        state.edit_rotation_euler.y = std::asin(sinp) * K_RAD2DEG_MULTIPLIER;
    }

    f32 siny_cosp = 2.0f * (rot.w * rot.z + rot.x * rot.y);
    f32 cosy_cosp = 1.0f - 2.0f * (rot.y * rot.y + rot.z * rot.z);
    state.edit_rotation_euler.z = std::atan2(siny_cosp, cosy_cosp) * K_RAD2DEG_MULTIPLIER;

    // Get scale
    state.edit_scale = transform_get_scale(t);
}

static void apply_transform_to_mesh(void) {
    if (state.editing_mesh_index < 0 || state.editing_mesh_index >= (i32)*state.mesh_count) {
        return;
    }

    mesh* m = &state.meshes[state.editing_mesh_index];
    transform* t = &m->transform;

    // Set position
    transform_set_position(t, state.edit_position);

    // Convert euler (degrees) to quaternion
    f32 pitch = state.edit_rotation_euler.x * K_DEG2RAD_MULTIPLIER;
    f32 yaw = state.edit_rotation_euler.y * K_DEG2RAD_MULTIPLIER;
    f32 roll = state.edit_rotation_euler.z * K_DEG2RAD_MULTIPLIER;

    quat rot = quat_from_axis_angle((vec3){1, 0, 0}, pitch, false);
    rot = quat_mul(quat_from_axis_angle((vec3){0, 1, 0}, yaw, false), rot);
    rot = quat_mul(quat_from_axis_angle((vec3){0, 0, 1}, roll, false), rot);
    transform_set_rotation(t, rot);

    // Set scale
    transform_set_scale(t, state.edit_scale);
}
