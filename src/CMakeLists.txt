find_package(fmt CONFIG)
find_package(spdlog CONFIG)
find_package(docopt CONFIG)
find_package(lodepng CONFIG)

# Generic test that uses conan libs
add_executable(
  awesome_game
  main.cpp
  color.hpp
  size.hpp
  point.hpp
  vector2d.hpp
  bitmap.hpp
  bitmap.cpp
  game_components.hpp
  game_hacking_lesson_00.cpp
  game_hacking_lesson_00.hpp
  game_hacking_lesson_01.cpp
  game_hacking_lesson_01.hpp)

target_link_libraries(
  awesome_game
  PRIVATE project_options
          project_warnings
          docopt::docopt
          fmt::fmt
          spdlog::spdlog
          lodepng::lodepng)

target_link_system_libraries(
  awesome_game
  PRIVATE
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_include_directories(awesome_game PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")
