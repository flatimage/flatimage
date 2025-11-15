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
#include "../macro.hpp"


namespace ns_db
{

using object_t = nlohmann::basic_json<>::object_t;
class Db;

namespace
{

namespace fs = std::filesystem;


using json_t = nlohmann::json;

template<typename T>
concept IsString =
     std::convertible_to<std::decay_t<T>, std::string>
  or std::constructible_from<std::string, std::decay_t<T>>;

} // anonymous namespace

using KeyType = json_t::value_t;

/**
 * @class Db
 * @brief A type-safe wrapper around nlohmann::json for database operations
 *
 * The Db class provides a user-friendly interface to nlohmann::json objects with
 * support for both owned and referenced JSON data. It can either own its JSON data
 * directly or wrap a reference to external JSON, enabling flexible use cases. The
 * class supports element access, serialization, deserialization, and common database
 * operations like clearing, erasing, and type checking. All operations are designed
 * to be exception-safe through the Value<T> error handling pattern.
 */
class Db
{
  private:
    /**
     * @brief Underlying JSON storage as a variant
     *
     * Stores either the JSON object directly (when owned) or a reference_wrapper
     * to external JSON data (when referenced). This dual-mode design allows flexibility
     * in ownership semantics while maintaining uniform access through the data() methods.
     */
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
    [[maybe_unused]] [[nodiscard]] Value<V> value() noexcept;
    template<typename F, IsString Ks>
    [[maybe_unused]] [[nodiscard]] decltype(auto) apply(F&& f, Ks&& ks);
    [[maybe_unused]] [[nodiscard]] Value<std::string> dump();
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
 * @brief Constructs a new Db object with an empty JSON object
 *
 * Initializes the database with an empty nlohmann::json object as the underlying storage.
 * The variant m_json is set to hold a json_t::object() by default.
 */
inline Db::Db() noexcept : m_json(json_t::object())
{
}

/**
 * @brief Constructs a new Db object from a reference to an existing JSON object
 *
 * Creates a database wrapper around an existing nlohmann::json object passed by
 * reference_wrapper. This allows the Db to operate on external JSON data without copying.
 * The variant m_json stores the reference_wrapper, enabling modification of the original
 * JSON object through the Db interface.
 *
 * @tparam T Type that is same as json_t (after removing cv and ref qualifiers)
 * @param json Reference wrapper containing the nlohmann::json object to wrap
 */
template<typename T> requires std::same_as<std::remove_cvref_t<T>, json_t>
inline Db::Db(std::reference_wrapper<T> const& json) noexcept : m_json(json)
{
}

/**
 * @brief Constructs a new Db object from a JSON object (move or copy)
 *
 * Creates a database by taking ownership of or copying a nlohmann::json object.
 * The variant m_json directly stores the json_t value, supporting both move and copy
 * construction depending on the value category of the passed argument.
 *
 * @tparam T Type that is same as json_t (after removing cv and ref qualifiers)
 * @param json The nlohmann::json object to store in the database (forwarded)
 */
template<typename T> requires std::same_as<std::remove_cvref_t<T>, json_t>
inline Db::Db(T&& json) noexcept : m_json(json)
{
}

/**
 * @brief Retrieves a mutable reference to the underlying JSON data
 *
 * Accesses the JSON object stored in the variant m_json. If the variant holds a
 * reference_wrapper, it extracts and returns the wrapped reference. Otherwise, it
 * returns the directly stored json_t object. This allows uniform access regardless
 * of whether the Db owns the JSON data or references external data.
 *
 * @return json_t& Mutable reference to the current JSON entry
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
 * @brief Retrieves a const reference to the underlying JSON data
 *
 * Provides const access to the JSON object by delegating to the non-const data()
 * method through const_cast. This maintains the same access logic for both const
 * and non-const contexts without code duplication.
 *
 * @return json_t const& Const reference to the current JSON entry
 */
inline json_t const& Db::data() const
{
  return const_cast<Db*>(this)->data();
}

/**
 * @brief Retrieves all keys from the current JSON object or array
 *
 * Extracts the keys from a structured JSON element (object or array). For JSON objects,
 * returns the property names. For arrays, returns string representations of indices.
 * Returns an empty vector if the JSON is not structured. Uses C++20 ranges to transform
 * the items view into a vector of strings.
 *
 * @return std::vector<std::string> Vector containing all keys as strings, or empty vector if invalid
 */
inline std::vector<std::string> Db::keys() const noexcept
{
  json_t const& json = data();
  return_if(not json.is_structured(), {}, "E::Invalid non-structured json access");
  return json.items()
    | std::views::transform([&](auto&& e) { return e.key(); })
    | std::ranges::to<std::vector<std::string>>();
}

/**
 * @brief Retrieves key-value pairs as Db objects from the current JSON
 *
 * Iterates over all entries in a structured JSON (object or array) and creates
 * a pair containing the key as a string and the value wrapped in a new Db object.
 * Returns an empty vector if the JSON is not structured. Each value is wrapped
 * in its own Db instance, allowing nested database operations.
 *
 * @return std::vector<std::pair<std::string,Db>> Vector of key-value pairs, or empty vector if invalid
 */
inline std::vector<std::pair<std::string,Db>> Db::items() const noexcept
{
  json_t const& json = data();
  return_if(not json.is_structured(), {}, "E::Invalid non-structured json access");
  return json.items()
    | std::views::transform([](auto&& e){ return std::make_pair(e.key(), Db{e.value()}); })
    | std::ranges::to<std::vector<std::pair<std::string,Db>>>();
}

/**
 * @brief Converts the current JSON entry to a specified type
 *
 * Performs type-safe conversion of the JSON entry to the requested type V. Supports:
 * - Db: Returns a new database wrapper for the current entry
 * - std::vector<std::string>: Converts JSON array to string vector (validates all elements are strings)
 * - String types: Converts JSON string values to string-constructible types
 * Returns an error if the conversion is not possible or the JSON type doesn't match expectations.
 *
 * @tparam V The target type to convert to (default: Db)
 * @return Value<V> The converted object on success, or error on type mismatch/invalid conversion
 */
template<typename V>
Value<V> Db::value() noexcept
{
  json_t& json = data();
  if constexpr ( std::same_as<V,Db> )
  {
    return Db{json};
  }
  else if constexpr ( ns_concept::IsVector<V> and ns_concept::Uniform<typename V::value_type, std::string>)
  {
    return_if(not json.is_array(), Error("D::Tried to create array with non-array entry"));
    return_if(std::any_of(json.begin(), json.end(), [](auto&& e){ return not e.is_string(); })
      , Error("D::Invalid key type for string array")
    );
    return std::ranges::subrange(json.begin(), json.end())
      | std::views::transform([](auto&& e){ return typename std::remove_cvref_t<V>::value_type(e); })
      | std::ranges::to<V>();
  }
  else if constexpr (ns_concept::StringConstructible<V>)
  {
    return ( json.is_string() )?
        Value<V>(std::string{json})
      : Error("D::Json element is not a string");
  }
  else
  {
    static_assert(std::is_same_v<V, V> == false, "Unsupported type V for value()");
    return Error("D::No viable type conversion");
  }
}

/**
 * @brief Serializes the current JSON data to a formatted string
 *
 * Converts the underlying nlohmann::json object to a string representation with
 * 2-space indentation for readability. Uses the Try macro to safely handle any
 * potential serialization errors and wrap them in the Value<std::string> result.
 *
 * @return Value<std::string> The formatted JSON string on success, or error on serialization failure
 */
inline Value<std::string> Db::dump()
{
  return Try(data().dump(2));
}

/**
 * @brief Checks whether the JSON data is empty
 *
 * Delegates to nlohmann::json's empty() method to determine if the underlying
 * JSON structure contains no elements. For objects, this means no key-value pairs;
 * for arrays, no elements; for strings, zero length.
 *
 * @return bool True if the JSON is empty, false otherwise
 */
inline bool Db::empty() const noexcept
{
  return data().empty();
}

/**
 * @brief Checks if the JSON contains a specific key
 *
 * Verifies whether the JSON object or array contains the specified key. Only valid
 * for structured JSON types (objects and arrays); returns false for primitives.
 * For objects, checks if the key exists as a property name. For arrays, checks if
 * the key is a valid index.
 *
 * @tparam T Type that satisfies the IsString concept (convertible to std::string)
 * @param key The key to search for in the JSON structure
 * @return bool True if the key exists in the JSON structure, false otherwise
 */
template<IsString T>
bool Db::contains(T&& key) const noexcept
{
  json_t const& json = data();
  return (json.is_object() || json.is_array()) && json.contains(key);
}

/**
 * @brief Retrieves the type of the current JSON element
 *
 * Returns an enumeration value from nlohmann::json::value_t that indicates the
 * JSON element's type (null, boolean, number_integer, number_unsigned, number_float,
 * object, array, string, binary, or discarded).
 *
 * @return KeyType Enumeration representing the JSON element's type
 */
inline KeyType Db::type() const noexcept
{
  return data().type();
}

/**
 * @brief Removes a key from the JSON structure
 *
 * Attempts to erase the specified key from the JSON data. Behavior depends on the JSON type:
 * - Array: Searches for an element matching the key string and removes it by index
 * - Object: Removes the key-value pair if the key exists
 * - Other types: No operation, returns false
 * Returns true if an element was successfully removed, false otherwise.
 *
 * @tparam T Type that satisfies the IsString concept (convertible to std::string)
 * @param key The identifier of the key to remove
 * @return bool True if the key was found and removed, false otherwise
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
 * @brief Resets the JSON data to an empty object
 *
 * Clears all existing content and reinitializes the underlying JSON data as an
 * empty object ({}). This operation discards all keys, values, and nested structures.
 */
inline void Db::clear()
{
  data() = json_t::object();
}

/**
 * @brief Assigns a new value to the underlying JSON data
 *
 * Replaces the current JSON content with the provided value using perfect forwarding.
 * The value is converted to a JSON-compatible type by nlohmann::json's assignment
 * operator. Returns a reference to this Db object to enable method chaining.
 *
 * @tparam T Type of the value to assign (automatically deduced)
 * @param value The value to assign to the JSON data
 * @return Db& Reference to this database object
 */
template<typename T>
Db& Db::operator=(T&& value)
{
  data() = value;
  return *this;
}

/**
 * @brief Accesses or creates a nested JSON entry by key
 *
 * Provides subscript-style access to JSON object properties. If the current JSON is
 * an object or empty, accesses (or creates) the specified key and returns a new Db
 * wrapping a reference to that nested element. If the JSON is not an object or empty,
 * returns a copy of the current Db (no operation). Enables nested access like db("key1")("key2").
 *
 * @param key The key to access or create in the JSON object
 * @return Db A new Db with a reference to the nested element, or the current object on failure
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
 * @brief Outputs the JSON data to an output stream
 *
 * Serializes the underlying nlohmann::json object and writes it to the provided
 * output stream. This enables standard stream operations like std::cout << db.
 * Uses nlohmann::json's built-in stream insertion operator for formatting.
 *
 * @param os The target output stream
 * @param db The database object to print
 * @return std::ostream& Reference to the output stream for chaining
 */
inline std::ostream& operator<<(std::ostream& os, Db const& db)
{
  os << db.data();
  return os;
}

/**
 * @brief Deserializes a JSON file into a Db object
 *
 * Reads a JSON file from the filesystem, parses its contents, and constructs a Db
 * object wrapping the parsed JSON data. Validates that the file exists and can be
 * opened before attempting to parse. Uses nlohmann::json::parse() for deserialization.
 * Returns an error if the file doesn't exist, cannot be opened, or contains invalid JSON.
 *
 * @param path_file_db Path to the JSON file to read
 * @return Value<Db> The deserialized Db object on success, or error on file/parse failure
 */
[[nodiscard]] inline Value<Db> read_file(fs::path const& path_file_db)
{
  return_if(not Try(fs::exists(path_file_db)), Error("D::Invalid db file '{}'", path_file_db.string()));
  // Parse a file
  auto f_parse_file = [](std::ifstream const& f) -> Value<json_t>
  {
    // Read to string
    std::string contents = ns_string::to_string(f.rdbuf());
    // Validate contents and return parsed result
    return Try(json_t::parse(contents));
  };
  // Open target file as read
  std::ifstream file(path_file_db, std::ios::in);
  return_if(not file.is_open(), Error("D::Failed to open '{}'", path_file_db.string()));
  // Try to parse
  return Db(Pop(f_parse_file(file)));
}

/**
 * @brief Serializes a Db object and writes it to a JSON file
 *
 * Converts the Db object's underlying JSON data to a formatted string with 2-space
 * indentation and writes it to the specified file. Opens the file in truncate mode
 * to overwrite any existing content. Closes the file after writing. Returns an error
 * if the file cannot be opened or the JSON cannot be serialized.
 *
 * @param path_file_db Path to the target JSON file
 * @param db The database object to serialize and write
 * @return Value<void> Success indicator, or error on file/serialization failure
 */
[[nodiscard]] inline Value<void> write_file(fs::path const& path_file_db, Db& db)
{
  std::ofstream file(path_file_db, std::ios::trunc);
  return_if(not file.is_open(), Error("D::Failed to open '{}' for writing", path_file_db.string()));
  file << std::setw(2) << Pop(db.dump());
  file.close();
  return Value<void>{};
}

/**
 * @brief Parses a JSON string and creates a Db object
 *
 * Converts a string representation of JSON data into a Db object. The input must
 * be convertible to std::string (via the StringRepresentable concept). Validates
 * that the string is non-empty before attempting to parse. Uses nlohmann::json::parse()
 * for deserialization. Returns an error if the string is empty or contains invalid JSON.
 *
 * @tparam S Type that can be represented as a string (string_view, const char*, etc.)
 * @param s The JSON string data to parse
 * @return Value<Db> The parsed database object on success, or error on parse failure
 */
template<ns_concept::StringRepresentable S>
Value<Db> from_string(S&& s)
{
  std::string data = ns_string::to_string(s);
  return_if(data.empty(), Error("D::Empty json data"));
  return Try(Db{json_t::parse(data)}, "D::Could not parse json file");
}

} // namespace ns_db

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
