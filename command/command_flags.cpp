#include "command_flags.hpp"

using std::string;

namespace command {

NamedFlag::NamedFlag(const string& name) : _name(name) {}
NamedFlag::~NamedFlag() {}

const string& NamedFlag::name() const { return _name; }

BooleanFlag::BooleanFlag(const string& name) : NamedFlag(name), _value(false) {}

BooleanFlag::~BooleanFlag() {}

void BooleanFlag::set() { _value = true; }
const bool& BooleanFlag::value() const { return _value; }

BooleanFlag::ptr BooleanFlag::create(const string& name)
{
  return ptr(new BooleanFlag(name));
}

ValueFlag::ValueFlag(const string& name) : NamedFlag(name) {}

ValueFlag::~ValueFlag() {}

////////////////////////////////////////////////////////////////////////////////
// Flags
//

namespace flags {

const bee::OrError<string> StringFlag::operator()(const string& value) const
{
  return value;
}

StringFlag string_flag;

const bee::OrError<int> IntFlag::operator()(const string& value) const
{
  try {
    return stoi(value);
  } catch (const std::invalid_argument&) {
    return bee::Error("Not a numerical value");
  } catch (const std::out_of_range&) {
    return bee::Error("Numerical overflow");
  }
}

IntFlag int_flag;

const bee::OrError<double> FloatFlag::operator()(const string& value) const
{
  try {
    return stod(value);
  } catch (const std::invalid_argument&) {
    return bee::Error("Not a numerical value");
  } catch (const std::out_of_range&) {
    return bee::Error("Numerical overflow");
  }
}

FloatFlag float_flag;

} // namespace flags

} // namespace command
