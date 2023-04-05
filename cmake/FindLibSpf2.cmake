# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindLibSpf2
-----------

Finds the LibSpf2 library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``LibSpf2::spf2``

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``LibSpf2_FOUND``
  true if libspf2 headers and libraries were found
``LibSpf2_INCLUDE_DIR``
  the directory containing LibSpf2 headers
``LibSpf2_INCLUDE_DIRS``
  list of the include directories needed to use LibSpf2
``LibSpf2_LIBRARIES``
  LibSpf2 libraries to be linked
``LibSpf2_VERSION``
  the version of LibSpf2 found

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``LibSpf2_INCLUDE_DIR``
  the directory containing LibSpf2 headers
``LibSpf2_LIBRARY``
  path to the LibSpf2 library

#]=======================================================================]

find_path(LibSpf2_INCLUDE_DIR
  NAMES spf2/spf.h
  DOC "libspf2 include directory"
  )

find_library(LibSpf2_LIBRARY
  NAMES spf2
  DOC "libspf2 library"
  )

mark_as_advanced(LibSpf2_INCLUDE_DIR LibSpf2_LIBRARY)

if(LibSpf2_INCLUDE_DIR AND EXISTS "${LibSpf2_INCLUDE_DIR}/spf2/spf_lib_version.h")
  file(STRINGS "${LibSpf2_INCLUDE_DIR}/spf2/spf_lib_version.h" _LibSpf2_VERSION
    REGEX "^#[ \t]*define[ \t]+SPF_LIB_VERSION_(MAJOR|MINOR|PATCH)[ \t]+[0-9]+[ \t]*$"
    )
  list(TRANSFORM _LibSpf2_VERSION
    REPLACE "[^0-9]+" ""
    )
  list(JOIN _LibSpf2_VERSION "." LibSpf2_VERSION)
  unset(_LibSpf2_VERSION)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibSpf2
  REQUIRED_VARS
    LibSpf2_LIBRARY
    LibSpf2_INCLUDE_DIR
  VERSION_VAR LibSpf2_VERSION
)

if(LibSpf2_FOUND)
  set(LibSpf2_LIBRARIES ${LibSpf2_LIBRARY})
  set(LibSpf2_INCLUDE_DIRS ${LibSpf2_INCLUDE_DIR})
  set(LibSpf2_DEFINITIONS ${PC_LibSpf2_CFLAGS_OTHER})

    if (NOT TARGET LibSpf2::spf2)
    add_library(LibSpf2::spf2 UNKNOWN IMPORTED)
    set_target_properties(LibSpf2::spf2 PROPERTIES
      IMPORTED_LOCATION "${LibSpf2_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${LibSpf2_INCLUDE_DIR}")
  endif ()
endif()

mark_as_advanced(
  LibSpf2_INCLUDE_DIR
  LibSpf2_LIBRARY
)
