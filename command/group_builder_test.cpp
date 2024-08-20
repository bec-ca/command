#include "command_builder.hpp"
#include "group_builder.hpp"

#include "bee/or_error.hpp"
#include "bee/testing.hpp"

using std::string;
using std::vector;

namespace command {
namespace {

bee::OrError<> example_app()
{
  P("Hello world");
  return bee::ok();
}

void run_cmd(const vector<string>& args, Cmd& cmd)
{
  vector<const char*> argv;
  for (auto& el : args) { argv.push_back(el.data()); }

  int output = cmd.main(argv.size(), argv.data(), bee::LogOutput::StdOut);
  P("exit_code=$", output);
}

TEST(basic)
{
  auto run_test = [&](const vector<string>& args) {
    auto builder = CommandBuilder("Sub command");
    auto grp =
      GroupBuilder("group").cmd("subcommand", builder.run(example_app)).build();
    run_cmd(args, grp);
  };

  P("--------------------------------------------");
  P("test 1");
  run_test({});

  P("--------------------------------------------");
  P("test 2");
  run_test({"binary"});

  P("--------------------------------------------");
  P("test 2");
  run_test({"binary", "subcommand"});

  P("--------------------------------------------");
  P("test 3");
  run_test({"binary", "nocmd"});

  P("--------------------------------------------");
  P("test 4");
  run_test({"binary", "help"});
}

} // namespace
} // namespace command
