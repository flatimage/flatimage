/**
 * @file relocate.hpp
 * @author Ruan Formigoni
 * @brief Used to copy execve the flatimage program
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#include <expected>
#include <string>
#include <system_error>
#include <filesystem>
#include <print>

#include "../macro.hpp"
#include "../lib/env.hpp"
#include "../lib/elf.hpp"
#include "../db/db.hpp"
#include "../std/expected.hpp"
#include "../config.hpp"

namespace ns_relocate
{

namespace
{

namespace fs = std::filesystem;

constexpr std::array<const char*,403> const arr_busybox_applet
{
  "[","[[","acpid","add-shell","addgroup","adduser","adjtimex","arch","arp","arping","ascii","ash","awk","base32","base64",
  "basename","bc","beep","blkdiscard","blkid","blockdev","bootchartd","brctl","bunzip2","bzcat","bzip2","cal","cat","chat",
  "chattr","chgrp","chmod","chown","chpasswd","chpst","chroot","chrt","chvt","cksum","clear","cmp","comm","conspy","cp","cpio",
  "crc32","crond","crontab","cryptpw","cttyhack","cut","date","dc","dd","deallocvt","delgroup","deluser","depmod","devmem",
  "df","dhcprelay","diff","dirname","dmesg","dnsd","dnsdomainname","dos2unix","dpkg","dpkg-deb","du","dumpkmap",
  "dumpleases","echo","ed","egrep","eject","env","envdir","envuidgid","ether-wake","expand","expr","factor","fakeidentd",
  "fallocate","false","fatattr","fbset","fbsplash","fdflush","fdformat","fdisk","fgconsole","fgrep","find","findfs",
  "flock","fold","free","freeramdisk","fsck","fsck.minix","fsfreeze","fstrim","fsync","ftpd","ftpget","ftpput","fuser",
  "getfattr","getopt","getty","grep","groups","gunzip","gzip","halt","hd","hdparm","head","hexdump","hexedit","hostid",
  "hostname","httpd","hush","hwclock","i2cdetect","i2cdump","i2cget","i2cset","i2ctransfer","id","ifconfig","ifdown",
  "ifenslave","ifplugd","ifup","inetd","init","insmod","install","ionice","iostat","ip","ipaddr","ipcalc","ipcrm","ipcs",
  "iplink","ipneigh","iproute","iprule","iptunnel","kbd_mode","kill","killall","killall5","klogd","last","less","link",
  "linux32","linux64","linuxrc","ln","loadfont","loadkmap","logger","login","logname","logread","losetup","lpd","lpq",
  "lpr","ls","lsattr","lsmod","lsof","lspci","lsscsi","lsusb","lzcat","lzma","lzop","makedevs","makemime","man","md5sum",
  "mdev","mesg","microcom","mim","mkdir","mkdosfs","mke2fs","mkfifo","mkfs.ext2","mkfs.minix","mkfs.vfat","mknod",
  "mkpasswd","mkswap","mktemp","modinfo","modprobe","more","mount","mountpoint","mpstat","mt","mv","nameif","nanddump",
  "nandwrite","nbd-client","nc","netstat","nice","nl","nmeter","nohup","nologin","nproc","nsenter","nslookup","ntpd","od",
  "openvt","partprobe","passwd","paste","patch","pgrep","pidof","ping","ping6","pipe_progress","pivot_root","pkill",
  "pmap","popmaildir","poweroff","powertop","printenv","printf","ps","pscan","pstree","pwd","pwdx","raidautorun","rdate",
  "rdev","readahead","readlink","readprofile","realpath","reboot","reformime","remove-shell","renice","reset",
  "resize","resume","rev","rm","rmdir","rmmod","route","rpm","rpm2cpio","rtcwake","run-init","run-parts","runlevel",
  "runsv","runsvdir","rx","script","scriptreplay","sed","seedrng","sendmail","seq","setarch","setconsole","setfattr",
  "setfont","setkeycodes","setlogcons","setpriv","setserial","setsid","setuidgid","sh","sha1sum","sha256sum",
  "sha3sum","sha512sum","showkey","shred","shuf","slattach","sleep","smemcap","softlimit","sort","split","ssl_client",
  "start-stop-daemon","stat","strings","stty","su","sulogin","sum","sv","svc","svlogd","svok","swapoff","swapon",
  "switch_root","sync","sysctl","syslogd","tac","tail","tar","taskset","tc","tcpsvd","tee","telnet","telnetd","test","tftp",
  "tftpd","time","timeout","top","touch","tr","traceroute","traceroute6","tree","true","truncate","ts","tsort","tty",
  "ttysize","tunctl","ubiattach","ubidetach","ubimkvol","ubirename","ubirmvol","ubirsvol","ubiupdatevol","udhcpc",
  "udhcpc6","udhcpd","udpsvd","uevent","umount","uname","unexpand","uniq","unix2dos","unlink","unlzma","unshare","unxz",
  "unzip","uptime","users","usleep","uudecode","uuencode","vconfig","vi","vlock","volname","w","wall","watch","watchdog",
  "wc","wget","which","who","whoami","whois","xargs","xxd","xz","xzcat","yes","zcat","zcip",
};

/**
 * @brief Relocate the binary (by copying it) from the image.
 *
 * The binary is copied from the image to the instance directory,
 * for an execve to be performed. This is done to free the main
 * flatimage file to mount the filesystems.
 *
 * @param argv Argument vector passed to the main program
 * @param offset Offset to the reserved space, past the elf and appended binaries
 * @return Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> relocate_impl(char** argv
  , uint32_t offset
  , fs::path const& path_file_self)
{
  // Save the original path before relocation
  ns_env::set("FIM_BIN_SELF", path_file_self, ns_env::Replace::Y);
  // This part of the code is executed to write the runner,
  // rightafter the code is replaced by the runner.
  // This is done because the current executable cannot mount itself.
  // Configure directory paths
  ns_config::Path const path = Pop(ns_config::Path::create());
  auto const dir = path.dir;
  auto const bin = path.bin;
  // Create directories
  Try(fs::create_directories(dir.global));
  Try(fs::create_directories(dir.app));
  Try(fs::create_directories(dir.app_bin));
  Try(fs::create_directories(dir.app_sbin));
  Try(fs::create_directories(dir.instance));
  // Starting offsets
  uint64_t offset_beg = 0;
  uint64_t offset_end = Pop(ns_elf::skip_elf_header(bin.self.c_str()));
  // Write by binary header offset
  auto f_write_from_header = [&](fs::path path_file, uint64_t offset_end)
    -> Value<std::pair<uint64_t,uint64_t>>
  {
    // Update offsets
    offset_beg = offset_end;
    offset_end = Pop(ns_elf::skip_elf_header(bin.self.c_str(),offset_beg)) + offset_beg;
    // Write binary only if it doesnt already exist
    if ( ! Try(fs::exists(path_file)) )
    {
      Pop(ns_elf::copy_binary(bin.self.string()
        , path_file
        , {offset_beg
        , offset_end})
      );
    }
    // Set permissions (with error_code - doesn't throw)
    Try(fs::permissions(path_file.c_str(), fs::perms::owner_all | fs::perms::group_all));
    // Return new values for offsets
    return std::make_pair(offset_beg, offset_end);
  };

  // Write by binary byte offset
  auto f_write_from_offset = [&](std::ifstream& file_binary, fs::path path_file, uint64_t offset_end)
    -> Value<std::pair<uint64_t,uint64_t>>
  {
    std::error_code ec;
    uint64_t offset_beg = offset_end;
    logger("D::Writting binary file '{}'", path_file);
    // Set file position
    file_binary.seekg(offset_beg);
    // Read size bytes (FATAL if fails)
    uint64_t size;
    return_if(not file_binary.read(reinterpret_cast<char*>(&size), sizeof(size)), Error("E::Could not read binary size"));
    // Open output file and write
    if (fs::exists(path_file, ec))
    {
      file_binary.seekg(offset_beg + size + sizeof(size));
    } // if
    else
    {
      // Read binary
      std::vector<char> buffer(size);
      return_if(not file_binary.read(buffer.data(), size), Error("E::Could not read binary"));
      // Open output binary file
      std::ofstream of{path_file, std::ios::out | std::ios::binary};
      return_if(not of.is_open(), Error("E::Could not open output file '{}'", path_file));
      // Write binary
      return_if(not of.write(buffer.data(), size), Error("E::Could not write binary file '{}'", path_file));
      // Set permissions (with error_code - doesn't throw)
      fs::permissions(path_file, fs::perms::owner_all | fs::perms::group_all, ec);
      log_if(ec, "E::Error on setting permissions of file '{}': {}", path_file.string(), ec.message());
    } // if
    // Return new values for offsets
    return std::make_pair(offset_beg, file_binary.tellg());
  };

  // Write binaries
  auto start = std::chrono::high_resolution_clock::now();
  std::ifstream file_binary{bin.self, std::ios::in | std::ios::binary};
  return_if(not file_binary.is_open(), Error("E::Could not open flatimage binary file"));
  constexpr static char const str_raw_json[] =
  {
    #embed FIM_FILE_TOOLS
  };
  std::tie(offset_beg, offset_end) = Pop(f_write_from_header(dir.instance / "fim_boot" , 0));
  // TODO: Make this compile-time with C++26 reflection features
  for(auto&& tool : Pop(Pop(ns_db::from_string(str_raw_json)).template value<std::vector<std::string>>()))
  {
    std::tie(offset_beg, offset_end) = Pop(f_write_from_offset(file_binary, dir.app_bin / tool, offset_end));
  }
  file_binary.close();
  auto f_symlink = [](auto&& points_to, auto&& saved_at)
  {
    if(fs::exists(saved_at)) { fs::remove(saved_at); }
    fs::create_symlink(points_to, saved_at);
  };
  // Create symlinks (with error_code - doesn't throw)
  fs::path path_file_dwarfs_aio = dir.app_bin / "dwarfs_aio";
  Try(f_symlink(path_file_dwarfs_aio, dir.app_bin / "dwarfs"));
  Try(f_symlink(path_file_dwarfs_aio, dir.app_bin / "mkdwarfs"));
  auto end = std::chrono::high_resolution_clock::now();

  // Create busybox symlinks, allow (symlinks exists) errors
  for(auto const& busybox_applet : arr_busybox_applet)
  {
    Try(f_symlink(dir.app_bin / "busybox", dir.app_sbin / busybox_applet));
  } // for

  // Filesystem starts here
  ns_env::set("FIM_OFFSET", std::to_string(offset_end).c_str(), ns_env::Replace::Y);
  return_if(offset_end != offset, Error("E::Broken image actual offset({}) != offset({})", offset_end, offset));
  logger("D::FIM_OFFSET: {}", offset_end);

  // Option to show offset and exit (to manually mount the fs with fuse2fs)
  if( getenv("FIM_MAIN_OFFSET") ){ std::println("{}", offset_end); exit(0); }

  // Print copy duration
  if ( getenv("FIM_DEBUG") != nullptr )
  {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    logger("D::Copy binaries finished in '{}' ms", elapsed.count());
  } // if

  // Launch Runner
  int code = execve(std::format("{}/fim_boot", dir.instance.string()).c_str(), argv, environ);
  return Error("E::Could not perform 'evecve({})': {}", code, strerror(errno));
}

} // namespace

/**
 * @brief Calls the implementation of relocate
 *
 * @param argv Argument vector passed to the main program
 * @param offset Offset to the reserved space, past the elf and appended binaries
 * @return Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> relocate(char** argv, int32_t offset)
{
  // Get path to self
  fs::path path_file_self = Try(fs::read_symlink("/proc/self/exe"));
  // If it is outside /tmp, move the binary
  if (Try(fs::file_size(path_file_self)) != Pop(ns_elf::skip_elf_header(path_file_self)))
  {
    Pop(relocate_impl(argv, offset, path_file_self), "E::Could not relocate binary");
  }
  return {};
}

} // namespace ns_relocate