#ifndef MY_AWESOME_GAME_TILE_SET_HPP
#define MY_AWESOME_GAME_TILE_SET_HPP


#include <cassert>
#include <filesystem>

#include "bitmap.hpp"
#include "color.hpp"


namespace lefticus::travels {

struct Tile_Set
{
  struct Animation
  {
    struct Frame
    {
      std::chrono::milliseconds duration;
      std::size_t tile_id;
    };
    std::vector<Frame> frames;
    [[nodiscard]] std::size_t get_frame(std::chrono::milliseconds clock) const
    {

      const auto animation_length = std::ranges::fold_left(
          frames | std::views::transform(std::mem_fn(&Frame::duration)),
          std::chrono::milliseconds{0},
          std::plus<>{}
          );

      auto offset = clock % animation_length;
      for (const auto &frame : frames) {
        if (offset <= frame.duration) { return frame.tile_id; }
        offset -= frame.duration;
      }
      // something went wrong
      return frames[0].tile_id;
    }
  };


  Tile_Set(const std::filesystem::path &image,
    Size tile_size_,
    std::size_t start_id_,
    std::map<std::size_t, Animation> animations_)
    : data{ load_png(image) }, tile_size{ tile_size_ }, sheet_size{ data.size().width / tile_size.width,
        data.size().height / tile_size.height },
      start_id{ start_id_ }, animations{ std::move(animations_) }
  {}

  // gets a view of the tile at a certain location
  [[nodiscard]] Vector2D_Span<const Color> at(Point point) const
  {
    return Vector2D_Span<const Color>(Point{ point.x * tile_size.width, point.y * tile_size.height }, tile_size, data);
  }

  [[nodiscard]] Vector2D_Span<const Color> at(std::size_t id, std::chrono::milliseconds clock) const
  {
    const auto id_to_get = id - start_id;

    if (animations.contains(id_to_get)) { return at(animations.at(id_to_get).get_frame(clock) + start_id); }

    return at(id);
  }

  [[nodiscard]] Vector2D_Span<const Color> at(std::size_t id) const
  {
    const auto id_to_get = id - start_id;

    const auto x = id_to_get % sheet_size.width;
    const auto y = id_to_get / sheet_size.width;

    return at(Point{ x, y });
  }


private:
  Vector2D<Color> data;
  Size tile_size;
  Size sheet_size;
  std::size_t start_id;
  std::map<std::size_t, Animation> animations;
};

}// namespace lefticus::travels

#endif// MY_AWESOME_GAME_TILE_SET_HPP
