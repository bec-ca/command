#include "cmd.hpp"

#include "command_base.hpp"

namespace command {

int Cmd::main(int argc, char* argv[]) const { return _base->main(argc, argv); }

} // namespace command
