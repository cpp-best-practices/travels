#ifndef AWESOME_GAME_BITMAP_HPP
#define AWESOME_GAME_BITMAP_HPP

#include <filesystem>
#include <ftxui/dom/node.hpp>

#include "color.hpp"
#include "size.hpp"
#include "vector2d.hpp"

namespace lefticus::travels {

// A simple way of representing a bitmap on screen using only characters
struct Bitmap : ftxui::Node
{
  explicit Bitmap(const Size size) : pixels(size) {}

  void ComputeRequirement() override
  {
    requirement_ = ftxui::Requirement{ .min_x = static_cast<int>(pixels.size().width),
      .min_y = static_cast<int>(pixels.size().height / 2),
      .selected_box{ 0, 0, 0, 0 } };
  }

  void Render(ftxui::Screen &screen) override
  {
    for (std::size_t cur_x = 0; cur_x < pixels.size().width; ++cur_x) {
      for (std::size_t cur_y = 0; cur_y < pixels.size().height / 2; ++cur_y) {
        auto &ftxui_pixel = screen.PixelAt(box_.x_min + static_cast<int>(cur_x), box_.y_min + static_cast<int>(cur_y));
        ftxui_pixel.character = "▄";
        const auto &top_color = pixels.at(Point{ cur_x, cur_y * 2 });
        const auto &bottom_color = pixels.at(Point{ cur_x, cur_y * 2 + 1 });
        ftxui_pixel.background_color = ftxui::Color{ top_color.R, top_color.G, top_color.B };
        ftxui_pixel.foreground_color = ftxui::Color{ bottom_color.R, bottom_color.G, bottom_color.B };
      }
    }
  }

  void draw_vertical_line(std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> color,
    std::size_t x,
    std::size_t start_y,
    std::size_t end_y)
  {
    for (std::size_t y = start_y; y <= end_y; ++y) { draw({ x, y }, color); }
  }

  void draw(std::pair<std::size_t, std::size_t> location, std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> color)
  {
    if (location.first < pixels.size().width && location.second < pixels.size().height) {
      pixels.at({location.first, location.second}) = Color{ std::get<0>(color), std::get<1>(color), std::get<2>(color) };
    }
  }

  void clear() { pixels = decltype(pixels)(pixels.size()); }


  Vector2D<Color> pixels;
};

Vector2D<Color> load_png(const std::filesystem::path &filename);

}// namespace lefticus::travels

#endif// AWESOME_GAME_BITMAP_HPP
