include(cmake/SystemLink.cmake)
include(cmake/LibFuzzer.cmake)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)


macro(travels_setup_options)
  option(travels_ENABLE_HARDENING "Enable hardening" ON)
  option(travels_ENABLE_COVERAGE "Enable coverage reporting" OFF)


  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)
    set(SUPPORTS_UBSAN ON)
  else()
    set(SUPPORTS_UBSAN OFF)
  endif()

  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
    set(SUPPORTS_ASAN OFF)
  else()
    set(SUPPORTS_ASAN ON)
  endif()

  travels_check_libfuzzer_support(LIBFUZZER_SUPPORTED)
  option(travels_BUILD_FUZZ_TESTS "Enable fuzz testing executable" ${LIBFUZZER_SUPPORTED})


  if(NOT PROJECT_IS_TOP_LEVEL OR travels_PACKAGING_MAINTAINER_MODE)
    option(travels_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(travels_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(travels_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(travels_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(travels_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(travels_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(travels_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(travels_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(travels_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(travels_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(travels_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(travels_ENABLE_PCH "Enable precompiled headers" OFF)
    option(travels_ENABLE_CACHE "Enable ccache" OFF)
  else()
    cmake_dependent_option(
      travels_ENABLE_GLOBAL_HARDENING
      "Attempt to push hardening options to built dependencies"
      ON
      travels_ENABLE_HARDENING
      OFF)

    option(travels_ENABLE_IPO "Enable IPO/LTO" ON)
    option(travels_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
    option(travels_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(travels_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${SUPPORTS_ASAN})
    option(travels_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(travels_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
    option(travels_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(travels_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(travels_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(travels_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
    option(travels_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
    option(travels_ENABLE_PCH "Enable precompiled headers" OFF)
    option(travels_ENABLE_CACHE "Enable ccache" ON)
  endif()

  if(NOT PROJECT_IS_TOP_LEVEL)
    mark_as_advanced(
      travels_ENABLE_IPO
      travels_WARNINGS_AS_ERRORS
      travels_ENABLE_USER_LINKER
      travels_ENABLE_SANITIZER_ADDRESS
      travels_ENABLE_SANITIZER_LEAK
      travels_ENABLE_SANITIZER_UNDEFINED
      travels_ENABLE_SANITIZER_THREAD
      travels_ENABLE_SANITIZER_MEMORY
      travels_ENABLE_UNITY_BUILD
      travels_ENABLE_CLANG_TIDY
      travels_ENABLE_CPPCHECK
      travels_ENABLE_COVERAGE
      travels_ENABLE_PCH
      travels_ENABLE_CACHE)
  endif()
endmacro()

macro(travels_global_options)
  if(travels_ENABLE_IPO)
    include(cmake/InterproceduralOptimization.cmake)
    travels_enable_ipo()
  endif()

  if(travels_ENABLE_HARDENING AND travels_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    set(ENABLE_UBSAN_MINIMAL_RUNTIME NOT travels_ENABLE_SANITIZER_UNDEFINED)
    travels_enable_hardening(travels_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()

macro(travels_local_options)
  if (PROJECT_IS_TOP_LEVEL)
    include(cmake/StandardProjectSettings.cmake)
  endif()

  add_library(travels_warnings INTERFACE)
  add_library(travels_options INTERFACE)

  include(cmake/CompilerWarnings.cmake)
  travels_set_project_warnings(
    travels_warnings
    ${travels_WARNINGS_AS_ERRORS}
    ""
    ""
    ""
    "")

  if(travels_ENABLE_USER_LINKER)
    include(cmake/Linker.cmake)
    configure_linker(travels_options)
  endif()

  include(cmake/Sanitizers.cmake)
  travels_enable_sanitizers(
    travels_options
    ${travels_ENABLE_SANITIZER_ADDRESS}
    ${travels_ENABLE_SANITIZER_LEAK}
    ${travels_ENABLE_SANITIZER_UNDEFINED}
    ${travels_ENABLE_SANITIZER_THREAD}
    ${travels_ENABLE_SANITIZER_MEMORY})

  set_target_properties(travels_options PROPERTIES UNITY_BUILD ${travels_ENABLE_UNITY_BUILD})

  if(travels_ENABLE_PCH)
    target_precompile_headers(
      travels_options
      INTERFACE
      <vector>
      <string>
      <utility>)
  endif()

  if(travels_ENABLE_CACHE)
    include(cmake/Cache.cmake)
    travels_enable_cache()
  endif()

  include(cmake/StaticAnalyzers.cmake)
  if(travels_ENABLE_CLANG_TIDY)
    travels_enable_clang_tidy(travels_options ${travels_WARNINGS_AS_ERRORS})
  endif()

  if(travels_ENABLE_CPPCHECK)
    travels_enable_cppcheck(${travels_WARNINGS_AS_ERRORS} "" # override cppcheck options
    )
  endif()

  if(travels_ENABLE_COVERAGE)
    include(cmake/Tests.cmake)
    travels_enable_coverage(travels_options)
  endif()

  if(travels_WARNINGS_AS_ERRORS)
    check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
    if(LINKER_FATAL_WARNINGS)
      # This is not working consistently, so disabling for now
      # target_link_options(travels_options INTERFACE -Wl,--fatal-warnings)
    endif()
  endif()

  if(travels_ENABLE_HARDENING AND NOT travels_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    set(ENABLE_UBSAN_MINIMAL_RUNTIME NOT travels_ENABLE_SANITIZER_UNDEFINED)
    travels_enable_hardening(travels_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()

endmacro()
