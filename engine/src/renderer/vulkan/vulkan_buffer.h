/**
 * @file vulkan_buffer.h
 * @author Syed Nofel Talha (syednofeltalha2@gmail.com)
 * @brief A Vulkan-specific data buffer.
 * @version 1.0
 * @date 2026-01-11
 * 
 * @copyright Ignis Game Engine is Copyright (c) Syed Nofel Talha 2025-2026
 * 
 */

#pragma once

#include "vulkan_types.inl"

/**
 * @brief Creates a new Vulkan buffer.
 * 
 * @param context A pointer to the Vulkan context.
 * @param size The size of the buffer in bytes.
 * @param usage The buffer usage flags (VkBufferUsageFlagBits)
 * @param memory_property_flags The memory property flags.
 * @param bind_on_create Indicates if this buffer should bind on creation.
 * @param use_freelist Indicates if a freelist should be used. If not, allocate/free functions should also not be used.
 * @param out_buffer A pointer to hold the newly-created buffer.
 * @return True on success; otherwise false.
 */
b8 vulkan_buffer_create(
    vulkan_context* context,
    u64 size,
    VkBufferUsageFlagBits usage,
    u32 memory_property_flags,
    b8 bind_on_create,
    b8 use_freelist,
    vulkan_buffer* out_buffer);

/**
 * @brief Destroys the given buffer.
 * 
 * @param context A pointer to the Vulkan context.
 * @param buffer A pointer to the buffer to be destroyed.
 */
void vulkan_buffer_destroy(vulkan_context* context, vulkan_buffer* buffer);


/**
 * @brief Locks (or maps) the buffer memory to a temporary location of host memory, which should be unlocked before 
 * shutdown or destruction.
 * 
 * @param context A pointer to the Vulkan context.
 * @param buffer A pointer to the buffer whose memory should be locked.
 * @param offset An offset in bytes to lock the memory at.
 * @param size The amount of memory to lock.
 * @param flags Flags to be used in the locking operation (VkMemoryMapFlags).
 * @return A pointer to a block of memory, mapped to the buffer's memory.
 */
void* vulkan_buffer_lock_memory(vulkan_context* context, vulkan_buffer* buffer, u64 offset, u64 size, u32 flags);

/**
 * @brief Unlocks (or unmaps) the buffer memory.
 * 
 * @param context A pointer to the Vulkan context.
 * @param buffer A pointer to the buffer whose memory should be unlocked.
 */
void vulkan_buffer_unlock_memory(vulkan_context* context, vulkan_buffer* buffer);

/**
 * @brief Allocates space from a vulkan buffer. Provides the offset at which the
 * allocation occurred. This will be required for data copying and freeing.
 * 
 * @param buffer A pointer to the buffer from which to allocate.
 * @param size The size in bytes to be allocated.
 * @param out_offset A pointer to hold the offset in bytes from the beginning of the buffer.
 * @return True on success; otherwise false.
 */
b8 vulkan_buffer_allocate(vulkan_buffer* buffer, u64 size, u64* out_offset);

/**
 * @brief Frees space in the vulkan buffer.
 * 
 * @param buffer A pointer to the buffer to free data from.
 * @param size The size in bytes to be freed.
 * @param offset The offset in bytes from the beginning of the buffer.
 * @return True on success; otherwise false.
 */
b8 vulkan_buffer_free(vulkan_buffer* buffer, u64 size, u64 offset);

/**
 * @brief Loads a data range into the given buffer at a given offset. Internally performs a map,
 * copy and unmap.
 * 
 * @param context A pointer to the Vulkan context.
 * @param buffer A pointer to the buffer to load into.
 * @param offset The offset in bytes from the beginning of the buffer.
 * @param size The amount of data in bytes that will be loaded.
 * @param flags Flags to be used in the locking operation (VkMemoryMapFlags).
 * @param data A pointer to the data to be loaded.
 */
void vulkan_buffer_load_data(vulkan_context* context, vulkan_buffer* buffer, u64 offset, u64 size, u32 flags, const void* data);

/**
 * @brief Copies a range of data from one buffer to another.
 * 
 * @param context A pointer to the Vulkan context.
 * @param pool The command pool to be used.
 * @param @deprecated fence A fence to be used.
 * @param queue The queue to be used.
 * @param source The source buffer.
 * @param source_offset The source buffer offset.
 * @param dest The destination buffer.
 * @param dest_offset The destination buffer offset.
 * @param size The size of the data in bytes to be copied.
 */
void vulkan_buffer_copy_to(
    vulkan_context* context,
    VkCommandPool pool,
    VkFence fence,
    VkQueue queue,
    VkBuffer source,
    u64 source_offset,
    VkBuffer dest,
    u64 dest_offset,
    u64 size);
