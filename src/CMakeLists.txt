include_directories("${CMAKE_SOURCE_DIR}/include")

set(grossd_OPTIONAL_SOURCES)

if(DNSBL)
  list(APPEND grossd_OPTIONAL_SOURCES
    check_dnsbl.c
    check_helo.c
    check_reverse.c
    helper_dns.c
    )
endif()

if(MILTER)
  list(APPEND grossd_OPTIONAL_SOURCES
    worker_milter.c
    )
endif()

if(SPF)
  list(APPEND grossd_OPTIONAL_SOURCES
    check_spf.c
    )
endif()

add_executable(grossd
  addrutils.c
  bloom.c
  bloommgr.c
  check_blocker.c
  check_random.c
  conf.c
  gross.c
  lookup3.c
  msgqueue.c
  proto_sjsms.c
  sha256.c
  srvstatus.c
  srvutils.c
  stats.c
  syncmgr.c
  thread_pool.c
  utils.c
  worker.c
  worker_postfix.c
  worker_sjsms.c
  ${grossd_OPTIONAL_SOURCES}
  )

target_link_libraries(grossd
  ${MATH_LIBRARY}
  Threads::Threads
  )

if(DNSBL)
  target_link_libraries(grossd c-ares::cares)
endif()

if(MILTER)
  target_link_libraries(grossd Sendmail::milter)
endif()

if(SPF)
  target_link_libraries(grossd LibSpf2::spf2)
endif()

install(TARGETS grossd
  DESTINATION ${CMAKE_INSTALL_SBINDIR}
  )

add_executable(gclient
  client_postfix.c
  client_sjsms.c
  gclient.c
  proto_sjsms.c
  utils.c
  )

add_library(grosscheck MODULE
  grosscheck.c
  proto_sjsms.c
  )
set_property(TARGET grosscheck
  PROPERTY PREFIX ""
  )
install(TARGETS grosscheck
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}
  )

if(BUILD_TESTING)

  add_executable(bloom
    bloom-test.c
    bloom.c
    sha256.c
    srvutils.c
    utils.c
    )
  target_link_libraries(bloom
    ${MATH_LIBRARY}
    Threads::Threads
    )
  add_test(bloom bloom)

  add_executable(counter
    bloom.c
    counter-test.c
    counter.c
    srvutils.c
    utils.c
    )
  target_link_libraries(counter
    ${MATH_LIBRARY}
    Threads::Threads
    )
  add_test(counter counter)

  if(DNSBL)
    add_executable(helper_dns
      bloom.c
      helper_dns-test.c
      helper_dns.c
      lookup3.c
      msgqueue.c
      srvutils.c
      utils.c
      )
    target_link_libraries(helper_dns
      ${MATH_LIBRARY}
      c-ares::cares
      Threads::Threads
      )
    add_test(helper_dns helper_dns)
  endif()

  add_executable(mask
    addrutils.c
    bloom.c
    mask-test.c
    srvutils.c
    utils.c
    )
  target_link_libraries(mask
    ${MATH_LIBRARY}
    Threads::Threads
    )
  add_test(mask mask)

  add_executable(msgqueue
    bloom.c
    msgqueue-test.c
    msgqueue.c
    srvutils.c
    utils.c
    )
  target_link_libraries(msgqueue
    ${MATH_LIBRARY}
    Threads::Threads
    )
  add_test(msgqueue msgqueue)

  add_executable(revaddr
    addrutils.c
    bloom.c
    reverse_ip-test.c
    srvutils.c
    utils.c
    )
  target_link_libraries(revaddr
    ${MATH_LIBRARY}
    Threads::Threads
    )
  add_test(revaddr revaddr)

  add_executable(sha256
    bloom.c
    sha256-test.c
    sha256.c
    srvutils.c
    utils.c
    )
  target_link_libraries(sha256
    ${MATH_LIBRARY}
    Threads::Threads
    )
  add_test(sha256 sha256)

endif(BUILD_TESTING)
