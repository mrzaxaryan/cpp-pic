# =============================================================================
# Common.cmake - Build System Orchestrator
# =============================================================================
# Includes sub-modules in dependency order. Each module handles a single
# responsibility: option validation, triple mapping, flag assembly, source
# collection, and post-build artifact generation.

include_guard(GLOBAL)

include(${CMAKE_SOURCE_DIR}/cmake/Options.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Triples.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/CompilerFlags.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Sources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/PostBuild.cmake)
