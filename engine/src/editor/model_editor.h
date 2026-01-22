/**
 * @file model_editor.h
 * @brief Runtime model editor for the Ignis engine.
 * Provides ImGui-based panels for model browsing, transform editing, and camera controls.
 */

#pragma once

#include "defines.h"
#include "math/math_types.h"
#include "resources/resource_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration for the model editor.
 */
typedef struct model_editor_config {
    /** @brief Pointer to the mesh array in application state. */
    struct mesh* meshes;
    /** @brief Pointer to the mesh count variable. */
    u32* mesh_count;
    /** @brief Maximum number of meshes that can be loaded. */
    u32 max_mesh_count;
    /** @brief Pointer to the camera position. */
    vec3* camera_position;
    /** @brief Pointer to the camera euler angles (pitch, yaw, roll). */
    vec3* camera_euler;
    /** @brief Pointer to the background/ambient color. */
    vec4* background_color;
} model_editor_config;

/**
 * @brief Initialize the model editor.
 * @param config The editor configuration.
 * @returns true on success, false on failure.
 */
KAPI b8 model_editor_initialize(model_editor_config config);

/**
 * @brief Shutdown the model editor.
 */
KAPI void model_editor_shutdown(void);

/**
 * @brief Render the model editor UI.
 * Should be called each frame during the ImGui frame.
 */
KAPI void model_editor_render(void);

/**
 * @brief Scan for available models in the assets directory.
 * Populates the internal list of available models.
 */
KAPI void model_editor_scan_models(void);

/**
 * @brief Load a model by name.
 * @param name The model name (without path or extension).
 * @returns true if loaded successfully, false otherwise.
 */
KAPI b8 model_editor_load_model(const char* name);

/**
 * @brief Unload a mesh at the given index.
 * @param index The mesh index to unload.
 * @returns true if unloaded successfully, false otherwise.
 */
KAPI b8 model_editor_unload_model(u32 index);

#ifdef __cplusplus
}
#endif
