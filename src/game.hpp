#ifndef AWESOME_GAME_GAME_HPP
#define AWESOME_GAME_GAME_HPP

#include <vector>
#include <filesystem>

namespace lefticus::awesome_game {

struct Game;

Game make_game(const std::vector<std::filesystem::path> &search_directories);

}// namespace lefticus::awesome_game


#endif
