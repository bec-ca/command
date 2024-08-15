#include "group_builder.hpp"

#include <string>

#include "command_base.hpp"

#include "bee/print.hpp"
#include "bee/string_util.hpp"

namespace command {
namespace {

////////////////////////////////////////////////////////////////////////////////
// CommandGroup
//

struct CommandGroup final : public CommandBase {
 private:
  struct HelpPrinter final : public CommandBase {
    HelpPrinter(CommandGroup& parent)
        : CommandBase("Prints this help"), _parent(parent)
    {}
    virtual int execute(
      const bee::LogOutput log_output,
      const bee::ArrayView<const std::string>) const override
    {
      _parent.print_help(log_output);
      return 0;
    }

   private:
    CommandGroup& _parent;
  };

 public:
  CommandGroup(
    const std::string_view& description, std::map<std::string, Cmd>&& handlers)
      : CommandBase(description), _handlers(std::move(handlers))
  {
    _add_cmd("help", Cmd(std::make_shared<HelpPrinter>(*this)));
  }

  virtual ~CommandGroup() {}

  void print_help(const bee::LogOutput log_output) const
  {
    PF(log_output, "Available comands:");

    size_t longest_name = 0;
    for (const auto& cmd : _handlers) {
      longest_name = std::max(longest_name, cmd.first.size());
    }

    for (const auto& cmd : _handlers) {
      PF(
        log_output,
        "  $  $",
        bee::right_pad_string(cmd.first, longest_name),
        cmd.second.description());
    }
  }

  virtual int execute(
    const bee::LogOutput log_output,
    const bee::ArrayView<const std::string> args) const override
  {
    if (args.empty()) {
      PF(log_output, "ERROR: No arguments given\n");
      print_help(log_output);
      return 1;
    }

    const std::string& cmd = args.front();
    auto it = _handlers.find(cmd);
    if (it == _handlers.end()) {
      PF(log_output, "Unknown command: $", cmd);
      print_help(log_output);
      return 1;
    } else {
      return it->second.execute(log_output, args.slice(1));
    }
  }

 private:
  void _add_cmd(const std::string_view& name, const Cmd& command)
  {
    _handlers.emplace(name, command);
  }

  std::map<std::string, Cmd> _handlers;
};

} // namespace

////////////////////////////////////////////////////////////////////////////////
// GroupBuilder
//

GroupBuilder::GroupBuilder(const std::string_view& description)
    : _description(description)
{}

GroupBuilder& GroupBuilder::cmd(
  const std::string_view& name, const Cmd& command)
{
  _handlers.emplace(name, std::move(command));
  return *this;
}

Cmd GroupBuilder::build()
{
  return Cmd(make_shared<CommandGroup>(_description, std::move(_handlers)));
}

} // namespace command
