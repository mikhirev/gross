set(libdir ${CMAKE_INSTALL_FULL_LIBDIR}/${PROJECT_NAME})
set(sysconfdir ${CMAKE_INSTALL_FULL_SYSCONFDIR})

configure_file(grossd.8.in grossd.8 @ONLY)
configure_file(grossd.conf.5.in grossd.conf.5 @ONLY)

unset(libdir)
unset(sysconfdir)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/grossd.8
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man8
  )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/grossd.conf.5
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man5
  )
