#pragma once

#include "bee/error.hpp"

#include "flag_spec.hpp"

#include <memory>

namespace command {

using opt_str = std::optional<std::string>;

struct AnonFlag {
 public:
  using ptr = std::shared_ptr<AnonFlag>;

  explicit AnonFlag(
    const opt_str& value_name, const opt_str& doc, bool required)
      : _value_name(value_name), _doc(doc), _required(required)
  {}

  virtual ~AnonFlag(){};

  virtual bee::OrError<bee::Unit> parse_value(const std::string& value) = 0;

  virtual bee::OrError<bee::Unit> finish_parsing() const = 0;

  std::string make_doc() const;

  bool is_required() const { return _required; }

  const opt_str& value_name() const;

 private:
  const opt_str _value_name;
  const opt_str _doc;
  const bool _required;
};

template <class P, class F> struct AnonFlagBase : public AnonFlag {
 public:
  using ptr = std::shared_ptr<F>;
  using value_type = typename P::value_type;

  virtual ~AnonFlagBase() {}

  const std::optional<value_type>& value() const { return _value; }

  static ptr create(
    const P& spec, const opt_str& value_name, const opt_str& doc)
  {
    return std::make_shared<F>(spec, value_name, doc);
  }

  virtual bee::OrError<bee::Unit> parse_value(const std::string& value) override
  {
    bail(parsed_value, _spec.of_string(value));
    _value = parsed_value;
    return bee::unit;
  }

  virtual bee::OrError<bee::Unit> finish_parsing() const override
  {
    if (is_required() && !_value.has_value()) {
      if (auto vn = value_name()) {
        return bee::Error::format(
          "Anon flag <$> is required, but not provided", *vn);
      } else {
        return bee::Error::format("Anon flag is required, but not provided");
      }
    }
    return bee::unit;
  }

 protected:
  explicit AnonFlagBase(
    const P& spec, const opt_str& value_name, const opt_str& doc, bool required)
      : AnonFlag(value_name, doc, required), _spec(spec)
  {}

 private:
  std::optional<value_type> _value;
  const P _spec;
};

template <class P>
struct AnonFlagTemplate : public AnonFlagBase<P, AnonFlagTemplate<P>> {
 public:
  using parent = AnonFlagBase<P, AnonFlagTemplate<P>>;

  virtual ~AnonFlagTemplate() {}

  explicit AnonFlagTemplate(
    const P& spec, const opt_str& value_name, const opt_str& doc)
      : parent(spec, value_name, doc, false)
  {}
};

template <class P>
struct RequiredAnonFlagTemplate
    : public AnonFlagBase<P, RequiredAnonFlagTemplate<P>> {
 public:
  using parent = AnonFlagBase<P, RequiredAnonFlagTemplate<P>>;

  virtual ~RequiredAnonFlagTemplate() {}

  const typename P::value_type& value() const { return *parent::value(); }

  explicit RequiredAnonFlagTemplate(
    const P& spec, const opt_str& value_name, const opt_str& doc)
      : parent(spec, value_name, doc, true)
  {}
};

struct NamedFlag {
 public:
  explicit NamedFlag(const std::string& name, const opt_str& doc);

  virtual ~NamedFlag();

  const std::string& name() const;

  const opt_str& doc() const;

  virtual std::string make_doc() const = 0;

 private:
  const std::string _name;
  const opt_str _doc;
};

struct BooleanFlag : public NamedFlag {
 public:
  using ptr = std::shared_ptr<BooleanFlag>;

  static ptr create(const std::string& name, const opt_str& doc);

  virtual ~BooleanFlag();

  void set();

  const bool& value() const;

  virtual std::string make_doc() const override;

 private:
  explicit BooleanFlag(const std::string& name, const opt_str& doc);

  bool _value;
};

struct ValueFlag : public NamedFlag {
 public:
  using ptr = std::shared_ptr<ValueFlag>;

  explicit ValueFlag(
    const std::string& name,
    const opt_str& value_name,
    const opt_str& doc,
    bool required);

  virtual ~ValueFlag();

  virtual bee::OrError<bee::Unit> parse_value(const std::string& value) = 0;

  virtual bee::OrError<bee::Unit> finish_parsing() const = 0;

  virtual std::string make_doc() const override;

  bool is_required() const { return _required; }

  virtual opt_str default_str() const = 0;

 private:
  const opt_str _value_name;
  const bool _required;
};

template <FlagSpec P> struct FlagTemplate : public ValueFlag {
 public:
  using value_type = typename P::value_type;
  using ptr = std::shared_ptr<FlagTemplate>;

  static ptr create(
    const std::string& name,
    const P& spec,
    const opt_str& value_name,
    const opt_str& doc)
  {
    return ptr(new FlagTemplate(name, spec, value_name, doc, std::nullopt));
  }

  const std::optional<value_type>& value() const
  {
    if (!_value.has_value()) {
      return _def;
    } else {
      return _value;
    }
  }

  virtual bee::OrError<bee::Unit> parse_value(const std::string& value) override
  {
    bail(parsed_value, _spec.of_string(value));
    _value.emplace(std::move(parsed_value));
    return bee::unit;
  };

  virtual bee::OrError<bee::Unit> finish_parsing() const override
  {
    if (is_required() && !value().has_value()) {
      return bee::Error::format(
        "Flag $ is required, but not provided", this->name());
    }
    return bee::unit;
  }

  virtual opt_str default_str() const override
  {
    if (_def.has_value()) { return _spec.to_string(*_def); }
    return std::nullopt;
  }

 protected:
  explicit FlagTemplate(
    const std::string& name,
    const P& spec,
    const opt_str& value_name,
    const opt_str& doc,
    const std::optional<value_type>& def,
    bool required = false)
      : ValueFlag(name, value_name, doc, required), _spec(spec), _def(def)
  {}

 private:
  const P _spec;
  const std::optional<value_type> _def;
  std::optional<value_type> _value;
};

template <class P> struct RequiredFlagTemplate : public FlagTemplate<P> {
 public:
  using ptr = std::shared_ptr<RequiredFlagTemplate>;
  using value_type = typename P::value_type;
  static ptr create(
    const std::string& name,
    const P& spec,
    const opt_str& value_name,
    const opt_str& doc,
    const std::optional<value_type>& def = std::nullopt)
  {
    return ptr(new RequiredFlagTemplate(name, spec, value_name, doc, def));
  }

  virtual bee::OrError<bee::Unit> finish_parsing() const override
  {
    if (!FlagTemplate<P>::value().has_value()) {
      return bee::Error::format(
        "Flag $ is required, but not provided", this->name());
    }
    return bee::unit;
  }

  const auto& value() const { return *FlagTemplate<P>::value(); }

 private:
  explicit RequiredFlagTemplate(
    const std::string& name,
    const P& spec,
    const opt_str& value_name,
    const opt_str& doc,
    const std::optional<value_type>& def)
      : FlagTemplate<P>(name, spec, value_name, doc, def, true)
  {}
};

namespace flags {

struct StringFlag {
  using value_type = std::string;
  bee::OrError<value_type> of_string(const std::string& value) const;
  std::string to_string(const std::string& value) const;
};

constexpr StringFlag string_flag;

struct IntFlag {
  using value_type = int;
  bee::OrError<value_type> of_string(const std::string& value) const;
  std::string to_string(int value) const;
};

constexpr IntFlag int_flag;

struct FloatFlag {
  using value_type = double;
  bee::OrError<value_type> of_string(const std::string& value) const;
  std::string to_string(float value) const;
};

constexpr FloatFlag float_flag;

} // namespace flags

using Flag = std::variant<ValueFlag::ptr, BooleanFlag::ptr>;

} // namespace command
