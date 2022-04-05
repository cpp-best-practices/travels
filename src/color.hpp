#ifndef MY_AWESOME_GAME_COLOR_HPP
#define MY_AWESOME_GAME_COLOR_HPP

#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>


// Todo: move this into `tools` project?
namespace lefticus::my_awesome_game {
template<typename Type> struct Basic_Color;

template<typename OutType, typename InType>
[[nodiscard]] constexpr Basic_Color<OutType> color_cast(Basic_Color<InType> input) noexcept
{
  if constexpr (std::is_same_v<OutType, InType>) { return input; }

  constexpr auto to_function = [] {
    if constexpr (std::is_floating_point_v<InType>) {
      if constexpr (std::is_floating_point_v<OutType>) {
        // just static cast from one floating point to_function another
        return [](const auto input_color_component) { return static_cast<OutType>(input_color_component); };
      } else {
        // from floating point to_function integral
        // we want to_function scale 0-1 to_function 0-maxint for given type
        return [](const auto input_color_component) {
          return static_cast<OutType>(std::llround(input_color_component * std::numeric_limits<OutType>::max()));
        };
      }
    } else {
      // input is not floating point
      if constexpr (std::is_integral_v<OutType>) {
        // and neither is output
        return [](const auto input_color_component) {
          // scale 0-1, then scale that to_function output type
          return static_cast<OutType>(std::llround(
            (static_cast<double>(input_color_component) / static_cast<double>(std::numeric_limits<InType>::max()))
            * static_cast<double>(std::numeric_limits<OutType>::max())));
        };
      } else {
        // output is floating point
        return [](const auto input_color_component) {
          return static_cast<OutType>(input_color_component) / static_cast<OutType>(std::numeric_limits<InType>::max());
        };
      }
    }
  }();

  return Basic_Color<OutType>{ to_function(input.R), to_function(input.G), to_function(input.B), to_function(input.A) };
}

template<typename Type> struct Basic_Color
{
  Type R{};
  Type G{};
  Type B{};
  Type A{};


  template<typename RHS> constexpr auto &operator+=(Basic_Color<RHS> rhs) noexcept
  {
    // from stackoverflow
    // Short answer:
    // if we want to overlay color0 over color1 both with some alpha then
    // a01 = (1 - a0)·a1 + a0
    // r01 = ((1 - a0)·a1·r1 + a0·r0) / a01
    // g01 = ((1 - a0)·a1·g1 + a0·g0) / a01
    // b01 = ((1 - a0)·a1·b1 + a0·b0) / a01

    const auto color1 = color_cast<double>(*this);
    const auto color0 = color_cast<double>(rhs);
    auto color01 = Basic_Color<double>{};

    color01.A = (1 - color0.A) * color1.A + color0.A;
    color01.R = ((1 - color0.A) * color1.A * color1.R + color0.A * color0.R) / color01.A;
    color01.G = ((1 - color0.A) * color1.A * color1.G + color0.A * color0.G) / color01.A;
    color01.B = ((1 - color0.A) * color1.A * color1.B + color0.A * color0.B) / color01.A;

    *this = color_cast<Type>(color01);
    return *this;
  }
};

using Color = Basic_Color<std::uint8_t>;
}// namespace lefticus::my_awesome_game

#endif// MY_AWESOME_GAME_COLOR_HPP
