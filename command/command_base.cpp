#include "command_base.hpp"

namespace command {

CommandBase::CommandBase(const std::string_view& description)
    : _description(description)
{}

CommandBase::~CommandBase() {}

const std::string& CommandBase::description() const { return _description; }

} // namespace command
