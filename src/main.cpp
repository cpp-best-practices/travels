#include <array>
#include <filesystem>
#include <functional>
#include <iostream>

#include <docopt/docopt.h>
#include <ftxui/component/component.hpp>// for Slider
#include <ftxui/component/screen_interactive.hpp>// for ScreenInteractive

#include <spdlog/sinks/base_sink.h>


#ifdef _MSC_VER
#pragma warning(disable : 4189)
#endif
#include <spdlog/spdlog.h>
#ifdef _MSC_VER
#pragma warning(default : 4189)
#endif


#include "bitmap.hpp"
#include "color.hpp"
#include "game.hpp"
#include "game_components.hpp"
#include "game_hacking_lesson_00.hpp"
#include "game_hacking_lesson_01.hpp"
#include "game_hacking_lesson_02.hpp"
#include "point.hpp"
#include "size.hpp"

// This file will be generated automatically when you run the CMake
// configuration step. It creates a namespace called `my_awesome_game`. You can modify
// the source template at `configured_files/config.hpp.in`.
#include <internal_use_only/config.hpp>


namespace lefticus::travels {


void draw(Bitmap &viewport, Point map_center, const Game &game, const Game_Map &map)
{
  const auto num_wide = viewport.pixels.size().width / game.tile_size.width;
  const auto num_high = viewport.pixels.size().height / game.tile_size.height;

  const auto x_offset = num_wide / 2;
  const auto y_offset = num_high / 2;

  const auto min_x = x_offset;
  const auto min_y = y_offset;

  const auto max_x = map.locations.size().width - x_offset - (num_wide % 2);
  const auto max_y = map.locations.size().height - y_offset - (num_high % 2);

  const auto center_map_location =
    Point{ std::clamp(map_center.x, min_x, max_x), std::clamp(map_center.y, min_y, max_y) };

  const auto upper_left_map_location = center_map_location - Point{ min_x, min_y };

  for (std::size_t cur_x = 0; cur_x < num_wide; ++cur_x) {
    for (std::size_t cur_y = 0; cur_y < num_high; ++cur_y) {
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


  for (std::size_t cur_x = 0; cur_x < num_wide; ++cur_x) {
    for (std::size_t cur_y = 0; cur_y < num_high; ++cur_y) {
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
    event_log.insert(event_log.begin(), fmt::to_string(formatted));
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
  auto bm = std::make_shared<Bitmap>(Size{ 64, 40 });// NOLINT magic numbers
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
        } else if (current_event == ftxui::Event::ArrowUp) {
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

          spdlog::trace("Moved to: {}, {}", location.x, location.y);

          auto enter_action = game.maps.at(game.current_map).locations.at(location).enter_action;
          if (enter_action) { enter_action(game, location, from); }
        }
      }();
    }


    draw(*bm, game);
  };

  auto screen = ftxui::ScreenInteractive::TerminalOutput();

  int counter = 0;

  auto last_time = std::chrono::steady_clock::now();

  std::string last_character;

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
      const auto message = fmt::format("Unhandled std::exception in game_iteration:\n\n{}", exception.what());
      game.popup_message = message;
      spdlog::critical(message);
    } catch (...) {
      constexpr static std::string_view message = "Unhandled unknown exception in game_iteration";
      game.popup_message = message;
      spdlog::critical(message);
    }

    last_time = new_time;


    ftxui::Elements text_components;
    text_components.push_back(ftxui::text("Frame: " + std::to_string(counter)));
    text_components.push_back(
      ftxui::text(fmt::format("Location: {{{},{}}}", game.player.map_location.x, game.player.map_location.y)));

    for (const auto &variable : game.display_variables) {
      if (game.variables.contains(variable)) {
        text_components.push_back(ftxui::text(fmt::format("{}: {}", variable, to_string(game.variables.at(variable)))));
      }
    }

    // now actually draw the game elements
    return ftxui::vbox({ ftxui::hbox({ bm | ftxui::border, ftxui::vbox(std::move(text_components)) | ftxui::border }),
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

  results.emplace_back(my_awesome_game::cmake::source_dir);
  results.push_back(std::filesystem::path(my_awesome_game::cmake::source_dir) / "resources");

  return results;
}

int main(int argc, const char **argv)
{
  try {
    static constexpr auto USAGE =
      R"(travels

    Usage:
          travels
          travels (-h | --help)
          travels --version
 Options:
          -h --help     Show this screen.
          --version     Show version.
)";

    spdlog::set_level(spdlog::level::trace);

    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
      { std::next(argv), std::next(argv, argc) },
      true,// show help if requested
      fmt::format("{} {}",
        my_awesome_game::cmake::project_name,
        my_awesome_game::cmake::project_version));// version string, acquired
                                                  // from config.hpp via CMake

    // to start the lessons, comment out this line
    auto game = lefticus::travels::make_game(resource_search_directories());

    // and uncomment this line
    //auto game = lefticus::travels::hacking::lesson_00::make_lesson();

    // we want to take over as the main spdlog sink
    auto log_sink = std::make_shared<lefticus::travels::log_sink<std::mutex>>();

    spdlog::set_default_logger(std::make_shared<spdlog::logger>("default", log_sink));

    spdlog::set_level(spdlog::level::trace);
    lefticus::travels::play_game(game, log_sink);
  } catch (const std::exception &e) {
    fmt::print("Unhandled exception in main: {}", e.what());
  }
}
