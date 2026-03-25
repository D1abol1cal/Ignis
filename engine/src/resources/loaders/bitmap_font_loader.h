/**
 * @file bitmap_font_loader.h
 * @author Syed Nofel Talha (syednofeltalha2@gmail.com)
 * @brief The loader for bitmap fonts.
 * @version 1.0
 * @date 2026-03-12
 * 
 * @copyright Ignis Game Engine is Copyright (c) Syed Nofel Talha 2025-2026
 * 
 */
#pragma once

#include "systems/resource_system.h"

/**
 * @brief Creates and returns a bitmap font resource loader.
 * 
 * @return The newly created resource loader.
 */
resource_loader bitmap_font_resource_loader_create();
