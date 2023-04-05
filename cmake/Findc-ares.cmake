#[=======================================================================[.rst:
Findc-ares
-------

Finds the c-ares library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``c-ares::cares``
The c-ares library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``c-ares_FOUND``
  True if the system has the c-ares library.
``c-ares_VERSION``
  The version of the c-ares library which was found.
``c-ares_INCLUDE_DIRS``
  Include directories needed to use c-ares.
``c-ares_LIBRARIES``
  Libraries needed to link to c-ares.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``c-ares_INCLUDE_DIR``
  The directory containing ``ares.h``.
``c-ares_LIBRARY``
  The path to the c-ares library.

#]=======================================================================]

find_package(PkgConfig QUIET)
pkg_check_modules(PC_c-ares QUIET libcares)

find_path(c-ares_INCLUDE_DIR
  NAMES ares.h
  PATHS ${PC_c-ares_INCLUDE_DIRS}
)
find_library(c-ares_LIBRARY
  NAMES cares
  PATHS ${PC_c-ares_LIBRARY_DIRS}
)

set(c-ares_VERSION ${PC_c-ares_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(c-ares
  REQUIRED_VARS
    c-ares_LIBRARY
    c-ares_INCLUDE_DIR
  VERSION_VAR c-ares_VERSION
)

if(c-ares_FOUND)
  set(c-ares_LIBRARIES ${c-ares_LIBRARY})
  set(c-ares_INCLUDE_DIRS ${c-ares_INCLUDE_DIR})
  set(c-ares_DEFINITIONS ${PC_c-ares_CFLAGS_OTHER})
endif()

if(c-ares_FOUND AND NOT TARGET c-ares::cares)
  add_library(c-ares::cares UNKNOWN IMPORTED)
  set_target_properties(c-ares::cares PROPERTIES
    IMPORTED_LOCATION "${c-ares_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_c-ares_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${c-ares_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
  c-ares_INCLUDE_DIR
  c-ares_LIBRARY
)
