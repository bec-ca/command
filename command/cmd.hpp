#pragma once

#include <memory>

namespace command {

struct CommandBase;

struct Cmd {
 public:
  explicit Cmd(const std::shared_ptr<CommandBase>& base) : _base(base) {}

  Cmd(const Cmd& other) = default;
  Cmd(Cmd&& other) = default;

  const std::shared_ptr<CommandBase>& base() const { return _base; }

  int main(int argc, char* argv[]) const;

 private:
  std::shared_ptr<CommandBase> _base;
};

} // namespace command
