#include "command_builder.hpp"

#include "command_base.hpp"

#include "bee/error.hpp"
#include "bee/string_util.hpp"
#include "bee/util.hpp"

#include <deque>
#include <stdexcept>
#include <type_traits>
#include <vector>

using bee::print_line;
using std::decay_t;
using std::deque;
using std::is_same_v;
using std::string;
using std::vector;

namespace command {
namespace {

const string& flag_name(const Flag& flag)
{
  return visit([&](auto flag) -> const string& { return flag->name(); }, flag);
}

bee::OrError<Flag> find_flag(const vector<Flag>& flags, const string& name)
{
  for (const auto& flag : flags) {
    if (name == flag_name(flag)) { return flag; }
  }
  return bee::Error::format("Unknown flag '$'", name);
}

bee::OrError<bee::Unit> parse_args(
  const vector<Flag>& named_flags,
  const vector<AnonFlag::ptr>& anon_flags,
  const deque<string>& args)
{
  size_t anon_flag_index = 0;
  for (size_t i = 0; i < args.size();) {
    const string& arg = args.at(i++);
    if (arg.front() == '-') {
      bail(flag, find_flag(named_flags, arg));
      bail_unit(visit(
        [&](auto flag) -> bee::OrError<bee::Unit> {
          using T = decay_t<decltype(flag)>;
          if constexpr (is_same_v<T, ValueFlag::ptr>) {
            if (i >= args.size()) {
              return bee::Error::format("No arguments for flag $", arg);
            }
            auto value = args.at(i++);
            auto err = flag->parse_value(value);
            if (err.is_error()) {
              return bee::Error::format(
                "Failed to parse flag $ with value '$': $",
                arg,
                value,
                err.error());
            }
            return bee::unit;
          } else if constexpr (is_same_v<T, BooleanFlag::ptr>) {
            flag->set();
            return bee::unit;
          } else {
            static_assert(bee::always_false_v<T> && "visit is not exhaustive");
          }
        },
        flag));
    } else {
      if (anon_flag_index >= anon_flags.size()) {
        return bee::Error::format("Unexpected anonymous argument '$'", arg);
      }
      auto err = anon_flags[anon_flag_index]->parse_value(arg);
      if (err.is_error()) {
        return bee::Error::format(
          "Failed to parse anon flag with value '$': $", arg, err.error());
      }
      anon_flag_index++;
    }
  }
  for (const auto& flag : named_flags) {
    bail_unit(visit(
      [&](auto flag) -> bee::OrError<bee::Unit> {
        using T = decay_t<decltype(flag)>;
        if constexpr (is_same_v<T, ValueFlag::ptr>) {
          return flag->finish_parsing();
        } else if constexpr (is_same_v<T, BooleanFlag::ptr>) {
          return bee::unit;
        } else {
          static_assert(bee::always_false_v<T> && "visit is not exhaustive");
        }
      },
      flag));
  }
  for (const auto& anon_flag : anon_flags) {
    bail_unit(anon_flag->finish_parsing());
  }
  return bee::unit;
}

////////////////////////////////////////////////////////////////////////////////
// Command
//

struct Command : public CommandBase {
 public:
  using handler_type = std::function<bee::OrError<bee::Unit>(void)>;

  Command(
    const string& description,
    const vector<Flag>& flags,
    const vector<AnonFlag::ptr>& anon_flags,
    handler_type handler)
      : CommandBase(description),
        _handler(handler),
        _flags(flags),
        _anon_flags(anon_flags)
  {}

  Command(Command&& other) = default;
  Command(const Command& other) = delete;
  Command& operator=(const Command& other) = delete;

  static ptr make(
    const std::string& description,
    const std::vector<Flag>& flags,
    const std::vector<AnonFlag::ptr>& anon_flags,
    handler_type handler)
  {
    return make_shared<Command>(description, flags, anon_flags, handler);
  }

  virtual ~Command() {}

  const std::string& description() const;

  virtual int execute(std::deque<std::string>&& args) const override
  {
    {
      auto err = parse_args(_flags, _anon_flags, args);
      if (err.is_error()) {
        print_line(err.error());
        print_help();
        return 1;
      }
    }
    auto err = _handler();
    if (err.is_error()) {
      print_line(err.error().msg());
      return 1;
    }

    return 0;
  }

  void print_help() const
  {
    print_line("Accepted flags:");
    if (!_flags.empty()) {
      for (const auto& flag : _anon_flags) {
        print_line("    <$>", flag->doc());
      }
      for (const auto& flag : _flags) { print_line("    $", flag_name(flag)); }
    }
  }

 private:
  handler_type _handler;
  std::vector<Flag> _flags;
  std::vector<AnonFlag::ptr> _anon_flags;
};

} // namespace

////////////////////////////////////////////////////////////////////////////////
// CommandBuilder
//

CommandBuilder::CommandBuilder(const string& description)
    : _description(description)
{}

FlagWrapper<BooleanFlag> CommandBuilder::no_arg(const std::string& name)
{
  auto flag = BooleanFlag::create(name);
  _flags.push_back(flag);
  return wrap_flag(flag);
}

Cmd CommandBuilder::run(CommandBuilder::handler_type handler)
{
  return Cmd(
    Command::make(_description, _flags, _anon_flags, std::move(handler)));
}

} // namespace command
