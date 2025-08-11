///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : desktop
///

#pragma once

#include <algorithm>
#include <filesystem>
#include <print>

#include "../../db/desktop.hpp"
#include "../../reserved/notify.hpp"
#include "../../reserved/icon.hpp"
#include "../../reserved/desktop.hpp"
#include "../../lib/subprocess.hpp"
#include "../../lib/image.hpp"
#include "../../lib/env.hpp"
#include "../../macro.hpp"
#include "../../config.hpp"
#include "icon.hpp"

namespace ns_desktop
{

using IntegrationItem = ns_db::ns_desktop::IntegrationItem;

namespace
{

constexpr std::string_view const template_dir_mimetype = "icons/hicolor/{}x{}/mimetypes";
constexpr std::string_view const template_dir_apps = "icons/hicolor/{}x{}/apps";
constexpr std::string_view const template_file_icon = "application-flatimage_{}.png";
constexpr std::string_view const template_dir_mimetype_scalable = "icons/hicolor/scalable/mimetypes";
constexpr std::string_view const template_dir_apps_scalable = "icons/hicolor/scalable/apps";
constexpr std::string_view const template_file_icon_scalable = "application-flatimage_{}.svg";
constexpr const std::array<uint32_t, 9> arr_sizes {16,22,24,32,48,64,96,128,256};

namespace fs = std::filesystem;


/**
 * @brief Constructs the path to the png icon file
 * 
 * @param name_app The flatimage application name
 * @param template_dir The template path to the icon's directory
 * @param size The size of the icon, e.g., 32x32, 64x64
 * @return The constructed icon path 
 */
[[nodiscard]] Expected<fs::path> get_path_file_icon_png(std::optional<std::string_view> name_app
  , std::string_view template_dir
  , uint32_t size) noexcept
{
  return Expect(ns_env::xdg_data_home<fs::path>())
    / std::vformat(template_dir, std::make_format_args(size, size))
    / name_app
      .transform([](auto&& e){ return std::vformat(template_file_icon, std::make_format_args(e)); })
      .value_or("application-flatimage.png");
}

/**
 * @brief Constructs the path to the png icon file
 * 
 * @param name_app The flatimage application name
 * @param template_dir The template path to the icon's directory
 * @return The constructed icon path 
 */
[[nodiscard]] Expected<fs::path> get_path_file_icon_svg(std::optional<std::string_view> name_app
  , std::string_view template_dir) noexcept
{
  return Expect(ns_env::xdg_data_home<fs::path>())
    / template_dir
    / name_app
      .transform([](auto&& e){ return std::vformat(template_file_icon_scalable, std::make_format_args(e)); })
      .value_or("application-flatimage.svg");
}

[[nodiscard]] Expected<std::string> read_json_from_binary(ns_config::FlatimageConfig const& config)
{
  return Expect(ns_reserved::ns_desktop::read(config.path_file_binary).transform([](auto&& e){ return std::string(e); }));
}

// write_json_to_binary() {{{
Expected<void> write_json_to_binary(ns_config::FlatimageConfig const& config
  , std::string_view str_raw_json)
{
  return ns_reserved::ns_desktop::write(config.path_file_binary, str_raw_json);
} // write_json_to_binary() }}}

// read_image_from_binary() {{{
Expected<std::pair<std::unique_ptr<char[]>,uint64_t>> read_image_from_binary(fs::path const& path_file_binary
  , uint64_t offset
  , uint64_t size)
{
  auto ptr_data = std::make_unique<char[]>(size);
  auto expected_bytes = ns_reserved::read(path_file_binary, offset, ptr_data.get(), size);
  qreturn_if(not expected_bytes, Unexpected(expected_bytes.error()));
  return std::make_pair(std::move(ptr_data), *expected_bytes);
} // read_image_from_binary() }}}

// integrate_desktop_entry() {{{
void integrate_desktop_entry(ns_db::ns_desktop::Desktop const& desktop, fs::path const& path_file_binary)
{
  auto xdg_data_home = ns_env::xdg_data_home<fs::path>();
  ereturn_if(not xdg_data_home, "XDG_DATA_HOME is undefined");

  // Create path to entry
  fs::path path_file_desktop = *xdg_data_home / "applications/flatimage-{}.desktop"_fmt(desktop.get_name());

  // Create parent directories for entry
  lec(fs::create_directories, path_file_desktop.parent_path());

  // Create desktop entry
  std::ofstream file_desktop(path_file_desktop, std::ios::out | std::ios::trunc);
  ereturn_if(not file_desktop.is_open(), "Could not open desktop file {}"_fmt(path_file_desktop));
  println(file_desktop, "[Desktop Entry]");
  println(file_desktop, "Name={}"_fmt(desktop.get_name()));
  println(file_desktop, "Type=Application");
  println(file_desktop, "Comment=FlatImage distribution of \"{}\""_fmt(desktop.get_name()));
  println(file_desktop, "TryExec={}"_fmt(path_file_binary));
  println(file_desktop, R"(Exec="{}" %F)"_fmt(path_file_binary));
  println(file_desktop, "Icon=application-flatimage_{}"_fmt(desktop.get_name()));
  println(file_desktop, "MimeType=application/flatimage_{}"_fmt(desktop.get_name()));
  println(file_desktop, "Categories={};"_fmt(ns_string::from_container(desktop.get_categories(), ';')));
  file_desktop.close();
} // integrate_desktop_entry() }}}

// is_update_mime_database() {{{
[[nodiscard]] inline bool is_update_mime_database(fs::path const& path_entry_mimetype)
{
  // Update if file does not exist
  dreturn_if(fs::exists(path_entry_mimetype), "Update mime due to missing source file", true);
  std::ifstream file_mimetype(path_entry_mimetype);
  // Update if can not open file
  dreturn_if(not file_mimetype.is_open(), "Update mime due to unaccessible source file for read", true);
  // Update if file name has changed
  std::string pattern = R"(<glob pattern="{}"/>")"_fmt(path_entry_mimetype.filename());
  for (std::string line; std::getline(file_mimetype, line);)
  {
    dreturn_if(line.contains(pattern), "Update mime due to changed file name", true);
  } // for
  return false;
} // is_update_mime_database() }}}

// integrate_mime_database() {{{
inline void integrate_mime_database(ns_db::ns_desktop::Desktop const& desktop, fs::path const& path_file_binary)
{
  auto xdg_data_home = ns_env::xdg_data_home<fs::path>();
  ereturn_if(not xdg_data_home, "Could not retrieve XDG_DATA_HOME");

  // Get application mimetype location
  fs::path path_entry_application_mimetype = *xdg_data_home / "mime/packages/flatimage-{}.xml"_fmt(desktop.get_name());
  lec(fs::create_directories, path_entry_application_mimetype.parent_path());

  // Check if should update mime database
  qreturn_if( not is_update_mime_database(path_entry_application_mimetype) );

  // Create application mimetype file
  std::ofstream file_application_mimetype(path_entry_application_mimetype);
  println(file_application_mimetype, R"(<?xml version="1.0" encoding="UTF-8"?>)");
  println(file_application_mimetype, R"(<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">)");
  println(file_application_mimetype, R"(  <mime-type type="application/flatimage_{}">)"_fmt(desktop.get_name()));
  println(file_application_mimetype, R"(    <comment>FlatImage Application</comment>)");
  println(file_application_mimetype, R"(    <glob weight="100" pattern="{}"/>)"_fmt(path_file_binary.filename()));
  println(file_application_mimetype, R"(    <sub-class-of type="application/x-executable"/>)");
  println(file_application_mimetype, R"(    <generic-icon name="application-x-executable"/>)");
  println(file_application_mimetype, R"(  </mime-type>)");
  println(file_application_mimetype, R"(</mime-info>)");
  file_application_mimetype.close();

  // Create flatimage mimetype file
  fs::path path_entry_flatimage_mimetype = *xdg_data_home / "mime/packages/flatimage.xml"_fmt(desktop.get_name());
  lec(fs::create_directories, path_entry_flatimage_mimetype.parent_path());
  std::ofstream file_flatimage_mimetype(path_entry_flatimage_mimetype);
  println(file_flatimage_mimetype, R"(<?xml version="1.0" encoding="UTF-8"?>)");
  println(file_flatimage_mimetype, R"(<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">)");
  println(file_flatimage_mimetype, R"(  <mime-type type="application/flatimage">)");
  println(file_flatimage_mimetype, R"(    <comment>FlatImage Application</comment>)");
  println(file_flatimage_mimetype, R"(    <magic>)");
  println(file_flatimage_mimetype, R"(      <match value="ELF" type="string" offset="1">)");
  println(file_flatimage_mimetype, R"(        <match value="0x46" type="byte" offset="8">)");
  println(file_flatimage_mimetype, R"(          <match value="0x49" type="byte" offset="9">)");
  println(file_flatimage_mimetype, R"(            <match value="0x01" type="byte" offset="10"/>)");
  println(file_flatimage_mimetype, R"(          </match>)");
  println(file_flatimage_mimetype, R"(        </match>)");
  println(file_flatimage_mimetype, R"(      </match>)");
  println(file_flatimage_mimetype, R"(    </magic>)");
  println(file_flatimage_mimetype, R"(    <glob weight="50" pattern="*.flatimage"/>)"_fmt(path_file_binary.filename()));
  println(file_flatimage_mimetype, R"(    <sub-class-of type="application/x-executable"/>)");
  println(file_flatimage_mimetype, R"(    <generic-icon name="application-x-executable"/>)");
  println(file_flatimage_mimetype, R"(  </mime-type>)");
  println(file_flatimage_mimetype, R"(</mime-info>)");
  file_flatimage_mimetype.close();

  // Update mime database
  auto opt_mime_database = ns_subprocess::search_path("update-mime-database");
  ereturn_if(not opt_mime_database.has_value(), "Could not find 'update-mime-database'");
  ns_subprocess::log(ns_subprocess::wait(*opt_mime_database, *xdg_data_home / "mime"), "update-mime-database");
} // integrate_mime_database() }}}

// integrate_icons_svg() {{{
void integrate_icons_svg(ns_db::ns_desktop::Desktop const& desktop, fs::path const& path_file_icon)
{
    // Path to mimetype icon
    auto path_icon_mimetype = get_path_file_icon_svg(desktop.get_name(), template_dir_mimetype_scalable);
    ereturn_if(not path_icon_mimetype, "Could not retrieve mimetype icon path");
    // Path to app icon
    auto path_icon_app = get_path_file_icon_svg(desktop.get_name(), template_dir_apps_scalable);
    ereturn_if(not path_icon_app, "Could not retrieve app icon path");
    // Copy mimetype icon
    if (std::error_code e; (fs::copy_file(path_file_icon, *path_icon_mimetype, fs::copy_options::skip_existing, e), e) )
    {
      ns_log::error()("Could not copy file '{}' with exit code '{}'", *path_icon_mimetype, e.value());
    } // if
    // Copy app icon
    if (std::error_code e; (fs::copy_file(path_file_icon, *path_icon_app, fs::copy_options::skip_existing, e), e) )
    {
      ns_log::error()("Could not copy file '{}' with exit code '{}'", *path_icon_app, e.value());
    } // if
} // integrate_icons_svg() }}}

// integrate_icons_png() {{{
void integrate_icons_png(ns_db::ns_desktop::Desktop const& desktop, fs::path const& path_file_icon)
{
  for(auto&& size : arr_sizes)
  {
    // Path to mimetype icon
    auto path_icon_mimetype = get_path_file_icon_png(desktop.get_name(), template_dir_mimetype, size);
    ereturn_if(not path_icon_mimetype, "Could not retrieve mimetype icon path");
    // Path to app icon
    std::error_code ec;
    fs::create_directories(path_icon_mimetype->parent_path(), ec);
    ereturn_if(ec, "Could not create parent directories of '{}'"_fmt(*path_icon_mimetype));
    auto path_icon_app = get_path_file_icon_png(desktop.get_name(), template_dir_apps, size);
    ereturn_if(not path_icon_app, "Could not retrieve app icon path");
    fs::create_directories(path_icon_app->parent_path(), ec);
    ereturn_if(ec, "Could not create parent directories of '{}'"_fmt(*path_icon_app));
    // Avoid overwrite
    if ( not fs::exists(*path_icon_mimetype) )
    {
      auto result = ns_image::resize(path_file_icon, *path_icon_mimetype, size, size, true);
      econtinue_if(not result.has_value(), result.error())
    } // if
    // Duplicate icon to app directory
    if (std::error_code e; (fs::copy_file(*path_icon_mimetype, *path_icon_app, fs::copy_options::skip_existing, e), e) )
    {
      ns_log::error()("Could not copy file '{}' with exit code '{}'", *path_icon_app, e.value());
    } // if
  } // for
} // integrate_icons_png() }}}

// integrate_icon_flatimage() {{{
void integrate_icon_flatimage()
{
    // Path to mimetype icon
    auto path_icon_mimetype = get_path_file_icon_svg(std::nullopt, template_dir_mimetype_scalable);
    // Path to app icon
    auto path_icon_app = get_path_file_icon_svg(std::nullopt, template_dir_apps_scalable);
    // Create mimetype icon
    std::ofstream file_icon_mimetype{*path_icon_mimetype};
    file_icon_mimetype << ns_icon::FLATIMAGE;
    file_icon_mimetype.close();
    // Create app icon
    std::ofstream file_icon_app{*path_icon_app};
    file_icon_app << ns_icon::FLATIMAGE;
    file_icon_app.close();
} // integrate_icon_flatimage() }}}

// integrate_icons() {{{
inline Expected<void> integrate_icons(ns_config::FlatimageConfig const& config, ns_db::ns_desktop::Desktop const& desktop)
{
  std::error_code ec;
  // Check for existing integration
  fs::path path_file_icon = Expect(get_path_file_icon_png(desktop.get_name(), template_dir_apps, 64)
    .or_else([&](auto&&) { return get_path_file_icon_svg(desktop.get_name(), template_dir_apps_scalable); }));
  qreturn_if(fs::exists(path_file_icon), Unexpected("Icons are integrated, found {}"_fmt(path_file_icon)));
  // Read picture from flatimage binary
  ns_reserved::ns_icon::Icon icon;
  auto expected_data_image = read_image_from_binary(config.path_file_binary
    , ns_reserved::FIM_RESERVED_OFFSET_ICON_BEGIN
    , sizeof(icon)
  );
  qreturn_if(not expected_data_image, Unexpected(expected_data_image.error()));
  std::memcpy(&icon, expected_data_image->first.get(), sizeof(icon));
  // Create temporary file to write icon to
  auto path_file_tmp_icon = config.path_dir_app / "icon.{}"_fmt(icon.m_ext);
  // Write icon to temporary file
  std::ofstream file_icon(path_file_tmp_icon);
  qreturn_if(not file_icon.is_open(), Unexpected("Could not open temporary icon file for desktop integration"));
  file_icon.write(icon.m_data, icon.m_size);
  file_icon.close();
  // Create icons
  if ( path_file_tmp_icon.string().ends_with(".svg") )
  {
    integrate_icons_svg(desktop, path_file_tmp_icon);
  }
  else
  {
    integrate_icons_png(desktop, path_file_tmp_icon);
  } // else
  integrate_icon_flatimage();
  // Remove temporary file
  fs::remove(path_file_tmp_icon, ec);
  return {};
} // integrate_icons() }}}

// integrate_bash() {{{
void integrate_bash(fs::path const& path_dir_home)
{
  fs::path path_file_bashrc = path_dir_home / ".bashrc";

  // If a backup was already made, then the integration process was completed
  fs::path path_file_bashrc_backup = path_dir_home / ".bashrc.flatimage.bak";
  dreturn_if(fs::exists(path_file_bashrc_backup), "FlatImage backup exists in {}"_fmt(path_file_bashrc_backup));

  // Location where flatimage stores desktop entries, icons, and mimetypes.
  fs::path path_dir_data = path_dir_home / ".local" / "share";

  // Check if XDG_DATA_HOME contain ~/.local/share
  if (const char * str_xdg_data_home = ns_env::get("XDG_DATA_HOME"); str_xdg_data_home != nullptr)
  {
    dreturn_if(fs::path(str_xdg_data_home) == path_dir_data, "Found '{}' in XDG_DATA_HOME"_fmt(path_dir_data));
  } // if

  // Check if XDG_DATA_DIRS contain ~/.local/share
  if (const char * str_xdg_data_dirs = ns_env::get("XDG_DATA_DIRS"); str_xdg_data_dirs != nullptr)
  {
    auto vec_path_dirs = ns_vector::from_string<std::vector<fs::path>>(str_xdg_data_dirs, ':');
    auto search = std::ranges::find(vec_path_dirs, path_dir_data, [](fs::path const& e)
    {
      return ns_exception::or_else([&]{ return fs::canonical(e); }, fs::path{});
    });
    dreturn_if(search != std::ranges::end(vec_path_dirs), "Found '{}' in XDG_DATA_DIRS"_fmt(path_dir_data));
  } // if

  // Backup if exists
  if (fs::exists(path_file_bashrc))
  {
    fs::copy_file(path_file_bashrc, path_file_bashrc_backup);
    ns_log::info()("Saved a backup of ~/.bashrc in '{}'", path_file_bashrc_backup);
  }

  // Integrate
  std::ofstream of_bashrc{path_file_bashrc, std::ios::app};
  of_bashrc << "export XDG_DATA_DIRS=\"$HOME/.local/share:$XDG_DATA_DIRS\"";
  of_bashrc.close();
  ns_log::info()("Modified XDG_DATA_DIRS in ~/.bashrc");
} // integrate_bash }}}

} // namespace

// integrate() {{{
inline Expected<void> integrate(ns_config::FlatimageConfig const& config)
{
  // Deserialize json from binary
  auto str_raw_json = Expect(read_json_from_binary(config)
    , "Could not read desktop json from binary: {}", __expected_ret.error()
  );
  auto desktop = Expect(ns_db::ns_desktop::deserialize(str_raw_json)
    , "Could not parse json data: {}", __expected_ret.error()
  );
  ns_log::debug()("Json desktop data: {}", str_raw_json);

  // Get HOME directory
  std::string_view cstr_home = Expect(ns_env::get_expected("HOME"));

  // Check if XDG_DATA_DIRS contains ~/.local/bin
  if (auto cstr_shell = ns_env::get_expected("SHELL"))
  {
    std::string str_shell{cstr_shell.value()};
    if ( cstr_shell and str_shell.ends_with("bash") )
    {
      integrate_bash(cstr_home);
    } // if
    else
    {
      ns_log::error()("Unsupported shell '{}' for integration", str_shell);
    } // else
  }
  else
  {
    ns_log::error()("SHELL environment variable is undefined");
  } // else

  // Create desktop entry
  if(desktop.get_integrations().contains(IntegrationItem::ENTRY))
  {
    ns_log::info()("Integrating desktop entry...");
    integrate_desktop_entry(desktop, config.path_file_binary);
  } // if

  // Create and update mime
  if(desktop.get_integrations().contains(IntegrationItem::MIMETYPE))
  {
    ns_log::info()("Integrating mime database...");
    integrate_mime_database(desktop,  config.path_file_binary);
  } // if

  // Create desktop icons
  if(desktop.get_integrations().contains(IntegrationItem::ICON))
  {
    ns_log::info()("Integrating desktop icons...");
    if(auto ret = integrate_icons(config, desktop))
    {
      ns_log::debug()("Could not integrate icons: '{}'", ret.error());
    }
  } // if

  // Check if should notify
  if (Expect(ns_reserved::ns_notify::read(config.path_file_binary)))
  {
    // Get bash binary
    auto path_file_binary_bash = ns_subprocess::search_path("bash");
    qreturn_if(not path_file_binary_bash, Unexpected("Could not find bash in PATH"));
    // Get possible icon paths
    auto path_file_icon = get_path_file_icon_png(desktop.get_name(), template_dir_apps, 64)
      .or_else([&](auto&&){ return get_path_file_icon_svg(desktop.get_name(), template_dir_apps_scalable); });
    qreturn_if(not path_file_icon, Unexpected("Could not find icon for notify-send"));
    // Path to mimetype icon
    std::ignore = ns_subprocess::Subprocess(path_file_binary_bash.value())
      .with_piped_outputs()
      .with_args("-c", "notify-send -i \"{}\" \"Started '{}' flatimage\""_fmt(*path_file_icon, desktop.get_name()))
      .spawn()
      .wait();
  } // if
  else
  {
    ns_log::debug("Notify is disabled");
  }
  
  return {};
} // integrate() }}}

// setup() {{{
inline Expected<void> setup(ns_config::FlatimageConfig const& config, fs::path const& path_file_json_src)
{
  // Create desktop struct with input json
  std::ifstream file_json_src{path_file_json_src};
  qreturn_if(not file_json_src.is_open()
      , Unexpected("Failed to open file '{}' for desktop integration"_fmt(path_file_json_src))
  );
  auto desktop = Expect(ns_db::ns_desktop::deserialize(file_json_src), "Failed to deserialize json: {}", __expected_ret.error());
  // Validate icon
  fs::path path_file_icon = Expect(desktop.get_path_file_icon(), "Could not retrieve icon path field from json: {}", __expected_ret.error());
  std::string str_ext = (path_file_icon.extension() == ".svg")? "svg"
    : (path_file_icon.extension() == ".png")? "png"
    : (path_file_icon.extension() == ".jpg" or path_file_icon.extension() == ".jpeg")? "jpg"
    : "";
  qreturn_if(str_ext.empty(), Unexpected("Icon extension '{}' is not supported"_fmt(path_file_icon.extension())));
  // Read icon into memory
  uintmax_t size_file_icon = fs::file_size(path_file_icon);
  qreturn_if(size_file_icon >= ns_reserved::FIM_RESERVED_OFFSET_ICON_END - ns_reserved::FIM_RESERVED_OFFSET_ICON_END
    , Unexpected("File is too large, '{}' bytes"_fmt(size_file_icon))
  );
  auto expected_image_data = read_image_from_binary(path_file_icon, 0, size_file_icon);
  qreturn_if(not expected_image_data, Unexpected("Could not read source image: {}"_fmt(expected_image_data.error())));
  // Create icon struct
  ns_reserved::ns_icon::Icon icon;
  std::memcpy(icon.m_ext, str_ext.data(), sizeof(icon.m_ext));
  std::memcpy(icon.m_data, expected_image_data->first.get(), expected_image_data->second);
  icon.m_size = expected_image_data->second;
  // Write icon struct to the flatimage binary
  Expect(ns_reserved::ns_icon::write(config.path_file_binary, icon), "Could not write image data: {}", __expected_ret.error());
  // Write json to flatimage binary, excluding the input icon path
  auto str_raw_json = Expect(ns_db::ns_desktop::serialize(desktop), "Failed to serialize desktop integration: {}" , __expected_ret.error());
  auto db = Expect(ns_db::from_string(str_raw_json), "Could not parse serialized json source: {}", __expected_ret.error());
  qreturn_if(not db.erase("icon"), Unexpected("Could not erase icon field"));
  Expect(ns_reserved::ns_desktop::write(config.path_file_binary, db.dump()));
  // Print written json
  println(db.dump());
  return {};
} // setup() }}}

// enable() {{{
inline void enable(ns_config::FlatimageConfig const& config, std::set<IntegrationItem> set_integrations)
{
  // Read json
  auto expected_json = read_json_from_binary(config);
  ereturn_if(not expected_json, "Could not read desktop json: {}"_fmt(expected_json.error()));
  // Deserialize json
  auto desktop = ns_db::ns_desktop::deserialize(expected_json.value());
  ereturn_if(not desktop, "Could not deserialize desktop json data");
  // Update integrations value
  desktop->set_integrations(set_integrations);
  std::ranges::transform(set_integrations
    , std::ostream_iterator<std::string>(std::cout, "\n")
    , [](auto&& item){ return std::string{item}; }
  );
  // Serialize json
  auto expected_str_raw_json = ns_db::ns_desktop::serialize(desktop.value());
  ereturn_if(not expected_str_raw_json, "Could not serialize json to enable desktop integration");
  // Write json
  auto expected = write_json_to_binary(config, *expected_str_raw_json);
  ereturn_if(not expected, "Failed to write json: {}"_fmt(expected.error()));
} // enable() }}}

} // namespace ns_desktop

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
