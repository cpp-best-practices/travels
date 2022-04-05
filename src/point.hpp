#ifndef MY_AWESOME_GAME_POINT_HPP
#define MY_AWESOME_GAME_POINT_HPP

#include <cstdint>

namespace lefticus::my_awesome_game {
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
};
}// namespace lefticus::my_awesome_game

#endif// MY_AWESOME_GAME_POINT_HPP
