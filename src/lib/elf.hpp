/**
 * @file elf.hpp
 * @author Ruan Formigoni
 * @brief A library for operations on ELF files
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

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
#include "../std/expected.hpp"

namespace ns_elf
{

namespace fs = std::filesystem;

#if defined(__LP64__)
#define ElfW(type) Elf64_ ## type
#else
#define ElfW(type) Elf32_ ## type
#endif

/**
 * @brief Copies the binary data between [offset.first, offset.second] from path_file_input to path_file_output
 * 
 * @param path_file_input The source file where to read the bytes from
 * @param path_file_output The target file where to write the bytes to
 * @param section The section[start,end] The section to read from the input and write to the output
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Value<void> copy_binary(fs::path const& path_file_input
  , fs::path const& path_file_output
  , std::pair<uint64_t,uint64_t> section)
{
  // Open source and output files
  std::ifstream f_in{path_file_input, std::ios::binary};
  qreturn_if(not f_in.is_open() , Error("E::Failed to open in file {}", path_file_input));
  std::ofstream f_out{path_file_output, std::ios::binary};
  qreturn_if(not f_out.is_open(), Error("E::Failed to open out file {}", path_file_output));
  // Calculate the size of the data to read
  uint64_t size = section.second - section.first;
  // Seek to the start offset in the input file
  f_in.seekg(section.first, std::ios::beg);
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
}

/**
 * @brief Skips the elf header starting from 'offset' and returns the offset to the first byte afterwards
 *
 * @param path_file_elf Path to the respective elf file
 * @param offset Offset where the elf section starts
 * @return Value<uint64_t> The offset to the first byte after the ELF header, or the respective error
 */
[[nodiscard]] inline Value<uint64_t> skip_elf_header(fs::path const& path_file_elf
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
  qreturn_if(file.ptr == nullptr, Error("E::Could not open file '{}': {}", path_file_elf, strerror(errno)));
  // Seek the position to read the header
  qreturn_if(fseek(file.ptr, offset, SEEK_SET) < 0
    , Error("E::Could not seek in file '{}': {}", path_file_elf, strerror(errno))
  )
  // read the header
  qreturn_if(fread(&header, sizeof(header), 1, file.ptr) != 1
    , Error("E::Could not read elf header")
  );
  // check so its really an elf file
  qreturn_if (std::memcmp(header.e_ident, ELFMAG, SELFMAG) != 0
    , Error("E::'{}' not an elf file", path_file_elf);
  );
  offset = header.e_shoff + (header.e_ehsize * header.e_shnum);
  return offset;
}

} // namespace ns_elf

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
