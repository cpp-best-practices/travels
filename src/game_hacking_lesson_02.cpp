#include "game_hacking_lesson_02.hpp"
#include "bitmap.hpp"
#include "game_components.hpp"
#include <set>

namespace lefticus::awesome_game::hacking::lesson_02 {

Game_Map make_map()// NOLINT cognitive complexity
{
  Game_Map map{ Size{ 10, 10 } };// NOLINT magic numbers

  auto colors_used = std::make_shared<std::set<Color>>();

  auto empty_draw = [](Vector2D_Span<Color> &pixels,
                      [[maybe_unused]] const Game &game,
                      [[maybe_unused]] Point map_location,
                      Layer layer) {
    if (layer == Layer::Foreground) { return; }
    fill(pixels, Color{ 5, 5, 25, 255 });// NOLINT magic number
  };

  auto cannot_enter = [](const Game &, Point, Direction) -> bool { return false; };

  auto water_draw = [](Vector2D_Span<Color> &pixels,
                      [[maybe_unused]] const Game &game,
                      [[maybe_unused]] Point map_location,
                      Layer layer) {
    if (layer == Layer::Foreground) { return; }
    fill(pixels, Color{ 0, 0, 250, 255 });// NOLINT magic number
  };


  auto wall_draw = [colors_used]([[maybe_unused]] Vector2D_Span<Color> &pixels,
                     [[maybe_unused]] const Game &game,
                     [[maybe_unused]] Point map_location,
                     Layer layer) {
    if (layer == Layer::Foreground) { return; }
    static constexpr auto wall_color = Color{ 100, 100, 100, 128 };

    // this gets the current second on the game's clock
    // then, using `%` divides that by 2, and takes the remainder
    // so if the current second count is even (evenly divisible by 2)
    // then the first color is displayed.
    // If it is odd (remainder of 1), then the second color is displayed.
    //
    // Your goal is to divide this into 3 options and display a 3rd
    // color.
    //
    // Once you have displayed the 3rd color, the door in the
    // bottom right will open!
    //
    // Some things to consider:
    //   * instead of dividing by 2, you probably want to divide by 3 (or more)
    //   * You'll need to add another case statement (what are the 3 possible remainders
    //     if you divide by 3?)
    //   * consider avoiding // NOLINT by actually naming your color!
    //
    // static constexpr auto MyPrettyBlue = Color{10,24, 200, 255}; // for example
    //
    // Also consider naming the other colors used!
    switch ((game.clock.count() / 1000) % 2) {// NOLINT magic number
    case 0:
      fill(pixels, Color{ 64, 128, 64, 255 });// NOLINT magic number
      break;
    case 1:
      fill(pixels, Color{ 128, 64, 64, 255 });// NOLINT magic number
      break;
    }

    colors_used->insert(pixels.at(Point{ 3, 3 }));


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

  // be default everything is an empty, passable location
  fill(map.locations, Location{ {}, {}, empty_draw, {} });

  fill_border(map.locations, Location{ {}, {}, water_draw, cannot_enter });

  const auto Flashing_Tile = Location{ {}, {}, wall_draw, cannot_enter };

  constexpr static auto special_location = Point{ 8, 8 };

  map.locations.at(Point{ 3, 4 }) = Flashing_Tile;
  map.locations.at(Point{ 2, 5 }) = Flashing_Tile;// NOLINT magic numbers
  map.locations.at(Point{ 1, 2 }) = Flashing_Tile;
  map.locations.at(Point{ 8, 6 }) = Flashing_Tile;// NOLINT magic numbers
  map.locations.at(Point{ 5, 5 }) = Flashing_Tile;// NOLINT magic numbers

  map.locations.at(Point{ 2, 1 }).enter_action = [](Game &game, Point, Direction) {
    game.last_message = "What is a Magic Number? {2,3}";
  };

  map.locations.at(Point{ 2, 3 }).enter_action = [](Game &game, Point, Direction) {
    game.last_message = "Magic Number?  https://en.wikipedia.org/wiki/Magic_number_(programming) {4, 3}";
  };

  map.locations.at(Point{ 4, 3 }).enter_action = [](Game &game, Point, Direction) {
    game.last_message = "It's a hard coded constant. This is generally a 'Code Smell'";
  };

  map.locations.at(special_location) = Flashing_Tile;
  map.locations.at(special_location).can_enter =
    [colors_used]([[maybe_unused]] const Game &game, Point, [[maybe_unused]] Direction direction) {
      return colors_used->size() > 2;
    };

  map.locations.at(special_location).enter_action = [](Game &game, Point, Direction) {
    game.last_message = "You opened the door! Now change the call to `play_game` to start lesson 03";
    Menu menu{ { "Continue Game", [](Game &menu_action_game) { menu_action_game.clear_menu(); } },
      { "Exit Game", [](Game &menu_action_game) { menu_action_game.exit_game = true; } } };
    game.set_menu(menu);
  };


  return map;
}

Game make_lesson()
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
            pixels.at(Point{ cur_x, cur_y }) += Color{ 128, 128, 0, 64 };// NOLINT Magic Number
          } else {
            pixels.at(Point{ cur_x, cur_y }) += Color{ 128, 128, 0, 255 };// NOLINT Magic Number
          }
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
}// namespace lefticus::awesome_game::hacking::lesson_02