#ifndef LEFTICUS_PRINT_HPP
#define LEFTICUS_PRINT_HPP

#include <cstdio>
#include <format>

namespace lefticus {
template<class... Args>

void print(std::FILE *stream, std::format_string<Args...> fmt, Args &&...args)
{
  std::fputs(std::format(fmt, std::forward<Args>(args)...).c_str(), stream);
}

template<class... Args> void print(std::format_string<Args...> fmt, Args &&...args)
{
  print(stdout, fmt, std::forward<Args>(args)...);
}

template<class... Args> void println(std::FILE *stream, std::format_string<Args...> fmt, Args &&...args)
{
  print(stream, "{}\n", std::format(fmt, std::forward<Args>(args)...));
}

template<class... Args> void println(std::format_string<Args...> fmt, Args &&...args)
{
  println(stdout, fmt, std::forward<Args>(args)...);
}
}

#endif
