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

// A simple way of representing a bitmap on screen using only characters
struct Bitmap : ftxui::Node
{
  Bitmap(std::size_t width, std::size_t height)// NOLINT same typed parameters adjacent to each other
    : width_(width), height_(height)
  {}

  Color &at(std::size_t x, std::size_t y) { return pixels.at(width_ * y + x); }

  void ComputeRequirement() override
  {
    requirement_ = ftxui::Requirement{
      .min_x = static_cast<int>(width_), .min_y = static_cast<int>(height_ / 2), .selected_box{ 0, 0, 0, 0 }
    };
  }

  void SetBox(ftxui::Box box) override { box_ = box; }

  void Render(ftxui::Screen &screen) override
  {
    for (std::size_t x = 0; x < width_; ++x) {
      for (std::size_t y = 0; y < height_ / 2; ++y) {
        auto &p = screen.PixelAt(box_.x_min + static_cast<int>(x), box_.y_min + static_cast<int>(y));
        p.character = "▄";
        const auto &top_color = at(x, y * 2);
        const auto &bottom_color = at(x, y * 2 + 1);
        p.background_color = ftxui::Color{ top_color.R, top_color.G, top_color.B };
        p.foreground_color = ftxui::Color{ bottom_color.R, bottom_color.G, bottom_color.B };
      }
    }
  }

  [[nodiscard]] auto width() const noexcept { return width_; }

  [[nodiscard]] auto height() const noexcept { return height_; }

  [[nodiscard]] auto &data() noexcept { return pixels; }

private:
  std::size_t width_;
  std::size_t height_;

  std::vector<Color> pixels = std::vector<Color>(width_ * height_, Color{});
};

void game_iteration_canvas()
{
  // this should probably have a `bitmap` helper function that does what you expect
  // similar to the other parts of FTXUI
  auto bm = std::make_shared<Bitmap>(50, 50);// NOLINT magic numbers
  auto small_bm = std::make_shared<Bitmap>(6, 6);// NOLINT magic numbers

  double fps = 0;

  std::size_t max_row = 0;
  std::size_t max_col = 0;

  // to do, add total game time clock also, not just current elapsed time
  auto game_iteration = [&](const std::chrono::steady_clock::duration elapsed_time) {
    // in here we simulate however much game time has elapsed. Update animations,
    // run character AI, whatever, update stats, etc

    // this isn't actually timing based for now, it's just updating the display however fast it can
    fps = 1.0
          / (static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(elapsed_time).count())
             / 1'000'000.0);// NOLINT magic numbers

    for (std::size_t row = 0; row < max_row; ++row) {
      for (std::size_t col = 0; col < bm->width(); ++col) { ++(bm->at(col, row).R); }
    }

    for (std::size_t row = 0; row < bm->height(); ++row) {
      for (std::size_t col = 0; col < max_col; ++col) { ++(bm->at(col, row).G); }
    }

    // for the fun of it, let's have a second window doing interesting things
    auto &small_bm_pixel =
      small_bm->data().at(static_cast<std::size_t>(elapsed_time.count()) % small_bm->data().size());

    switch (elapsed_time.count() % 3) {
    case 0:
      small_bm_pixel.R += 11;// NOLINT Magic Number
      break;
    case 1:
      small_bm_pixel.G += 11;// NOLINT Magic Number
      break;
    case 2:
      small_bm_pixel.B += 11;// NOLINT Magic Number
      break;
    }


    ++max_row;
    if (max_row >= bm->height()) { max_row = 0; }
    ++max_col;
    if (max_col >= bm->width()) { max_col = 0; }
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
