#include "game_hacking_lesson_00.hpp"
#include "bitmap.hpp"
#include "game_components.hpp"


namespace lefticus::awesome_game::hacking::lesson_00 {
Game_Map make_map()// NOLINT cognitive complexity
{
  Game_Map map{ Size{ 10, 10 } };// NOLINT magic numbers


  auto empty_draw = [](Vector2D_Span<Color> &pixels,
                      [[maybe_unused]] const Game &game,
                      [[maybe_unused]] Point map_location,
                      Layer layer) {
    if (layer == Layer::Foreground) { return; }
    // just a grey
    fill(pixels, Color{ 25, 25, 25, 255 });// NOLINT magic number
  };

  auto cannot_enter = [](const Game &, Point, Direction) -> bool { return false; };

  auto water_draw = [](Vector2D_Span<Color> &pixels,
                      [[maybe_unused]] const Game &game,
                      [[maybe_unused]] Point map_location,
                      Layer layer) {
    if (layer == Layer::Foreground) { return; }
    fill(pixels, Color{ 0, 0, 250, 255 });// NOLINT magic number
  };


  auto wall_draw = []([[maybe_unused]] Vector2D_Span<Color> &pixels,
                     [[maybe_unused]] const Game &game,
                     [[maybe_unused]] Point map_location,
                     Layer layer) {
    if (layer == Layer::Foreground) { return; }

    static constexpr auto wall_color = Color{ 100, 100, 100, 128 };

    // We fill in the wall with a color, the color alternates by the second
    // game clock is milliseconds (1000 per second)
    // divide by 1000 to get the current second, the % 2
    // gives you the remainder of current second divided by 2.
    // Result is that even numbered seconds have the first color
    // and odd numbered seconds have the second color
    switch ((game.clock.count() / 1000) % 2) {// NOLINT magic number
    case 0:
      fill(pixels, Color{ 64, 128, 64, 255 });// NOLINT magic number
      break;
    case 1:
      fill(pixels, Color{ 128, 64, 64, 255 });// NOLINT magic number
      break;
    }


    // Fill each of the 4 walls with the wall color if the wall is closed.
    // if the wall is open (can_enter_from) that direction, the do not draw the wall
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

  // the entire border of the map is surrounded by water
  fill_border(map.locations, Location{ {}, {}, water_draw, cannot_enter });


  const auto Flashing_Tile = Location{ {}, {}, wall_draw, cannot_enter };

  constexpr static auto special_location = Point{ 8, 8 };

  // Fill in the map locations with flashing tiles and hints

  map.locations.at(Point{ 3, 4 }) = Flashing_Tile;
  map.locations.at(Point{ 2, 5 }) = Flashing_Tile;// NOLINT magic numbers
  map.locations.at(Point{ 1, 2 }) = Flashing_Tile;
  map.locations.at(Point{ 8, 6 }) = Flashing_Tile;// NOLINT magic numbers
  map.locations.at(Point{ 5, 5 }) = Flashing_Tile;// NOLINT magic numbers

  map.locations.at(Point{ 2, 1 }).enter_action = [](Game &game, Point, Direction) {
    game.last_message = "Hint: you'll have to edit code. go to location {4,3}";
  };

  map.locations.at(Point{ 4, 3 }).enter_action = [](Game &game, Point, Direction) {
    game.last_message = "Hint: You need file game_hacking_lesson_00.cpp. go to location {8,8}";
  };

  map.locations.at(Point{ 7, 7 }).enter_action// NOLINT
    = [](Game &game, Point, Direction) { game.last_message = "A wall is blocking your way"; };
  map.locations.at(Point{ 8, 7 }).enter_action// NOLINT
    = [](Game &game, Point, Direction) { game.last_message = "You need to remove the wall"; };
  map.locations.at(Point{ 7, 8 }).enter_action// NOLINT
    = [](Game &game, Point, Direction) {
        game.last_message = fmt::format("Look for 'special_location' ({}:{})", __FILE__, __LINE__);
      };

  map.locations.at(special_location) = Flashing_Tile;
  map.locations.at(special_location).can_enter = [](const Game &, Point, [[maybe_unused]] Direction direction) {
    // || means "or"
    // this means you can currently enter the code from either
    // the South || (or) the East...
    // but you need to be able to enter from the North or the West!
    //
    //        North
    //         ---
    //  West  |   |  East
    //         ---
    //        South
    //
    // Try changing the code below and see how the game changes
    return direction == Direction::South || direction == Direction::East;
  };

  map.locations.at(special_location).enter_action = [](Game &game, Point, Direction) {
    game.last_message = "You found the secret room! Now change the call to `play_game` to start lesson 01";
    Menu menu;
    menu.items.emplace_back("Continue Game", [](Game &menu_action_game) { menu_action_game.clear_menu(); });
    menu.items.emplace_back("Exit Game", [](Game &menu_action_game) { menu_action_game.exit_game = true; });
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

  // player's starting map location is {1,1}
  player.map_location = { 1, 1 };

  // Draw a circle-like thing for the player
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
    "Welcome to 'Learning C++ With Game Hacking Lesson 00'\n\n"
    "This lesson is to just get started with the project, but you will also learn some basic logic and how to get "
    "around the source code.\n\n"
    "Your job is to get into the special square in the bottom right corner of the map.\n\n"
    "But to do that you'll need to modify the source code!\n"
    "(Be sure to look for clues in the bottom message box :) )";

  return retval;
}
}// namespace lefticus::awesome_game::hacking::lesson_00