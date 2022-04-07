#include <array>
#include <filesystem>
#include <functional>
#include <iostream>

#include <docopt/docopt.h>
#include <ftxui/component/component.hpp>// for Slider
#include <ftxui/component/screen_interactive.hpp>// for ScreenInteractive

#include <spdlog/spdlog.h>

#include "bitmap.hpp"
#include "color.hpp"
#include "game.hpp"
#include "game_components.hpp"
#include "point.hpp"
#include "size.hpp"

// This file will be generated automatically when you run the CMake
// configuration step. It creates a namespace called `my_awesome_game`. You can modify
// the source template at `configured_files/config.hpp.in`.
#include <internal_use_only/config.hpp>


namespace lefticus::my_awesome_game {


void draw(Bitmap &viewport, Point map_center, const Game &game, const Game_Map &map)
{
  const auto num_wide = viewport.pixels.size().width / game.tile_size.width;
  const auto num_high = viewport.pixels.size().width / game.tile_size.height;

  const auto x_offset = num_wide / 2;
  const auto y_offset = num_high / 2;

  const auto min_x = x_offset;
  const auto min_y = y_offset;

  const auto max_x = map.locations.size().width - x_offset - (num_wide % 2);
  const auto max_y = map.locations.size().height - y_offset - (num_wide % 2);

  const auto center_map_location =
    Point{ std::clamp(map_center.x, min_x, max_x), std::clamp(map_center.y, min_y, max_y) };

  const auto upper_left_map_location = center_map_location - Point{ min_x, min_y };

  for (std::size_t cur_x = 0; cur_x < num_wide; ++cur_x) {
    for (std::size_t cur_y = 0; cur_y < num_high; ++cur_y) {
      auto span = Vector2D_Span<Color>(
        Point{ cur_x * game.tile_size.width, cur_y * game.tile_size.width }, game.tile_size, viewport.pixels);
      const auto map_location = Point{ cur_x, cur_y } + upper_left_map_location;
      map.locations.at(map_location).draw(span, game, map_location);
    }
  }

  const auto character_relative_location = game.player.map_location - upper_left_map_location;

  const auto character_location = Point{ character_relative_location.x * game.tile_size.width,
    character_relative_location.y * game.tile_size.height };

  auto character_span = Vector2D_Span<Color>(character_location, game.tile_size, viewport.pixels);

  game.player.draw(character_span, game, game.player.map_location);
}

void draw(Bitmap &viewport, const Game &game)
{
  if (game.maps.contains(game.current_map)) {
    draw(viewport, game.player.map_location, game, game.maps.at(game.current_map));
  }
}

struct Displayed_Menu
{
  Displayed_Menu(Menu menu_, Game &game) : menu{ std::move(menu_) }
  {
    ftxui::Components menu_lines;

    std::ranges::transform(menu.items, std::back_inserter(menu_lines), [&game](auto &item) {
      return ftxui::Button(item.text, [&game, &item]() { item.action(game); });
    });

    buttons = ftxui::Container::Vertical(menu_lines);
  }

  Menu menu;
  ftxui::Component buttons;
};

void game_iteration_canvas()// NOLINT cognitive complexity
{
  auto game = make_game();

  Displayed_Menu current_menu{ Menu{}, game };

  // this should probably have a `bitmap` helper function that does what you expect
  // similar to the other parts of FTXUI
  auto bm = std::make_shared<Bitmap>(Size{ 40, 40 });// NOLINT magic numbers
  auto small_bm = std::make_shared<Bitmap>(Size{ 6, 6 });// NOLINT magic numbers

  double fps = 0;
  auto start_time = std::chrono::steady_clock::now();


  ftxui::Event last_event;
  ftxui::Event current_event;

  // to do, add total game time clock also, not just current elapsed time
  auto game_iteration = [&](const std::chrono::steady_clock::duration elapsed_time) {
    // in here we simulate however much game time has elapsed. Update animations,
    // run character AI, whatever, update stats, etc

    fps = 1.0
          / (static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(elapsed_time).count())
             / 1'000'000.0);// NOLINT magic numbers

    const auto game_clock =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);

    game.clock = game_clock;

    [&] {
      if (game.has_menu()) {
        return;
      }

      if (current_event != last_event) {
        auto location = game.player.map_location;
        const auto last_location = location;

        Direction from{};

        if (current_event == ftxui::Event::ArrowUp) {
          --location.y;
          from = Direction::South;
        } else if (current_event == ftxui::Event::ArrowDown) {
          ++location.y;
          from = Direction::North;
        } else if (current_event == ftxui::Event::ArrowLeft) {
          --location.x;
          from = Direction::East;
        } else if (current_event == ftxui::Event::ArrowRight) {
          ++location.x;
          from = Direction::West;
        } else {
          return;
        }

        if (game.maps.at(game.current_map).can_enter_from(game, location, from)) {
          auto exit_action = game.maps.at(game.current_map).locations.at(last_location).exit_action;
          if (exit_action) { exit_action(game, last_location, from); }

          game.player.map_location = location;

          auto enter_action = game.maps.at(game.current_map).locations.at(location).enter_action;
          if (enter_action) { enter_action(game, location, from); }
        }
      }
    }();


    draw(*bm, game);
  };

  auto screen = ftxui::ScreenInteractive::TerminalOutput();

  int counter = 0;

  auto last_time = std::chrono::steady_clock::now();

  std::string last_character;

  auto container = ftxui::Container::Vertical({});

  auto key_press = ftxui::CatchEvent(container, [&](const ftxui::Event &event) {
    if (game.has_menu()) {
      return false;
    } else {
      last_event = std::exchange(current_event, event);
      return true;
    }
  });

  auto make_layout = [&] {
    // This code actually processes the draw event
    const auto new_time = std::chrono::steady_clock::now();

    if (game.has_new_menu()) { current_menu = Displayed_Menu{ game.get_menu(), game };
      key_press->DetachAllChildren();
      key_press->Add(current_menu.buttons);
    }

    ++counter;
    // we will dispatch to the game_iteration function, where the work happens
    game_iteration(new_time - last_time);
    last_time = new_time;

    // now actually draw the game elements
    return ftxui::vbox({ ftxui::hbox({ (game.has_menu() ? current_menu.buttons->Render() : bm) | ftxui::border,
                           ftxui::vbox({ ftxui::text("Frame: " + std::to_string(counter)),
                             ftxui::text("FPS: " + std::to_string(fps)),
                             ftxui::text("Character: " + last_character),
                             small_bm | ftxui::border }) }),
      ftxui::text("Message: " + game.last_message) });
  };



  auto renderer = ftxui::Renderer(game.has_menu() ? current_menu.buttons : key_press, make_layout);

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
}// namespace lefticus::my_awesome_game

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

    lefticus::my_awesome_game::game_iteration_canvas();

    //    consequence_game();
  } catch (const std::exception &e) {
    fmt::print("Unhandled exception in main: {}", e.what());
  }
}
