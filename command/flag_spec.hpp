#pragma once

#include <string>

#include "bee/error.hpp"

namespace command {

template <class T>
concept FlagSpec =
  requires(const T a, const typename T::value_type b, const std::string str) {
    {
      a.to_string(b)
    } -> std::convertible_to<std::string>;
    {
      a.of_string(str)
    } -> std::convertible_to<bee::OrError<typename T::value_type>>;
  };

template <class T>
concept StringParser = requires(std::string str, T v) {
  {
    T::of_string(str)
  } -> std::convertible_to<bee::OrError<T>>;
  {
    v.to_string()
  } -> std::convertible_to<std::string>;
};

template <StringParser T> struct CustomFlag {
 public:
  using value_type = T;
  bee::OrError<value_type> of_string(const std::string& value) const
  {
    return T::of_string(value);
  }
  std::string to_string(const T& value) const { return value.to_string(); }
};

template <StringParser T> constexpr CustomFlag<T> create_flag_spec()
{
  return CustomFlag<T>();
}

} // namespace command
