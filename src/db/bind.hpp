///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : bind
///

#pragma once


#include <string>

#include "db.hpp"

#include "../std/enum.hpp"

namespace ns_db::ns_bind
{

ENUM(Type, RO, RW, DEV);
struct Bind
{
  size_t index;
  fs::path path_src;
  fs::path path_dst;
  Type type;
}; // struct: Bind

// struct Binds {{{
struct Binds
{
  private:
    std::vector<Bind> m_binds;
  public:
    Binds(std::string_view raw_json)
    {
      auto db = ns_db::from_string(raw_json).value();
      for(auto [key,value] : db.items())
      {
        auto type = value("type").template value<std::string>()
          .transform_error([](auto&&){ return "bind: Failed to read type value"; })
          .value();
        m_binds.push_back(Bind {
          .index = std::stoull(key),
          .path_src = value("src").template value<std::string>()
            .transform_error([](auto&&){ return "bind: Failed to read src value"; })
            .value(),
          .path_dst = value("dst").template value<std::string>()
            .transform_error([](auto&&){ return "bind: Failed to read dst value"; })
            .value(),
          .type = (type == "ro")? Type::RO : (type == "rw")? Type::RW : Type::DEV
        });
      }
    }
    std::vector<Bind> const& get() const { return this->m_binds; }
    void erase(size_t index)
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
}; // Binds }}}

// deserialize() {{{
inline std::expected<Binds,std::string> deserialize(std::ifstream& stream_raw_json) noexcept
{
  std::stringstream ss;
  ss << stream_raw_json.rdbuf();
  return ns_exception::to_expected([&]{ return Binds(ss.str()); });
}
// deserialize() }}}

// serialize() {{{
inline std::expected<Db,std::string> serialize(Binds const& binds) noexcept
{
  return ns_exception::to_expected([&]
  {
    ns_db::Db db;
    for (auto&& bind : binds.get())
    {
      db(std::to_string(bind.index))("src") = bind.path_src;
      db(std::to_string(bind.index))("dst") = bind.path_dst;
      db(std::to_string(bind.index))("type") = (bind.type == Type::RO)? "ro" : (bind.type == Type::RW)? "rw" : "dev";
    } // for
    return db;
  });
} // serialize() }}}

} // namespace ns_db::ns_bind

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
