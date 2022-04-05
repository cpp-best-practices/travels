#ifndef MY_AWESOME_GAME_VECTOR2D_HPP
#define MY_AWESOME_GAME_VECTOR2D_HPP

#include <stdexcept>
#include <vector>

#include "point.hpp"
#include "size.hpp"


namespace lefticus::my_awesome_game {


template<typename Contained> class Vector2D
{
  Size size_;
  std::vector<Contained> data_ = decltype(data_)(size_.width * size_.height, Contained{});


  void validate_position(const Point point) const
  {
    if (point.x >= size_.width || point.y >= size_.height) { throw std::range_error("index out of range"); }
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

  std::reference_wrapper<Vector2D<Contained>> data_;

public:
  Vector2D_Span(const Point origin, const Size size, Vector2D<Contained> &data)
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
}// namespace lefticus::my_awesome_game

#endif// MY_AWESOME_GAME_VECTOR2D_HPP
