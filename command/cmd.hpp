#pragma once

#include <memory>

#include "bee/array_view.hpp"
#include "bee/log_output.hpp"

namespace command {

struct CommandBase;

struct Cmd {
 public:
  explicit Cmd(std::shared_ptr<CommandBase>&& base);
  ~Cmd();

  Cmd(const Cmd& other) = default;
  Cmd(Cmd&& other) = default;

  int main(
    int argc,
    const char* const* argv,
    bee::LogOutput log_output = bee::LogOutput::StdErr) const;

  const std::string& description() const;
  int execute(
    bee::LogOutput log_output, bee::ArrayView<const std::string> flags) const;

 private:
  std::shared_ptr<CommandBase> _base;
};

} // namespace command
