#include "group_builder.hpp"

#include <string>
#include <variant>

#include "command_base.hpp"
#include "command_builder.hpp"

#include "bee/string_util.hpp"

using std::deque;
using std::map;
using std::max;
using std::string;

namespace command {

namespace {

////////////////////////////////////////////////////////////////////////////////
// CommandGroup
//

struct CommandGroup : public CommandBase {
 public:
  CommandGroup(const string& description, map<string, Cmd>&& handlers)
      : CommandBase(description), _handlers(std::move(handlers))
  {
    _add_cmd("help", CommandBuilder("Prints this help").run([&] {
      this->print_help();
      return bee::ok();
    }));
  }

  virtual ~CommandGroup() {}

  void print_help() const
  {
    P("Available comands:");

    size_t longest_name = 0;
    for (const auto& cmd : _handlers) {
      longest_name = max(longest_name, cmd.first.size());
    }

    for (const auto& cmd : _handlers) {
      P("  $  $",
        bee::right_pad_string(cmd.first, longest_name),
        cmd.second.base()->description());
    }
  }

  virtual int execute(std::deque<std::string>&& args) const override
  {
    if (args.empty()) {
      P("ERROR: No arguments given\n");
      print_help();
      return 1;
    }

    string cmd = args[0];

    auto it = _handlers.find(cmd);
    if (it == _handlers.end()) {
      P("Unknown command: $", cmd);
      print_help();
      return 1;
    } else {
      args.pop_front();
      return it->second.base()->execute(std::move(args));
    }
  }

 private:
  void _add_cmd(const string& name, const Cmd& command)
  {
    _handlers.emplace(name, command);
  }

  std::map<std::string, Cmd> _handlers;
};

} // namespace

////////////////////////////////////////////////////////////////////////////////
// GroupBuilder
//

GroupBuilder::GroupBuilder(const string& description)
    : _description(description)
{}

GroupBuilder& GroupBuilder::cmd(const std::string& name, const Cmd& command)
{
  _handlers.emplace(name, std::move(command));
  return *this;
}

Cmd GroupBuilder::build()
{
  return Cmd(make_shared<CommandGroup>(_description, std::move(_handlers)));
}

} // namespace command
