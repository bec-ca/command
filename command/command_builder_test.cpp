#include <stdexcept>

#include "command_builder.hpp"

#include "bee/format_optional.hpp"
#include "bee/format_vector.hpp"
#include "bee/location.hpp"
#include "bee/or_error.hpp"
#include "bee/print.hpp"
#include "bee/testing.hpp"

using std::optional;
using std::string;
using std::vector;

namespace command {
namespace {

using namespace command::flags;

void run_command(const vector<string>& args, const Cmd& cmd)
{
  static const std::string binary = "binary";
  vector<const char*> argv = {binary.data()};
  for (auto& el : args) { argv.push_back(el.data()); }

  int output = cmd.main(argv.size(), argv.data(), bee::LogOutput::StdOut);
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
    auto sflag = builder.optional("--str", flags::String);
    auto iflag = builder.optional("--int", flags::Int);
    auto bflag = builder.no_arg("--bool");
    auto asflag = builder.anon(flags::String, "asflag");
    auto aiflag = builder.anon(flags::Int, "aiflag");
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
    auto oflag = builder.optional("--str", flags::String);
    auto rflag = builder.required("--filename", flags::String);
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
    auto flag1 =
      builder.optional_with_default("--flag", flags::String, "foobar");
    auto flag2 = builder.optional("--other", flags::String);
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
    auto rflag = builder.required_anon(flags::String, "rflag");
    auto oflag = builder.anon(flags::String, "oflag");
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

TEST(repeated_anon)
{
  int test_count = 1;

  auto run_test = [&](vector<string> args) {
    P("test $", test_count++);
    P("args: '$'", args);
    auto builder = CommandBuilder("Sub command");
    auto aflag = builder.anon(flags::String, "aflag");
    auto rflag = builder.repeated_anon(flags::String, "rflag");
    run_command(std::move(args), builder.run([=]() {
      P("aflag:$ rflag:$ ", *aflag, *rflag);
      return bee::ok();
    }));
    P("------------------------------------");
  };

  run_test({});
  run_test({"foobar.txt"});
  run_test({"foobar.txt", "value"});
  run_test({"foobar.txt", "value", "other"});
  run_test({"--", "cmd", "--flag", "--other-flag"});
}

TEST(exception)
{
  auto builder = CommandBuilder("Sub command");
  run_command({}, builder.run([=]() {
    throw bee::Exn(bee::Location("filename.cpp", 10), "Failed");
    return bee::ok();
  }));
}

TEST(std_exception)
{
  auto run_test = [&]() {
    auto builder = CommandBuilder("Sub command");
    run_command({}, builder.run([=]() {
      throw std::runtime_error("error");
      return bee::ok();
    }));
  };

  P(bee::try_with([&]() -> bee::OrError<> {
    run_test();
    return bee::ok();
  }));
}

} // namespace
} // namespace command
