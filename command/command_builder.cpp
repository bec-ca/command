#include "command_builder.hpp"

#include <deque>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "command_base.hpp"

#include "bee/error.hpp"
#include "bee/string_util.hpp"
#include "bee/util.hpp"
#include "command/command_flags.hpp"

using std::decay_t;
using std::deque;
using std::is_same_v;
using std::string;
using std::vector;

namespace command {
namespace {

FlagDoc make_doc(const Flag& flag)
{
  return visit(
    [&](const auto& flag) -> FlagDoc { return flag->make_doc(); }, flag);
}

bool is_required(const Flag& flag)
{
  return visit(
    [&]<class T>(const T& flag) -> bool {
      if constexpr (std::is_same_v<T, ValueFlag::ptr>) {
        return flag->is_required();
      } else {
        return false;
      }
    },
    flag);
}

const string& flag_name(const Flag& flag)
{
  return visit([&](auto flag) -> const string& { return flag->name(); }, flag);
}

bee::OrError<Flag> find_flag(const vector<Flag>& flags, const string& name)
{
  for (const auto& flag : flags) {
    if (name == flag_name(flag)) { return flag; }
  }
  return bee::Error::fmt("Unknown flag '$'", name);
}

bee::OrError<> parse_args(
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
        [&](auto flag) -> bee::OrError<> {
          using T = decay_t<decltype(flag)>;
          if constexpr (is_same_v<T, ValueFlag::ptr>) {
            if (i >= args.size()) {
              return bee::Error::fmt("No arguments for flag $", arg);
            }
            auto value = args.at(i++);
            auto err = flag->parse_value(value);
            if (err.is_error()) {
              return bee::Error::fmt(
                "Failed to parse flag $ with value '$': $",
                arg,
                value,
                err.error());
            }
            return bee::ok();
          } else if constexpr (is_same_v<T, BooleanFlag::ptr>) {
            flag->set();
            return bee::ok();
          } else {
            static_assert(bee::always_false_v<T> && "visit is not exhaustive");
          }
        },
        flag));
    } else {
      if (anon_flag_index >= anon_flags.size()) {
        return bee::Error::fmt("Unexpected anonymous argument '$'", arg);
      }
      auto err = anon_flags[anon_flag_index]->parse_value(arg);
      if (err.is_error()) {
        return bee::Error::fmt(
          "Failed to parse anon flag with value '$': $", arg, err.error());
      }
      anon_flag_index++;
    }
  }
  for (const auto& flag : named_flags) {
    bail_unit(visit(
      [&](auto flag) -> bee::OrError<> {
        using T = decay_t<decltype(flag)>;
        if constexpr (is_same_v<T, ValueFlag::ptr>) {
          return flag->finish_parsing();
        } else if constexpr (is_same_v<T, BooleanFlag::ptr>) {
          return bee::ok();
        } else {
          static_assert(bee::always_false_v<T> && "visit is not exhaustive");
        }
      },
      flag));
  }
  for (const auto& anon_flag : anon_flags) {
    bail_unit(anon_flag->finish_parsing());
  }
  return bee::ok();
}

////////////////////////////////////////////////////////////////////////////////
// Command
//

bool by_optional(const Flag& f1, const Flag& f2)
{
  bool req1 = is_required(f1);
  bool req2 = is_required(f2);
  return req1 && !req2;
}

struct Command : public CommandBase {
 public:
  Command(
    const string& description,
    const vector<Flag>& flags,
    const vector<AnonFlag::ptr>& anon_flags,
    handler_type handler)
      : CommandBase(description),
        _handler(handler),
        _flags(flags),
        _anon_flags(anon_flags)
  {
    std::stable_sort(_flags.begin(), _flags.end(), by_optional);
    _show_help = BooleanFlag::create("--help", "Displays this help");
    _flags.push_back(_show_help);
  }

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
      if (_show_help->value()) {
        print_help();
        return 0;
      }

      if (err.is_error()) {
        P("ERROR: $\n", err.error());
        print_help();
        return 1;
      }
    }

    auto err = _handler();
    if (err.is_error()) {
      P("Application exited with error:");
      P(err.error().full_msg());
      return 1;
    }

    return 0;
  }

  void print_help() const
  {
    vector<FlagDoc> docs;
    P("Accepted flags:");
    for (const auto& flag : _anon_flags) { docs.push_back(flag->make_doc()); }
    for (const auto& flag : _flags) { docs.push_back(make_doc(flag)); }

    int max_left = 0;
    for (const auto& doc : docs) {
      max_left = std::max<int>(max_left, doc.left.size());
    }

    for (const auto& doc : docs) {
      string line = "    " + bee::right_pad_string(doc.left, max_left);
      if (doc.right.has_value() && !doc.right->empty()) {
        line += "  ";
        line += *doc.right;
      }
      P(line);
    }
  }

 private:
  handler_type _handler;
  std::vector<Flag> _flags;
  std::vector<AnonFlag::ptr> _anon_flags;
  BooleanFlag::ptr _show_help;
};

} // namespace

////////////////////////////////////////////////////////////////////////////////
// CommandBuilder
//

CommandBuilder::CommandBuilder(const string& description)
    : _description(description)
{}

FlagWrapper<BooleanFlag> CommandBuilder::no_arg(
  const std::string& name, const opt_str& doc)
{
  auto flag = BooleanFlag::create(name, doc);
  _flags.push_back(flag);
  return wrap_flag(flag);
}

Cmd CommandBuilder::run(handler_type handler)
{
  return Cmd(
    Command::make(_description, _flags, _anon_flags, std::move(handler)));
}

} // namespace command
