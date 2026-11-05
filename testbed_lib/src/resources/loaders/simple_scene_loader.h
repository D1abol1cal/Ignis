/**
 * @file simple_scene_loader.h
 * @author Syed Nofel Talha (syednofeltalha2@gmail.com)
 * @brief Loader for simple scene files.
 * @version 1.0
 * @date 2026-03-29
 * 
 * @copyright Ignis Game Engine is Copyright (c) Syed Nofel Talha 2025-2026
 * 
 */

#pragma once

#include "systems/resource_system.h"

/**
 * @brief Creates and returns a simple scene resource loader.
 * 
 * @return The newly created resource loader.
 */
resource_loader simple_scene_resource_loader_create(void);