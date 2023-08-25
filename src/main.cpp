#include <array>
#include <filesystem>
#include <functional>
#include <iostream>

#include <CLI/CLI.hpp>
#include <ftxui/component/component.hpp>// for Slider
#include <ftxui/component/screen_interactive.hpp>// for ScreenInteractive

#include <spdlog/sinks/base_sink.h>

#include <spdlog/spdlog.h>

#include <libraycaster/renderer.hpp>

#include "bitmap.hpp"
#include "color.hpp"
#include "game.hpp"
#include "game_components.hpp"
#include "point.hpp"
#include "print.hpp"
#include "size.hpp"

#include "scripted_game.hpp"

// This file will be generated automatically when you run the CMake
// configuration step. It creates a namespace called `travels`. You can modify
// the source template at `configured_files/config.hpp.in`.
#include <internal_use_only/config.hpp>


namespace lefticus::travels {


void draw(Bitmap &viewport, Point map_center, const Game &game, const Game_Map &map)
{
  viewport.clear();
  const auto num_wide = viewport.pixels.size().width / game.tile_size.width;
  const auto num_high = viewport.pixels.size().height / game.tile_size.height;

  const auto x_offset = num_wide / 2;
  const auto y_offset = num_high / 2;

  const auto x1 = x_offset;
  const auto y1 = y_offset;

  const auto x2 = map.locations.size().width - x_offset - (num_wide % 2);
  const auto y2 = map.locations.size().height - y_offset - (num_high % 2);

  const auto min_x = std::min(x1, x2);
  const auto max_x = std::max(x1, x2);

  const auto min_y = std::min(y1, y2);
  const auto max_y = std::max(y1, y2);

  // I don't like this code. It needs to be cleaned up and made so a small map will center
  const auto center_map_location = Point{ std::clamp(map_center.x, min_x, max_x), std::clamp(map_center.y, min_y, max_y) };

  const auto upper_left_map_location =
    center_map_location - Point{ num_wide > map.locations.size().width? center_map_location.x : min_x,
                                           num_high > map.locations.size().height ? center_map_location.y : min_y };

  for (std::size_t cur_x = 0; cur_x < std::min(num_wide, map.locations.size().width); ++cur_x) {
    for (std::size_t cur_y = 0; cur_y < std::min(num_high, map.locations.size().height); ++cur_y) {
      auto span = Vector2D_Span<Color>(
        Point{ cur_x * game.tile_size.width, cur_y * game.tile_size.width }, game.tile_size, viewport.pixels);
      const auto map_location = Point{ cur_x, cur_y } + upper_left_map_location;
      map.locations.at(map_location).draw(span, game, map_location, Layer::Background);
    }
  }

  const auto character_relative_location = game.player.map_location - upper_left_map_location;

  const auto character_location = Point{ character_relative_location.x * game.tile_size.width,
    character_relative_location.y * game.tile_size.height };

  auto character_span = Vector2D_Span<Color>(character_location, game.tile_size, viewport.pixels);

  game.player.draw(character_span, game, game.player.map_location);


  for (std::size_t cur_x = 0; cur_x < std::min(num_wide, map.locations.size().width); ++cur_x) {
    for (std::size_t cur_y = 0; cur_y < std::min(num_high, map.locations.size().height); ++cur_y) {
      auto span = Vector2D_Span<Color>(
        Point{ cur_x * game.tile_size.width, cur_y * game.tile_size.width }, game.tile_size, viewport.pixels);
      const auto map_location = Point{ cur_x, cur_y } + upper_left_map_location;
      map.locations.at(map_location).draw(span, game, map_location, Layer::Foreground);
    }
  }
}

void draw(Bitmap &viewport, const Game &game)
{
  if (game.maps.contains(game.current_map)) {
    draw(viewport, game.player.map_location, game, game.maps.at(game.current_map));
  }
}

void draw_3d(Bitmap &viewport, const Game &game)
{
  if (game.maps_3d.contains(game.current_map)) {
    lefticus::raycaster::render(viewport,
      viewport.pixels.size().width,
      viewport.pixels.size().height,
      std::span<const lefticus::raycaster::Segment<float>>(game.maps_3d.at(game.current_map).map.segments),
      game.player.camera);
  }
}

ftxui::ButtonOption Animated(ftxui::Color background,// NOLINT
  ftxui::Color foreground,// NOLINT
  ftxui::Color background_active,// NOLINT
  ftxui::Color foreground_active)// NOLINT
{
  ftxui::ButtonOption option;
  option.transform = [](const ftxui::EntryState &state) {
    auto element = ftxui::text(state.label);
    if (state.focused) { element |= ftxui::bold; }
    return element;
  };
  option.animated_colors.foreground.Set(foreground, foreground_active);
  option.animated_colors.background.Set(background, background_active);
  return option;
}

ftxui::ButtonOption Animated()
{
  return Animated(ftxui::Color::Black,
    ftxui::Color::GrayLight,//
    ftxui::Color::GrayDark,
    ftxui::Color::White);
}
struct Displayed_Menu
{
  Displayed_Menu(Menu menu_, Game &game) : menu{ std::move(menu_) }
  {
    ftxui::Components menu_lines;

    for (const auto &item : menu.items) {
      if (!item.visible || item.visible(game)) {
        menu_lines.push_back(ftxui::Button(
          item.text, [&game, &item]() { item.action(game); }, Animated()));
      }
    }

    buttons = ftxui::Container::Vertical(menu_lines);
  }

  Menu menu;
  ftxui::Component buttons;
};


template<typename Mutex> class log_sink : public spdlog::sinks::base_sink<Mutex>
{
public:
  log_sink() = default;
  std::vector<std::string> event_log;

protected:
  void sink_it_(const spdlog::details::log_msg &msg) override
  {
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    event_log.insert(event_log.begin(), formatted);
  }

  void flush_() override {}
};


// todo make PR back into FTXUI?
class CatchEventBase : public ftxui::ComponentBase
{
public:
  // Constructor.
  explicit CatchEventBase(std::function<bool(ftxui::Event)> on_event) : on_event_(std::move(on_event)) {}

  // Component implementation.
  bool OnEvent(ftxui::Event event) override
  {
    if (on_event_(event)) {
      return true;
    } else {
      return ComponentBase::OnEvent(event);
    }
  }

  [[nodiscard]] bool Focusable() const override { return true; }

protected:
  std::function<bool(ftxui::Event)> on_event_;
};

ftxui::Component CatchEvent(ftxui::Component child, std::function<bool(ftxui::Event event)> on_event)
{
  auto out = Make<CatchEventBase>(std::move(on_event));
  out->Add(std::move(child));
  return out;
}


void play_game(Game &game, std::shared_ptr<log_sink<std::mutex>> log_sink)// NOLINT cognitive complexity
{

  Displayed_Menu current_menu{ Menu{}, game };
  bool show_log = false;

  auto clear_popup_button = ftxui::Button("OK", [&]() { game.popup_message.clear(); });
  auto close_log = ftxui::Button("Close", [&] { show_log = false; });

  // this should probably have a `bitmap` helper function that does what you expect
  // similar to the other parts of FTXUI
  auto bm = std::make_shared<Bitmap>(Size{ 80, 56 });// NOLINT magic numbers
  auto small_bm = std::make_shared<Bitmap>(Size{ 6, 6 });// NOLINT magic numbers

  double fps = 0;
  auto start_time = std::chrono::steady_clock::now();

  std::vector<ftxui::Event> events;


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

    char last_3d_location = ' ';

    while (!events.empty()) {
      const auto current_event = events.front();
      events.erase(events.begin());

      [&] {
        auto location = game.player.map_location;
        const auto last_location = location;

        Direction from{};

        if (current_event.is_character() && current_event.character() == "l") {
          show_log = true;
          return;
        } else {
          if (game.current_map_type == Game::Map_Type::Map_2D) {
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
          } else {
            const auto walls =
              std::span<const lefticus::raycaster::Segment<float>>(game.maps_3d.at(game.current_map).map.segments);
            if (current_event == ftxui::Event::ArrowUp) {
              game.player.camera.try_move(.1f, walls);
            } else if (current_event == ftxui::Event::ArrowDown) {
              game.player.camera.try_move(-.1f, walls);
            } else if (current_event == ftxui::Event::ArrowLeft) {
              game.player.camera.rotate(-.1f);
            } else if (current_event == ftxui::Event::ArrowRight) {
              game.player.camera.rotate(.1f);
            }

            const auto new_3d_location =
              game.get_current_map_3d().map.get_first_intersection(game.player.camera.location).value_or(' ');
            if (new_3d_location != last_3d_location) {
              const auto action = game.get_current_map_3d().enter_actions.find(new_3d_location);
              if (action != game.get_current_map_3d().enter_actions.end()) { action->second(game, new_3d_location); }
              last_3d_location = new_3d_location;
            }

            return;
          }
        }

        if (game.maps.at(game.current_map).can_enter_from(game, location, from)) {
          auto exit_action = game.maps.at(game.current_map).locations.at(last_location).exit_action;
          if (exit_action) { exit_action(game, last_location, from); }

          game.player.map_location = location;

          spdlog::trace("Moved to: {}, {}", location.x, location.y);

          auto enter_action = game.maps.at(game.current_map).locations.at(location).enter_action;
          if (enter_action) { enter_action(game, location, from); }
        }
      }();
    }

    if (game.current_map_type == Game::Map_Type::Map_2D) {
      draw(*bm, game);
    } else {
      draw_3d(*bm, game);
    }
  };

  auto screen = ftxui::ScreenInteractive::TerminalOutput();

  int counter = 0;

  auto last_time = std::chrono::steady_clock::now();

  auto container = ftxui::Container::Vertical({});

  auto key_press = lefticus::travels::CatchEvent(container, [&](const ftxui::Event &event) {
    events.push_back(event);
    return false;
  });

  auto make_layout = [&] {
    // This code actually processes the draw event
    const auto new_time = std::chrono::steady_clock::now();


    ++counter;

    if (game.exit_game) { screen.ExitLoopClosure()(); }

    // we will dispatch to the game_iteration function, where the work happens
    try {
      game_iteration(new_time - last_time);
    } catch (const std::exception &exception) {
      const auto message = std::format("Unhandled std::exception in game_iteration:\n\n{}", exception.what());
      game.popup_message = message;
      spdlog::critical(message);
    } catch (...) {
      constexpr static std::string_view message = "Unhandled unknown exception in game_iteration";
      game.popup_message = message;
      spdlog::critical(message);
    }

    last_time = new_time;


    auto make_text_components = [&] {
      ftxui::Elements text_components;
      text_components.push_back(ftxui::text("Frame: " + std::to_string(counter)));
      text_components.push_back(
        ftxui::text(std::format("Location: {{{},{}}}", game.player.map_location.x, game.player.map_location.y)));

      for (const auto &variable : game.display_variables) {
        if (game.variables.contains(variable)) {
          text_components.push_back(
            ftxui::text(std::format("{}: {}", variable, to_string(game.variables.at(variable)))));
        }
      }
      return text_components;
    };

    // now actually draw the game elements
    return ftxui::vbox({ ftxui::hbox({ bm | ftxui::border, ftxui::vbox(make_text_components()) | ftxui::border | ftxui::flex }),
      ftxui::text("Message: " + game.last_message) | ftxui::border });
  };


  // todo at some point replace this with a renderer that detects and uses the 'focus' flag
  auto game_renderer = ftxui::Renderer(key_press, make_layout);

  auto menu_renderer =
    ftxui::Renderer(current_menu.buttons, [&] { return current_menu.buttons->Render() | ftxui::border; });

  auto popup_renderer = ftxui::Renderer(clear_popup_button, [&] {
    ftxui::Elements paragraphs;

    std::string paragraph;
    for (const auto character : game.popup_message) {
      if (character == '\n') {
        if (paragraph.empty()) {
          paragraphs.push_back(ftxui::separatorEmpty());
        } else {
          paragraphs.emplace_back(ftxui::paragraphAlignLeft(paragraph));
          paragraph.clear();
        }
      } else {
        paragraph.push_back(character);
      }
    }

    if (!paragraph.empty()) { paragraphs.emplace_back(ftxui::paragraphAlignLeft(paragraph)); }

    paragraphs.push_back(ftxui::separatorEmpty());

    paragraphs.push_back(clear_popup_button->Render() | ftxui::center);


    return ftxui::vbox(paragraphs) | ftxui::border;
  });

  int selected_log_entry = 0;
  auto log_menu = ftxui::Menu(&log_sink->event_log, &selected_log_entry);

  auto log_renderer = ftxui::Renderer(ftxui::Container::Vertical({ log_menu, close_log }), [&] {
    return ftxui::vbox({ log_menu->Render() | ftxui::vscroll_indicator | ftxui::frame
                           | ftxui::size(ftxui::HEIGHT, ftxui::LESS_THAN, 15),// NOLINT magic numbers
             close_log->Render() })
           | ftxui::border;
  });

  int depth = 0;

  auto main_container = ftxui::Container::Tab({ game_renderer, menu_renderer, popup_renderer, log_renderer }, &depth);

  auto main_renderer = ftxui::Renderer(main_container, [&] {
    if (show_log) {
      depth = 3;
    } else if (game.has_popup_message()) {
      depth = 2;
    } else if (game.has_menu()) {
      depth = 1;
    } else {
      depth = 0;
    }

    if (game.has_new_menu()) {
      current_menu = Displayed_Menu{ game.get_menu(), game };
      menu_renderer->DetachAllChildren();
      menu_renderer->Add(current_menu.buttons);
    }

    ftxui::Element document = game_renderer->Render();

    if (depth > 0) {
      if (game.has_menu()) {
        document = ftxui::dbox({ document, menu_renderer->Render() | ftxui::clear_under | ftxui::center });
      }
    }

    if (depth > 1) {
      if (game.has_popup_message()) {
        document = ftxui::dbox({ document, popup_renderer->Render() | ftxui::clear_under | ftxui::center });
      }
    }

    if (depth > 2) {
      document = ftxui::dbox({ document, log_renderer->Render() | ftxui::clear_under | ftxui::center });
    }

    return document;
  });


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

  screen.Loop(main_renderer);

  refresh_ui_continue = false;
  refresh_ui.join();
}
}// namespace lefticus::travels


std::vector<std::filesystem::path> resource_search_directories()
{
  std::vector<std::filesystem::path> results;

  auto current_path = std::filesystem::current_path();

  while (current_path.has_relative_path()) {
    results.push_back(current_path);
    results.push_back(current_path / "resources");
    current_path = current_path.parent_path();
  }

  results.emplace_back(travels::cmake::source_dir);
  results.push_back(std::filesystem::path(travels::cmake::source_dir) / "resources");

  return results;
}

int main(int argc, const char **argv)
{
  try {
    CLI::App app{ std::format("{} version {}", travels::cmake::project_name, travels::cmake::project_version) };

    bool show_version = false;
    app.add_flag("--version", show_version, "Show version information");

    CLI11_PARSE(app, argc, argv);

    if (show_version) {
      lefticus::print("{}\n", travels::cmake::project_version);
      return EXIT_SUCCESS;
    }

    spdlog::set_level(spdlog::level::trace);

    // to start the lessons, comment out this line

    //auto game = lefticus::travels::make_game(resource_search_directories());

    Scripted_Game game{ resource_search_directories()};
    game.eval(R"(
(load_tiled_map game "main" "travels/tiled/tiles/Map.tmj")
(load_tiled_map game "tencin" "travels/tiled/tiles/Tencin.tmj")
(load_tiled_map game "home" "travels/tiled/tiles/Home.tmj")

(load_3d_map game "maze" "travels/maze.txt")

(define North 0.0)
(define East 1.570)
(define South 3.141)
(define West 4.712)

(set_location_action game "tencin" "exit_location" (lambda (game) (teleport_to_2d game "main" "tencin_exit")))
(set_location_action game "home" "exit_location" (lambda (game) (teleport_to_2d game "main" "home_exit")))

(set_location_action game "main" "tencin_entrance" (lambda (game) (teleport_to_2d game "tencin" "entry_location")))
(set_location_action game "main" "home_entrance" (lambda (game) (teleport_to_2d game "home" "entry_location")))

(set_location_action game "main" "maze_right" (lambda (game) (teleport_to_3d game "maze" "1" West)))
(set_location_action game "main" "maze_left" (lambda (game) (teleport_to_3d game "maze" "2" East)))

(set_3d_location_action game "maze" "w" (lambda (game) (teleport_to_2d game "main" "maze_left_exit")))
(set_3d_location_action game "maze" "e" (lambda (game) (teleport_to_2d game "main" "maze_right_exit")))

(teleport_to_2d game "home" "start_location")
)");

    // we want to take over as the main spdlog sink
    auto log_sink = std::make_shared<lefticus::travels::log_sink<std::mutex>>();

    spdlog::set_default_logger(std::make_shared<spdlog::logger>("default", log_sink));

    spdlog::set_level(spdlog::level::trace);
    lefticus::travels::play_game(game.game, log_sink);
  } catch (const std::exception &e) {
    lefticus::print("Unhandled exception in main: {}", e.what());
  }
}
