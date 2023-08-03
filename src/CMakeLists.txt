find_package(ftxui CONFIG)
find_package(spdlog CONFIG)
find_package(docopt CONFIG)
find_package(lodepng CONFIG)
find_package(nlohmann_json CONFIG)
find_package(CLI11 CONFIG)

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
  tile_set.hpp
  game_components.cpp)

target_link_libraries(travels PRIVATE travels_options travels_warnings)

target_link_system_libraries(
  travels
  PRIVATE
  CLI11::CLI11
  spdlog::spdlog
  lodepng
  nlohmann_json::nlohmann_json
  libraycaster::libraycaster
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_include_directories(travels PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")
