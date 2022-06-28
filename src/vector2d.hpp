#ifndef AWESOME_GAME_VECTOR2D_HPP
#define AWESOME_GAME_VECTOR2D_HPP

#include <fmt/format.h>
#include <stdexcept>
#include <vector>

#include "point.hpp"
#include "size.hpp"


namespace lefticus::travels {


template<typename Contained> class Vector2D
{
  Size size_;
  std::vector<Contained> data_ = decltype(data_)(size_.width * size_.height, Contained{});


  void validate_position(const Point point) const
  {
    if (point.x >= size_.width || point.y >= size_.height) {
      throw std::range_error(fmt::format(
        "index out of range, got: ({},{}), allowed ({}, {})", point.x, point.y, size_.width - 1, size_.height - 1));
    }
  }


public:
  explicit Vector2D(const Size size) : size_(size) {}

  [[nodiscard]] auto size() const noexcept { return size_; }

  [[nodiscard]] Contained &at(const Point point)
  {
    validate_position(point);
    return data_.at(point.y * size_.width + point.x);
  }

  [[nodiscard]] const Contained &at(const Point point) const
  {
    validate_position(point);
    return data_.at(point.y * size_.width + point.x);
  }
};

template<typename Contained> class Vector2D_Span
{
  Point origin_;
  Size size_;

  using reference_type =
    std::conditional_t<std::is_const_v<Contained>, const Vector2D<std::remove_const_t<Contained>>, Vector2D<Contained>>;

  std::reference_wrapper<reference_type> data_;

public:
  Vector2D_Span(const Point origin, const Size size, reference_type &data)
    : origin_{ origin }, size_{ size }, data_{ data }
  {}

  [[nodiscard]] Size size() const noexcept { return size_; }

  void validate_position(const Point point) const
  {
    if (point.x >= size_.width || point.y >= size_.height) { throw std::range_error("index out of range"); }
  }


  [[nodiscard]] const Contained &at(const Point point) const
  {
    validate_position(point);
    return data_.get().at(point + origin_);
  }

  [[nodiscard]] Contained &at(const Point point)
  {
    validate_position(point);
    return data_.get().at(point + origin_);
  }
};

void fill(auto &vector2d, const auto &value)
{
  for (std::size_t y = 0; y < vector2d.size().height; ++y) {
    for (std::size_t x = 0; x < vector2d.size().width; ++x) { vector2d.at(Point{ x, y }) = value; }
  }
}


void fill_border(auto &vector2d, const auto &value)
{
  for (std::size_t y = 0; y < vector2d.size().height; ++y) {
    for (std::size_t x = 0; x < vector2d.size().width; ++x) {
      if (y == 0 || x == 0 || x == vector2d.size().width - 1 || y == vector2d.size().height - 1) {
        vector2d.at({ x, y }) = value;
      }
    }
  }
}

void fill_line(auto &vector2d, Point from, const Point to, const auto &value)
{
  auto plot = [&](auto x, auto y) {
    vector2d.at({ static_cast<std::size_t>(x), static_cast<std::size_t>(y) }) = value;
  };

  auto plotLine = [&plot](std::ptrdiff_t x0, std::ptrdiff_t y0, std::ptrdiff_t x1, std::ptrdiff_t y1) {
    auto dx = std::abs(x1 - x0);
    auto sx = x0 < x1 ? 1 : -1;
    auto dy = -std::abs(y1 - y0);
    auto sy = y0 < y1 ? 1 : -1;
    auto error = dx + dy;

    while (true) {
      plot(x0, y0);
      if (x0 == x1 && y0 == y1) { break; }

      auto e2 = 2 * error;
      if (e2 >= dy) {
        if (x0 == x1) { break; }
        error += dy;
        x0 += sx;
      }

      if (e2 <= dx) {
        if (y0 == y1) { break; }
        error += dx;
        y0 += sy;
      }
    }
  };

  plotLine(static_cast<std::ptrdiff_t>(from.x),
    static_cast<std::ptrdiff_t>(from.y),
    static_cast<std::ptrdiff_t>(to.x),
    static_cast<std::ptrdiff_t>(to.y));
}


}// namespace lefticus::travels

#endif// AWESOME_GAME_VECTOR2D_HPP
