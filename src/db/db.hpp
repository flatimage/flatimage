/**
 * @file db.hpp
 * @author Ruan Formigoni
 * @brief A database that interfaces with nlohmann json
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <variant>

#include "../std/expected.hpp"
#include "../std/concept.hpp"
#include "../common.hpp"
#include "../macro.hpp"


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
    [[maybe_unused]] [[nodiscard]] std::vector<std::string> keys() const noexcept;
    [[maybe_unused]] [[nodiscard]] std::vector<std::pair<std::string, Db>> items() const noexcept;
    template<typename V = Db>
    [[maybe_unused]] [[nodiscard]] Expected<V> value() noexcept;
    template<typename V>
    [[maybe_unused]] [[nodiscard]] V value_or_default(IsString auto&& k, V&& v = V{}) const noexcept;
    template<typename F, IsString Ks>
    [[maybe_unused]] [[nodiscard]] decltype(auto) apply(F&& f, Ks&& ks);
    [[maybe_unused]] [[nodiscard]] std::string dump();
    // Capacity
    [[maybe_unused]] [[nodiscard]] bool empty() const noexcept;
    // Lookup
    template<IsString T>
    [[maybe_unused]] [[nodiscard]] bool contains(T&& t) const noexcept;
    [[maybe_unused]] [[nodiscard]] KeyType type() const noexcept;
    // Modifiers
    template<IsString T>
    [[maybe_unused]] [[nodiscard]] bool erase(T&& t);
    [[maybe_unused]] void clear();
    json_t& data();
    json_t const& data() const;
    // Operators
    template<typename T>
    Db& operator=(T&& t);
    [[maybe_unused]] [[nodiscard]] Db operator()(std::string const& t);
    // Friends
    friend std::ostream& operator<<(std::ostream& os, Db const& db);
};

/**
 * @brief Construct a new Db:: Db object
 */
inline Db::Db() noexcept : m_json(json_t::parse("{}"))
{
}

/**
 * @brief Construct a new Db:: Db object
 * 
 * @param json Constructs a new Db with the nlohmann_json underlying type
 */
template<typename T> requires std::same_as<std::remove_cvref_t<T>, json_t>
inline Db::Db(std::reference_wrapper<T> const& json) noexcept : m_json(json)
{
}

/**
 * @brief Construct a new Db:: Db object
 * 
 * @param json Constructs a new Db with the nlohmann_json underlying type
 */
template<typename T> requires std::same_as<std::remove_cvref_t<T>, json_t>
inline Db::Db(T&& json) noexcept : m_json(json)
{
}

/**
 * @brief Gets the current json entry
 * 
 * @return json_t& A reference to the current json entry
 */
inline json_t& Db::data()
{
  if (std::holds_alternative<std::reference_wrapper<json_t>>(m_json))
  {
    return std::get<std::reference_wrapper<json_t>>(m_json).get();
  }

  return std::get<json_t>(m_json);
}

/**
 * @brief Gets the current json entry
 * 
 * @return json_t& A reference to the current json entry
 */
inline json_t const& Db::data() const
{
  return const_cast<Db*>(this)->data();
}

/**
 * @brief Get the keys of each json entry
 * 
 * @return std::vector<std::string> Json keys as strings
 */
inline std::vector<std::string> Db::keys() const noexcept
{
  return data().items()
    | std::views::transform([&](auto&& e) { return e.key(); })
    | std::ranges::to<std::vector<std::string>>();
}

/**
 * @brief Gets the keys and values of the current json entry
 * 
 * @return std::vector<std::pair<std::string,Db>> A list of pairs, the key and the value as a database entry
 */
inline std::vector<std::pair<std::string,Db>> Db::items() const noexcept
{
  return data().items()
    | std::views::transform([](auto&& e){ return std::make_pair(e.key(), Db{e.value()}); })
    | std::ranges::to<std::vector<std::pair<std::string,Db>>>();
}

/**
 * @brief Get the value of the current json entry
 * 
 * @tparam V The type to convert the entry to
 * @return Expected<V> The object of V or the respective error
 */
template<typename V>
Expected<V> Db::value() noexcept
{
  json_t& json = data();
  if constexpr ( std::same_as<V,Db> )
  {
    return Db{json};
  }
  else if constexpr ( ns_concept::IsVector<V> )
  {
    qreturn_if(not json.is_array(), std::unexpected("Tried to create array with non-array entry"));
    return std::ranges::subrange(json.begin(), json.end())
      | std::views::transform([](auto&& e){ return typename std::remove_cvref_t<V>::value_type(e); })
      | std::ranges::to<V>();
  }
  else
  {
    return ( json.is_string() )?
        Expected<V>(std::string{json})
      : std::unexpected("Json element is not a string");
  }
}

/**
 * @brief Get the current value of the json entry or a provided one (defaults to default-constructed)
 * 
 * @tparam V The target type
 * @param k The key of the accessed json entry
 * @param v The alternative value to return (optional)
 * @return V The accessed value or the provided alternative
 */
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
  }
  else
  {
    return V{json};
  }
} 

/**
 * @brief Dumps the current json into a string
 * 
 * @return std::string The corresponding string
 */
inline std::string Db::dump()
{
  return data().dump(2);
}

/**
 * @brief Checks if the json is empty
 * 
 * @return The boolean result
 */
inline bool Db::empty() const noexcept
{
  return data().empty();
}

/**
 * @brief Checks if the json contains the provided key
 * 
 * @param key 
 * @return The boolean result
 */
template<IsString T>
bool Db::contains(T&& key) const noexcept
{
  return data().contains(key);
}

/**
 * @brief Get the type of the current json element
 * 
 * @return KeyType An enumeration of the current json element type
 */
inline KeyType Db::type() const noexcept
{
  return data().type();
}

/**
 * @brief Erases the provided key from the database
 * 
 * If the current json entry is:
 *  - An array, it erases the entry from the array
 *  - An object, it erases the key/value if exists
 * 
 * @param key 
 * @return True if the key was removed, false otherwise
 */
template<IsString T>
bool Db::erase(T&& key)
{
  auto str_key = ns_string::to_string(key);

  json_t& json = data();

  if ( json.is_array() )
  {
    auto it_search = std::find(json.begin(), json.end(), str_key);
    if ( it_search == json.end() ) { return false; }
    json.erase(std::distance(json.begin(), it_search));
    return true;
  }
  else if(json.is_object())
  {
    return json.erase(str_key) > 0;
  }

  return false;
}

/**
 * @brief Clears the current json contents
 */
inline void Db::clear()
{
  data() = "{}";
}

/**
 * @brief Assigns a value to the current json entry
 * 
 * @param value The value to assign
 * @return Db& A reference to *this
 */
template<typename T>
Db& Db::operator=(T&& value)
{
  data() = value;
  return *this;
}

/**
 * @brief Accesses or creates the key
 * 
 * @param key The key to access or create 
 * @return Db A new database object with a reference to the accessed key, on failure it returns the current object
 */
inline Db Db::operator()(std::string const& key)
{
  json_t& json = data();

  if(json.is_object() or json.empty())
  {
    return Db{std::reference_wrapper<json_t>(json[key])};
  }

  return *this;
}

/**
 * @brief Prints the database to the target stream
 * 
 * @param os The target stream
 * @param db The database to print to the stream
 * @return std::ostream& A reference to the target stream
 */
inline std::ostream& operator<<(std::ostream& os, Db const& db)
{
  os << db.data();
  return os;
}

/**
 * @brief Creates a database from the given input file
 * 
 * @param path_file_db The json file used to create the database from
 * @return Expected<Db> The deserialized Db object
 */
[[nodiscard]] inline Expected<Db> read_file(fs::path const& path_file_db)
{
  std::error_code ec;
  qreturn_if(not fs::exists(path_file_db, ec), std::unexpected("Invalid db file '{}'"_fmt(path_file_db)));
  // Parse a file
  auto f_parse_file = [&path_file_db](std::ifstream const& f) -> Expected<json_t>
  {
    // Read to string
    std::string contents = ns_string::to_string(f.rdbuf());
    // Validate contents and return parsed result
    qreturn_if (json_t::accept(contents),  json_t::parse(contents));
    // Failed to parse
    return std::unexpected("Failed to parse db '{}'"_fmt(path_file_db));
  };
  // Open target file as read
  std::ifstream file(path_file_db, std::ios::in);
  qreturn_if(not file.is_open(), std::unexpected("Failed to open '{}'"_fmt(path_file_db)));
  // Try to parse
  return Db(Expect(f_parse_file(file)));
}

/**
 * @brief Writes the database object to the target json file
 * 
 * @param path_file_db The target json file
 * @param db The database object
 * @return Expected<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Expected<void> write_file(fs::path const& path_file_db, Db& db)
{
  std::ofstream file(path_file_db, std::ios::trunc);
  qreturn_if(not file.is_open(), std::unexpected("Failed to open '{}' for writing"));
  file << std::setw(2) << db.dump();
  file.close();
  return Expected<void>{};
}

/**
 * @brief Creates a database from a literal json string
 * 
 * @tparam S The type must be representable as a string
 * @param s The string representable json data
 * @return Expected<Db> The database or the respective error
 */
template<ns_concept::StringRepresentable S>
Expected<Db> from_string(S&& s)
{
  std::string str_json = ns_string::to_string(s);
  qreturn_if(str_json.empty(), std::unexpected("Empty json data"));
  qreturn_if(not json_t::accept(str_json), std::unexpected("Could not parse json file"));
  return Db(json_t::parse(str_json));
}

} // namespace ns_db

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
