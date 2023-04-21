#include "game_components.hpp"
#include "tile_set.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#ifdef _MSC_VER
#pragma warning(disable : 4189)
#endif
#include <spdlog/spdlog.h>
#ifdef _MSC_VER
#pragma warning(default : 4189)
#endif

namespace lefticus::travels {

Game_Map load_tiled_map(const std::filesystem::path &map_json, const std::vector<std::filesystem::path> &search_paths)
{
  if (map_json.is_absolute()) {
    spdlog::warn("Passed an absolute map_json path and a set of search paths, that doesn't make sense");
    return load_tiled_map(map_json);
  }

  for (const auto &search_path : search_paths) {
    spdlog::debug("Trying search path '{}'", search_path.string());
    const auto path = search_path / map_json;
    if (std::error_code error; std::filesystem::is_regular_file(path, error)) { return load_tiled_map(path); }
  }

  throw std::runtime_error(fmt::format("Unable to find map in any search path: {}", map_json.string()));
}


Game_Map load_tiled_map(const std::filesystem::path &map_json)// NOLINT cognitive complexity
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

  const Size tile_size{ map_file["tilewidth"], map_file["tileheight"] };
  const Size map_size{ map_file["width"], map_file["height"] };

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
        const std::size_t tile_id = tile["id"];

        bool passable = true;

        if (tile.contains("properties")) {
          for (const auto &property : tile["properties"]) {
            // cppcheck-suppress useStlAlgorithm
            if (property["name"] == "passable") { passable = property["value"]; }
          }
        }

        result.back().properties[start_gid + tile_id] = Tile_Set::Tile_Properties{ .passable = passable };
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

      std::size_t cur_x = 0;
      std::size_t cur_y = 0;
      for (const auto &tile : layer["data"]) {

        const std::size_t tileid = tile;

        points[Point{ cur_x, cur_y }].push_back(
          Layer_Info{ .tileid = tileid, .background = background, .foreground = foreground });

        ++cur_x;
        if (cur_x == width) {
          cur_x = 0;
          ++cur_y;
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
          for (std::size_t cur_y = 0; cur_y < pixels.size().height; ++cur_y) {
            for (std::size_t cur_x = 0; cur_x < pixels.size().width; ++cur_x) {
              const Point current_pixel{ cur_x, cur_y };

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
      return std::all_of(tiles.begin(), tiles.end(), [&](const auto &tile) {
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


}// namespace lefticus::travels
