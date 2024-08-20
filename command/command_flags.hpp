#pragma once

#include <memory>
#include <vector>

#include "flag_spec.hpp"

#include "bee/or_error.hpp"

namespace command {

using opt_str = std::optional<std::string>;
using opt_strview = std::optional<std::string_view>;

struct FlagDoc {
 public:
  std::string left;
  std::optional<std::string> right;
};

struct AnonFlag {
 public:
  using ptr = std::shared_ptr<AnonFlag>;

  explicit AnonFlag(
    const opt_strview& value_name,
    const opt_strview& doc,
    bool required,
    bool repeated)
      : _value_name(value_name),
        _doc(doc),
        _required(required),
        _repeated(repeated)
  {}

  virtual ~AnonFlag() {};

  virtual bee::OrError<> parse_value(const std::string_view& value) = 0;

  virtual bee::OrError<> finish_parsing() const = 0;

  FlagDoc make_doc() const;

  bool is_required() const { return _required; }

  bool is_repeated() const { return _repeated; }

  const opt_str& value_name() const;

 private:
  const std::optional<std::string> _value_name;
  const std::optional<std::string> _doc;
  const bool _required;
  const bool _repeated;
};

template <class S, class F> struct AnonFlagBase : public AnonFlag {
 public:
  using ptr = std::shared_ptr<F>;
  using value_type = typename S::value_type;

  virtual ~AnonFlagBase() {}

  static ptr create(
    const S& spec, const opt_strview& value_name, const opt_strview& doc)
  {
    return std::make_shared<F>(spec, value_name, doc);
  }

  virtual bee::OrError<> parse_value(const std::string_view& value) override
  {
    if (!is_repeated() && !_value.empty()) {
      return bee::Error("Flag already set");
    }
    bail(parsed_value, _spec.of_string(value));
    _value.push_back(std::move(parsed_value));
    return bee::ok();
  }

  virtual bee::OrError<> finish_parsing() const override
  {
    if (is_required() && _value.empty()) {
      if (auto vn = value_name()) {
        return bee::Error::fmt(
          "Anon flag <$> is required, but not provided", *vn);
      } else {
        return bee::Error::fmt("Anon flag is required, but not provided");
      }
    }
    return bee::ok();
  }

 protected:
  explicit AnonFlagBase(
    const S& spec,
    const opt_strview& value_name,
    const opt_strview& doc,
    bool required,
    bool repeated)
      : AnonFlag(value_name, doc, required, repeated), _spec(spec)
  {}

  const std::vector<value_type>& value() const { return _value; }

 private:
  std::vector<value_type> _value;
  const S _spec;
};

template <class S>
struct AnonFlagTemplate : public AnonFlagBase<S, AnonFlagTemplate<S>> {
 public:
  using parent = AnonFlagBase<S, AnonFlagTemplate<S>>;
  using value_type = typename S::value_type;

  const std::optional<value_type>& value() const
  {
    if (!_value_opt.has_value()) {
      const auto& value = parent::value();
      if (!value.empty()) { _value_opt = value[0]; }
    }
    return _value_opt;
  }

  explicit AnonFlagTemplate(
    const S& spec, const opt_strview& value_name, const opt_strview& doc)
      : parent(spec, value_name, doc, false, false)
  {}

 private:
  mutable std::optional<value_type> _value_opt;
};

template <class S>
struct RequiredAnonFlagTemplate
    : public AnonFlagBase<S, RequiredAnonFlagTemplate<S>> {
 public:
  using parent = AnonFlagBase<S, RequiredAnonFlagTemplate<S>>;

  const typename S::value_type& value() const { return parent::value()[0]; }

  explicit RequiredAnonFlagTemplate(
    const S& spec, const opt_strview& value_name, const opt_strview& doc)
      : parent(spec, value_name, doc, true, false)
  {}
};

template <class S>
struct RepeatedAnonFlagTemplate
    : public AnonFlagBase<S, RepeatedAnonFlagTemplate<S>> {
 public:
  using parent = AnonFlagBase<S, RepeatedAnonFlagTemplate<S>>;

  const std::vector<typename S::value_type>& value() const
  {
    return parent::value();
  }

  explicit RepeatedAnonFlagTemplate(
    const S& spec, const opt_strview& value_name, const opt_strview& doc)
      : parent(spec, value_name, doc, false, true)
  {}
};

struct NamedFlag {
 public:
  explicit NamedFlag(const std::string_view& name, const opt_strview& doc);

  virtual ~NamedFlag();

  const std::string& name() const;

  const opt_str& doc() const;

  virtual FlagDoc make_doc() const = 0;

 private:
  const std::string _name;
  const opt_str _doc;
};

struct BooleanFlag : public NamedFlag {
 public:
  using ptr = std::shared_ptr<BooleanFlag>;

  static ptr create(const std::string_view& name, const opt_strview& doc);

  virtual ~BooleanFlag();

  void set();

  const bool& value() const;

  virtual FlagDoc make_doc() const override;

 private:
  explicit BooleanFlag(const std::string_view& name, const opt_strview& doc);

  bool _value;
};

struct ValueFlag : public NamedFlag {
 public:
  using ptr = std::shared_ptr<ValueFlag>;

  explicit ValueFlag(
    const std::string_view& name,
    const opt_strview& value_name,
    const opt_strview& doc,
    bool required);

  virtual ~ValueFlag();

  virtual bee::OrError<> parse_value(const std::string_view& value) = 0;

  virtual bee::OrError<> finish_parsing() const = 0;

  virtual FlagDoc make_doc() const override;

  bool is_required() const { return _required; }

  virtual opt_str default_str() const = 0;

 private:
  const opt_str _value_name;
  const bool _required;
};

template <FlagSpec S> struct FlagTemplate : public ValueFlag {
 public:
  using value_type = typename S::value_type;
  using ptr = std::shared_ptr<FlagTemplate>;

  static ptr create(
    const std::string_view& name,
    const S& spec,
    const opt_strview& value_name,
    const opt_strview& doc)
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

  virtual bee::OrError<> parse_value(const std::string_view& value) override
  {
    bail(parsed_value, _spec.of_string(value));
    _value.emplace(std::move(parsed_value));
    return bee::ok();
  };

  virtual bee::OrError<> finish_parsing() const override
  {
    if (is_required() && !value().has_value()) {
      return bee::Error::fmt(
        "Flag $ is required, but not provided", this->name());
    }
    return bee::ok();
  }

  virtual opt_str default_str() const override
  {
    if (_def.has_value()) { return _spec.to_string(*_def); }
    return std::nullopt;
  }

 protected:
  explicit FlagTemplate(
    const std::string_view& name,
    const S& spec,
    const opt_strview& value_name,
    const opt_strview& doc,
    const std::optional<value_type>& def,
    bool required = false)
      : ValueFlag(name, value_name, doc, required), _spec(spec), _def(def)
  {}

 private:
  const S _spec;
  const std::optional<value_type> _def;
  std::optional<value_type> _value;
};

template <class S> struct RequiredFlagTemplate : public FlagTemplate<S> {
 public:
  using ptr = std::shared_ptr<RequiredFlagTemplate>;
  using value_type = typename S::value_type;
  static ptr create(
    const std::string_view& name,
    const S& spec,
    const opt_strview& value_name,
    const opt_strview& doc,
    const std::optional<value_type>& def = std::nullopt)
  {
    return ptr(new RequiredFlagTemplate(name, spec, value_name, doc, def));
  }

  virtual bee::OrError<> finish_parsing() const override
  {
    if (!FlagTemplate<S>::value().has_value()) {
      return bee::Error::fmt(
        "Flag $ is required, but not provided", this->name());
    }
    return bee::ok();
  }

  const auto& value() const { return *FlagTemplate<S>::value(); }

 private:
  explicit RequiredFlagTemplate(
    const std::string_view& name,
    const S& spec,
    const opt_strview& value_name,
    const opt_strview& doc,
    const std::optional<value_type>& def)
      : FlagTemplate<S>(name, spec, value_name, doc, def, !def.has_value())
  {}
};

namespace flags {

struct StringFlag {
  using value_type = std::string;
  bee::OrError<value_type> of_string(const std::string_view& value) const;
  std::string to_string(const std::string_view& value) const;
};

constexpr StringFlag String;

struct IntFlag {
  using value_type = int;
  bee::OrError<value_type> of_string(const std::string_view& value) const;
  std::string to_string(int value) const;
};

constexpr IntFlag Int;

struct FloatFlag {
  using value_type = double;
  bee::OrError<value_type> of_string(const std::string_view& value) const;
  std::string to_string(float value) const;
};

constexpr FloatFlag Float;

} // namespace flags

using Flag = std::variant<ValueFlag::ptr, BooleanFlag::ptr>;

} // namespace command
