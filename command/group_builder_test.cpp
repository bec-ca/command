#include "group_builder.hpp"

#include "command_builder.hpp"

#include "bee/error.hpp"
#include "bee/format.hpp"
#include "bee/format_optional.hpp"
#include "bee/testing.hpp"

using bee::print_line;
using std::string;
using std::vector;

namespace command {
namespace {

bee::OrError<bee::Unit> example_app()
{
  print_line("Hello world");
  return bee::unit;
}

void run_cmd(vector<string> args, Cmd& cmd)
{
  vector<char*> argv;
  for (auto& el : args) { argv.push_back(el.data()); }

  int output = cmd.main(argv.size(), argv.data());
  print_line("exit_code=$", output);
}

TEST(basic)
{
  auto run_test = [&](const vector<string>& args) {
    auto builder = CommandBuilder("Sub command");
    auto grp =
      GroupBuilder("group").cmd("subcommand", builder.run(example_app)).build();
    run_cmd(args, grp);
  };

  print_line("--------------------------------------------");
  print_line("test 1");
  run_test({});

  print_line("--------------------------------------------");
  print_line("test 2");
  run_test({"binary"});

  print_line("--------------------------------------------");
  print_line("test 2");
  run_test({"binary", "subcommand"});

  print_line("--------------------------------------------");
  print_line("test 3");
  run_test({"binary", "nocmd"});

  print_line("--------------------------------------------");
  print_line("test 4");
  run_test({"binary", "help"});
}

} // namespace
} // namespace command
