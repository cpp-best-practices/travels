#include "game.hpp"
#include "bitmap.hpp"
#include "game_components.hpp"
#include <set>

namespace lefticus::awesome_game {

Game_Map make_map()
{
  auto map = load_tiled_map("/home/jason/my_awesome_game/resources/tiled/tiles/Map.tmj");

  map.locations.at(Point{4,5}).can_enter // NOLINT magic numbers
    = [](const Game &, Point, Direction){ return true; };
  map.locations.at(Point{4,5}).enter_action // NOLINT magic numbers
    = [](Game &game, Point, Direction){ game.current_map = "store"; game.player.map_location = Point{6,6}; }; // NOLINT magic numbers


  return map;
}

Game_Map make_store()
{
  auto map = load_tiled_map("/home/jason/my_awesome_game/resources/tiled/tiles/Store.tmj");

  return map;
}

Game make_game()
{
  Game retval{};
  retval.maps.emplace("main", make_map());
  retval.maps.emplace("store", make_store());
  retval.current_map = "main";
  retval.tile_size = Size{ 8, 8 };// NOLINT Magic Number

  retval.variables["Task"] = "Exit game";
  retval.display_variables.emplace_back("Task");


  Character player;
  player.map_location = { 14, 17 }; // NOLINT Magic Number
  player.draw =
    [](Vector2D_Span<Color> &pixels, [[maybe_unused]] const Game &game, [[maybe_unused]] Point map_location) {

      const auto &tile = game.maps.at("main").tile_sets.front().at(98); // NOLINT magic number
      for (std::size_t cur_x = 0; cur_x < pixels.size().width; ++cur_x) {
        for (std::size_t cur_y = 0; cur_y < pixels.size().height; ++cur_y) {
          pixels.at(Point{cur_x, cur_y}) += tile.at(Point{cur_x, cur_y});
        }
      }
    };


  retval.player = player;

  retval.popup_message =
    "Welcome to 'Learning C++ With Game Hacking Lesson 02'\n\n"
    "Your job, again, is to get into the special square in the bottom right corner of the map, and you'll need to "
    "modify the code.\n\n"
    "We need to talk about 'magic numbers' in this code, but you'll have the fun of changing some colors!";

  return retval;
}
}// namespace lefticus::awesome_game
