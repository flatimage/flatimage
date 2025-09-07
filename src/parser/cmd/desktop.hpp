/**
 * @file desktop.hpp
 * @author Ruan Formigoni
 * @brief Manages the desktop integration
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

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

/**
 * @brief Integrates the desktop entry '.desktop'
 * 
 * @param desktop Desktop integration class
 * @param path_file_binary Path to the flatimage binary
 * @return Expected<void> 
 */
[[nodiscard]] Expected<void> integrate_desktop_entry(ns_db::ns_desktop::Desktop const& desktop, fs::path const& path_file_binary)
{
  std::error_code ec;
  auto xdg_data_home = Expect(ns_env::xdg_data_home<fs::path>());
  // Create path to entry
  fs::path path_file_desktop = xdg_data_home / "applications/flatimage-{}.desktop"_fmt(desktop.get_name());
  // Create parent directories for entry
  fs::create_directories(path_file_desktop.parent_path(), ec);
  qreturn_if(ec, Unexpected("Could not create directories {}"_fmt(ec.message())));
  // Create desktop entry
  std::ofstream file_desktop(path_file_desktop, std::ios::out | std::ios::trunc);
  qreturn_if(not file_desktop.is_open(), std::unexpected("Could not open desktop file {}"_fmt(path_file_desktop)));
  file_desktop << "[Desktop Entry]" << '\n';
  file_desktop << "Name={}"_fmt(desktop.get_name()) << '\n';
  file_desktop << "Type=Application" << '\n';
  file_desktop << "Comment=FlatImage distribution of \"{}\""_fmt(desktop.get_name()) << '\n';
  file_desktop << "TryExec={}"_fmt(path_file_binary) << '\n';
  file_desktop << R"(Exec="{}" %F)"_fmt(path_file_binary) << '\n';
  file_desktop << "Icon=application-flatimage_{}"_fmt(desktop.get_name()) << '\n';
  file_desktop << "MimeType=application/flatimage_{}"_fmt(desktop.get_name()) << '\n';
  file_desktop << "Categories={};"_fmt(ns_string::from_container(desktop.get_categories(), ';')) << '\n';
  file_desktop.close();
  return{};
}

/**
 * @brief Checks if the mime database should be updated based on `<glob pattern=`
 * 
 * @param path_entry_mimetype The path to the mimetype entry
 * @return bool If the mime should be updated or not
 */
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
}

/**
 * @brief Integrates the flatimage mime package the mime package database
 * 
 * @param desktop The desktop object
 * @param path_file_binary The path to the flatimage binary
 */
[[nodiscard]] inline Expected<void> integrate_mime_database(ns_db::ns_desktop::Desktop const& desktop, fs::path const& path_file_binary)
{
  std::error_code ec;
  auto xdg_data_home = Expect(ns_env::xdg_data_home<fs::path>());
  // Get application mimetype location
  fs::path path_file_xml = xdg_data_home / "mime/packages/flatimage-{}.xml"_fmt(desktop.get_name());
  fs::path path_dir_xml = path_file_xml.parent_path();
  qreturn_if(not fs::exists(path_dir_xml, ec) and not fs::create_directories(path_dir_xml, ec)
    , Unexpected("Could not create upper mimetype directories: {}"_fmt(path_dir_xml))
  );
  // Check if should update mime database
  if(not is_update_mime_database(path_file_xml) )
  {
    return {};
  }
  // Create application mimetype file
  std::ofstream file_xml(path_file_xml);
  qreturn_if(not file_xml.is_open(), Unexpected("Could not open '{}'"_fmt(path_file_xml)));
  file_xml << R"(<?xml version="1.0" encoding="UTF-8"?>)" << '\n';
  file_xml << R"(<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">)" << '\n';
  file_xml << R"(  <mime-type type="application/flatimage_{}">)"_fmt(desktop.get_name()) << '\n';
  file_xml << R"(    <comment>FlatImage Application</comment>)" << '\n';
  file_xml << R"(    <glob weight="100" pattern="{}"/>)"_fmt(path_file_binary.filename()) << '\n';
  file_xml << R"(    <sub-class-of type="application/x-executable"/>)" << '\n';
  file_xml << R"(    <generic-icon name="application-x-executable"/>)" << '\n';
  file_xml << R"(  </mime-type>)" << '\n';
  file_xml << R"(</mime-info>)" << '\n';
  file_xml.close();
  // Create flatimage mimetype file
  fs::path path_file_xml_generic = xdg_data_home / "mime/packages/flatimage.xml";
  std::ofstream file_xml_generic(path_file_xml_generic);
  qreturn_if(not file_xml_generic.is_open(), Unexpected("Could not open '{}'"_fmt(path_file_xml_generic)));
  file_xml_generic << R"(<?xml version="1.0" encoding="UTF-8"?>)" << '\n';
  file_xml_generic << R"(<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">)" << '\n';
  file_xml_generic << R"(  <mime-type type="application/flatimage">)" << '\n';
  file_xml_generic << R"(    <comment>FlatImage Application</comment>)" << '\n';
  file_xml_generic << R"(    <magic>)" << '\n';
  file_xml_generic << R"(      <match value="ELF" type="string" offset="1">)" << '\n';
  file_xml_generic << R"(        <match value="0x46" type="byte" offset="8">)" << '\n';
  file_xml_generic << R"(          <match value="0x49" type="byte" offset="9">)" << '\n';
  file_xml_generic << R"(            <match value="0x01" type="byte" offset="10"/>)" << '\n';
  file_xml_generic << R"(          </match>)" << '\n';
  file_xml_generic << R"(        </match>)" << '\n';
  file_xml_generic << R"(      </match>)" << '\n';
  file_xml_generic << R"(    </magic>)" << '\n';
  file_xml_generic << R"(    <glob weight="50" pattern="*.flatimage"/>)" << '\n';
  file_xml_generic << R"(    <sub-class-of type="application/x-executable"/>)" << '\n';
  file_xml_generic << R"(    <generic-icon name="application-x-executable"/>)" << '\n';
  file_xml_generic << R"(  </mime-type>)" << '\n';
  file_xml_generic << R"(</mime-info>)" << '\n';
  file_xml_generic.close();
  // Update mime database
  auto path_bin_mime = Expect(ns_env::search_path("update-mime-database"));
  Expect(ns_subprocess::log(ns_subprocess::wait(path_bin_mime, xdg_data_home / "mime"), "update-mime-database"));
  return {};
}

/**
 * @brief Integrates an svg icon for the flatimage mimetype
 * 
 * @param desktop The desktop entry object
 * @param path_file_icon The path to the icon file to integrate
 */
[[nodiscard]] inline Expected<void> integrate_icons_svg(ns_db::ns_desktop::Desktop const& desktop, fs::path const& path_file_icon)
{
  std::error_code e;
  // Path to mimetype icon
  auto path_icon_mimetype = Expect(get_path_file_icon_svg(desktop.get_name(), template_dir_mimetype_scalable));
  // Path to app icon
  auto path_icon_app = Expect(get_path_file_icon_svg(desktop.get_name(), template_dir_apps_scalable));
  // Copy mimetype icon
  if (fs::copy_file(path_file_icon, path_icon_mimetype, fs::copy_options::skip_existing, e))
  {
    ns_log::error()("Could not copy file '{}' with exit code '{}'", path_icon_mimetype, e.value());
  }
  // Copy app icon
  if (fs::copy_file(path_file_icon, path_icon_app, fs::copy_options::skip_existing, e))
  {
    ns_log::error()("Could not copy file '{}' with exit code '{}'", path_icon_app, e.value());
  }
  return {};
}

/**
 * @brief Integrates PNG icons for the flatimage mimetype
 * 
 * @param desktop The desktop entry object
 * @param path_file_icon The path to the icon file to integrate
 */
[[nodiscard]] inline Expected<void> integrate_icons_png(ns_db::ns_desktop::Desktop const& desktop, fs::path const& path_file_icon)
{
  std::error_code ec;
  auto f_create_parent = [&ec](fs::path const& path_src) -> Expected<void>
  {
    if(not fs::exists(path_src, ec) and not fs::create_directories(path_src, ec))
    {
      return Unexpected("Could not create parent directories of '{}': '{}'"_fmt(path_src, ec.message()));
    }
    return {};
  };
  for(auto&& size : arr_sizes)
  {
    // Path to mimetype and application icons
    fs::path path_icon_mimetype = Expect(get_path_file_icon_png(desktop.get_name(), template_dir_mimetype, size));
    Expect(f_create_parent(path_icon_mimetype));
    auto path_icon_app = Expect(get_path_file_icon_png(desktop.get_name(), template_dir_apps, size));
    Expect(f_create_parent(path_icon_app));
    // Avoid overwrite
    qcontinue_if (fs::exists(path_icon_mimetype, ec));
    // Copy icon with target widthXheight
    Expect(ns_image::resize(path_file_icon, path_icon_mimetype, size, size, true));
    // Duplicate icon to app directory
    if (not fs::copy_file(path_icon_mimetype, path_icon_app, fs::copy_options::skip_existing, ec))
    {
      ns_log::error()("Could not copy file '{}': '{}'", path_icon_app, ec.message());
    }
  }
  return {};
}

/**
 * @brief Integrates the svg icon for the 'flatimage' mimetype
 * 
 * @return Expected<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Expected<void> integrate_icon_flatimage()
{
  auto f_write_icon = [](fs::path const& path_src) -> Expected<void>
  {
    std::ofstream file_src(path_src, std::ios::out | std::ios::trunc);
    qreturn_if(not file_src.is_open(), Unexpected("Failed to open file '{}'"_fmt(path_src)));
    file_src << ns_icon::FLATIMAGE;
    file_src.close();
    return {};
  };
  // Path to mimetype icon
  auto path_icon_mimetype = Expect(get_path_file_icon_svg(std::nullopt, template_dir_mimetype_scalable));
  Expect(f_write_icon(path_icon_mimetype));
  // Path to app icon
  auto path_icon_app = Expect(get_path_file_icon_svg(std::nullopt, template_dir_apps_scalable));
  Expect(f_write_icon(path_icon_app));
  return {};
}

/**
 * @brief Integrate flatimage icons in the current $HOME directory
 * 
 * @param config FlatImage configuration file
 * @param desktop Desktop object
 * @return Expected<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Expected<void> integrate_icons(ns_config::FlatimageConfig const& config, ns_db::ns_desktop::Desktop const& desktop)
{
  std::error_code ec;
  // Check for existing integration
  fs::path path_file_icon = Expect(get_path_file_icon_png(desktop.get_name(), template_dir_apps, 64)
    .or_else([&](auto&&) { return get_path_file_icon_svg(desktop.get_name(), template_dir_apps_scalable); })
  );
  qreturn_if(fs::exists(path_file_icon), Unexpected("Icons are integrated, found {}"_fmt(path_file_icon)));
  // Read picture from flatimage binary
  ns_reserved::ns_icon::Icon icon = Expect(ns_reserved::ns_icon::read(config.path_file_binary));
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
    Expect(integrate_icons_svg(desktop, path_file_tmp_icon));
  }
  else
  {
    Expect(integrate_icons_png(desktop, path_file_tmp_icon));
  }
  Expect(integrate_icon_flatimage());
  // Remove temporary file
  fs::remove(path_file_tmp_icon, ec);
  return {};
}

} // namespace

/**
 * @brief Integrates flatimage desktop data in current system
 * 
 * @param config Flatimage configuration object
 * @return Expected<void> Nothing or success, or the respective error
 */
[[nodiscard]] inline Expected<void> integrate(ns_config::FlatimageConfig const& config)
{
  // Deserialize json from binary
  auto str_raw_json = Expect(ns_reserved::ns_desktop::read(config.path_file_binary)
    , "Could not read desktop json from binary: {}", __expected_ret.error()
  );
  auto desktop = Expect(ns_db::ns_desktop::deserialize(str_raw_json)
    , "Could not parse json data: {}", __expected_ret.error()
  );
  ns_log::debug()("Json desktop data: {}", str_raw_json);

  // Get HOME directory
  fs::path path_dir_home = Expect(ns_env::get_expected("HOME"));

  // Create desktop entry
  if(desktop.get_integrations().contains(IntegrationItem::ENTRY))
  {
    ns_log::info()("Integrating desktop entry...");
    Expect(integrate_desktop_entry(desktop, config.path_file_binary));
  }
  // Create and update mime
  if(desktop.get_integrations().contains(IntegrationItem::MIMETYPE))
  {
    ns_log::info()("Integrating mime database...");
    Expect(integrate_mime_database(desktop,  config.path_file_binary));
  }
  // Create desktop icons
  if(desktop.get_integrations().contains(IntegrationItem::ICON))
  {
    ns_log::info()("Integrating desktop icons...");
    if(auto ret = integrate_icons(config, desktop))
    {
      ns_log::debug()("Could not integrate icons: '{}'", ret.error());
    }
  }
  // Check if should notify
  if (Expect(ns_reserved::ns_notify::read(config.path_file_binary)))
  {
    // Get bash binary
    fs::path path_file_binary_bash = Expect(ns_env::search_path("bash"));
    // Get possible icon paths
    fs::path path_file_icon = Expect(get_path_file_icon_png(desktop.get_name(), template_dir_apps, 64)
      .or_else([&](auto&&){ return get_path_file_icon_svg(desktop.get_name(), template_dir_apps_scalable); })
    );
    // Path to mimetype icon
    std::ignore = ns_subprocess::Subprocess(path_file_binary_bash)
      .with_piped_outputs()
      .with_args("-c", "notify-send -i \"{}\" \"Started '{}' flatimage\""_fmt(path_file_icon, desktop.get_name()))
      .spawn()
      .wait();
  }
  else
  {
    ns_log::debug("Notify is disabled");
  }
  
  return {};
}

/**
 * @brief Setup desktop integration in FlatImage
 * 
 * @param config FlatImage configuration object
 * @param path_file_json_src Path to the json which contains configuration data
 * @return Expected<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Expected<void> setup(ns_config::FlatimageConfig const& config, fs::path const& path_file_json_src)
{
  std::error_code ec;
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
  auto image_data = ({
    std::streamsize size_file_icon = fs::file_size(path_file_icon, ec);
    qreturn_if(ec, Unexpected("Could not get size of file '{}': {}"_fmt(path_file_icon, ec.message())));
    qreturn_if(static_cast<uint64_t>(size_file_icon) >= ns_reserved::FIM_RESERVED_OFFSET_ICON_END - ns_reserved::FIM_RESERVED_OFFSET_ICON_BEGIN
      , Unexpected("File is too large, '{}' bytes"_fmt(size_file_icon))
    );
    std::unique_ptr<char[]> ptr_data = std::make_unique<char[]>(size_file_icon);
    std::streamsize bytes = Expect(ns_reserved::read(path_file_icon, 0, ptr_data.get(), size_file_icon));
    qreturn_if(bytes != size_file_icon
      , Unexpected("Icon read bytes '{}' do not match target size of '{}'", bytes, size_file_icon)
    );
    std::make_pair(std::move(ptr_data), bytes);
  });
  // Create icon struct
  ns_reserved::ns_icon::Icon icon;
  std::memcpy(icon.m_ext, str_ext.data(), sizeof(icon.m_ext));
  std::memcpy(icon.m_data, image_data.first.get(), image_data.second);
  icon.m_size = image_data.second;
  // Write icon struct to the flatimage binary
  Expect(ns_reserved::ns_icon::write(config.path_file_binary, icon), "Could not write image data: {}", __expected_ret.error());
  // Write json to flatimage binary, excluding the input icon path
  auto str_raw_json = Expect(ns_db::ns_desktop::serialize(desktop), "Failed to serialize desktop integration: {}" , __expected_ret.error());
  auto db = Expect(ns_db::from_string(str_raw_json), "Could not parse serialized json source: {}", __expected_ret.error());
  qreturn_if(not db.erase("icon"), Unexpected("Could not erase icon field"));
  Expect(ns_reserved::ns_desktop::write(config.path_file_binary, db.dump()));
  // Print written json
  std::println("{}", db.dump());
  return {};
}

/**
 * @brief Enables desktop integration for FlatImage 
 * 
 * @param config The FlatImage configuration object
 * @param set_integrations The set with integrations to enable
 * @return Expected<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Expected<void> enable(ns_config::FlatimageConfig const& config, std::set<IntegrationItem> set_integrations)
{
  // Read json
  auto str_json = Expect(ns_reserved::ns_desktop::read(config.path_file_binary));
  // Deserialize json
  auto desktop = Expect(ns_db::ns_desktop::deserialize(str_json));
  // Update integrations value
  desktop.set_integrations(set_integrations);
  // Print to standard output
  for(auto const& str_integration : set_integrations)
  {
    std::println("{}", std::string{str_integration});
  }
  // Serialize json
  auto str_raw_json = Expect(ns_db::ns_desktop::serialize(desktop));
  // Write json
  Expect(ns_reserved::ns_desktop::write(config.path_file_binary, str_raw_json));
  return {};
}

} // namespace ns_desktop

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
