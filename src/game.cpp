#include "game.hpp"
#include "bitmap.hpp"
#include "game_components.hpp"


namespace lefticus::my_awesome_game {
Game_Map make_map()// NOLINT cognitive complexity
{
  Game_Map map{ Size{ 10, 10 } };// NOLINT magic numbers


  auto empty_draw =
    [](Vector2D_Span<Color> &pixels, [[maybe_unused]] const Game &game, [[maybe_unused]] Point map_location) {
      fill(pixels, Color{ 5, 5, 25, 255 });// NOLINT magic number
    };

  auto cannot_enter = [](const Game &, Point, Direction) -> bool { return false; };

  auto water = [](
                 Vector2D_Span<Color> &pixels, [[maybe_unused]] const Game &game, [[maybe_unused]] Point map_location) {
    fill(pixels, Color{ 0, 0, 250, 255 });// NOLINT magic number
  };


  auto wall_draw = []([[maybe_unused]] Vector2D_Span<Color> &pixels,
                     [[maybe_unused]] const Game &game,
                     [[maybe_unused]] Point map_location) {
    static constexpr auto wall_color = Color{ 100, 100, 100, 128 };


    switch ((game.clock.count() / 1000) % 2) {// NOLINT magic number
    case 0:
      fill(pixels, Color{ 64, 128, 64, 255 });// NOLINT magic number
      break;
    case 1:
      fill(pixels, Color{ 128, 64, 64, 255 });// NOLINT magic number
      break;
    }


    if (!game.maps.at(game.current_map).can_enter_from(game, map_location, Direction::East)) {
      fill_line(pixels,
        Point{ pixels.size().width - 1, 0 },
        Point{ pixels.size().width - 1, pixels.size().height - 1 },
        wall_color);
    }

    if (!game.maps.at(game.current_map).can_enter_from(game, map_location, Direction::West)) {
      fill_line(pixels, Point{ 0, 0 }, Point{ 0, pixels.size().height - 1 }, wall_color);
    }

    if (!game.maps.at(game.current_map).can_enter_from(game, map_location, Direction::North)) {
      fill_line(pixels, Point{ 0, 0 }, Point{ pixels.size().width - 1, 0 }, wall_color);
    }

    if (!game.maps.at(game.current_map).can_enter_from(game, map_location, Direction::South)) {
      fill_line(pixels,
        Point{ 0, pixels.size().height - 1 },
        Point{ pixels.size().width - 1, pixels.size().height - 1 },
        wall_color);
    }
  };

  for (std::size_t cur_x = 0; cur_x < map.locations.size().width; ++cur_x) {
    for (std::size_t cur_y = 0; cur_y < map.locations.size().height; ++cur_y) {
      map.locations.at(Point{ cur_x, cur_y }).draw = empty_draw;
    }
  }

  fill_border(map.locations, Location{ {}, {}, water, cannot_enter });

  const auto Flashing_Tile = Location{ {}, {}, wall_draw, cannot_enter };

  constexpr static auto special_location = Point{ 8, 8 };

  map.locations.at(Point{ 3, 4 }) = Flashing_Tile;
  map.locations.at(Point{ 2, 5 }) = Flashing_Tile;// NOLINT magic numbers
  map.locations.at(Point{ 1, 2 }) = Flashing_Tile;
  map.locations.at(Point{ 8, 6 }) = Flashing_Tile;// NOLINT magic numbers
  map.locations.at(Point{ 5, 5 }) = Flashing_Tile;// NOLINT magic numbers

  map.locations.at(Point{ 2, 1 }).enter_action = [](Game &game, Point, Direction) {
    game.last_message = "Hint: go to location {4,3}";
  };

  map.locations.at(Point{ 4, 3 }).enter_action = [](Game &game, Point, Direction) {
    game.last_message = "Hint: go to location {8,8}";
  };

  map.locations.at(Point{ 7, 7 }).enter_action// NOLINT
    = [](Game &game, Point, Direction) { game.last_message = "A wall is blocking your way"; };
  map.locations.at(Point{ 8, 7 }).enter_action// NOLINT
    = [](Game &game, Point, Direction) { game.last_message = "You need to remove the wall"; };
  map.locations.at(Point{ 7, 8 }).enter_action// NOLINT
    = [](Game &game, Point, Direction) { game.last_message = "Look for 'special_location' in the source code"; };

  map.locations.at(special_location) = Flashing_Tile;
  map.locations.at(special_location).can_enter = [](const Game &, Point, Direction direction) {
    return direction == Direction::South || direction == Direction::East;
  };

  map.locations.at(special_location).exit_action = [](Game &game, Point, Direction) { game.last_message = ""; };

  map.locations.at(special_location).enter_action = [](Game &game, Point, Direction) {
    game.last_message = "You found the secret room!";
    Menu menu;
    menu.items.push_back(
      Menu::MenuItem{ "Continue Game", [](Game &menu_action_game) { menu_action_game.clear_menu(); } });
    menu.items.push_back(
      Menu::MenuItem{ "Exit Game", [](Game &menu_action_game) { menu_action_game.exit_game = true; } });
    game.set_menu(menu);
  };


  return map;
}

Game make_game()
{
  Game retval{};
  retval.maps.emplace("main", make_map());
  retval.current_map = "main";
  retval.tile_size = Size{ 8, 8 };// NOLINT Magic Number

  retval.variables["Task"] = "Exit game";
  retval.display_variables.emplace_back("Task");

  Character player;
  player.map_location = { 1, 1 };
  player.draw =
    [](Vector2D_Span<Color> &pixels, [[maybe_unused]] const Game &game, [[maybe_unused]] Point map_location) {
      for (std::size_t cur_x = 2; cur_x < pixels.size().width - 2; ++cur_x) {
        for (std::size_t cur_y = 2; cur_y < pixels.size().height - 2; ++cur_y) {
          if ((cur_x == 2 && cur_y == 2) || (cur_x == 2 && cur_y == pixels.size().height - 3)
              || (cur_x == pixels.size().width - 3 && cur_y == pixels.size().height - 3)
              || (cur_x == pixels.size().width - 3 && cur_y == 2)) {
            pixels.at(Point{ cur_x, cur_y }) += Color{ 128, 128, 0, 64 };// NOLINT
          } else {
            pixels.at(Point{ cur_x, cur_y }) += Color{ 128, 128, 0, 255 };// NOLINT
          }
        }
      }
    };


  retval.player = player;

  retval.popup_message =
    "Welcome to 'Learning C++ With Game Hacking!' Your job is to get into the special square in the bottom right "
    "corner of the map. But to do that you'll need to modify the source code!";

  return retval;
}
}// namespace lefticus::my_awesome_game