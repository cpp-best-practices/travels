#ifndef MY_AWESOME_GAME_TILE_SET_HPP
#define MY_AWESOME_GAME_TILE_SET_HPP


#include <cassert>
#include <filesystem>

#include "bitmap.hpp"
#include "color.hpp"


namespace lefticus::awesome_game {

struct Tile_Set
{
  struct Tile_Properties
  {
    bool passable = false;
  };

  Tile_Set(const std::filesystem::path &image, Size tile_size_, std::size_t start_id_)
    : data{ load_png(image) }, tile_size{ tile_size_ },
      sheet_size{ data.size().width / tile_size.width, data.size().height / tile_size.height }, start_id{ start_id_ }
  {}

  // gets a view of the tile at a certain location
  [[nodiscard]] Vector2D_Span<const Color> at(Point point) const
  {
    return Vector2D_Span<const Color>(Point{ point.x * tile_size.width, point.y * tile_size.height }, tile_size, data);
  }

  [[nodiscard]] Vector2D_Span<const Color> at(std::size_t id) const
  {
    const auto id_to_get = id - start_id;
    const auto x = id_to_get % sheet_size.width;
    const auto y = id_to_get / sheet_size.width;

    return at(Point{ x, y });
  }

  std::map<std::size_t, Tile_Properties> properties;

private:
  Vector2D<Color> data;
  Size tile_size;
  Size sheet_size;
  std::size_t start_id;
};

}// namespace lefticus::awesome_game

#endif// MY_AWESOME_GAME_TILE_SET_HPP
