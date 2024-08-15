#pragma once

#include <map>
#include <string>

#include "cmd.hpp"

namespace command {

struct GroupBuilder {
 public:
  GroupBuilder(const std::string_view& description);

  GroupBuilder& cmd(const std::string_view& name, const Cmd& command);

  Cmd build();

  const std::string& description() const;

 private:
  std::map<std::string, Cmd> _handlers;

  std::string _description;
};

} // namespace command
