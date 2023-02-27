#include "command_base.hpp"

using std::deque;
using std::string;

namespace command {

CommandBase::CommandBase(const string& description) : _description(description)
{}

CommandBase::~CommandBase() {}

int CommandBase::main(int argc, char* argv[]) const
{
  deque<string> args;
  for (int i = 1; i < argc; i++) { args.push_back(argv[i]); }

  return execute(std::move(args));
}

const string& CommandBase::description() const { return _description; }

} // namespace command
