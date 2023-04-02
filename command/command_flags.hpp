#pragma once

#include "bee/error.hpp"

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
    bail(parsed_value, _spec(value));
    _value = parsed_value;
    return bee::unit;
  }

  virtual bee::OrError<bee::Unit> finish_parsing() const override
  {
    if (is_required() && !_value.has_value()) {
      return bee::Error::format("Anon flag is required, but not provided");
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

  std::string doc() const;

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

 private:
  const opt_str _value_name;
  const bool _required;
};

template <class P> struct FlagTemplate : public ValueFlag {
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

  const std::optional<value_type>& value() const { return _value; }

  virtual bee::OrError<bee::Unit> parse_value(const std::string& value) override
  {
    bail(parsed_value, _spec(value));
    _value = std::move(parsed_value);
    return bee::unit;
  };

  virtual bee::OrError<bee::Unit> finish_parsing() const override
  {
    if (is_required() && !_value.has_value()) {
      return bee::Error::format(
        "Flag $ is required, but not provided", this->name());
    }
    return bee::unit;
  }

 protected:
  explicit FlagTemplate(
    const std::string& name,
    const P& spec,
    const opt_str& value_name,
    const opt_str& doc,
    const std::optional<value_type>& def,
    bool required = false)
      : ValueFlag(name, value_name, doc, required), _spec(spec), _value(def)
  {}

 private:
  const P _spec;
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

  virtual bee::OrError<bee::Unit> finish_parsing() const
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
  const bee::OrError<value_type> operator()(const std::string& value) const;
} extern string_flag;

struct IntFlag {
  using value_type = int;
  const bee::OrError<value_type> operator()(const std::string& value) const;
} extern int_flag;

struct FloatFlag {
  using value_type = double;
  const bee::OrError<value_type> operator()(const std::string& value) const;
} extern float_flag;

template <class T>
concept string_parser = requires(std::string str) {
                          {
                            T::of_string(str)
                            } -> std::convertible_to<bee::OrError<T>>;
                        };

template <class T> struct flag_of_value_type_t {
 public:
  using value_type = T;
  const bee::OrError<value_type> operator()(const std::string& value) const
  {
    return T::of_string(value);
  }
};

template <class T>
constexpr flag_of_value_type_t<T> flag_of_value_type()
  requires string_parser<T>
{
  return flag_of_value_type_t<T>();
}

} // namespace flags

using Flag = std::variant<ValueFlag::ptr, BooleanFlag::ptr>;

} // namespace command
