#pragma once

#include <string>

#include "bee/or_error.hpp"

namespace command {

template <class T>
concept FlagSpec = requires(
  const T& a, const typename T::value_type& b, const std::string& str) {
  { a.to_string(b) } -> std::convertible_to<std::string>;
  {
    a.of_string(str)
  } -> std::convertible_to<bee::OrError<typename T::value_type>>;
};

template <class T>
concept HasOfString = requires(const std::string& str, const T& v) {
  { T::of_string(str) } -> std::convertible_to<bee::OrError<T>>;
};

template <class T>
concept ConstructibleFromString = std::constructible_from<T, std::string>;

template <class T>
concept StringParser = requires(const std::string& str, const T& v) {
  { HasOfString<T> || ConstructibleFromString<T> };
  { v.to_string() } -> std::convertible_to<std::string>;
};

template <StringParser T> struct CustomFlag {
 public:
  using value_type = T;
  bee::OrError<value_type> of_string(const std::string_view& str) const
  {
    if constexpr (ConstructibleFromString<T>) {
      return static_cast<T>(str);
    } else {
      return T::of_string(str);
    }
  }
  std::string to_string(const T& value) const { return value.to_string(); }
};

template <StringParser T> constexpr CustomFlag<T> create_flag_spec()
{
  return CustomFlag<T>();
}

} // namespace command
