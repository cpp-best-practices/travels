#include "game_components.hpp"
#include "tile_set.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace lefticus::awesome_game {

Game_Map load_tiled_map(const std::filesystem::path &map_json)// NOLINT cofnitive complexity
{
  const auto parent_path = map_json.parent_path();

  const auto load_json = [](const std::filesystem::path &json_file) {
    spdlog::debug("Loading JSON: {}", json_file.string());
    std::ifstream input(json_file);
    if (!input.good()) { spdlog::error("Unable to open JSON file"); }
    nlohmann::json json;
    input >> json;
    return json;
  };

  const auto map_file = load_json(map_json);

  Size tile_size{ map_file["tilewidth"], map_file["tileheight"] };
  Size map_size{ map_file["width"], map_file["height"] };

  Game_Map map{ map_size };

  map.tile_sets = [&] {
    std::vector<Tile_Set> result;
    for (const auto &tileset : map_file["tilesets"]) {
      const std::size_t start_gid = tileset["firstgid"];
      const std::filesystem::path tsj_path = tileset["source"];

      const auto tsj = load_json(parent_path / tsj_path);
      const std::filesystem::path tsj_image_path = tsj["image"];
      result.emplace_back(parent_path / tsj_image_path, tile_size, start_gid);

      for (const auto &tile : tsj["tiles"]) {
        std::size_t id = tile["id"];

        bool passable = true;

        if (tile.contains("properties")) {
          for (const auto &property : tile["properties"]) {
            if (property["name"] == "passable") { passable = property["value"]; }// cppcheck-suppress useStlAlgorithm
          }
        }

        result.back().properties[start_gid + id] = Tile_Set::Tile_Properties{ .passable = passable };
      }
    }
    return result;
  }();


  struct Layer_Info
  {
    // cppcheck is simply wrong here

    std::size_t tileid;// cppcheck-suppress unusedStructMember
    bool background;// cppcheck-suppress unusedStructMember
    bool foreground;// cppcheck-suppress unusedStructMember
  };

  std::map<Point, std::vector<Layer_Info>> points;

  for (const auto &layer : map_file["layers"]) {
    if (layer["type"] == "tilelayer" && layer["visible"] == true) {
      // std::size_t x = layer["x"];
      // std::size_t y = layer["y"];
      const std::size_t width = layer["width"];
      // std::size_t height = layer["height"];

      bool background = false;
      bool foreground = false;

      if (layer.contains("properties")) {
        for (const auto &property : layer["properties"]) {
          if (property["name"] == "foreground") {
            foreground = property["value"];
          } else if (property["name"] == "background") {
            background = property["value"];
          }
        }
      }

      std::size_t x = 0;
      std::size_t y = 0;
      for (const auto &tile : layer["data"]) {

        std::size_t tileid = tile;

        points[Point{ x, y }].push_back(
          Layer_Info{ .tileid = tileid, .background = background, .foreground = foreground });

        ++x;
        if (x == width) {
          x = 0;
          ++y;
        }
      }
    }
  }


  for (const auto &[point, tile_data] : points) {
    map.locations.at(point).draw = [tiles = tile_data](
                                     Vector2D_Span<Color> &pixels, const Game &game, Point, Layer layer) {
      const auto &tile_sets = game.get_current_map().tile_sets;
      bool first_tile = true;
      for (const auto &tile : tiles) {
        if (tile.tileid == 0) { continue; }

        if ((layer == Layer::Background && !tile.foreground) || (layer == Layer::Foreground && tile.foreground)) {
          const auto &tile_pixels = tile_sets[0].at(tile.tileid);
          for (std::size_t y = 0; y < pixels.size().height; ++y) {
            for (std::size_t x = 0; x < pixels.size().width; ++x) {
              Point current_pixel{ x, y };

              if (first_tile && !tile.foreground) {
                pixels.at(current_pixel) = tile_pixels.at(current_pixel);
              } else {
                pixels.at(current_pixel) += tile_pixels.at(current_pixel);
              }
            }
          }
          first_tile = false;
        }
      }
    };


    map.locations.at(point).can_enter = [tiles = tile_data](const Game &game, Point, Direction) {
      const auto &tile_sets = game.get_current_map().tile_sets;
      return std::ranges::all_of(tiles, [&](const auto &tile) {
        return tile.foreground || tile.background || tile.tileid == 0
               || tile_sets[0].properties.at(tile.tileid).passable;
      });
    };
  }

  return map;
}

Menu::MenuItem::MenuItem(std::string text_,
  std::function<void(Game &)> action_,
  std::function<bool(const Game &)> visible_)
  : text{ std::move(text_) }, action{ std::move(action_) }, visible{ std::move(visible_) }
{}

Menu::MenuItem::MenuItem(std::string text_, std::string message_, std::function<bool(const Game &)> visible_)
  : MenuItem(
    std::move(text_),
    [message = std::move(message_)](Game &game) { game.popup_message = message; },
    std::move(visible_))
{}

Menu::MenuItem exit_menu()
{
  return { "Exit", [](Game &game) { game.clear_menu(); } };
}

Menu::MenuItem set_flag(std::string text, std::string message, variable var)
{
  return { std::move(text), [message = std::move(message), var = std::move(var)](Game &game) {
            game.popup_message = message;
            if (game.variables[var.name] != Variable{ true }) {
              game.variables[var.name] = true;
              game.set_menu(game.get_menu());
            }
          } };
}

Menu::MenuItem check_flag(std::string text, std::string message, variable var)
{
  return { std::move(text), std::move(message), std::move(var) == true };
}


}// namespace lefticus::awesome_game