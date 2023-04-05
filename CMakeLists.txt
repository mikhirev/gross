cmake_minimum_required(VERSION 3.12)

project(
  gross
  VERSION 1.1.0
  LANGUAGES C
  )

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)
include(CTest)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckTypeSize)
include(CMakeDependentOption)

find_library(MATH_LIBRARY m DOC "standard math library")

find_package(Threads REQUIRED)
if(NOT CMAKE_USE_PTHREADS_INIT)
  message(SEND_ERROR "POSIX threads not found")
endif()

find_file(NETINET_IN_H "netinet/in.h")
if(NETINET_IN_H)
  add_compile_definitions(HAVE_NETINET_IN_H)
endif()

check_symbol_exists(clock_gettime "time.h" HAVE_CLOCK_GETTIME)
check_symbol_exists(CLOCK_MONOTONIC "time.h" HAVE_CLOCK_MONOTONIC)
check_symbol_exists(CLOCK_HIRES "time.h" HAVE_CLOCK_HIRES)
check_symbol_exists(CLOCK_REALTIME "time.h" HAVE_CLOCK_REALTIME)
check_symbol_exists(gettimeofday "sys/time.h" HAVE_GETTIMEOFDAY)

if(HAVE_CLOCK_GETTIME)
  if(HAVE_CLOCK_MONOTONIC)
    add_compile_definitions(USE_CLOCK_MONOTONIC)
  elseif(HAVE_CLOCK_HIRES)
    add_compile_definitions(USE_CLOCK_HIRES)
  elseif(HAVE_CLOCK_REALTIME)
    add_compile_definitions(USE_CLOCK_REALTIME)
  elseif(HAVE_GETTIMEOFDAY)
    add_compile_definitions(USE_GETTIMEOFDAY)
  else()
    message(SEND_ERROR "no suitable clock type found")
  endif()
elseif(HAVE_GETTIMEOFDAY)
  add_compile_definitions(USE_GETTIMEOFDAY)
else()
  message(SEND_ERROR "no suitable clock type found")
endif()

set(CMAKE_REQUIRED_INCLUDES "semaphore.h")
set(CMAKE_REQUIRED_LIBRARIES Threads::Threads)
check_function_exists(sem_init HAVE_SEM_INIT)
check_function_exists(sem_open HAVE_SEM_OPEN)
unset(CMAKE_REQUIRED_INCLUDES)
unset(CMAKE_REQUIRED_LIBRARIES)

if(HAVE_SEM_INIT)
elseif(HAVE_SEM_OPEN)
  add_compile_definitions(USE_SEM_OPEN)
else()
  message(SEND_ERROR "cannot compile without semaphores")
endif()

check_include_file("stdbool.h" HAVE_STDBOOL_H)
if(HAVE_STDBOOL_H)
  add_compile_definitions(HAVE_STDBOOL_H)
endif()

set(CMAKE_EXTRA_INCLUDE_FILES "stdbool.h")

check_type_size(bool BOOL)
if(HAVE_BOOL)
  add_compile_definitions(HAVE_BOOL)
endif()

unset(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "unistd.h")

check_type_size(useconds_t USECONDS_T)
if(HAVE_USECONDS_T)
  add_compile_definitions(HAVE_USECONDS_T)
endif()

unset(CMAKE_EXTRA_INCLUDE_FILES)

find_package(c-ares)
find_package(Milter)
find_package(LibSpf2)

cmake_dependent_option(DNSBL "Enable DNSBL checking" ON c-ares_FOUND OFF)
cmake_dependent_option(SPF "Enable SPF checking" ON LibSpf2_FOUND OFF)
cmake_dependent_option(MILTER "Enable milter support" ON Milter_FOUND OFF)

if(DNSBL)
  add_compile_definitions(DNSBL)
endif()

if(MILTER)
  add_compile_definitions(MILTER)
endif()

if(SPF)
  add_compile_definitions(SPF)
endif()

add_compile_definitions(
  "CONFIGFILE=\"${SYSCONFDIR}/grossd.conf\""
  "VERSION=\"${CMAKE_PACKAGE_VERSION}\""
  )

add_subdirectory("src")
add_subdirectory("man")

install(FILES doc/examples/grossd.conf
  TYPE SYSCONF
  )
