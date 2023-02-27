#pragma once

#include "cmd.hpp"

#include <map>
#include <string>

namespace command {

struct GroupBuilder {
 public:
  GroupBuilder(const std::string& description);

  GroupBuilder& cmd(const std::string& name, const Cmd& command);

  Cmd build();

  const std::string& description() const;

 private:
  std::map<std::string, Cmd> _handlers;

  std::string _description;
};

} // namespace command
