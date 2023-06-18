#include "command_base.hpp"
#include "command_builder.hpp"

#include "bee/format_optional.hpp"
#include "bee/format_vector.hpp"
#include "bee/testing.hpp"

using std::deque;
using std::optional;
using std::string;
using std::vector;

namespace command {
namespace {

using namespace command::flags;

void run_command(vector<string> args, const Cmd& cmd)
{
  deque<string> d(args.begin(), args.end());
  int output = cmd.base()->execute(std::move(d));
  P("exit_code=$", output);
}

bee::OrError<> print_flags(
  const optional<string>& sflag,
  const optional<int>& iflag,
  const bool& bflag,
  const optional<string>& asflag,
  const optional<int>& aiflag)
{
  P(sflag);
  P(iflag);
  P(bflag);
  P(asflag);
  P(aiflag);
  return bee::ok();
}

TEST(args)
{
  int test_count = 1;

  auto run_test = [&](vector<string> args) {
    P("test $", test_count++);
    P("args: '$'", args);
    auto builder = CommandBuilder("Sub command");
    auto sflag = builder.optional("--str", string_flag);
    auto iflag = builder.optional("--int", int_flag);
    auto bflag = builder.no_arg("--bool");
    auto asflag = builder.anon(string_flag, "asflag");
    auto aiflag = builder.anon(int_flag, "aiflag");
    run_command(std::move(args), builder.run([=]() {
      return print_flags(*sflag, *iflag, *bflag, *asflag, *aiflag);
    }));
    P("");
  };

  run_test({});
  run_test({"--help"});
  run_test({"--str", "string"});
  run_test({"--int", "123"});
  run_test({"--bool"});
  run_test({"--str"});
  run_test({"--int", "abc"});
  run_test({"--int", "123a"});
  run_test({"--bool", "--int", "123", "--str", "yo"});
  run_test({"anon"});
  run_test({"anon", "anon"});
  run_test({"anon", "123"});
}

TEST(required_args)
{
  int test_count = 1;

  auto run_test = [&](vector<string> args) {
    P("test $", test_count++);
    P("args: '$'", args);
    auto builder = CommandBuilder("Sub command");
    auto oflag = builder.optional("--str", string_flag);
    auto rflag = builder.required("--filename", string_flag);
    run_command(std::move(args), builder.run([=]() {
      P("oflag:$ rflag:$", *oflag, *rflag);
      return bee::ok();
    }));
    P("------------------------------------");
  };

  run_test({});
  run_test({"--help"});
  run_test({"--str", "string"});
  run_test({"--filename", "foobat.txt"});
  run_test({"--filename", "foobat.txt", "--str", "value"});
}

TEST(optional_with_default)
{
  int test_count = 1;

  auto run_test = [&](vector<string> args) {
    P("test $", test_count++);
    P("args: '$'", args);
    auto builder = CommandBuilder("Sub command");
    auto flag1 = builder.optional_with_default("--flag", string_flag, "foobar");
    auto flag2 = builder.optional("--other", string_flag);
    run_command(std::move(args), builder.run([=]() {
      P("flag1:$ flag2:$", *flag1, *flag2);
      return bee::ok();
    }));
    P("------------------------------------");
  };

  run_test({});
  run_test({"--help"});
  run_test({"--invalid-flag"});
  run_test({"--flag", "foo"});
  run_test({"--other", "bar"});
  run_test({"--flag", "bar", "--other", "foo"});
}

TEST(required_anon)
{
  int test_count = 1;

  auto run_test = [&](vector<string> args) {
    P("test $", test_count++);
    P("args: '$'", args);
    auto builder = CommandBuilder("Sub command");
    auto rflag = builder.required_anon(string_flag, "rflag");
    auto oflag = builder.anon(string_flag, "oflag");
    run_command(std::move(args), builder.run([=]() {
      P("rflag:$ oflag:$", *rflag, *oflag);
      return bee::ok();
    }));
    P("------------------------------------");
  };

  run_test({});
  run_test({"foobat.txt"});
  run_test({"foobat.txt", "value"});
}

} // namespace
} // namespace command
