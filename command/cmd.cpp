#include "cmd.hpp"

#include <vector>

#include "command_base.hpp"

#include "bee/array_view.hpp"
#include "bee/log_output.hpp"

namespace command {

Cmd::Cmd(std::shared_ptr<CommandBase>&& base) : _base(std::move(base)) {}

Cmd::~Cmd() {}

int Cmd::main(
  const int argc,
  const char* const* const argv,
  const bee::LogOutput log_output) const
{
  std::vector<std::string> args;
  for (int i = 1; i < argc; i++) { args.push_back(argv[i]); }
  return execute(log_output, args);
}

int Cmd::execute(
  const bee::LogOutput log_output,
  const bee::ArrayView<const std::string> flags) const
{
  return _base->execute(log_output, flags);
}

const std::string& Cmd::description() const { return _base->description(); }

} // namespace command
