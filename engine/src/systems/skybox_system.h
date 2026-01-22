/**
 * @file skybox_system.h
 * @brief Skybox system for loading and rendering cubemap skyboxes.
 */

#pragma once

#include "defines.h"
#include "math/math_types.h"

/** @brief Maximum number of skyboxes that can be registered. */
#define SKYBOX_MAX_COUNT 16

/** @brief Maximum length of skybox name. */
#define SKYBOX_NAME_MAX_LENGTH 64

/**
 * @brief Configuration for skybox system initialization.
 */
typedef struct skybox_system_config {
    /** @brief Base path for skybox assets (e.g., "../assets/skyboxes"). */
    const char* skybox_base_path;
} skybox_system_config;

/**
 * @brief Initializes the skybox system.
 * @param memory_requirement Pointer to hold memory requirement.
 * @param state Pointer to state memory, or 0 for sizing query.
 * @param config Configuration for the system.
 * @return True on success, false on failure.
 */
b8 skybox_system_initialize(u64* memory_requirement, void* state, skybox_system_config config);

/**
 * @brief Shuts down the skybox system.
 * @param state Pointer to state memory.
 */
void skybox_system_shutdown(void* state);

/**
 * @brief Scans the skybox directory and returns available skybox names.
 * @param out_names Pointer to array of names (caller must free).
 * @param out_count Pointer to hold count of skyboxes found.
 * @return True on success, false on failure.
 */
b8 skybox_system_get_available(char*** out_names, u32* out_count);

/**
 * @brief Frees the list of available skybox names.
 * @param names Array of names to free.
 * @param count Number of names in array.
 */
void skybox_system_free_available_list(char** names, u32 count);

/**
 * @brief Loads a skybox by name.
 * @param name The name of the skybox folder.
 * @return True on success, false on failure.
 */
b8 skybox_system_load(const char* name);

/**
 * @brief Unloads the currently loaded skybox.
 */
void skybox_system_unload(void);

/**
 * @brief Gets the name of the currently loaded skybox.
 * @return The name, or empty string if none loaded.
 */
const char* skybox_system_get_current_name(void);

/**
 * @brief Checks if a skybox is currently loaded.
 * @return True if a skybox is loaded, false otherwise.
 */
b8 skybox_system_is_loaded(void);

/**
 * @brief Renders the skybox. Should be called first in the world renderpass.
 * @param projection The projection matrix.
 * @param view The view matrix.
 * @param render_frame_number The current render frame number (for instance update tracking).
 */
void skybox_system_render(mat4 projection, mat4 view, u64 render_frame_number);
