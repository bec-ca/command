#pragma once

#include <string>

#include "bee/array_view.hpp"
#include "bee/log_output.hpp"

namespace command {

struct CommandBase {
 public:
  CommandBase(const std::string_view& description);

  virtual ~CommandBase();

  virtual int execute(
    bee::LogOutput log_output,
    bee::ArrayView<const std::string> flags) const = 0;

  const std::string& description() const;

 private:
  std::string _description;
};

} // namespace command
