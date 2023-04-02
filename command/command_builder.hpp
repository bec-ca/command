#pragma once

#include "cmd.hpp"
#include "command_flags.hpp"

#include "bee/error.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace command {

template <class T> struct FlagWrapper {
 public:
  FlagWrapper(const std::shared_ptr<T>& flag) : _flag(flag) {}

  const auto* operator->() const { return &_flag->value(); }
  const auto& operator*() const { return _flag->value(); }

 private:
  std::shared_ptr<T> _flag;
};

template <class T> auto wrap_flag(const std::shared_ptr<T>& flag)
{
  return FlagWrapper<T>(flag);
}

struct CommandBuilder {
 public:
  using handler_type = std::function<bee::OrError<bee::Unit>(void)>;

  CommandBuilder(const std::string& description);

  FlagWrapper<BooleanFlag> no_arg(
    const std::string& name, const opt_str& doc = std::nullopt);

  template <class S>
  auto optional(
    const std::string& name,
    const S& spec,
    const opt_str& value_name = std::nullopt,
    const opt_str& doc = std::nullopt)
  {
    auto flag = FlagTemplate<S>::create(name, spec, value_name, doc);
    _flags.push_back(flag);
    return wrap_flag(flag);
  }

  template <class S>
  auto optional_with_default(
    const std::string& name,
    const S& spec,
    const typename S::value_type& def,
    const opt_str& value_name = std::nullopt,
    const opt_str& doc = std::nullopt)
  {
    auto flag =
      RequiredFlagTemplate<S>::create(name, spec, value_name, doc, def);
    _flags.push_back(flag);
    return wrap_flag(flag);
  }

  template <class S>
  auto required(
    const std::string& name,
    const S& spec,
    const opt_str& value_name = std::nullopt,
    const opt_str& doc = std::nullopt)
  {
    auto flag = RequiredFlagTemplate<S>::create(name, spec, value_name, doc);
    _flags.push_back(flag);
    return wrap_flag(flag);
  }

  template <class S>
  auto anon(
    const S& spec, const opt_str& value_name, const opt_str& doc = std::nullopt)
  {
    auto flag = AnonFlagTemplate<S>::create(spec, value_name, doc);
    _anon_flags.push_back(flag);
    return wrap_flag(flag);
  }

  template <class S>
  auto required_anon(
    const S& spec, const opt_str& value_name, const opt_str& doc = std::nullopt)
  {
    auto flag = RequiredAnonFlagTemplate<S>::create(spec, value_name, doc);
    _anon_flags.push_back(flag);
    return wrap_flag(flag);
  }

  Cmd run(handler_type handler);

  const std::string& description() const;

 private:
  std::string _description;
  std::vector<Flag> _flags;

  std::vector<AnonFlag::ptr> _anon_flags;
};

} // namespace command
