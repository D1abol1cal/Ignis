/**
 * @file skybox_system.c
 * @brief Skybox system implementation.
 */

#include "skybox_system.h"

#include "core/logger.h"
#include "core/kmemory.h"
#include "core/kstring.h"
#include "math/kmath.h"
#include "platform/filesystem.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "systems/geometry_system.h"
#include "systems/shader_system.h"
#include "renderer/renderer_frontend.h"

// Skybox face names (order: +X, -X, +Y, -Y, +Z, -Z)
static const char* face_names[6] = {
    "right",   // +X
    "left",    // -X
    "top",     // +Y
    "bottom",  // -Y
    "front",   // +Z
    "back"     // -Z
};

typedef struct skybox_state {
    char current_name[SKYBOX_NAME_MAX_LENGTH];
    b8 is_loaded;
    texture cubemap_texture;
    texture_map cubemap_map;
    geometry* cube_geometry;
    u32 shader_id;
    u32 shader_instance_id;
    u64 render_frame_number;
    char base_path[512];
} skybox_state;

static skybox_state* state_ptr = 0;

// Forward declarations
static b8 load_cubemap_faces(const char* skybox_path, u8** out_face_pixels, u32* out_width, u32* out_height, u8* out_channels);
static void free_face_pixels(u8** face_pixels, u32 width, u32 height, u8 channels);
static b8 create_skybox_cube(void);

b8 skybox_system_initialize(u64* memory_requirement, void* state, skybox_system_config config) {
    *memory_requirement = sizeof(skybox_state);

    if (!state) {
        return true;
    }

    state_ptr = (skybox_state*)state;
    kzero_memory(state_ptr, sizeof(skybox_state));

    if (config.skybox_base_path) {
        string_ncopy(state_ptr->base_path, config.skybox_base_path, 512);
    } else {
        string_ncopy(state_ptr->base_path, "../assets/skyboxes", 512);
    }

    // Get skybox shader
    state_ptr->shader_id = shader_system_get_id("Shader.Builtin.Skybox");
    if (state_ptr->shader_id == INVALID_ID) {
        KWARN("Skybox shader not found. Skybox system will not function.");
        return true;  // Not fatal, just won't have skybox
    }

    // Create cube geometry for skybox
    if (!create_skybox_cube()) {
        KERROR("Failed to create skybox cube geometry.");
        return false;
    }

    KINFO("Skybox system initialized.");
    return true;
}

void skybox_system_shutdown(void* state) {
    if (state_ptr) {
        skybox_system_unload();

        if (state_ptr->cube_geometry) {
            geometry_system_release(state_ptr->cube_geometry);
            state_ptr->cube_geometry = 0;
        }

        state_ptr = 0;
    }
}

b8 skybox_system_get_available(char*** out_names, u32* out_count) {
    if (!state_ptr || !out_names || !out_count) {
        return false;
    }

    // For now, return a hardcoded list
    // TODO: Implement directory scanning
    *out_count = 0;
    *out_names = 0;

    // Check for default skybox
    char check_path[512];
    string_format(check_path, "%s/default/right.png", state_ptr->base_path);

    if (filesystem_exists(check_path)) {
        *out_count = 1;
        *out_names = kallocate(sizeof(char*) * 1, MEMORY_TAG_ARRAY);
        (*out_names)[0] = string_duplicate("default");
    }

    return true;
}

void skybox_system_free_available_list(char** names, u32 count) {
    if (names) {
        for (u32 i = 0; i < count; i++) {
            if (names[i]) {
                u32 len = string_length(names[i]) + 1;
                kfree(names[i], len, MEMORY_TAG_STRING);
            }
        }
        kfree(names, sizeof(char*) * count, MEMORY_TAG_ARRAY);
    }
}

b8 skybox_system_load(const char* name) {
    if (!state_ptr) {
        KERROR("Skybox system not initialized.");
        return false;
    }

    if (state_ptr->shader_id == INVALID_ID) {
        KERROR("Skybox shader not available.");
        return false;
    }

    // Unload current skybox if any
    if (state_ptr->is_loaded) {
        skybox_system_unload();
    }

    // Load cubemap faces
    u8* face_pixels[6] = {0};
    u32 width, height;
    u8 channels;

    if (!load_cubemap_faces(name, face_pixels, &width, &height, &channels)) {
        KERROR("Failed to load skybox '%s'.", name);
        return false;
    }

    // Create cubemap texture
    state_ptr->cubemap_texture.width = width;
    state_ptr->cubemap_texture.height = height;
    state_ptr->cubemap_texture.channel_count = channels;
    state_ptr->cubemap_texture.generation = INVALID_ID;
    string_ncopy(state_ptr->cubemap_texture.name, name, TEXTURE_NAME_MAX_LENGTH);

    renderer_cubemap_create((const u8**)face_pixels, &state_ptr->cubemap_texture);

    // Free face pixel data
    free_face_pixels(face_pixels, width, height, channels);

    // Set up the texture map for the cubemap
    state_ptr->cubemap_map.texture = &state_ptr->cubemap_texture;
    state_ptr->cubemap_map.use = TEXTURE_USE_MAP_DIFFUSE;  // Generic usage
    state_ptr->cubemap_map.filter_minify = TEXTURE_FILTER_MODE_LINEAR;
    state_ptr->cubemap_map.filter_magnify = TEXTURE_FILTER_MODE_LINEAR;
    state_ptr->cubemap_map.repeat_u = TEXTURE_REPEAT_CLAMP_TO_EDGE;
    state_ptr->cubemap_map.repeat_v = TEXTURE_REPEAT_CLAMP_TO_EDGE;
    state_ptr->cubemap_map.repeat_w = TEXTURE_REPEAT_CLAMP_TO_EDGE;

    // Acquire sampler resources for the texture map
    if (!renderer_texture_map_acquire_resources(&state_ptr->cubemap_map)) {
        KERROR("Failed to acquire texture map resources for skybox cubemap.");
        renderer_cubemap_destroy(&state_ptr->cubemap_texture);
        return false;
    }

    // Acquire shader instance resources
    shader* s = shader_system_get_by_id(state_ptr->shader_id);
    if (!s) {
        KERROR("Failed to get skybox shader.");
        renderer_texture_map_release_resources(&state_ptr->cubemap_map);
        renderer_cubemap_destroy(&state_ptr->cubemap_texture);
        return false;
    }

    texture_map* maps[1] = { &state_ptr->cubemap_map };
    if (!renderer_shader_acquire_instance_resources(s, maps, &state_ptr->shader_instance_id)) {
        KERROR("Failed to acquire skybox shader instance resources.");
        renderer_texture_map_release_resources(&state_ptr->cubemap_map);
        renderer_cubemap_destroy(&state_ptr->cubemap_texture);
        return false;
    }

    // Store name and mark as loaded
    string_ncopy(state_ptr->current_name, name, SKYBOX_NAME_MAX_LENGTH);
    state_ptr->is_loaded = true;
    state_ptr->render_frame_number = (u64)-1;  // Max u64 value to force first update

    KINFO("Skybox '%s' loaded successfully.", name);
    return true;
}

void skybox_system_unload(void) {
    if (state_ptr && state_ptr->is_loaded) {
        // Release shader instance resources
        shader* s = shader_system_get_by_id(state_ptr->shader_id);
        if (s && state_ptr->shader_instance_id != INVALID_ID) {
            renderer_shader_release_instance_resources(s, state_ptr->shader_instance_id);
            state_ptr->shader_instance_id = INVALID_ID;
        }

        // Release texture map resources
        renderer_texture_map_release_resources(&state_ptr->cubemap_map);
        kzero_memory(&state_ptr->cubemap_map, sizeof(texture_map));

        // Destroy cubemap texture
        renderer_cubemap_destroy(&state_ptr->cubemap_texture);
        kzero_memory(&state_ptr->cubemap_texture, sizeof(texture));

        state_ptr->current_name[0] = 0;
        state_ptr->is_loaded = false;
    }
}

const char* skybox_system_get_current_name(void) {
    return state_ptr ? state_ptr->current_name : "";
}

b8 skybox_system_is_loaded(void) {
    return state_ptr ? state_ptr->is_loaded : false;
}

void skybox_system_render(mat4 projection, mat4 view, u64 render_frame_number) {
    if (!state_ptr || !state_ptr->is_loaded || !state_ptr->cube_geometry) {
        return;
    }

    // Debug: Log that we're rendering (comment out after confirming)
    // KTRACE("Skybox rendering...");

    // Use the skybox shader
    if (!shader_system_use_by_id(state_ptr->shader_id)) {
        KERROR("Failed to use skybox shader.");
        return;
    }

    // Get the shader
    shader* s = shader_system_get_by_id(state_ptr->shader_id);
    if (!s) {
        return;
    }

    // Bind globals and set global uniforms
    if (!renderer_shader_bind_globals(s)) {
        KERROR("Failed to bind skybox shader globals.");
        return;
    }

    // Set projection uniform
    if (!shader_system_uniform_set("projection", &projection)) {
        KERROR("Failed to set skybox projection uniform.");
        return;
    }

    // Set view uniform (no need to remove translation - shader does this)
    if (!shader_system_uniform_set("view", &view)) {
        KERROR("Failed to set skybox view uniform.");
        return;
    }

    // Apply globals
    shader_system_apply_global();

    // Bind and apply instance
    shader_system_bind_instance(state_ptr->shader_instance_id);

    // Check if we need to update the instance
    b8 needs_update = state_ptr->render_frame_number != render_frame_number;
    if (!shader_system_apply_instance(needs_update)) {
        KERROR("Failed to apply skybox shader instance.");
        return;
    }
    state_ptr->render_frame_number = render_frame_number;

    // Draw the skybox cube
    geometry_render_data render_data;
    render_data.geometry = state_ptr->cube_geometry;
    render_data.model = mat4_identity();  // Skybox doesn't need a model transform

    renderer_draw_geometry(render_data);
}

// --- Private functions ---

static b8 load_cubemap_faces(const char* skybox_name, u8** out_face_pixels, u32* out_width, u32* out_height, u8* out_channels) {
    // Load each face using resource system
    // The image loader expects paths relative to ../assets/textures/
    // So we use ../skyboxes/<name>/<face> to go up from textures to skyboxes

    for (u32 i = 0; i < 6; i++) {
        // Build resource path: ../skyboxes/<skybox_name>/<face_name>
        // This will resolve to ../assets/textures/../skyboxes/<name>/<face> = ../assets/skyboxes/<name>/<face>
        char resource_name[512];
        string_format(resource_name, "../skyboxes/%s/%s", skybox_name, face_names[i]);

        resource img_resource;
        if (!resource_system_load(resource_name, RESOURCE_TYPE_IMAGE, &img_resource)) {
            KERROR("Failed to load skybox face '%s' for skybox '%s'.", face_names[i], skybox_name);
            // Only free if we have loaded at least one face (i > 0 means we have dimensions)
            if (i > 0) {
                free_face_pixels(out_face_pixels, *out_width, *out_height, *out_channels);
            }
            return false;
        }

        image_resource_data* img = (image_resource_data*)img_resource.data;

        if (i == 0) {
            *out_width = img->width;
            *out_height = img->height;
            *out_channels = img->channel_count;
        } else {
            // Verify dimensions match
            if (img->width != *out_width || img->height != *out_height) {
                KERROR("Skybox face '%s' has different dimensions than other faces.", face_names[i]);
                resource_system_unload(&img_resource);
                free_face_pixels(out_face_pixels, *out_width, *out_height, *out_channels);
                return false;
            }
        }

        // Copy pixel data
        u32 size = img->width * img->height * img->channel_count;
        out_face_pixels[i] = kallocate(size, MEMORY_TAG_TEXTURE);
        kcopy_memory(out_face_pixels[i], img->pixels, size);

        resource_system_unload(&img_resource);
    }

    return true;
}

static void free_face_pixels(u8** face_pixels, u32 width, u32 height, u8 channels) {
    u32 size = width * height * channels;
    for (u32 i = 0; i < 6; i++) {
        if (face_pixels[i]) {
            kfree(face_pixels[i], size, MEMORY_TAG_TEXTURE);
            face_pixels[i] = 0;
        }
    }
}

static b8 create_skybox_cube(void) {
    // Create a unit cube for skybox (viewed from inside, so faces wound inward)
    // Each face has 2 triangles, vertices wound counter-clockwise when viewed from inside
    f32 vertices[] = {
        // Back face (looking toward -Z from inside)
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        // Front face (looking toward +Z from inside)
        -1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        // Left face (looking toward -X from inside)
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        // Right face (looking toward +X from inside)
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
        // Bottom face (looking toward -Y from inside)
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        // Top face (looking toward +Y from inside)
        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f
    };

    geometry_config config;
    config.vertex_size = sizeof(f32) * 3;  // Just position
    config.vertex_count = 36;
    config.vertices = vertices;
    config.index_size = 0;
    config.index_count = 0;
    config.indices = 0;
    string_ncopy(config.name, "skybox_cube", GEOMETRY_NAME_MAX_LENGTH);
    string_ncopy(config.material_name, "", MATERIAL_NAME_MAX_LENGTH);

    state_ptr->cube_geometry = geometry_system_acquire_from_config(config, true);
    return state_ptr->cube_geometry != 0;
}
