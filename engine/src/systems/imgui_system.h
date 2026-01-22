/**
 * @file imgui_system.h
 * @brief ImGui integration system for the Ignis engine.
 * Provides immediate mode GUI capabilities using Dear ImGui with Vulkan backend.
 */

#pragma once

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the ImGui system.
 * Creates descriptor pool, initializes ImGui contexts for Vulkan and Win32.
 * @returns true on success, false on failure.
 */
KAPI b8 imgui_system_initialize(void);

/**
 * @brief Shutdown the ImGui system.
 * Cleans up all ImGui resources.
 */
KAPI void imgui_system_shutdown(void);

/**
 * @brief Begin a new ImGui frame.
 * Call at the start of each frame before any ImGui calls.
 */
KAPI void imgui_system_begin_frame(void);

/**
 * @brief End the ImGui frame.
 * Call after all ImGui calls for the frame, before rendering.
 */
KAPI void imgui_system_end_frame(void);

/**
 * @brief Render ImGui draw commands.
 * Records ImGui draw commands to the current command buffer.
 * Should be called during the UI renderpass.
 */
KAPI void imgui_system_render(void);

/**
 * @brief Check if ImGui wants to capture input.
 * Use this to determine if input should be passed to the game or consumed by ImGui.
 * @returns true if ImGui wants keyboard/mouse input.
 */
KAPI b8 imgui_wants_input(void);

/**
 * @brief Process a Windows message for ImGui.
 * Should be called from the Windows message handler.
 * @param hwnd Window handle.
 * @param msg Message type.
 * @param w_param WPARAM.
 * @param l_param LPARAM.
 * @returns Non-zero if ImGui handled the message.
 */
KAPI i64 imgui_process_win32_message(void* hwnd, u32 msg, u64 w_param, i64 l_param);

#ifdef __cplusplus
}
#endif
