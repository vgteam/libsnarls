# Specify the minimum version for CMake
cmake_minimum_required(VERSION 3.10)

# This defines default install directories like "lib"
include(GNUInstallDirs)

# Project's name
project(libsnarls)
# We build using c++14
set(CMAKE_CXX_STANDARD 14)

# Use all standard-compliant optimizations
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g")

# Let cmake decide where to put the output files, allowing for out-of-tree builds.

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    # We are probably an external project. Don't use @rpath in Mac builds'
    # install_name fields (AKA LC_ID_DYLIB in otool -l output). Populate with
    # an absolute path instead. This will let us actually find the library when
    # we use it as a CMake external project and don't fully install it to any
    # normal lib directory.
    message("libsnarls is root project or external_project")
    set (CMAKE_MACOSX_RPATH OFF)
else()
    # We are probably an add_subdirectory. We will expect to be in the root
    # project's lib directory, so we do want to have our non-installed
    # install_name use @rpath.
    message("libsnarls is add_subdirectory project")
    set (CMAKE_MACOSX_RPATH ON)
endif()

# The install_name gets modified on installation to be this.
set (CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

# libvgio (full build using its cmake config)
# Produces VGio::vgio and VGio::vgio_static targets.
# Comes along with libhandlegraph, which it depends on.
add_subdirectory("deps/libvgio")

add_library(snarls_objs OBJECT
    src/chain.cpp
    src/snarl.cpp
    src/visit.cpp
    src/snarl_finder.cpp
    src/net_graph.cpp
    src/handle_graph_snarl_finder.cpp
    src/snarl_manager.cpp
    src/integrated_snarl_finder.cpp
    src/snarl_traversal.cpp  
    src/include/snarls/visit.hpp
    src/include/snarls/snarl_traversal.hpp
    src/include/snarls/snarl_manager.hpp
    src/include/snarls/snarl.hpp
    src/include/snarls/chain.hpp
    src/include/snarls/handle_graph_snarl_finder.hpp
    src/include/snarls/snarl_finder.hpp
    src/include/snarls/integrated_snarl_finder.hpp
    src/include/snarls/net_graph.hpp
    src/include/snarls/pair_hash.hpp
    src/include/snarls/vg_types.hpp
  )
  
set(snarls_INCLUDES
  "src/include"
)

# Targets we depend on that are real library targets that we can install.
set(snarls_TARGET_DEPS
  VGio::vgio
)

# Libs should be the dependency targets and any extra per-platform lib flags
set(snarls_LIBS
  ${snarls_TARGET_DEPS}
  ${PLATFORM_EXTRA_LIB_FLAGS}
)

# Use the include directory when building the objects.
target_include_directories(snarls_objs PUBLIC ${snarls_INCLUDES})
# Build objects position-independent to allow a shared library
set_target_properties(snarls_objs PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

if (CMAKE_MAJOR_VERSION EQUAL "3" AND (CMAKE_MINOR_VERSION EQUAL "10" OR CMAKE_MINOR_VERSION EQUAL "11"))
    # Before CMake 3.12 we can't ise target_link_libraries on an object library to convey the need to use depencies' include directories
    get_target_property(vgio_INCLUDE VGio::vgio INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(snarls_objs PUBLIC ${vgio_INCLUDE})
else()
    # Set all the libs as public dependencies of the object library
    target_link_libraries(snarls_objs PUBLIC ${snarls_LIBS})
endif()

# Use the include directory when building the objects.
# It can't be picked up via dependency by the other libraries even if it's public.
target_include_directories(snarls_objs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/include")

# Build objects position-independent to allow a shared library
set_target_properties(snarls_objs PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# Make static and shared versions with the same base name.
# Make sure to give them interface include directories that depending targets can use.
add_library(snarls_shared SHARED $<TARGET_OBJECTS:snarls_objs>)
set_target_properties(snarls_shared PROPERTIES OUTPUT_NAME snarls)
set_target_properties(snarls_shared PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
# Make sure we can find our dependency libraries after installation, if we aren't installing into the system search path
set_target_properties(snarls_shared PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
target_include_directories(snarls_shared INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src/include")
add_library(snarls_static STATIC $<TARGET_OBJECTS:snarls_objs>)
set_target_properties(snarls_static PROPERTIES OUTPUT_NAME snarls)
target_include_directories(snarls_static INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src/include")

# On older CMake, libraries weren't added as dependencies of the object library
# (which is not yet possible) so we need to add them as dependencies of the
# actual library. On Mac, we always need to do this, because you can't link a
# library there without the libraries it depends on at runtime.
target_link_libraries(snarls_shared PUBLIC ${snarls_LIBS})
target_link_libraries(snarls_static PUBLIC ${snarls_LIBS})

# Set up for installability
install(TARGETS snarls_shared snarls_static 
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY src/include/snarls
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.hpp"
)
