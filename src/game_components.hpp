#ifndef MY_AWESOME_GAME_GAME_COMPONENTS_HPP
#define MY_AWESOME_GAME_GAME_COMPONENTS_HPP

#include <functional>
#include <chrono>
#include <map>
#include <optional>
#include <variant>

#include "color.hpp"
#include "vector2d.hpp"

namespace lefticus::my_awesome_game
{

struct Game;

enum struct Direction { North, South, East, West };

struct Location
{
  std::function<void(Game &, Point, Direction)> enter_action;
  std::function<void(Game &, Point, Direction)> exit_action;
  std::function<void(Vector2D_Span<Color> &, const Game &, Point)> draw;
  std::function<bool(const Game &, Point, Direction)> can_enter;
};

struct Character
{
  Point map_location{};
  std::function<void(Vector2D_Span<Color> &, const Game &, Point)> draw;
};


struct Game_Map
{
  explicit Game_Map(const Size size) : locations{ size } {}
  Vector2D<Location> locations;
  [[nodiscard]] bool can_enter_from(const Game &game, Point location, Direction from) const
  {
    const auto &map_location = locations.at(location);
    if (map_location.can_enter) {
      return map_location.can_enter(game, location, from);
    } else {
      return true;
    }
  }
};

using Variable = std::variant<double, std::int64_t, std::string>;

struct Game
{

  std::map<std::string, Game_Map> maps;
  Character player;
  std::function<void (Game &)> start_game;

  std::map<std::string, Variable> variables;
  std::vector<std::string> display_variables;
  std::string current_map;
  std::chrono::milliseconds clock;
  Size tile_size;

  std::string last_message;
};

}

#endif// MY_AWESOME_GAME_GAME_COMPONENTS_HPP
