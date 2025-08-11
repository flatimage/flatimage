///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : elf
///

#pragma once

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <elf.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <expected>

#include "../macro.hpp"
#include "../common.hpp"

namespace ns_elf
{

namespace fs = std::filesystem;

#if defined(__LP64__)
#define ElfW(type) Elf64_ ## type
#else
#define ElfW(type) Elf32_ ## type
#endif

// copy_binary() {{{
// Copies the binary data between [offset.first, offset.second] from path_file_input to path_file_output
[[maybe_unused]] [[nodiscard]] inline Expected<void> copy_binary(fs::path const& path_file_input
  , fs::path const& path_file_output
  , std::pair<uint64_t,uint64_t> offset)
{
  // Open source and output files
  std::ifstream f_in{path_file_input, std::ios::binary};
  qreturn_if(not f_in.is_open() , Unexpected("Failed to open in file {}\n"_fmt(path_file_input)));
  std::ofstream f_out{path_file_output, std::ios::binary};
  qreturn_if(not f_out.is_open(), Unexpected("Failed to open out file {}\n"_fmt(path_file_output)));
  // Calculate the size of the data to read
  uint64_t size = offset.second - offset.first;
  // Seek to the start offset in the input file
  f_in.seekg(offset.first, std::ios::beg);
  // Read in chunks
  const size_t size_buf = 4096;
  char buffer[size_buf];
  while( size > 0 )
  {
    std::streamsize read_size = static_cast<std::streamsize>(std::min(static_cast<uint64_t>(size_buf), size));
    f_in.read(buffer, read_size);
    f_out.write(buffer, read_size);
    size -= read_size;
  } // while
  return {};
} // function: copy_binary

// }}}

// skip_elf_header() {{{
// Skips the elf header starting from 'offset' and returns the offset to the first byte afterwards
[[nodiscard]] [[maybe_unused]] inline Expected<uint64_t> skip_elf_header(fs::path const& path_file_elf
  , uint64_t offset = 0)
{
  struct File
  {
    FILE* ptr;
    File(FILE* ptr) : ptr(ptr) {};
    ~File() { if(ptr) { ::fclose(ptr); } }
  };
  // Either Elf64_Ehdr or Elf32_Ehdr depending on architecture.
  ElfW(Ehdr) header;
  // Try to open header file
  File file(fopen(path_file_elf.c_str(), "rb"));
  qreturn_if(file.ptr == nullptr, Unexpected("Could not open file '{}': {}"_fmt(path_file_elf, strerror(errno))));
  // Seek the position to read the header
  qreturn_if(fseek(file.ptr, offset, SEEK_SET) < 0
    , Unexpected("Could not seek in file '{}': {}"_fmt(path_file_elf, strerror(errno)))
  )
  // read the header
  qreturn_if(fread(&header, sizeof(header), 1, file.ptr) != 1
    , Unexpected("Could not read elf header")
  );
  // check so its really an elf file
  qreturn_if (std::memcmp(header.e_ident, ELFMAG, SELFMAG) != 0
    , Unexpected("'{}' not an elf file"_fmt(path_file_elf));
  );
  offset = header.e_shoff + (header.e_ehsize * header.e_shnum);
  return offset;
} // }}}

} // namespace ns_elf

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
