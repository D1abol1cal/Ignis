/**
 * @file identifier.h
 * @author Syed Nofel Talha (syednofeltalha2@gmail.com)
 * @brief Contains a system for creating numeric identifiers.
 * @version 1.0
 * @date 2026-04-14
 *
 * @copyright Ignis Game Engine is Copyright (c) Syed Nofel Talha 2025-2026
 *
 */

#pragma once

#include "defines.h"

/**
 * @brief Acquires a new identifier for the given owner.
 *
 * @param owner The owner of the identifier.
 * @return The new identifier.
 */
u32 identifier_aquire_new_id(void* owner);

/**
 * @brief Releases the given identifier, which can then be used
 * again.
 *
 * @param id The identifier to be released.
 */
void identifier_release_id(u32 id);
