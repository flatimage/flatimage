/**
 * @file bind.hpp
 * @author Ruan Formigoni
 * @brief Used to manage the bindings from the host to the sandbox
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once


#include <string>

#include "db.hpp"
#include "../std/expected.hpp"

#include "../std/enum.hpp"

namespace ns_db::ns_bind
{

ENUM(Type, RO, RW, DEV);

/**
 * @struct Bind
 * @brief Represents a single bind mount from host to guest
 * 
 * A Bind contains the configuration for mounting a host filesystem path into
 * the sandbox. It specifies the source path on the host, the destination path
 * inside the sandbox, the access type (read-only, read-write, or device), and
 * an index for ordering within the bind mount collection.
 */
struct Bind
{
  /**
   * @brief Sequential index of this bind mount in the collection
   * 
   * Used for ordering and identification of bind mounts. Automatically
   * maintained by the Binds container to ensure consistency.
   */
  size_t index;
  
  /**
   * @brief Source path on the host system
   * 
   * The filesystem path that will be mounted into the sandbox. Can be
   * a directory, file, device, or any accessible path.
   */
  fs::path path_src;
  
  /**
   * @brief Destination path inside the sandbox
   * 
   * The mount point where the source will be accessible within the
   * sandboxed environment.
   */
  fs::path path_dst;
  
  /**
   * @brief Access type for this bind mount
   * 
   * Determines whether the path is mounted read-only (RO), read-write (RW),
   * or as a device (DEV). Controls the permissions available to the sandboxed
   * application for this mount.
   */
  Type type;
};

/**
 * @struct Binds
 * @brief Container for multiple bind mount configurations
 * 
 * Manages a collection of Bind entries representing all bind mounts configured
 * for a sandboxed application. Provides methods for adding, removing, and
 * querying bind mounts. Supports serialization to and deserialization from JSON.
 * Maintains index consistency automatically when entries are added or removed.
 */
struct Binds
{
  private:
    /**
     * @brief Internal vector storing all bind mount configurations
     * 
     * Holds the actual Bind entries in order. Access is provided through
     * public methods to maintain invariants such as index consistency.
     */
    std::vector<Bind> m_binds;
  public:
    Binds() = default;
    std::vector<Bind> const& get() const;
    void push_back(Bind const& bind);
    void erase(size_t index);
    bool empty() const noexcept;
    friend Value<Binds> deserialize(std::string_view raw_json);
};

/**
 * @brief Retrieves the current list of bind mounts
 *
 * Returns a const reference to the internal vector containing all configured
 * bind mount entries. Does not create a copy; callers receive direct read-only
 * access to the underlying storage.
 * 
 * @return std::vector<Bind> const& Const reference to the vector of bind mounts
 */
[[nodiscard]] inline std::vector<Bind> const& Binds::get() const
{
  return this->m_binds;
}

/**
 * @brief Appends a new bind mount to the collection
 *
 * Adds the provided bind mount configuration to the internal vector. The bind
 * mount will be appended to the end of the list, maintaining insertion order.
 * No validation is performed; the caller is responsible for ensuring the bind
 * configuration is valid.
 * 
 * @param bind The bind mount configuration to add to the collection
 */
inline void Binds::push_back(Bind const& bind)
{
  m_binds.push_back(bind);
}

/**
 * @brief Removes a bind mount by its index
 *
 * Searches for and removes the bind mount with the specified index from the internal
 * vector. After removal, reindexes all remaining bind mounts sequentially starting
 * from 0 to maintain consistent indexing. Logs whether the element was found and
 * removed or if no element with the given index existed.
 * 
 * @param index The zero-based index of the bind mount to remove
 */
inline void Binds::erase(size_t index)
{
  if (std::erase_if(m_binds, [&](auto&& bind){ return bind.index == index; }) > 0)
  {
    logger("I::Erase element with index '{}'", index);
  }
  else
  {
    logger("I::No element with index '{}' found", index);
  }
  for(long i{}; auto& bind : m_binds) { bind.index = i++; }
}

/**
 * @brief Checks whether the bind mount collection is empty
 *
 * Tests if the internal vector contains any bind mount entries. Returns true
 * if no bindings are currently configured, false otherwise.
 * 
 * @return bool True if no bind mounts are present, false otherwise
 */
inline bool Binds::empty() const noexcept
{
  return m_binds.empty();
}

/**
 * @brief Deserializes JSON string into a Binds object
 *
 * Parses a JSON string containing bind mount configurations and constructs a Binds
 * object. Each JSON entry should have "src", "dst", and "type" fields. The type field
 * must be "ro" (read-only), "rw" (read-write), or "dev" (device). Entries with invalid
 * indices are logged as warnings and skipped. Successfully parsed entries are added
 * to the Binds collection with their respective indices, source paths, destination
 * paths, and access types.
 * 
 * @param raw_json The JSON string containing bind mount configurations
 * @return Value<Binds> The deserialized Binds object on success, or error on parse failure
 */
[[nodiscard]] inline Value<Binds> deserialize(std::string_view raw_json)
{
  Binds binds;

  auto db = Pop(ns_db::from_string(raw_json));

  for(auto [key,value] : db.items())
  {
    auto type = Pop(value("type").template value<std::string>());
    if (auto index_result = Catch(std::stoull(key)))
    {
      binds.m_binds.push_back(Bind
      {
        .index = index_result.value(),
        .path_src = Pop(value("src").template value<std::string>()),
        .path_dst = Pop(value("dst").template value<std::string>()),
        .type = (type == "ro")? Type::RO
          : (type == "rw")? Type::RW
          : (type == "rw")? Type::DEV
          : Type::NONE
      });
    }
    else
    {
      logger("W::Failed to parse bind index '{}'", key);
      continue;
    }
  }
  return binds;
}

/**
 * @brief Deserializes JSON from an input stream into a Binds object
 *
 * Reads the entire contents of the input file stream into a string buffer and
 * delegates to the string_view overload of deserialize() for parsing. This
 * provides a convenient interface for reading bind configurations directly
 * from file streams without manual buffer management.
 * 
 * @param stream_raw_json Input file stream containing JSON bind mount data
 * @return Value<Binds> The deserialized Binds object on success, or error on read/parse failure
 */
[[nodiscard]] inline Value<Binds> deserialize(std::ifstream& stream_raw_json) noexcept
{
  std::stringstream ss;
  ss << stream_raw_json.rdbuf();
  return deserialize(ss.str());
}

/**
 * @brief Serializes a Binds object into a JSON database
 *
 * Converts the Binds collection into a structured Db (JSON) object. Each bind mount
 * is stored as a numbered entry with its index as the key. Each entry contains three
 * fields: "src" (source path), "dst" (destination path), and "type" (access mode as
 * "ro", "rw", or "dev"). The resulting Db can be dumped to a string or written to a file.
 * 
 * @param binds The Binds object containing bind mount configurations to serialize
 * @return Value<Db> The JSON database representation on success, or error on failure
 */
[[nodiscard]] inline Value<Db> serialize(Binds const& binds) noexcept
{
  ns_db::Db db;
  for (auto&& bind : binds.get())
  {
    db(std::to_string(bind.index))("src") = bind.path_src;
    db(std::to_string(bind.index))("dst") = bind.path_dst;
    db(std::to_string(bind.index))("type") = (bind.type == Type::RO)? "ro" : (bind.type == Type::RW)? "rw" : "dev";
  }
  return db;
}

} // namespace ns_db::ns_bind

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
