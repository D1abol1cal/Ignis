/**
 * @file imgui_system.cpp
 * @brief ImGui integration system implementation for the Ignis engine.
 */

#include "imgui_system.h"

// Windows headers for Win32 types
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_win32.h"

extern "C" {
#include "core/logger.h"
#include "core/kmemory.h"
#include "renderer/vulkan/vulkan_types.inl"
}

// Forward declarations for external C functions
extern "C" {
    vulkan_context* vulkan_get_context(void);
    void* platform_get_hwnd(void);
}

// Static state
static VkDescriptorPool g_imgui_descriptor_pool = VK_NULL_HANDLE;
static b8 g_initialized = false;

// Helper to check Vulkan results
static void check_vk_result(VkResult err) {
    if (err != VK_SUCCESS) {
        KERROR("ImGui Vulkan error: %d", err);
    }
}

b8 imgui_system_initialize(void) {
    if (g_initialized) {
        KWARN("ImGui system already initialized.");
        return true;
    }

    vulkan_context* context = vulkan_get_context();
    if (!context) {
        KERROR("Failed to get Vulkan context for ImGui initialization.");
        return false;
    }

    void* hwnd = platform_get_hwnd();
    if (!hwnd) {
        KERROR("Failed to get window handle for ImGui initialization.");
        return false;
    }

    // Create descriptor pool for ImGui
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
    pool_info.pPoolSizes = pool_sizes;

    VkResult result = vkCreateDescriptorPool(
        context->device.logical_device,
        &pool_info,
        context->allocator,
        &g_imgui_descriptor_pool
    );

    if (result != VK_SUCCESS) {
        KERROR("Failed to create ImGui descriptor pool.");
        return false;
    }

    // Initialize ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);

    // Get the UI renderpass handle - we need to find it
    // Look for ui_renderpass in the registered passes
    vulkan_renderpass* ui_renderpass = nullptr;
    for (u32 i = 0; i < VULKAN_MAX_REGISTERED_RENDERPASSES; ++i) {
        renderpass* pass = &context->registered_passes[i];
        if (pass->internal_data) {
            // Check if this is the UI renderpass by checking its properties
            vulkan_renderpass* vk_pass = (vulkan_renderpass*)pass->internal_data;
            // The UI renderpass typically has no clear flags (composites on top)
            if (!(pass->clear_flags & RENDERPASS_CLEAR_COLOUR_BUFFER_FLAG)) {
                ui_renderpass = vk_pass;
                break;
            }
        }
    }

    if (!ui_renderpass) {
        // Fallback: use the first valid renderpass
        for (u32 i = 0; i < VULKAN_MAX_REGISTERED_RENDERPASSES; ++i) {
            renderpass* pass = &context->registered_passes[i];
            if (pass->internal_data) {
                ui_renderpass = (vulkan_renderpass*)pass->internal_data;
                break;
            }
        }
    }

    if (!ui_renderpass) {
        KERROR("Failed to find renderpass for ImGui.");
        return false;
    }

    // Initialize Vulkan backend
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = context->instance;
    init_info.PhysicalDevice = context->device.physical_device;
    init_info.Device = context->device.logical_device;
    init_info.QueueFamily = context->device.graphics_queue_index;
    init_info.Queue = context->device.graphics_queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = g_imgui_descriptor_pool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = context->swapchain.image_count;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = context->allocator;
    init_info.CheckVkResultFn = check_vk_result;
    init_info.RenderPass = ui_renderpass->handle;

    if (!ImGui_ImplVulkan_Init(&init_info)) {
        KERROR("Failed to initialize ImGui Vulkan backend.");
        return false;
    }

    KINFO("ImGui system initialized successfully.");
    g_initialized = true;
    return true;
}

void imgui_system_shutdown(void) {
    if (!g_initialized) {
        return;
    }

    vulkan_context* context = vulkan_get_context();
    if (context) {
        vkDeviceWaitIdle(context->device.logical_device);
    }

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_imgui_descriptor_pool != VK_NULL_HANDLE && context) {
        vkDestroyDescriptorPool(
            context->device.logical_device,
            g_imgui_descriptor_pool,
            context->allocator
        );
        g_imgui_descriptor_pool = VK_NULL_HANDLE;
    }

    g_initialized = false;
    KINFO("ImGui system shut down.");
}

void imgui_system_begin_frame(void) {
    if (!g_initialized) {
        return;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void imgui_system_end_frame(void) {
    if (!g_initialized) {
        return;
    }

    ImGui::Render();
}

void imgui_system_render(void) {
    if (!g_initialized) {
        return;
    }

    vulkan_context* context = vulkan_get_context();
    if (!context) {
        return;
    }

    ImDrawData* draw_data = ImGui::GetDrawData();
    if (draw_data) {
        VkCommandBuffer cmd = context->graphics_command_buffers[context->image_index].handle;
        ImGui_ImplVulkan_RenderDrawData(draw_data, cmd);
    }
}

b8 imgui_wants_input(void) {
    if (!g_initialized) {
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse || io.WantCaptureKeyboard;
}

// Windows message handler for ImGui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

i64 imgui_process_win32_message(void* hwnd, u32 msg, u64 w_param, i64 l_param) {
    if (!g_initialized) {
        return 0;
    }

    return ImGui_ImplWin32_WndProcHandler((HWND)hwnd, msg, (WPARAM)w_param, (LPARAM)l_param);
}
