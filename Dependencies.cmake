include(cmake/CPM.cmake)
include(FetchContent)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
function(travels_setup_dependencies)

  FetchContent_Declare(
    lodepng
    GIT_REPOSITORY https://github.com/lvandeve/lodepng.git
    GIT_TAG        master
    )

  FetchContent_GetProperties(lodepng)
  if(NOT lodepng_POPULATED)
    FetchContent_Populate(lodepng)
    add_library(lodepng STATIC ${lodepng_SOURCE_DIR}/lodepng.cpp)
    target_link_libraries(lodepng PRIVATE travels_options)
    target_include_directories(lodepng PUBLIC ${lodepng_SOURCE_DIR})
  endif()

  # For each dependency, see if it's
  # already been provided to us by a parent project

  if(NOT TARGET spdlog::spdlog)
    cpmaddpackage(
      NAME
      spdlog
      VERSION
      1.11.0
      GITHUB_REPOSITORY
      "gabime/spdlog"
      OPTIONS
      "SPDLOG_USE_STD_FORMAT ON")
  endif()

  if(NOT TARGET Catch2::Catch2WithMain)
    cpmaddpackage("gh:catchorg/Catch2@3.3.2")
  endif()

  if(NOT TARGET CLI11::CLI11)
    cpmaddpackage("gh:CLIUtils/CLI11@2.3.2")
  endif()

  if(NOT TARGET ftxui::screen)
    cpmaddpackage("gh:ArthurSonzogni/FTXUI#main")
  endif()

  if(NOT TARGET cons_expr::cons_expr)
    cpmaddpackage("gh:lefticus/cons_expr#main")
  endif()

  if(NOT TARGET libraycaster::libraycaster)
    cpmaddpackage("gh:lefticus/libraycaster#main")
  endif()


  if(NOT TARGET nlohmann_json::nlohmann_json)
    cpmaddpackage("gh:nlohmann/json@3.11.2")
  endif()

endfunction()
