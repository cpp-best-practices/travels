#include <lodepng.h>

#include "bitmap.hpp"

namespace lefticus::awesome_game {
Vector2D<Color> load_png(const std::filesystem::path &filename)
{
  std::vector<unsigned char> image;// the raw pixels
  unsigned width{};
  unsigned height{};

  // decode
  unsigned error = lodepng::decode(image, width, height, filename.string());

  // if there's an error, display it
  if (error != 0) {
    throw std::runtime_error(fmt::format("lodepng decoder error {}: {}", error, lodepng_error_text(error)));
  }

  // the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
  //
  Vector2D<Color> results{ Size{ static_cast<std::size_t>(width), static_cast<std::size_t>(height) } };

  std::size_t position = 0;
  for (std::size_t cur_y = 0; cur_y < results.size().height; ++cur_y) {
    for (std::size_t cur_x = 0; cur_x < results.size().width; ++cur_x) {
      auto &color = results.at(Point{ cur_x, cur_y });
      // cppcheck is wrong about this.
      color.R = image[position++];// cppcheck-suppress danglingTempReference
      color.G = image[position++];// cppcheck-suppress danglingTempReference
      color.B = image[position++];// cppcheck-suppress danglingTempReference
      color.A = image[position++];// cppcheck-suppress danglingTempReference
    }
  }

  return results;
}
}// namespace lefticus::awesome_game