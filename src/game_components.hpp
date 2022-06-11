#ifndef AWESOME_GAME_GAME_COMPONENTS_HPP
#define AWESOME_GAME_GAME_COMPONENTS_HPP

#include <chrono>
#include <filesystem>
#include <fmt/format.h>
#include <functional>
#include <map>
#include <optional>
#include <variant>

#include "color.hpp"
#include "tile_set.hpp"
#include "vector2d.hpp"

namespace lefticus::travels {

struct Game;

enum struct Direction { North, South, East, West };
enum struct Layer { Background, Foreground };

struct Location
{
  std::function<void(Game &, Point, Direction)> enter_action;
  std::function<void(Game &, Point, Direction)> exit_action;
  std::function<void(Vector2D_Span<Color> &, const Game &, Point, Layer)> draw;
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

  std::vector<Tile_Set> tile_sets;

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

Game_Map load_tiled_map(const std::filesystem::path &map_json, const std::vector<std::filesystem::path> &search_paths);
Game_Map load_tiled_map(const std::filesystem::path &map_json);


using Variable = std::variant<double, std::int64_t, std::string, bool>;

template<typename Comparitor> struct Variable_Comparison
{
  Comparitor comparitor;

  bool operator()(const Game &game) const { return comparitor(game); }
};

template<typename T> Variable_Comparison(T t) -> Variable_Comparison<T>;


template<typename LHS, typename RHS> auto operator&&(Variable_Comparison<LHS> lhs, Variable_Comparison<RHS> rhs)
{
  return Variable_Comparison{ [lhs = std::move(lhs.comparitor), rhs = std::move(rhs.comparitor)](
                                const Game &game) { return lhs(game) && rhs(game); } };
}

template<typename LHS, typename RHS> auto operator||(Variable_Comparison<LHS> lhs, Variable_Comparison<RHS> rhs)
{
  return Variable_Comparison{ [lhs = std::move(lhs.comparitor), rhs = std::move(rhs.comparitor)](
                                const Game &game) { return lhs(game) || rhs(game); } };
}

struct variable
{
  std::string name;
};


inline std::string to_string(const Variable &variable)
{
  return std::visit([](const auto &value) { return fmt::format("{}", value); }, variable);
}

struct Menu
{
  struct MenuItem
  {
    std::string text;
    std::function<void(Game &)> action;
    std::function<bool(const Game &)> visible;
    MenuItem(std::string text_, std::string message_, std::function<bool(const Game &)> = {});

    template<typename Compare>
    MenuItem(std::string text_, std::string message_, Variable_Comparison<Compare> comp)
      : MenuItem(std::move(text_), std::move(message_), std::function<bool(const Game &)>(std::move(comp.comparitor)))
    {}

    MenuItem(std::string text_, std::function<void(Game &)> action_, std::function<bool(const Game &)> visible_ = {});

    template<typename Compare>
    MenuItem(std::string text_, std::function<void(Game &)> action_, Variable_Comparison<Compare> comp)
      : MenuItem(std::move(text_), std::move(action_), std::function<bool(const Game &)>(std::move(comp.comparitor)))
    {}
  };

  std::vector<MenuItem> items;

  Menu() = default;

  explicit Menu(std::initializer_list<MenuItem> items_) : items{ items_ } {}
};

struct Game
{

  std::map<std::string, Game_Map> maps;
  Character player;
  std::function<void(Game &)> start_game;

  // enable transparent comparators for std::string
  std::map<std::string, Variable, std::less<>> variables;
  std::vector<std::string> display_variables;
  std::string current_map;
  std::chrono::milliseconds clock;
  Size tile_size;

  std::string last_message;
  std::string popup_message;


  bool exit_game = false;

  [[nodiscard]] Game_Map &get_current_map() { return maps.at(current_map); }
  [[nodiscard]] const Game_Map &get_current_map() const { return maps.at(current_map); }

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

// cppcheck is wrong about these wanting to be passed by const &.
// that is because these are sinks

template<typename Value> auto operator==(variable var, Value value)
{
  return Variable_Comparison{ [name = std::move(var).name, value = Variable{ std::move(value) }](
                                const Game &game) { return game.variables.at(name) == value; } };
}

template<typename Value> auto operator==(Value value, variable var)
{
  return Variable_Comparison{ [name = std::move(var).name, value = Variable{ std::move(value) }](
                                const Game &game) { return value == game.variables.at(name); } };
}


template<typename Value> auto operator!=(variable var, Value value)
{
  return Variable_Comparison{ [name = std::move(var).name, value = Variable{ std::move(value) }](
                                const Game &game) { return game.variables.at(name) != value; } };
}

template<typename Value> auto operator!=(Value value, variable var)
{
  return Variable_Comparison{ [name = std::move(var).name, value = Variable{ std::move(value) }](
                                const Game &game) { return value != game.variables.at(name); } };
}

template<typename Value> auto operator<(variable var, Value value)
{
  return Variable_Comparison{ [name = std::move(var).name, value = Variable{ std::move(value) }](
                                const Game &game) { return game.variables.at(name) < value; } };
}

template<typename Value> auto operator<(Value value, variable var)
{
  return Variable_Comparison{ [name = std::move(var).name, value = Variable{ std::move(value) }](
                                const Game &game) { return value < game.variables.at(name); } };
}


template<typename Value> auto operator<=(variable var, Value value)
{
  return Variable_Comparison{ [name = std::move(var).name, value = Variable{ std::move(value) }](
                                const Game &game) { return game.variables.at(name) <= value; } };
}

template<typename Value> auto operator<=(Value value, variable var)
{
  return Variable_Comparison{ [name = std::move(var).name, value = Variable{ std::move(value) }](
                                const Game &game) { return value <= game.variables.at(name); } };
}

template<typename Value> auto operator>(variable var, Value value)
{
  return Variable_Comparison{ [name = std::move(var).name, value = Variable{ std::move(value) }](
                                const Game &game) { return game.variables.at(name) > value; } };
}

template<typename Value> auto operator>(Value value, variable var)
{
  return Variable_Comparison{ [name = std::move(var).name, value = Variable{ std::move(value) }](
                                const Game &game) { return value > game.variables.at(name); } };
}

template<typename Value> auto operator>=(variable var, Value value)
{
  return Variable_Comparison{ [name = std::move(var).name, value = Variable{ std::move(value) }](
                                const Game &game) { return game.variables.at(name) >= value; } };
}

template<typename Value> auto operator>=(Value value, variable var)
{
  return Variable_Comparison{ [name = std::move(var).name, value = Variable{ std::move(value) }](
                                const Game &game) { return value >= game.variables.at(name); } };
}

Menu::MenuItem exit_menu();
Menu::MenuItem set_flag(std::string text, std::string message, variable var);
Menu::MenuItem check_flag(std::string text, std::string message, variable var);

}// namespace lefticus::travels

#endif// AWESOME_GAME_GAME_COMPONENTS_HPP
