#ifndef MY_AWESOME_GAME_GAME_COMPONENTS_HPP
#define MY_AWESOME_GAME_GAME_COMPONENTS_HPP

#include <chrono>
#include <functional>
#include <map>
#include <optional>
#include <variant>

#include "color.hpp"
#include "vector2d.hpp"

namespace lefticus::my_awesome_game {

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

struct Menu
{
  struct MenuItem
  {
    std::string text;
    std::function<void(Game &)> action;
  };

  std::vector<MenuItem> items;
};

using Variable = std::variant<double, std::int64_t, std::string, bool>;

inline std::string to_string(const Variable &variable)
{
  return std::visit([](const auto &value) { return fmt::format("{}", value); }, variable);
}

struct Game
{

  std::map<std::string, Game_Map> maps;
  Character player;
  std::function<void(Game &)> start_game;

  std::map<std::string, Variable> variables;
  std::vector<std::string> display_variables;
  std::string current_map;
  std::chrono::milliseconds clock;
  Size tile_size;

  std::string last_message;
  std::string popup_message;


  bool exit_game = false;

  [[nodiscard]] bool has_popup_message() const { return !popup_message.empty(); }

  [[nodiscard]] bool has_new_menu() const { return menu_is_new; }

  [[nodiscard]] bool has_menu() const { return static_cast<bool>(menu); }

  [[nodiscard]] Menu get_menu()
  {
    if (menu) {
      menu_is_new = false;
      return *menu;
    } else {
      return Menu{};
    }
  }

  void set_menu(Menu menu_)
  {
    menu_is_new = true;
    menu = std::move(menu_);
  }

  void clear_menu()
  {
    menu_is_new = false;
    menu.reset();
  }

private:
  std::optional<Menu> menu;
  bool menu_is_new = false;
};

}// namespace lefticus::my_awesome_game

#endif// MY_AWESOME_GAME_GAME_COMPONENTS_HPP
