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
 * @brief A binding from the host to the guest
 */
struct Bind
{
  size_t index;
  fs::path path_src;
  fs::path path_dst;
  Type type;
};

struct Binds
{
  private:
    std::vector<Bind> m_binds;
  public:
    Binds() = default;
    std::vector<Bind> const& get() const;
    void push_back(Bind const& bind);
    void erase(size_t index);
    bool empty() const noexcept;
    friend Expected<Binds> deserialize(std::string_view raw_json);
};

/**
 * @brief Get current bindings
 * 
 * @return The vector of bindings
 */
[[nodiscard]] inline std::vector<Bind> const& Binds::get() const
{
  return this->m_binds;
}

/**
 * @brief Pushes a novel binding into the binding vector
 * 
 * @param index The index of the binding to erase
 */
inline void Binds::push_back(Bind const& bind)
{
  m_binds.push_back(bind);
}

/**
 * @brief Erases a binding at the given index
 * 
 * @param index The index of the binding to erase
 */
inline void Binds::erase(size_t index)
{
  if (std::erase_if(m_binds, [&](auto&& bind){ return bind.index == index; }) > 0)
  {
    ns_log::info()("Erase element with index '{}'", index);
  }
  else
  {
    ns_log::info()("No element with index '{}' found", index);
  }
  for(long i{}; auto& bind : m_binds) { bind.index = i++; }
}

/**
 * @brief Checks if the bindings are empty
 * 
 * @return The boolean result
 */
inline bool Binds::empty() const noexcept
{
  return m_binds.empty();
}

/**
 * @brief Factory method that creates a `Binds` object from a json database
 * 
 * @param raw_json The json string
 * @return The `Binds` class or the respective error
 */
[[nodiscard]] inline Expected<Binds> deserialize(std::string_view raw_json)
{
  Binds binds;

  auto db = Expect(ns_db::from_string(raw_json));

  for(auto [key,value] : db.items())
  {
    auto type = Expect(value("type").template value<std::string>());
    binds.m_binds.push_back(Bind
    {
      .index = std::stoull(key),
      .path_src = Expect(value("src").template value<std::string>()),
      .path_dst = Expect(value("dst").template value<std::string>()),
      .type = (type == "ro")? Type::RO
        : (type == "rw")? Type::RW
        : (type == "rw")? Type::DEV
        : Type::NONE
    });
  }
  return binds;
}

/**
 * @brief Deserialize a json file into the `Binds` class
 * 
 * @param stream_raw_json The stream which contains the json data 
 * @return The `Binds` class or the respective error
 */
[[nodiscard]] inline Expected<Binds> deserialize(std::ifstream& stream_raw_json) noexcept
{
  std::stringstream ss;
  ss << stream_raw_json.rdbuf();
  return deserialize(ss.str());
}

/**
 * @brief Serialize a `Binds` object into a json database
 * 
 * @param binds The bindings which to serialize
 * @return The json database or the respective error
 */
[[nodiscard]] inline Expected<Db> serialize(Binds const& binds) noexcept
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
