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

  void teleport_to_2d(engine_type::string_type map_name, engine_type::string_type location_name)
  {
    game.teleport_to_2d(std::string(engine.strings.view(map_name)), std::string(engine.strings.view(location_name)));
  }

  void teleport_to_3d(engine_type::string_type map_name, engine_type::string_type location_name, float angle)
  {
    game.teleport_to_3d(
      std::string(engine.strings.view(map_name)), std::string(engine.strings.view(location_name)), angle);
  }

  void set_3d_location_action(engine_type::string_type map_name,
    engine_type::string_type location_name,
    engine_type::SExpr callable)
  {
    auto &map = game.maps_3d.at(std::string(engine.strings.view(map_name)));
    const char location_char = std::string(engine.strings.view(location_name)).front();


    map.enter_actions.emplace(location_char,
      [this, callable = engine.make_callable<void(Scripted_Game *)>(callable)](
        lefticus::travels::Game &, char) { callable(engine, this); });
  }

  void set_location_action(engine_type::string_type map_name,
    engine_type::string_type location_name,
    engine_type::SExpr callable)
  {
    auto &map = game.maps.at(std::string(engine.strings.view(map_name)));
    map.locations.at(map.location_names[std::string(engine.strings.view(location_name))]).enter_action =
      [this, callable = engine.make_callable<void(Scripted_Game *)>(callable)](
        lefticus::travels::Game &, lefticus::travels::Point, lefticus::travels::Direction) { callable(engine, this); };
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
