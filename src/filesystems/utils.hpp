#include <string>
#include <filesystem>
#include <ranges>

#include "../std/expected.hpp"
#include "../macro.hpp"


namespace ns_filesystems::ns_utils
{

namespace
{

namespace fs = std::filesystem;

}

/** @brief Checks if a filesystem path is currently in use by any active process.
 *
 * Scans all processes in /proc/[pid]/mountinfo to determine if the given path
 * is mounted or actively used. Iterates through each process's mount
 * information and returns true if the path is found in any mountinfo entry,
 * indicating the path is busy/in-use.
 *
 * @param path_dir The filesystem path to check for busy status.
 * @return true if the path is found in any process's mount information
 * (indicating busy state), false if no processes have mounted or are using the
 * path.
 */
inline bool is_busy(fs::path const& path_dir)
{
  [[maybe_unused]] auto __expected_fn = [](auto&&){ return std::nullopt; };
  // Check all processes in /proc
  std::vector<std::string> const pids = std::filesystem::directory_iterator("/proc")
    // Get directories only
    | std::views::filter([](auto&& e){ return fs::is_directory(e); })
    // Get string file names
    | std::views::transform([](auto&& e){ return e.path().filename().string(); })
    // Drop empty strings
    | std::views::filter([](auto&& e){ return not e.empty(); })
    // Filter & transform to pid
    | std::views::filter([](auto&& e){ return Catch(std::stoi(e)).has_value(); })
    | std::ranges::to<std::vector<std::string>>();

  for (std::string const& pid : pids)
  {
    // Open mountinfo file
    std::ifstream file_info("/proc/" + pid + "/mountinfo");
    continue_if(not file_info.is_open());
    // Iterate through file lines
    auto lines = std::ranges::istream_view<std::string>(file_info);
    auto it = std::ranges::find_if(lines, [&](auto const& line)
    {
      return line.find(path_dir) != std::string::npos;
    });
    return_if(it != lines.end(), true, "D::Busy '{}' due to '{}'", path_dir, *it);
  }
  return false;
}

} // namespace ns_filesystems::ns_utils