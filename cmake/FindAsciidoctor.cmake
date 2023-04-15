set(Asciidoctor_FOUND FALSE)

find_program(Asciidoctor_EXECUTABLE
  NAMES asciidoctor
  DOC "asciidoctor executable"
  )
mark_as_advanced(Asciidoctor_EXECUTABLE)

if(Asciidoctor_EXECUTABLE)
  set(Asciidoctor_FOUND TRUE)
  if(NOT TARGET Asciidoctor::asciidoctor)
    add_executable(Asciidoctor::asciidoctor IMPORTED)
    if(EXISTS "${Asciidoctor_EXECUTABLE}")
      set_target_properties(Asciidoctor::asciidoctor
        PROPERTIES IMPORTED_LOCATION "${Asciidoctor_EXECUTABLE}"
        )
    endif()
  endif()
endif()

function(asciidoctor_add_document _target)
  cmake_parse_arguments(PARSE_ARGV 1 _adoc
    ""
    "BACKEND;TYPE;SOURCE"
    "ATTRIBUTES"
    )

  if(NOT _adoc_SOURCE)
    message(FATAL_ERROR "asciidoctor_add_document requires SOURCE argument")
  endif()

  if(NOT IS_ABSOLUTE ${_adoc_SOURCE})
    set(_adoc_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${_adoc_SOURCE}")
  endif()

  set(_asciidoctor_options "--out-file=${_target}")

  if(_adoc_BACKEND)
    list(APPEND _asciidoctor_options "--backend=${_adoc_BACKEND}")
  endif()

  if(_adoc_TYPE)
    list(APPEND _asciidoctor_options "--doctype=${_adoc_TYPE}")
  endif()

  foreach(_attr ${_adoc_ATTRIBUTES})
    list(APPEND _asciidoctor_options "--attribute=${_attr}")
  endforeach()

  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${_target}"
    DEPENDS ${_adoc_SOURCE}
    COMMAND Asciidoctor::asciidoctor ${_asciidoctor_options} ${_adoc_SOURCE}
    VERBATIM
    )

  add_custom_target("${_target}" ALL
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${_target}"
    )
endfunction()
