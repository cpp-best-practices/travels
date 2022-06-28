#ifndef AWESOME_GAME_GAME_HPP
#define AWESOME_GAME_GAME_HPP

#include <filesystem>
#include <vector>

namespace lefticus::travels {

struct Game;

Game make_game(const std::vector<std::filesystem::path> &search_directories);

}// namespace lefticus::travels


#endif
