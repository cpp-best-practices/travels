#include "game.hpp"
#include "bitmap.hpp"
#include "game_components.hpp"


namespace lefticus::my_awesome_game {
Game_Map make_map()// NOLINT cognitive complexity
{
  Game_Map map{ Size{ 10, 10 } };// NOLINT magic numbers

  auto solid_draw = []([[maybe_unused]] Vector2D_Span<Color> &pixels,
                      [[maybe_unused]] const Game &game,
                      [[maybe_unused]] Point map_location) {
    for (std::size_t cur_x = 0; cur_x < pixels.size().width; ++cur_x) {
      for (std::size_t cur_y = 0; cur_y < pixels.size().height; ++cur_y) {
        if (cur_x == 0 || cur_y == 0 || cur_x == pixels.size().width - 1 || cur_y == pixels.size().height - 1) {
          pixels.at(Point{ cur_x, cur_y }) = Color{ 128, 128, 128, 255 };// NOLINT Magic numbers
        } else {
          switch ((game.clock.count() / 1000) % 2) {// NOLINT Magic numbers
          case 0:
            pixels.at(Point{ cur_x, cur_y }) = Color{ 255, 255, 255, 255 };// NOLINT Magic numbers
            break;
          case 1:// NOLINT Magic Numbers
            pixels.at(Point{ cur_x, cur_y }) = Color{ 200, 240, 240, 255 };// NOLINT Magic numbers
            break;
          }
        }
      }
    }
  };

  auto cannot_enter = [](const Game &, Point, Direction) -> bool { return false; };

  auto empty_draw = []([[maybe_unused]] Vector2D_Span<Color> &pixels,
                      [[maybe_unused]] const Game &game,
                      [[maybe_unused]] Point map_location) {
    for (std::size_t cur_x = 0; cur_x < pixels.size().width; ++cur_x) {
      for (std::size_t cur_y = 0; cur_y < pixels.size().height; ++cur_y) {
        pixels.at(Point{ cur_x, cur_y }) = Color{ 10, 10, 10, 255 };// NOLINT Magic numbers
      }
    }
  };

  for (std::size_t cur_x = 0; cur_x < map.locations.size().width; ++cur_x) {
    for (std::size_t cur_y = 0; cur_y < map.locations.size().height; ++cur_y) {
      map.locations.at(Point{ cur_x, cur_y }).draw = empty_draw;
    }
  }

  map.locations.at(Point{ 2, 3 }).draw = solid_draw;
  map.locations.at(Point{ 2, 3 }).can_enter = cannot_enter;
  map.locations.at(Point{ 1, 4 }).draw = solid_draw;
  map.locations.at(Point{ 1, 4 }).can_enter = cannot_enter;
  map.locations.at(Point{ 0, 2 }).draw = solid_draw;
  map.locations.at(Point{ 0, 2 }).can_enter = [](const Game &, Point, Direction direction) {
    return direction == Direction::South;
  };

  map.locations.at(Point{0,3}).enter_action = [](Game &game, Point, Direction) {
    game.last_message = "There is a secret entrance to the north";
  };
  map.locations.at(Point{ 0, 3 }).exit_action = [](Game &game, Point, Direction) {
    game.last_message = "";
  };


  return map;
}

Game make_game()
{
  Game retval;
  retval.maps.emplace("main", make_map());
  retval.current_map = "main";
  retval.tile_size = Size{ 8, 8 };// NOLINT Magic Number

  Character player;
  player.draw = [player_bitmap = load_png("player.png")]([[maybe_unused]] Vector2D_Span<Color> &pixels,
                  [[maybe_unused]] const Game &game,
                  [[maybe_unused]] Point map_location) {
    // with with a fully saturated red at 50% alpha
    for (std::size_t cur_x = 0; cur_x < pixels.size().width; ++cur_x) {
      for (std::size_t cur_y = 0; cur_y < pixels.size().height; ++cur_y) {
        pixels.at(Point{ cur_x, cur_y }) += player_bitmap.at(Point{ cur_x, cur_y });
      }
    }
  };


  retval.player = player;

  return retval;
}
}// namespace lefticus::my_awesome_game