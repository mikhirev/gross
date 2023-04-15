list(APPEND DOC_ATTRIBUTES
  "libdir=${CMAKE_INSTALL_FULL_LIBDIR}/${PROJECT_NAME}"
  "sysconfdir=${CMAKE_INSTALL_FULL_SYSCONFDIR}"
  )

if(NOT DNSBL)
  list(APPEND DOC_ATTRIBUTES no-dnsbl)
endif()

if(NOT MILTER)
  list(APPEND DOC_ATTRIBUTES no-milter)
endif()

if(NOT SPF)
  list(APPEND DOC_ATTRIBUTES no-spf)
endif()

function(gross_add_man _name _sect)
  asciidoctor_add_document("${_name}.${_sect}"
    SOURCE "${_name}.${_sect}.adoc"
    BACKEND manpage
    TYPE manpage
    ATTRIBUTES ${DOC_ATTRIBUTES}
    )

  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${_name}.${_sect}"
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man${_sect}"
    )
endfunction()

if(BUILD_DOCS)
  gross_add_man(grossd 8)
  gross_add_man(grossd.conf 5)
endif()
