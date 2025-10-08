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
#include "../std/filesystem.hpp"

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
[[nodiscard]] inline Expected<void> relocate_impl(char** argv, uint32_t offset)
{
  std::error_code ec;
  // This part of the code is executed to write the runner,
  // rightafter the code is replaced by the runner.
  // This is done because the current executable cannot mount itself.

  // Get path to called executable
  qreturn_if(!fs::exists("/proc/self/exe", ec), Unexpected("Error retrieving executable path for self"));
  auto path_absolute = fs::read_symlink("/proc/self/exe", ec);
  // Helper to create directories
  auto f_dir_create_if_not_exists = [](fs::path const& p) -> Expected<fs::path>
  {
    std::error_code ec;
    qreturn_if (not fs::exists(p, ec) and not fs::create_directories(p, ec)
      , Unexpected("Failed to create directory {}"_fmt(p))
    );
    return p;
  };
  // Create base dir
  fs::path path_dir_base = Expect(f_dir_create_if_not_exists("/tmp/fim"));
  // Make the temporary directory name
  fs::path path_dir_app = Expect(f_dir_create_if_not_exists(path_dir_base / "app" / "{}_{}"_fmt(FIM_COMMIT, FIM_TIMESTAMP)));
  // Create bin dir
  fs::path path_dir_app_bin = Expect(f_dir_create_if_not_exists(path_dir_app / "bin"));
  // Create sbin dir
  fs::path path_dir_app_sbin = Expect(f_dir_create_if_not_exists(path_dir_app / "sbin"));
  // Set variables
  ns_env::set("FIM_DIR_GLOBAL", path_dir_base.c_str(), ns_env::Replace::Y);
  ns_env::set("FIM_DIR_APP", path_dir_app.c_str(), ns_env::Replace::Y);
  ns_env::set("FIM_DIR_APP_BIN", path_dir_app_bin.c_str(), ns_env::Replace::Y);
  ns_env::set("FIM_DIR_APP_SBIN", path_dir_app_sbin.c_str(), ns_env::Replace::Y);
  ns_env::set("FIM_FILE_BINARY", path_absolute.c_str(), ns_env::Replace::Y);
  // Create instance directory
  fs::path path_dir_instance = Expect(f_dir_create_if_not_exists("{}/{}/{}"_fmt(path_dir_app, "instance", std::to_string(getpid()))));
  ns_env::set("FIM_DIR_INSTANCE", path_dir_instance.c_str(), ns_env::Replace::Y);
  // Path to directory with mount points
  fs::path path_dir_mount = Expect(f_dir_create_if_not_exists(path_dir_instance / "mount"));
  ns_env::set("FIM_DIR_MOUNT", path_dir_mount.c_str(), ns_env::Replace::Y);

  // Starting offsets
  uint64_t offset_beg = 0;
  uint64_t offset_end = ns_elf::skip_elf_header(path_absolute.c_str()).value();
  // Write by binary header offset
  auto f_write_from_header = [&](fs::path path_file, uint64_t offset_end)
    -> Expected<std::pair<uint64_t,uint64_t>>
  {
    // Update offsets
    offset_beg = offset_end;
    offset_end = ns_elf::skip_elf_header(path_absolute.c_str(),offset_beg).value() + offset_beg;
    // Write binary only if it doesnt already exist
    if ( ! fs::exists(path_file, ec) )
    {
      Expect(ns_elf::copy_binary(path_absolute.string()
        , path_file
        , {offset_beg
        , offset_end})
      );
    }
    // Set permissions
    fs::permissions(path_file.c_str(), fs::perms::owner_all | fs::perms::group_all, ec);
    // Return new values for offsets
    return std::make_pair(offset_beg, offset_end);
  };

  // Write by binary byte offset
  auto f_write_from_offset = [&](std::ifstream& file_binary, fs::path path_file, uint64_t offset_end)
    -> Expected<std::pair<uint64_t,uint64_t>>
  {
    std::error_code ec;
    uint64_t offset_beg = offset_end;
    ns_log::debug()("Writting binary file '{}'", path_file);
    // Set file position
    file_binary.seekg(offset_beg);
    // Read size bytes (FATAL if fails)
    uint64_t size;
    qreturn_if(not file_binary.read(reinterpret_cast<char*>(&size), sizeof(size))
      , Unexpected("Could not read binary size")
    );
    // Open output file and write 
    if (fs::exists(path_file, ec))
    {
      file_binary.seekg(offset_beg + size + sizeof(size));
    } // if
    else
    {
      // Read binary
      std::vector<char> buffer(size);
      qreturn_if(not file_binary.read(buffer.data(), size), Unexpected("Could not read binary"));
      // Open output binary file
      std::ofstream of{path_file, std::ios::out | std::ios::binary};
      qreturn_if(not of.is_open(), Unexpected("Could not open output file '{}'"_fmt(path_file)));
      // Write binary
      qreturn_if(not of.write(buffer.data(), size), Unexpected("Could not write binary file '{}'"_fmt(path_file)));
      // Set permissions
      fs::permissions(path_file, fs::perms::owner_all | fs::perms::group_all, ec);
      elog_if(ec, "Error on setting permissions of file '{}': {}"_fmt(path_file, ec.message()));
    } // if
    // Return new values for offsets
    return std::make_pair(offset_beg, file_binary.tellg());
  };

  // Write binaries
  auto start = std::chrono::high_resolution_clock::now();
  std::ifstream file_binary{path_absolute, std::ios::in | std::ios::binary};
  qreturn_if(not file_binary.is_open(), Unexpected("Could not open flatimage binary file"));
  constexpr static char const str_raw_json[] =
  {
    #embed FIM_FILE_TOOLS
  };
  std::tie(offset_beg, offset_end) = Expect(f_write_from_header(path_dir_instance / "fim_boot" , 0));
  // TODO: Make this compile-time with C++26 reflection features
  for(auto&& tool : Expect(Expect(ns_db::from_string(str_raw_json)).template value<std::vector<std::string>>()))
  {
    std::tie(offset_beg, offset_end) = Expect(f_write_from_offset(file_binary, path_dir_app_bin / tool, offset_end));
  }
  file_binary.close();
  // Create symlinks
  fs::path path_file_dwarfs_aio = path_dir_app_bin / "dwarfs_aio";
  fs::create_symlink(path_file_dwarfs_aio, path_dir_app_bin / "dwarfs", ec);
  fs::create_symlink(path_file_dwarfs_aio, path_dir_app_bin / "mkdwarfs", ec);
  auto end = std::chrono::high_resolution_clock::now();

  // Create busybox symlinks, allow (symlinks exists) errors
  for(auto const& busybox_applet : arr_busybox_applet)
  {
    fs::create_symlink(path_dir_app_bin / "busybox", path_dir_app_sbin / busybox_applet, ec);
  } // for

  // Filesystem starts here
  ns_env::set("FIM_OFFSET", std::to_string(offset_end).c_str(), ns_env::Replace::Y);
  qreturn_if(offset_end != offset
    , Unexpected("Broken image actual offset({}) != offset({})"_fmt(offset_end, offset))
  );
  ns_log::debug()("FIM_OFFSET: {}", offset_end);

  // Option to show offset and exit (to manually mount the fs with fuse2fs)
  if( getenv("FIM_MAIN_OFFSET") ){ std::println("{}", offset_end); exit(0); }

  // Print copy duration
  if ( getenv("FIM_DEBUG") != nullptr )
  {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    ns_log::debug()("Copy binaries finished in '{}' ms", elapsed.count());
  } // if

  // Launch Runner
  int code = execve("{}/fim_boot"_fmt(path_dir_instance).c_str(), argv, environ);
  return Unexpected("Could not perform 'evecve({})': {}"_fmt(code, strerror(errno)));
}

} // namespace

/**
 * @brief Calls the implementation of relocate
 * 
 * @param argv Argument vector passed to the main program
 * @param offset Offset to the reserved space, past the elf and appended binaries
 * @return Nothing on success, or the respective error
 */
[[nodiscard]] inline Expected<void> relocate(char** argv, int32_t offset)
{
  // Get path to self
  fs::path path_file_self = Expect(ns_filesystem::ns_path::file_self(), "Could not relocate binary: {}", __expected_ret.error());
  // If it is outside /tmp, move the binary
  if ( fs::file_size(path_file_self) != ns_elf::skip_elf_header(path_file_self) )
  {
    Expect(relocate_impl(argv, offset), "Could not relocate binary: {}", __expected_ret.error());
  }
  return {};
}

} // namespace ns_relocate