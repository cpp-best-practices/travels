#ifndef AWESOME_GAME_POINT_HPP
#define AWESOME_GAME_POINT_HPP

#include <cstdint>

namespace lefticus::awesome_game {
struct Point
{
  std::size_t x;
  std::size_t y;

  [[nodiscard]] friend constexpr auto operator+(Point lhs, Point rhs) noexcept
  {
    return Point{ lhs.x + rhs.x, lhs.y + rhs.y };
  }

  [[nodiscard]] friend constexpr auto operator-(Point lhs, Point rhs) noexcept
  {
    return Point{ lhs.x - rhs.x, lhs.y - rhs.y };
  }


  friend auto operator<=>(const Point &, const Point &) = default;
};
}// namespace lefticus::awesome_game

#endif// AWESOME_GAME_POINT_HPP
