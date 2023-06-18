#include "command_flags.hpp"

#include <vector>

using std::string;

namespace command {

////////////////////////////////////////////////////////////////////////////////
// AnonFlag
//

FlagDoc AnonFlag::make_doc() const
{
  auto value_name = [&]() {
    auto value_name =
      _value_name.has_value() ? F("<$>", *_value_name) : "<VALUE>";
    if (!_required) {
      return F("[$]", value_name);
    } else {
      return value_name;
    }
  };
  return {
    .left = value_name(),
    .right = _doc,
  };
}

const opt_str& AnonFlag::value_name() const { return _value_name; }

////////////////////////////////////////////////////////////////////////////////
// NamedFlag
//

NamedFlag::NamedFlag(const string& name, const opt_str& doc)
    : _name(name), _doc(doc)
{}
NamedFlag::~NamedFlag() {}

const string& NamedFlag::name() const { return _name; }
const opt_str& NamedFlag::doc() const { return _doc; }

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

FlagDoc BooleanFlag::make_doc() const
{
  return {
    .left = F("[$]", name()),
    .right = doc(),
  };
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

FlagDoc ValueFlag::make_doc() const
{
  auto value_name = _value_name.has_value() ? F("<$>", *_value_name) : "_";
  auto name_and_value = F("$ $", name(), value_name);
  if (!is_required()) { name_and_value = F("[$]", name_and_value); }

  string doc_str;
  if (const auto& d = doc()) { doc_str = *d; }
  if (auto def_value = default_str()) {
    if (!doc_str.empty()) { doc_str += ' '; };
    doc_str += F("[default = $]", *def_value);
  }
  return {
    .left = name_and_value,
    .right = doc_str,
  };
}

////////////////////////////////////////////////////////////////////////////////
// Flags
//

namespace flags {

////////////////////////////////////////////////////////////////////////////////
// StringFlag
//

bee::OrError<string> StringFlag::of_string(const string& value) const
{
  return value;
}

std::string StringFlag::to_string(const string& value) const { return value; }

////////////////////////////////////////////////////////////////////////////////
// IntFlag
//

bee::OrError<int> IntFlag::of_string(const string& value) const
{
  try {
    return stoi(value);
  } catch (const std::invalid_argument&) {
    return bee::Error("Not a numerical value");
  } catch (const std::out_of_range&) {
    return bee::Error("Numerical overflow");
  }
}

std::string IntFlag::to_string(int value) const { return F(value); }

////////////////////////////////////////////////////////////////////////////////
// FlagFlag
//

bee::OrError<double> FloatFlag::of_string(const string& value) const
{
  try {
    return stod(value);
  } catch (const std::invalid_argument&) {
    return bee::Error("Not a numerical value");
  } catch (const std::out_of_range&) {
    return bee::Error("Numerical overflow");
  }
}

std::string FloatFlag::to_string(float value) const { return F(value); }

} // namespace flags

} // namespace command
