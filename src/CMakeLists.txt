find_package(fmt CONFIG)
find_package(spdlog CONFIG)
find_package(docopt CONFIG)
find_package(lodepng CONFIG)

# Generic test that uses conan libs
add_executable(intro main.cpp color.hpp size.hpp point.hpp vector2d.hpp bitmap.hpp bitmap.cpp game_components.hpp game.cpp game.hpp)
target_link_libraries(
  intro
  PRIVATE project_options
          project_warnings
          docopt::docopt
          fmt::fmt
          spdlog::spdlog
          lodepng::lodepng)

target_link_system_libraries(
  intro
  PRIVATE
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_include_directories(intro PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")
