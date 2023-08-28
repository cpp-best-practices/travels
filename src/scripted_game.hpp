#include "bitmap.hpp"
#include "game.hpp"
#include "game_components.hpp"
#include <cons_expr/cons_expr.hpp>
#include <cons_expr/utility.hpp>

struct Scripted_Game
{
  using engine_type = lefticus::cons_expr<std::size_t, char, int, float, 128, 2048, 512, Scripted_Game *>;

  lefticus::travels::Game game;

  void load_tiled_map(engine_type::string_type name, engine_type::string_type path)
  {
    game.maps.emplace(std::string(engine.strings.view(name)),
      lefticus::travels::load_tiled_map(engine.strings.view(path), m_search_paths));
  }

  void load_3d_map(engine_type::string_type name, engine_type::string_type path)
  {
    game.maps_3d.emplace(std::string(engine.strings.view(name)),
      lefticus::travels::load_3d_map(engine.strings.view(path), m_search_paths));
  }
  void show_popup(engine_type::string_type name)
  {
    game.popup_message = std::string(engine.strings.view(name));
  }

  void set_menu(engine_type::list_type list)
  {
    lefticus::travels::Menu menu;
    for (const auto &menu_item : engine.values[list]) {
      if (const auto *item = engine.get_if<engine_type::list_type>(&menu_item); item != nullptr) {
        if (item->size >= 2) {
          const auto *text = engine.get_if<engine_type::string_type>(&engine.values[(*item)[0]]);
          const auto *message = engine.get_if<engine_type::string_type>(&engine.values[(*item)[1]]);

          if (text != nullptr && message != nullptr) {

            std::function<bool(const lefticus::travels::Game &)> test{ [](const auto &) { return true; } };
            std::function<void(lefticus::travels::Game &)> action{ [msg = std::string{ engine.strings.view(*message) }](
                                                                     lefticus::travels::Game &game_) {
              if (!msg.empty()) { game_.popup_message = msg; }
            } };

            for (const auto &element : engine.values[item->sublist(2)]) {
              if (const auto *element_list = engine.get_if<engine_type::list_type>(&element); element_list != nullptr) {
                const auto *identifier =
                  engine.get_if<engine_type::identifier_type>(&engine.values[(*element_list)[0]]);
                const auto action_or_test = engine.values[(*element_list)[1]];

                if (identifier != nullptr) {
                  if (engine.strings.view(identifier->value) == "action") {
                    action = [this,
                               action,
                               callable = engine.make_callable<void()>(
                                 engine.eval(engine.global_scope, action_or_test))](lefticus::travels::Game &game_) {
                      action(game_);
                      callable(engine);
                    };
                  } else if (engine.strings.view(identifier->value) == "test") {
                    test = [this,
                             test,
                             callable = engine.make_callable<bool()>(engine.eval(engine.global_scope, action_or_test))](
                             const lefticus::travels::Game &game_) -> bool {
                      auto result = callable(engine);
                      return test(game_) && result.value();
                    };
                  }
                }
              }
            }
            menu.items.emplace_back(std::string(engine.strings.view(*text)), action, test);
          }
        }
      }
    }
    menu.items.emplace_back(lefticus::travels::exit_menu());

    game.set_menu(menu);
  }

  void teleport_to_2d(engine_type::string_type map_name, engine_type::string_type location_name)
  {
    game.teleport_to_2d(std::string(engine.strings.view(map_name)), std::string(engine.strings.view(location_name)));
  }

  void teleport_to_3d(engine_type::string_type map_name, engine_type::string_type location_name, float angle)
  {
    game.teleport_to_3d(
      std::string(engine.strings.view(map_name)), std::string(engine.strings.view(location_name)), angle);
  }

  bool get_boolean(engine_type::string_type bool_name)
  {
    auto var = game.variables[std::string(engine.strings.view(bool_name))];
    if (std::holds_alternative<bool>(var)) {
      return std::get<bool>(var);
    } else {
      return false;
    }
  }

  void set_boolean(engine_type::string_type bool_name, const bool value)
  {
    game.variables[std::string(engine.strings.view(bool_name))] = value;
  }

  void set_3d_location_action(engine_type::string_type map_name,
    engine_type::string_type location_name,
    engine_type::SExpr callable)
  {
    auto &map = game.maps_3d.at(std::string(engine.strings.view(map_name)));
    const char location_char = std::string(engine.strings.view(location_name)).front();


    map.enter_actions.emplace(location_char,
      [this, callable = engine.make_callable<void()>(callable)](lefticus::travels::Game &, char) { callable(engine); });
  }

  void set_location_action(engine_type::string_type map_name,
    engine_type::string_type location_name,
    engine_type::SExpr callable)
  {
    auto &map = game.maps.at(std::string(engine.strings.view(map_name)));
    map.locations.at(map.location_names[std::string(engine.strings.view(location_name))]).enter_action =
      [this, callable = engine.make_callable<void()>(callable)](
        lefticus::travels::Game &, lefticus::travels::Point, lefticus::travels::Direction) { callable(engine); };
  }

  void eval(const std::string_view script)
  {
    const auto result =
      engine.sequence(engine.global_scope, std::get<engine_type::list_type>(engine.parse(script).first.value));

    const auto result_string = lefticus::to_string(engine, true, result);
    spdlog::trace("script executed '{}': '{}'", script, result_string);
  }


  static consteval engine_type make_engine()
  {
    engine_type engine;
    engine.add<&Scripted_Game::load_tiled_map>("load_tiled_map");
    engine.add<&Scripted_Game::teleport_to_2d>("teleport_to_2d");
    engine.add<&Scripted_Game::set_location_action>("set_location_action");
    engine.add<&Scripted_Game::set_3d_location_action>("set_3d_location_action");
    engine.add<&Scripted_Game::teleport_to_3d>("teleport_to_3d");
    engine.add<&Scripted_Game::load_3d_map>("load_3d_map");
    engine.add<&Scripted_Game::set_menu>("set_menu");
    engine.add<&Scripted_Game::get_boolean>("get_boolean");
    engine.add<&Scripted_Game::set_boolean>("set_boolean");
    engine.add<&Scripted_Game::show_popup>("show_popup");
    return engine;
  }

  explicit Scripted_Game(std::vector<std::filesystem::path> search_paths) : m_search_paths{ std::move(search_paths) }
  {
    engine.add("game", this);
    game.tile_size = lefticus::travels::Size{ 8, 8 };// NOLINT Magic Number

    lefticus::travels::Character player;

    player.draw = [](lefticus::travels::Vector2D_Span<lefticus::travels::Color> &pixels,
                    [[maybe_unused]] const lefticus::travels::Game &cur_game,
                    [[maybe_unused]] lefticus::travels::Point map_location) {
      const auto &tile = cur_game.maps.at("main").tile_sets.front().at(98);// NOLINT magic number
      for (std::size_t cur_x = 0; cur_x < pixels.size().width; ++cur_x) {
        for (std::size_t cur_y = 0; cur_y < pixels.size().height; ++cur_y) {
          pixels.at(lefticus::travels::Point{ cur_x, cur_y }) += tile.at(lefticus::travels::Point{ cur_x, cur_y });
        }
      }
    };


    game.player = player;
  }

  engine_type engine = make_engine();
  std::vector<std::filesystem::path> m_search_paths;
};
