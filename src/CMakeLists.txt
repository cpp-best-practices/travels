find_package(fmt CONFIG)
find_package(ftxui CONFIG)
find_package(spdlog CONFIG)
find_package(docopt CONFIG)
find_package(lodepng CONFIG)
find_package(nlohmann_json CONFIG)

# Generic test that uses conan libs
add_executable(
  travels
  main.cpp
  color.hpp
  size.hpp
  point.hpp
  vector2d.hpp
  bitmap.hpp
  bitmap.cpp
  game.cpp
  game.hpp
  game_components.hpp
  game_hacking_lesson_00.cpp
  game_hacking_lesson_00.hpp
  game_hacking_lesson_01.cpp
  game_hacking_lesson_01.hpp
  game_hacking_lesson_02.cpp
  game_hacking_lesson_02.hpp
  tile_set.hpp
  game_components.cpp)

target_link_libraries(
  travels
  PRIVATE project_options
          project_warnings
          docopt::docopt
          fmt::fmt
          spdlog::spdlog
          lodepng::lodepng
          nlohmann_json::nlohmann_json)

target_link_system_libraries(
  travels
  PRIVATE
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_include_directories(travels PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")
