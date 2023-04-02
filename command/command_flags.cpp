#include "command_flags.hpp"

using std::string;

namespace command {

////////////////////////////////////////////////////////////////////////////////
// AnonFlag
//

std::string AnonFlag::make_doc() const
{
  auto value_name = _value_name.has_value() ? *_value_name : "VALUE";
  auto doc = _doc.has_value() ? bee::format(": $", *_doc) : "";
  if (!_required) { value_name = bee::format("[$]", value_name); }
  return bee::format("<$>$", value_name, doc);
}

////////////////////////////////////////////////////////////////////////////////
// NamedFlag
//

NamedFlag::NamedFlag(const string& name, const opt_str& doc)
    : _name(name), _doc(doc)
{}
NamedFlag::~NamedFlag() {}

const string& NamedFlag::name() const { return _name; }
string NamedFlag::doc() const
{
  return _doc.has_value() ? bee::format(" $", *_doc) : "";
}

////////////////////////////////////////////////////////////////////////////////
// BooleanFlag
//

BooleanFlag::BooleanFlag(const string& name, const opt_str& doc)
    : NamedFlag(name, doc), _value(false)
{}

BooleanFlag::~BooleanFlag() {}

void BooleanFlag::set() { _value = true; }
const bool& BooleanFlag::value() const { return _value; }

BooleanFlag::ptr BooleanFlag::create(const string& name, const opt_str& doc)
{
  return ptr(new BooleanFlag(name, doc));
}

string BooleanFlag::make_doc() const
{
  return bee::format("$ $", name(), doc());
}

////////////////////////////////////////////////////////////////////////////////
// ValueFlag
//

ValueFlag::ValueFlag(
  const string& name,
  const opt_str& value_name,
  const opt_str& doc,
  bool required)
    : NamedFlag(name, doc), _value_name(value_name), _required(required)
{}

ValueFlag::~ValueFlag() {}

string ValueFlag::make_doc() const
{
  auto value_name = _value_name.has_value() ? *_value_name : "_";
  auto name_and_value = bee::format("$ <$>", name(), value_name);
  name_and_value =
    is_required() ? name_and_value : bee::format("[$]", name_and_value);
  return bee::format("$ $", name_and_value, doc());
}

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
