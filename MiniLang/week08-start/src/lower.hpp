/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#pragma once

#include "ast.hpp"

namespace mini {

// Accepts the parser's tree: internal WhileStatement nodes do not exist yet.
Program lower(Program program);

} // namespace mini
