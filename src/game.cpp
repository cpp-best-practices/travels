#include "game.hpp"
#include "bitmap.hpp"
#include "game_components.hpp"

namespace lefticus::travels {

Game_Map make_map(std::span<const std::filesystem::path> search_directories)
{
  auto map = load_tiled_map("travels/tiled/tiles/Map.tmj", search_directories);

  map.locations.at(map.location_names["maze_right"]).enter_action = [](Game &game, Point, Direction) {
    game.teleport_to_3d("maze", "1", Direction::West);
  };

  map.locations.at(map.location_names["maze_left"]).enter_action = [](Game &game, Point, Direction) {
    game.teleport_to_3d("maze", "2", Direction::East);
  };

  map.locations.at(map.location_names["tencin_entrance"]).enter_action = [](Game &game, Point, Direction) {
    game.teleport_to_2d("tencin", "entry_location");
  };

  map.locations.at(map.location_names["home_entrance"]).enter_action = [](Game &game, Point, Direction) {
    game.teleport_to_2d("home", "entry_location");
  };

  return map;
}

Game_Map make_home(const std::vector<std::filesystem::path> &search_directories)
{
  auto map = load_tiled_map("travels/tiled/tiles/Home.tmj", search_directories);
  map.locations.at(map.location_names["exit_location"]).enter_action = [](Game &game, Point, Direction) {
    game.teleport_to_2d("main", "home_exit");
  };

  return map;
}

Game_Map make_tencins_home(const std::vector<std::filesystem::path> &search_directories)
{
  auto map = load_tiled_map("travels/tiled/tiles/Tencin.tmj", search_directories);
  map.locations.at(map.location_names["exit_location"]).enter_action = [](Game &game, Point, Direction) {
    game.teleport_to_2d("main", "tencin_exit");
  };

  return map;
}


Game_Map make_store(const std::vector<std::filesystem::path> &search_directories)
{
  auto map = load_tiled_map("travels/tiled/tiles/Store.tmj", search_directories);

  map.locations.at(Point{ 7, 6 }).enter_action// NOLINT magic numbers
    = [](Game &game, Point, Direction) {
        game.current_map = "main";
        game.player.map_location = Point{ 4, 6 };// NOLINT magic numbers
      };

  map.locations.at(Point{ 3, 3 }).enter_action = [](Game &game, Point, Direction) {
    game.set_menu(Menu{ { "Ask about town",
                          "This is the quiet town of 'Quad Corners'. The economy has been down for the last few years. "
                          "People have been moving away. It's a bit depressing, really." },
      set_flag("Tell about present",
        "Ah yes, the Xstation6, all of the kids want one. You'll probably have a hard time finding one of those around "
        "here.",
        variable{ "xstation" }),
      check_flag("Ask about Xstation6", "I think bob has one.", variable{ "xstation" }),

      exit_menu() });
  };

  return map;
}

Game make_game(const std::vector<std::filesystem::path> &search_directories)
{
  Game retval{};
  retval.maps.emplace("main", make_map(search_directories));
  retval.maps.emplace("store", make_store(search_directories));
  retval.maps.emplace("home", make_home(search_directories));
  retval.maps.emplace("tencin", make_tencins_home(search_directories));


  retval.tile_size = Size{ 8, 8 };// NOLINT Magic Number

  retval.variables["Cash"] = 50;// NOLINT
  retval.variables["xstation"] = false;
  retval.display_variables.emplace_back("Cash");


  const auto make_maze = [] {
    constexpr static std::string_view maze_string(R"(
 ############
 #          #
 # # ##### ###/
 #  #       1 e
 ## #  #######`
 #  # #
 # # ##
 #    #
&#### #
w 2#  #
%#    #
 ######
)");

    Game_3D_Map maze{ lefticus::raycaster::make_map<float>(maze_string), {} };
    maze.enter_actions.emplace('w', [](Game &game, char) {
      game.teleport_to_2d("main", "maze_left_exit");
    });

    maze.enter_actions.emplace('e', [](Game &game, char) {
      game.teleport_to_2d("main", "maze_right_exit");
    });

    return maze;
  };

  retval.maps_3d.emplace("maze", make_maze());


  Character player;
  player.map_location = retval.get_current_map().location_names["start_location"];


  player.draw =
    [](Vector2D_Span<Color> &pixels, [[maybe_unused]] const Game &game, [[maybe_unused]] Point map_location) {
      const auto &tile = game.maps.at("main").tile_sets.front().at(98);// NOLINT magic number
      for (std::size_t cur_x = 0; cur_x < pixels.size().width; ++cur_x) {
        for (std::size_t cur_y = 0; cur_y < pixels.size().height; ++cur_y) {
          pixels.at(Point{ cur_x, cur_y }) += tile.at(Point{ cur_x, cur_y });
        }
      }
    };


  retval.player = player;

  retval.teleport_to_2d("home", "start_location");

  retval.popup_message =
    R"(I'm so bored, and I've gone through all of the books in my collection!

I'd like to visit my new neighbor to the west, but my parents left and locked the gate.

I'm pretty sure there's a path through the woods that starts in the north east of my property...
)";

  return retval;
}
}// namespace lefticus::travels
