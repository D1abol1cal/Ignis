/**
 * @file shader_loader.h
 * @author Syed Nofel Talha (syednofeltalha2@gmail.com)
 * @brief A resource loader that handles shader config resources.
 * @version 1.0
 * @date 2022-02-28
 * 
 * @copyright Ignis Game Engine is Copyright (c) Syed Nofel Talha 2025-2026
 * 
 */

#pragma once

#include "systems/resource_system.h"

/**
 * @brief Creates and returns a shader resource loader.
 * 
 * @return The newly created resource loader.
 */
resource_loader shader_resource_loader_create();
