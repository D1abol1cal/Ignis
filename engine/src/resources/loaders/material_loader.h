/**
 * @file material_loader.h
 * @author Syed Nofel Talha (syednofeltalha2@gmail.com)
 * @brief A resource loader that handles material resources.
 * @version 1.0
 * @date 2022-01-11
 * 
 * @copyright Ignis Game Engine is Copyright (c) Syed Nofel Talha 2025-2026
 * 
 */

#pragma once

#include "systems/resource_system.h"

/**
 * @brief Creates and returns a material resource loader.
 * 
 * @return The newly created resource loader.
 */
resource_loader material_resource_loader_create();
