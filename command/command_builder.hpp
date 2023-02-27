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

  FlagWrapper<BooleanFlag> no_arg(const std::string& name);

  template <class S> auto optional(const std::string& name, const S& spec)
  {
    auto flag = FlagTemplate<S>::create(name, spec);
    _flags.push_back(flag);
    return wrap_flag(flag);
  }

  template <class S>
  auto optional_with_default(
    const std::string& name, const S& spec, const typename S::value_type& def)
  {
    auto flag = RequiredFlagTemplate<S>::create(name, spec, def);
    _flags.push_back(flag);
    return wrap_flag(flag);
  }

  template <class S> auto required(const std::string& name, const S& spec)
  {
    auto flag = RequiredFlagTemplate<S>::create(name, spec);
    _flags.push_back(flag);
    return wrap_flag(flag);
  }

  template <class S> auto anon(const S& spec, const std::string& doc)
  {
    auto flag = AnonFlagTemplate<S>::create(spec, doc);
    _anon_flags.push_back(flag);
    return wrap_flag(flag);
  }

  template <class S> auto required_anon(const S& spec, const std::string& doc)
  {
    auto flag = RequiredAnonFlagTemplate<S>::create(spec, doc);
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
