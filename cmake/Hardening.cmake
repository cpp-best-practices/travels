include(CheckCXXCompilerFlag)

macro(
  travels_enable_hardening
  target
  global
  ubsan_minimal_runtime)

  message(STATUS "** DEBUG: travels_enable_hardening called with target='${target}' global='${global}' ubsan='${ubsan_minimal_runtime}'")
  message(STATUS "** Enabling Hardening (Target ${target}) **")

  # Initialize as lists, not strings
  set(NEW_COMPILE_OPTIONS)
  set(NEW_LINK_OPTIONS)
  set(NEW_CXX_DEFINITIONS)

  if(MSVC)
    list(APPEND NEW_COMPILE_OPTIONS /sdl /DYNAMICBASE /guard:cf /NXCOMPAT)
    message(STATUS "*** MSVC flags: /sdl /DYNAIMCBASE /guard:cf /NXCOMPAT")

  elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang|GNU")
    # target_compile_definitions expects bare names, NOT -D flags!
    list(APPEND NEW_CXX_DEFINITIONS _GLIBCXX_ASSERTIONS)
    message(STATUS "*** GLIBC++ Assertions (vector[], string[], ...) enabled")

    if(NOT
       "${CMAKE_BUILD_TYPE}"
       STREQUAL
       "Debug")
      list(APPEND NEW_COMPILE_OPTIONS -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3)
      message(STATUS "*** g++/clang _FORTIFY_SOURCE=3 enabled")
    endif()

    #    check_cxx_compiler_flag(-fpie PIE)
    #if(PIE)
    #  set(NEW_COMPILE_OPTIONS ${NEW_COMPILE_OPTIONS} -fpie)
    #  set(NEW_LINK_OPTIONS ${NEW_LINK_OPTIONS} -pie)
    #
    #  message(STATUS "*** g++/clang PIE mode enabled")
    #else()
    #  message(STATUS "*** g++/clang PIE mode NOT enabled (not supported)")
    #endif()

    check_cxx_compiler_flag(-fstack-protector-strong STACK_PROTECTOR)
    if(STACK_PROTECTOR)
      list(APPEND NEW_COMPILE_OPTIONS -fstack-protector-strong)
      message(STATUS "*** g++/clang -fstack-protector-strong enabled")
    else()
      message(STATUS "*** g++/clang -fstack-protector-strong NOT enabled (not supported)")
    endif()

    check_cxx_compiler_flag(-fcf-protection CF_PROTECTION)
    if(CF_PROTECTION)
      list(APPEND NEW_COMPILE_OPTIONS -fcf-protection)
      message(STATUS "*** g++/clang -fcf-protection enabled")
    else()
      message(STATUS "*** g++/clang -fcf-protection NOT enabled (not supported)")
    endif()

    check_cxx_compiler_flag(-fstack-clash-protection CLASH_PROTECTION)
    if(CLASH_PROTECTION)
      if(LINUX OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        list(APPEND NEW_COMPILE_OPTIONS -fstack-clash-protection)
        message(STATUS "*** g++/clang -fstack-clash-protection enabled")
      else()
        message(STATUS "*** g++/clang -fstack-clash-protection NOT enabled (clang on non-Linux)")
      endif()
    else()
      message(STATUS "*** g++/clang -fstack-clash-protection NOT enabled (not supported)")
    endif()
  endif()

  if(${ubsan_minimal_runtime})
    check_cxx_compiler_flag("-fsanitize=undefined -fno-sanitize-recover=undefined -fsanitize-minimal-runtime"
                            MINIMAL_RUNTIME)
    if(MINIMAL_RUNTIME)
      list(APPEND NEW_COMPILE_OPTIONS -fsanitize=undefined -fno-sanitize-recover=undefined -fsanitize-minimal-runtime)
      message(STATUS "*** ubsan minimal runtime enabled")
    else()
      message(STATUS "*** ubsan minimal runtime NOT enabled (not supported)")
    endif()
  else()
    message(STATUS "*** ubsan minimal runtime NOT enabled (not requested)")
  endif()

  message(STATUS "** Hardening Compiler Flags: ${NEW_COMPILE_OPTIONS}")
  message(STATUS "** Hardening Linker Flags: ${NEW_LINK_OPTIONS}")
  message(STATUS "** Hardening Compiler Defines: ${NEW_CXX_DEFINITIONS}")

  message(STATUS "** DEBUG: Checking global='${global}' evaluates to: ${${global}}")
  if(${global})
    message(STATUS "** Setting hardening options globally for all dependencies")
    # Convert lists to space-separated strings for CMAKE_CXX_FLAGS
    string(REPLACE ";" " " NEW_COMPILE_OPTIONS_STR "${NEW_COMPILE_OPTIONS}")
    string(REPLACE ";" " " NEW_LINK_OPTIONS_STR "${NEW_LINK_OPTIONS}")
    # For definitions, add -D prefix since CMAKE_CXX_FLAGS expects -D flags
    set(DEF_FLAGS)
    foreach(def ${NEW_CXX_DEFINITIONS})
      list(APPEND DEF_FLAGS "-D${def}")
    endforeach()
    string(REPLACE ";" " " NEW_CXX_DEFINITIONS_STR "${DEF_FLAGS}")

    message(STATUS "** DEBUG: Before - CMAKE_CXX_FLAGS = '${CMAKE_CXX_FLAGS}'")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${NEW_COMPILE_OPTIONS_STR}" )
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${NEW_LINK_OPTIONS_STR}" )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${NEW_CXX_DEFINITIONS_STR}" )
    message(STATUS "** DEBUG: After - CMAKE_CXX_FLAGS = '${CMAKE_CXX_FLAGS} ${NEW_COMPILE_OPTIONS_STR} ${NEW_CXX_DEFINITIONS_STR}'")
  else()
    target_compile_options(${target} INTERFACE ${NEW_COMPILE_OPTIONS})
    target_link_options(${target} INTERFACE ${NEW_LINK_OPTIONS})
    target_compile_definitions(${target} INTERFACE ${NEW_CXX_DEFINITIONS})
  endif()
endmacro()
