#include "game.hpp"
#include "bitmap.hpp"
#include "game_components.hpp"
#include <set>

namespace lefticus::awesome_game {

Game_Map make_map(const std::vector<std::filesystem::path> &search_directories)
{
  auto map = load_tiled_map("awesome_game/tiled/tiles/Map.tmj", search_directories);

  map.locations.at(Point{ 4, 5 }).can_enter// NOLINT magic numbers
    = [](const Game &, Point, Direction) { return true; };
  map.locations.at(Point{ 4, 5 }).enter_action// NOLINT magic numbers
    = [](Game &game, Point, Direction) {
        game.current_map = "store";
        game.player.map_location = Point{ 6, 6 };// NOLINT magic numbers
      };

  map.locations.at(Point{ 4, 6 }).enter_action// NOLINT magic numbers
    = [](Game &game, Point, Direction) { game.last_message = "A store"; };

  map.locations.at(Point{ 4, 6 }).exit_action// NOLINT magic numbers
    = [](Game &game, Point, Direction) { game.last_message = ""; };

  return map;
}

Game_Map make_store(const std::vector<std::filesystem::path> &search_directories)
{
  auto map = load_tiled_map("awesome_game/tiled/tiles/Store.tmj", search_directories);

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
  retval.current_map = "main";
  retval.tile_size = Size{ 8, 8 };// NOLINT Magic Number

  retval.variables["Cash"] = 50;// NOLINT
  retval.variables["xstation"] = false;
  retval.display_variables.emplace_back("Cash");


  Character player;
  player.map_location = { 14, 17 };// NOLINT Magic Number
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

  retval.popup_message =
    "Welcome to Quad Corners.\n\nThis town is your last stop on your delivery run. Unfortunately shortly after you "
    "arrived, some heavy rains came and washed out the road.\n\nTomorrow is your kid's birthday, and you still don't "
    "have a present!\n\nYou're not a slacker, you were planning to get it on the way home. Now you have to see if you "
    "can find it here, instead.";

  return retval;
}
}// namespace lefticus::awesome_game
