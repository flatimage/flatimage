///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : db
///

#pragma once

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <variant>

#include "../common.hpp"
#include "../macro.hpp"
#include "../std/concept.hpp"


namespace ns_db
{

using object_t = nlohmann::basic_json<>::object_t;
class Db;

namespace
{

namespace fs = std::filesystem;


using json_t = nlohmann::json;
using Exception = json_t::exception;

template<typename T>
concept IsString =
     std::convertible_to<std::decay_t<T>, std::string>
  or std::constructible_from<std::string, std::decay_t<T>>;

} // anonymous namespace

using KeyType = json_t::value_t;

// class Db {{{
class Db
{
  private:
    std::variant<json_t, std::reference_wrapper<json_t>> m_json;
  public:
    // Constructors
    Db() noexcept;
    template<typename T> requires std::same_as<std::remove_cvref_t<T>, json_t>
    explicit Db(T&& json) noexcept;
    template<typename T> requires std::same_as<std::remove_cvref_t<T>, json_t>
    explicit Db(std::reference_wrapper<T> const& json) noexcept;
    // Element access
    [[nodiscard]] std::vector<std::string> keys() const noexcept;
    [[nodiscard]] std::vector<std::pair<std::string, Db>> items() const noexcept;
    template<typename V = Db>
    [[nodiscard]] std::expected<V,std::string> value() noexcept;
    template<typename V>
    [[nodiscard]] V value_or_default(IsString auto&& k, V&& v = V{}) const noexcept;
    template<typename F, IsString Ks>
    [[nodiscard]] decltype(auto) apply(F&& f, Ks&& ks);
    [[nodiscard]] std::string dump();
    // Capacity
    [[nodiscard]] bool empty() const noexcept;
    // Lookup
    template<IsString T>
    [[nodiscard]] bool contains(T&& t) const noexcept;
    [[nodiscard]] KeyType type() const noexcept;
    // Modifiers
    template<IsString T>
    [[nodiscard]] bool erase(T&& t);
    void clear();
    json_t& data();
    json_t const& data() const;
    // Operators
    template<typename T>
    T operator=(T&& t);
    [[nodiscard]] Db operator()(std::string const& t);
    // Friends
    friend std::ostream& operator<<(std::ostream& os, Db const& db);
}; // class: Db }}}

// Db::Db() {{{
inline Db::Db() noexcept
{
  m_json = json_t::parse("{}");
} // Db::Db() }}}

// Db::Db() {{{
template<typename T> requires std::same_as<std::remove_cvref_t<T>, json_t>
inline Db::Db(std::reference_wrapper<T> const& json) noexcept
{
  m_json = json;
} // Db::Db() }}}

// Db::Db() {{{
template<typename T> requires std::same_as<std::remove_cvref_t<T>, json_t>
inline Db::Db(T&& json) noexcept
{
  m_json = json;
} // Db::Db() }}}

// data() {{{
inline json_t& Db::data()
{
  if (std::holds_alternative<std::reference_wrapper<json_t>>(m_json))
  {
    return std::get<std::reference_wrapper<json_t>>(m_json).get();
  } // if

  return std::get<json_t>(m_json);
} // data() }}}

// data() const {{{
inline json_t const& Db::data() const
{
  return const_cast<Db*>(this)->data();
} // data() const }}}

// keys() {{{
inline std::vector<std::string> Db::keys() const noexcept
{
  return data().items()
    | std::views::transform([&](auto&& e) { return e.key(); })
    | std::ranges::to<std::vector<std::string>>();
} // keys() }}}

// items() {{{
inline std::vector<std::pair<std::string,Db>> Db::items() const noexcept
{
  return data().items()
    | std::views::transform([](auto&& e){ return std::make_pair(e.key(), Db{e.value()}); })
    | std::ranges::to<std::vector<std::pair<std::string,Db>>>();
} // items() }}}

// value() {{{
template<typename V>
std::expected<V, std::string> Db::value() noexcept
{
  json_t& json = data();
  if constexpr ( std::same_as<V,Db> )
  {
    return Db{json};
  } // if
  else if constexpr ( ns_concept::IsVector<V> )
  {
    qreturn_if(not json.is_array(), std::unexpected("Tried to create array with non-array entry"));
    return std::ranges::subrange(json.begin(), json.end())
      | std::views::transform([](auto&& e){ return typename std::remove_cvref_t<V>::value_type(e); })
      | std::ranges::to<V>();
  } // else if
  else
  {
    return ( json.is_string() )?
        std::expected<V,std::string>(std::string{json})
      : std::unexpected("Json element is not a string");
  } // else
} // value() }}}

// value_or_default() {{{
template<typename V>
V Db::value_or_default(IsString auto&& k, V&& v) const noexcept
{
  // Read current json
  json_t json = data();
  // Check if has value
  ereturn_if(not json.contains(k), "Could not find '{}' in database"_fmt(k), v);
  // Access value
  json = json[k];
  // Return range or string type
  if constexpr ( ns_concept::IsVector<V> )
  {
    ereturn_if(not json.is_array(), "Tried to access non-array as array in DB with key '{}'"_fmt(k), v);
    return std::ranges::subrange(json.begin(), json.end())
      | std::views::transform([](auto&& e){ return typename std::remove_cvref_t<V>::value_type(e); })
      | std::ranges::to<V>();
  } // if
  else
  {
    return V{json};
  } // else
} // value_or_default() }}}

// apply() {{{
// Key exists or is created, and is accessed
template<typename F, IsString Ks>
decltype(auto) Db::apply(F&& f, Ks&& ks)
{
  if (auto access = value(std::forward<Ks>(ks)))
  {
    return ns_exception::to_expected([&]{ return f(*access); });
  } // if
  return std::unexpected("Could not apply function");
} // apply() }}}

// dump() {{{
inline std::string Db::dump()
{
  return data().dump(2);
} // dump() }}}

// empty() {{{
inline bool Db::empty() const noexcept
{
  return data().empty();
} // empty() }}}

// contains() {{{
template<IsString T>
bool Db::contains(T&& t) const noexcept
{
  return data().contains(t);
} // contains() }}}

// contains() {{{
inline KeyType Db::type() const noexcept
{
  return data().type();
} // contains() }}}

// erase() {{{
template<IsString T>
bool Db::erase(T&& t)
{
  json_t& json = data();

  auto key = ns_string::to_string(t);

  if ( json.is_array() )
  {
    // Search in array & erase if there is a match
    auto it_search = std::find(json.begin(), json.end(), key);
    if ( it_search == json.end() ) { return false; }
    json.erase(std::distance(json.begin(), it_search));
    return true;
  }

  // Erase returns the number of elements removed
  return json.erase(key) > 0;
} // erase() }}}

// clear() {{{
inline void Db::clear()
{
  data() = "{}";
} // clear() }}}

// operator= {{{
template<typename T>
T Db::operator=(T&& t)
{
  data() = t;
  return t;
} // operator= }}}

// operator() {{{
// Key exists or is created, and is accessed
inline Db Db::operator()(std::string const& t)
{
  json_t& json = data();

  // Access key
  try
  {
    return Db{std::reference_wrapper<json_t>(json[t])};
  } // try
  catch(std::exception const& e)
  {
    "Failed to parse key '{}': {}"_throw(t, e.what());
  } // catch

  // Unreachable, used to suppress no return warning
  return Db{std::reference_wrapper<json_t>(json[t])};
} // operator() }}}

// operator<< {{{
inline std::ostream& operator<<(std::ostream& os, Db const& db)
{
  os << db.data();
  return os;
} // operator<< }}}

// read_file() {{{
auto read_file(IsString auto&& t) -> std::expected<Db, std::string>
{
  fs::path path_file_db{ns_string::to_string(t)};
  // Parse a file
  auto f_parse_file = [](std::ifstream const& f) -> std::optional<json_t>
  {
    // Read to string
    std::string contents = ns_string::to_string(f.rdbuf());
    // Validate contents and return parsed result
    qreturn_if (json_t::accept(contents),  json_t::parse(contents));
    // Failed to parse
    return std::nullopt;
  };
  // Open target file as read
  std::ifstream file(path_file_db, std::ios::in);
  qreturn_if(not file.is_open(), std::unexpected("Failed to open '{}'"_fmt(path_file_db)));
  // Try to parse
  auto optional_json = f_parse_file(file);
  qreturn_if(not optional_json, std::unexpected("Failed to parse db '{}'"_fmt(path_file_db)));
  // Return parsed database
  return Db(optional_json.value());
} // function: read_file }}}

// write_file() {{{
inline auto write_file(fs::path const& path_file_db, Db& db) -> std::expected<void, std::string>
{
  std::ofstream file(path_file_db, std::ios::trunc);
  qreturn_if(not file.is_open(), std::unexpected("Failed to open '{}' for writing"));
  file << std::setw(2) << db.dump();
  file.close();
  return std::expected<void, std::string>{};
} // function: write_file }}}

// from_string() {{{
template<ns_concept::StringRepresentable S>
std::expected<Db, std::string> from_string(S&& s)
{
  qreturn_if(not json_t::accept(ns_string::to_string(s)), std::unexpected("Could not parse json file"));
  return Db(json_t::parse(ns_string::to_string(s)));
} // function: from_string }}}

} // namespace ns_db

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
