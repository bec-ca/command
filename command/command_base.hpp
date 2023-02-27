#pragma once

#include <deque>
#include <memory>
#include <string>

namespace command {

struct CommandBase {
 public:
  CommandBase(const std::string& description);

  using ptr = std::shared_ptr<CommandBase>;
  virtual ~CommandBase();

  int main(int argc, char* argv[]) const;
  virtual int execute(std::deque<std::string>&& flags) const = 0;

  const std::string& description() const;

 private:
  std::string _description;
};

} // namespace command
