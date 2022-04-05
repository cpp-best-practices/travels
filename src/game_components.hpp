#ifndef MY_AWESOME_GAME_GAME_COMPONENTS_HPP
#define MY_AWESOME_GAME_GAME_COMPONENTS_HPP

#include <functional>
#include <chrono>
#include <map>

#include "color.hpp"
#include "vector2d.hpp"

namespace lefticus::my_awesome_game
{

enum struct Direction { North, South, East, West };

struct Location
{
  std::function<void()> action;
  std::function<void(Vector2D_Span<Color> &, std::chrono::milliseconds, Point)> draw;
  std::function<bool(std::chrono::milliseconds, Point, Direction)> can_enter;
};

struct Character
{
  Point map_location{};
  std::function<void(Vector2D_Span<Color> &, std::chrono::milliseconds, Point)> draw;
};


struct Game_Map
{
  explicit Game_Map(const Size size) : locations{ size } {}
  Vector2D<Location> locations;
  [[nodiscard]] bool can_enter_from(std::chrono::milliseconds game_clock, Point location, Direction from) const
  {
    const auto &map_location = locations.at(location);
    if (map_location.can_enter) {
      return map_location.can_enter(game_clock, location, from);
    } else {
      return true;
    }
  }
};

struct Game
{
  
  std::map<std::string, Game_Map> maps;
  Character character;
  std::function<void (Game &)> start_game;

};

}

#endif// MY_AWESOME_GAME_GAME_COMPONENTS_HPP
