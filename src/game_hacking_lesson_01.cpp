#include "game_hacking_lesson_01.hpp"
#include "bitmap.hpp"
#include "game_components.hpp"


namespace lefticus::travels::hacking::lesson_01 {

bool button_pressed(const Game &game) { return get<bool>(game.variables.at("ButtonPressed")); }

bool &button_pressed(Game &game) { return get<bool>(game.variables.at("ButtonPressed")); }


Game_Map make_map()// NOLINT cognitive complexity
{
  Game_Map map{ Size{ 10, 10 } };// NOLINT magic numbers

  auto button_draw = [](Vector2D_Span<Color> &pixels,
                       [[maybe_unused]] const Game &game,
                       [[maybe_unused]] Point map_location,
                       Layer layer) {
    if (layer == Layer::Background) {
      if (button_pressed(game)) {
        // Light grey if button is pressed
        fill(pixels, Color{ 45, 45, 45, 255 });// NOLINT magic number
      } else {
        // dark grey if button is not pressed
        fill(pixels, Color{ 15, 15, 15, 255 });// NOLINT magic number
      }
    }
  };

  auto empty_draw = [](Vector2D_Span<Color> &pixels,
                      [[maybe_unused]] const Game &game,
                      [[maybe_unused]] Point map_location,
                      Layer layer) {
    if (layer == Layer::Background) {
      fill(pixels, Color{ 5, 5, 25, 255 });// NOLINT magic number
    }
  };

  auto cannot_enter = [](const Game &, Point, Direction) -> bool { return false; };

  auto water_draw = [](Vector2D_Span<Color> &pixels,
                      [[maybe_unused]] const Game &game,
                      [[maybe_unused]] Point map_location,
                      Layer layer) {
    if (layer == Layer::Background) {
      fill(pixels, Color{ 0, 0, 250, 255 });// NOLINT magic number
    }
  };


  auto wall_draw = []([[maybe_unused]] Vector2D_Span<Color> &pixels,
                     [[maybe_unused]] const Game &game,
                     [[maybe_unused]] Point map_location,
                     Layer layer) {
    if (layer == Layer::Foreground) { return; }

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
    game.last_message = "A hidden space will activate a button! Go find it!";
  };

  map.locations.at(Point{ 5, 6 }).draw// NOLINT magic numbers
    = button_draw;

  map.locations.at(Point{ 5, 6 }).enter_action// NOLINT magic numbers
    = [](Game &game, Point, Direction) {
        // TODO: Update this to use std::source_location once clang supports it
        game.last_message =
          fmt::format("You need to fix the \"ButtonPressed\" variable code! ({}:{})", __FILE__, __LINE__);
        // `!` means "Not"
        // so if you step on this tile then it will invert the state of the button
        // from true to false and from false to true.
        //
        // don't get too hung up on this weird `std::get<bool>` thing yet!
        //
        // At least...that's the idea. Problem is there's a typo. Do you see it?!
        game.variables["BottonPressed"] = !std::get<bool>(game.variables["ButtonPressed"]);
      };


  map.locations.at(special_location) = Flashing_Tile;
  map.locations.at(special_location).can_enter = [](const Game &game, Point, [[maybe_unused]] Direction direction) {
    return direction == Direction::West && button_pressed(game);
  };

  map.locations.at(special_location).enter_action = [](Game &game, Point, Direction) {
    game.last_message = "You opened the door! Now change the call to `play_game` to start lesson 02";
    const Menu menu{ { "Continue Game", [](Game &menu_action_game) { menu_action_game.clear_menu(); } },
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

  retval.variables["ButtonPressed"] = false;
  retval.display_variables.emplace_back("ButtonPressed");

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
    "Welcome to 'Learning C++ With Game Hacking Lesson 01'\n\n"
    "Your job, again, is to get into the special square in the bottom right corner of the map, and you'll need to "
    "modify the code.\n\n"
    "There is a hidden button, but it's currently broken :(. Fortunately there are clues again.";

  return retval;
}
}// namespace lefticus::travels::hacking::lesson_01
