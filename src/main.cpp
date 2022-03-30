#include <array>
#include <functional>
#include <iostream>
#include <random>

#include <docopt/docopt.h>
#include <ftxui/component/captured_mouse.hpp>// for ftxui
#include <ftxui/component/component.hpp>// for Slider
#include <ftxui/component/screen_interactive.hpp>// for ScreenInteractive
#include <spdlog/spdlog.h>

// This file will be generated automatically when you run the CMake
// configuration step. It creates a namespace called `my_awesome_game`. You can modify
// the source template at `configured_files/config.hpp.in`.
#include <internal_use_only/config.hpp>


struct Color
{
  std::uint8_t R{};
  std::uint8_t G{};
  std::uint8_t B{};
};

struct Size
{
  std::size_t width;
  std::size_t height;
};

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


template<typename Contained> class Vector2D
{
  Size size_;
  std::vector<Contained> data_ = decltype(data_)(size_.width * size_.height, Contained{});


  void validate_position(const Point p) const
  {
    if (p.x >= size_.width || p.y >= size_.height) { throw std::range_error("index out of range"); }
  }


public:
  explicit Vector2D(const Size size) : size_(size) {}

  [[nodiscard]] auto size() const noexcept { return size_; }

  [[nodiscard]] Contained &at(const Point p)
  {
    validate_position(p);
    return data_.at(p.y * size_.width + p.x);
  }

  [[nodiscard]] const Contained &at(const Point p) const
  {
    validate_position(p);
    return data_.at(p.y * size_.width + p.x);
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

  void validate_position(const Point p) const
  {
    if (p.x >= size_.width || p.y >= size_.height) { throw std::range_error("index out of range"); }
  }


  [[nodiscard]] const Contained &at(const Point p) const
  {
    validate_position(p);
    return data_.get().at(p + origin_);
  }

  [[nodiscard]] Contained &at(const Point p)
  {
    validate_position(p);
    return data_.get().at(p + origin_);
  }
};

// A simple way of representing a bitmap on screen using only characters
struct Bitmap : ftxui::Node
{
  explicit Bitmap(const Size s) : pixels(s) {}

  void ComputeRequirement() override
  {
    requirement_ = ftxui::Requirement{ .min_x = static_cast<int>(pixels.size().width),
      .min_y = static_cast<int>(pixels.size().height / 2),
      .selected_box{ 0, 0, 0, 0 } };
  }

  void Render(ftxui::Screen &screen) override
  {
    for (std::size_t x = 0; x < pixels.size().width; ++x) {
      for (std::size_t y = 0; y < pixels.size().height / 2; ++y) {
        auto &p = screen.PixelAt(box_.x_min + static_cast<int>(x), box_.y_min + static_cast<int>(y));
        p.character = "▄";
        const auto &top_color = pixels.at(Point{ x, y * 2 });
        const auto &bottom_color = pixels.at(Point{ x, y * 2 + 1 });
        p.background_color = ftxui::Color{ top_color.R, top_color.G, top_color.B };
        p.foreground_color = ftxui::Color{ bottom_color.R, bottom_color.G, bottom_color.B };
      }
    }
  }

  Vector2D<Color> pixels;
};

struct Location
{
  std::function<void()> action;
  std::function<void(Vector2D_Span<Color> &, std::chrono::milliseconds, Point)> draw;
};


struct Game_Map
{
  explicit Game_Map(const Size size) : locations{ size } {}
  Vector2D<Location> locations;
};


Game_Map make_map()
{
  Game_Map map{ Size{ 5, 5 } };// NOLINT magic numbers

  auto solid_draw = []([[maybe_unused]] Vector2D_Span<Color> &pixels,
                      [[maybe_unused]] std::chrono::milliseconds game_clock,
                      [[maybe_unused]] Point map_location) {
    for (std::size_t x = 0; x < pixels.size().width; ++x) {
      for (std::size_t y = 0; y < pixels.size().height; ++y) {
        if (x == 0 || y == 0 || x == pixels.size().width - 1 || y == pixels.size().height - 1) {
          pixels.at(Point{ x, y }) = Color{ 128, 128, 128 };// NOLINT Magic numbers
        } else {
          switch ((game_clock.count() / 1000) % 2) { // NOLINT Magic numbers
            case 0:
              pixels.at(Point{ x, y }) = Color{ 255, 255, 255 };// NOLINT Magic numbers
              break;
            case 1: // NOLINT Magic Numbers
              pixels.at(Point{ x, y }) = Color{ 200, 240, 240 };// NOLINT Magic numbers
              break;
          }


        }
      }
    }
  };

  auto empty_draw = []([[maybe_unused]] Vector2D_Span<Color> &pixels,
                      [[maybe_unused]] std::chrono::milliseconds game_clock,
                      [[maybe_unused]] Point map_location) {

  };

  for (std::size_t x = 0; x < map.locations.size().width; ++x) {
    for (std::size_t y = 0; y < map.locations.size().height; ++y) { map.locations.at(Point{ x, y }).draw = empty_draw; }
  }

  map.locations.at(Point{ 2, 3 }).draw = solid_draw;
  map.locations.at(Point{ 1, 4 }).draw = solid_draw;
  map.locations.at(Point{ 0, 2 }).draw = solid_draw;


  return map;
}

void draw(Bitmap &viewport, Size tile_size, Point map_center, std::chrono::milliseconds time, const Game_Map &map)
{
  const auto num_wide = viewport.pixels.size().width / tile_size.width;
  const auto num_high = viewport.pixels.size().width / tile_size.height;

  const auto x_offset = num_wide / 2;
  const auto y_offset = num_high / 2;

  const auto min_x = x_offset;
  const auto min_y = y_offset;

  const auto max_x = num_wide - x_offset;
  const auto max_y = num_high - y_offset;

  const auto center_map_location =
    Point{ std::clamp(map_center.x, min_x, max_x), std::clamp(map_center.y, min_y, max_y) };

  const auto upper_left_map_location = center_map_location - Point{ min_x, min_y };

  for (std::size_t x = 0; x < num_wide; ++x) {
    for (std::size_t y = 0; y < num_high; ++y) {
      auto span = Vector2D_Span<Color>(Point{ x * tile_size.width, y * tile_size.width }, tile_size, viewport.pixels);
      const auto map_location = Point{ x, y } + upper_left_map_location;
      map.locations.at(map_location).draw(span, time, map_location);
    }
  }
}

void game_iteration_canvas()
{
  const auto map = make_map();

  // this should probably have a `bitmap` helper function that does what you expect
  // similar to the other parts of FTXUI
  auto bm = std::make_shared<Bitmap>(Size{ 40, 40 });// NOLINT magic numbers
  auto small_bm = std::make_shared<Bitmap>(Size{ 6, 6 });// NOLINT magic numbers

  double fps = 0;
  auto start_time = std::chrono::steady_clock::now();


  // to do, add total game time clock also, not just current elapsed time
  auto game_iteration = [&](const std::chrono::steady_clock::duration elapsed_time) {
    // in here we simulate however much game time has elapsed. Update animations,
    // run character AI, whatever, update stats, etc

    // this isn't actually timing based for now, it's just updating the display however fast it can
    fps = 1.0
          / (static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(elapsed_time).count())
             / 1'000'000.0);// NOLINT magic numbers

    const auto game_clock = std::chrono::steady_clock::now() - start_time;

    draw(*bm,
      Size{ 8, 8 },// NOLINT magic number
      Point{ 0, 0 },
      std::chrono::duration_cast<std::chrono::milliseconds>(game_clock),
      map);
  };

  auto screen = ftxui::ScreenInteractive::TerminalOutput();

  int counter = 0;

  auto last_time = std::chrono::steady_clock::now();

  auto make_layout = [&] {
    // This code actually processes the draw event
    const auto new_time = std::chrono::steady_clock::now();

    ++counter;
    // we will dispatch to the game_iteration function, where the work happens
    game_iteration(new_time - last_time);
    last_time = new_time;

    // now actually draw the game elements
    return ftxui::hbox({ bm | ftxui::border,
      ftxui::vbox({ ftxui::text("Frame: " + std::to_string(counter)),
        ftxui::text("FPS: " + std::to_string(fps)),
        small_bm | ftxui::border }) });
  };

  auto container = ftxui::Container::Vertical({});

  auto renderer = ftxui::Renderer(container, make_layout);

  std::atomic<bool> refresh_ui_continue = true;

  // This thread exists to make sure that the event queue has an event to
  // process at approximately a rate of 30 FPS
  std::thread refresh_ui([&] {
    while (refresh_ui_continue) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(1.0s / 30.0);// NOLINT magic numbers
      screen.PostEvent(ftxui::Event::Custom);
    }
  });

  screen.Loop(renderer);

  refresh_ui_continue = false;
  refresh_ui.join();
}

int main(int argc, const char **argv)
{
  try {
    static constexpr auto USAGE =
      R"(intro

    Usage:
          intro
          intro (-h | --help)
          intro --version
 Options:
          -h --help     Show this screen.
          --version     Show version.
)";

    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
      { std::next(argv), std::next(argv, argc) },
      true,// show help if requested
      fmt::format("{} {}",
        my_awesome_game::cmake::project_name,
        my_awesome_game::cmake::project_version));// version string, acquired
                                                  // from config.hpp via CMake

    game_iteration_canvas();

    //    consequence_game();
  } catch (const std::exception &e) {
    fmt::print("Unhandled exception in main: {}", e.what());
  }
}
