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

#include "../../std/expected.hpp"
#include "../../db/desktop.hpp"
#include "../../reserved/icon.hpp"
#include "../../reserved/desktop.hpp"
#include "../../lib/subprocess.hpp"
#include "../../lib/image.hpp"
#include "../../lib/env.hpp"
#include "../../std/filesystem.hpp"
#include "../../macro.hpp"
#include "../../config.hpp"
#include "icon.hpp"

namespace ns_desktop
{

using IntegrationItem = ns_db::ns_desktop::IntegrationItem;

namespace
{

// MIME type icon
constexpr std::string_view const template_dir_mimetype = "icons/hicolor/{}x{}/mimetypes";
constexpr std::string_view const template_file_mime = "application-flatimage_{}.png";
// Application icon
constexpr std::string_view const template_dir_apps = "icons/hicolor/{}x{}/apps";
constexpr std::string_view const template_file_app = "flatimage_{}.png";
// MIME type icon (scalable)
constexpr std::string_view const template_dir_mime_scalable = "icons/hicolor/scalable/mimetypes";
constexpr std::string_view const template_file_mime_scalable = "application-flatimage_{}.svg";
// Application type icon (scalable)
constexpr std::string_view const template_dir_apps_scalable = "icons/hicolor/scalable/apps";
constexpr std::string_view const template_file_app_scalable = "flatimage_{}.svg";
// PNG target image sizes
constexpr const std::array<uint32_t, 9> arr_sizes {16,22,24,32,48,64,96,128,256};

namespace fs = std::filesystem;


/**
 * @brief Constructs the path to the png icon file
 * 
 * @param name_app The flatimage application name
 * @param size The size of the icon, e.g., 32x32, 64x64
 * @return Value<std::pair<fs::path,fs::path>> Paths to the mime type and application icons,
 * or the respective error
 */
[[nodiscard]] Value<std::pair<fs::path,fs::path>> get_path_file_icon_png(std::string_view name_app, uint32_t size)
{
  fs::path path_file_mime = Pop(ns_env::xdg_data_home<fs::path>())
    / std::vformat(template_dir_mimetype, std::make_format_args(size, size))
    / std::vformat(template_file_mime, std::make_format_args(name_app));
  fs::path path_file_app = Pop(ns_env::xdg_data_home<fs::path>())
    / std::vformat(template_dir_apps, std::make_format_args(size, size))
    / std::vformat(template_file_app, std::make_format_args(name_app));
  return std::make_pair(path_file_mime,path_file_app);
}

/**
 * @brief Constructs the path to the svg icon file
 * 
 * @param name_app The flatimage application name
 * @return Value<std::pair<fs::path,fs::path>> Paths to the mime type and application icons,
 * or the respective error
 */
[[nodiscard]] Value<std::pair<fs::path,fs::path>> get_path_file_icon_svg(std::string_view name_app)
{
  fs::path path_file_mime = Pop(ns_env::xdg_data_home<fs::path>())
    / template_dir_mime_scalable
    / std::vformat(template_file_mime_scalable, std::make_format_args(name_app));
  fs::path path_file_app = Pop(ns_env::xdg_data_home<fs::path>())
    / template_dir_apps_scalable
    / std::vformat(template_file_app_scalable, std::make_format_args(name_app));
  return std::make_pair(path_file_mime,path_file_app);
}

/**
 * @brief Get the file path to the desktop entry
 * 
 * @return Value<fs::path> The path to the desktop entry file, or the respective error
 */
[[nodiscard]] Value<fs::path> get_path_file_desktop(ns_db::ns_desktop::Desktop const& desktop)
{
  auto xdg_data_home = Pop(ns_env::xdg_data_home<fs::path>());
  return xdg_data_home / std::format("applications/flatimage-{}.desktop", desktop.get_name());
}

/**
 * @brief Get the file path to the application specific mimetype file
 * 
 * @return Value<fs::path> The path to the mimetype file, or the respective error
 */
[[nodiscard]] Value<fs::path> get_path_file_mimetype(ns_db::ns_desktop::Desktop const& desktop)
{
  fs::path xdg_data_home = Pop(ns_env::xdg_data_home<fs::path>());
  return xdg_data_home / std::format("mime/packages/flatimage-{}.xml", desktop.get_name());
}

/**
 * @brief Get the file path to the generic mimetype file
 * 
 * @return Value<fs::path> The path to the generic mimetype file, or the respective error
 */
[[nodiscard]] Value<fs::path> get_path_file_mimetype_generic()
{
  fs::path xdg_data_home = Pop(ns_env::xdg_data_home<fs::path>());
  return xdg_data_home / "mime/packages/flatimage.xml";
}

/**
 * @brief Generates the desktop entry
 * 
 * @param desktop Desktop integration class
 * @param path_file_binary Path to the flatimage binary
 * @param os The output stream in which to write the desktop entry
 * @return Value<void> Nothing on success, or the respective error
 */
[[nodiscard]] Value<void> generate_desktop_entry(ns_db::ns_desktop::Desktop const& desktop
  , fs::path const& path_file_binary
  , std::ostream& os)
{
  // Create desktop entry
  os << "[Desktop Entry]" << '\n';
  os << std::format("Name={}", desktop.get_name()) << '\n';
  os << "Type=Application" << '\n';
  os << std::format("Comment=FlatImage distribution of \"{}\"", desktop.get_name()) << '\n';
  os << std::format(R"(Exec="{}" %F)", path_file_binary.string()) << '\n';
  os << std::format("Icon=flatimage_{}", desktop.get_name()) << '\n';
  os << std::format("MimeType=application/flatimage_{};", desktop.get_name()) << '\n';
  os << std::format("Categories={};", ns_string::from_container(desktop.get_categories(), ';'));
  return{};
}

/**
 * @brief Integrates the desktop entry '.desktop'
 * 
 * @param desktop Desktop integration class
 * @param path_file_binary Path to the flatimage binary
 * @return Value<void> Nothing on success, or the respective error
 */
[[nodiscard]] Value<void> integrate_desktop_entry(ns_db::ns_desktop::Desktop const& desktop, fs::path const& path_file_binary)
{
  // Create path to entry
  fs::path path_file_desktop = Pop(get_path_file_desktop(desktop));
  // Create parent directories for entry
  Try(fs::create_directories(path_file_desktop.parent_path()));
  logger("I::Integrating desktop entry...");
  // Open desktop file
  std::ofstream file_desktop(path_file_desktop, std::ios::out | std::ios::trunc);
  qreturn_if(not file_desktop.is_open() , Error("E::Could not open desktop file {}", path_file_desktop));
  // Generate entry
  Pop(generate_desktop_entry(desktop, path_file_binary, file_desktop));
  return {};
}

/**
 * @brief Checks if the mime database should be updated based on `<glob pattern=`
 * 
 * @param path_entry_mimetype The path to the mimetype entry
 * @return bool If the mime should be updated or not
 */
[[nodiscard]] inline bool is_update_mime_database(fs::path const& path_file_binary, fs::path const& path_entry_mimetype)
{
  // Update if file does not exist
  dreturn_if(not fs::exists(path_entry_mimetype), "Update mime due to missing source file", true);
  // Update if can not open file
  std::ifstream file_mimetype(path_entry_mimetype);
  dreturn_if(not file_mimetype.is_open(), "Update mime due to unaccessible source file for read", true);
  // Update if file name has changed
  std::string pattern = std::format(R"(pattern="{}")", path_file_binary.filename().string());
  for (std::string line; std::getline(file_mimetype, line);)
  {
    dreturn_if(line.contains(pattern), "Mime pattern file name checks...", false);
  } // for
  return true;
}

/**
 * @brief Runs update-mime-database on the current XDG_DATA_HOME diretory
 * 
 * @return Value<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> update_mime_database()
{
  fs::path xdg_data_home = Pop(ns_env::xdg_data_home());
  fs::path path_bin_mime = Pop(ns_env::search_path("update-mime-database"));
  logger("I::Updating mime database...");
  Try(ns_subprocess::Subprocess(path_bin_mime)
    .with_args(xdg_data_home / "mime", "update-mime-database")
    .with_log_stdio()
    .wait());
  return {};
}

/**
 * @brief Generates the flatimage generic mime package
 */
[[nodiscard]] inline Value<void> integrate_mime_database_generic()
{
  std::error_code ec;
  fs::path path_file_xml_generic = Pop(get_path_file_mimetype_generic());
  std::ofstream file_xml_generic(path_file_xml_generic, std::ios::out | std::ios::trunc);
  qreturn_if(not file_xml_generic.is_open(), Error("E::Could not open '{}'", path_file_xml_generic));
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
  file_xml_generic << R"(    <generic-icon name="application-flatimage"/>)" << '\n';
  file_xml_generic << R"(  </mime-type>)" << '\n';
  file_xml_generic << R"(</mime-info>)" << '\n';
  file_xml_generic.close();
  return {};
}

/**
 * @brief Generates the flatimage app specific mime package
 * 
 * @param desktop The desktop object
 * @param path_file_binary The path to the flatimage binary
 * @param os The output stream in which to write the mime package
 */
[[nodiscard]] inline Value<void> generate_mime_database(ns_db::ns_desktop::Desktop const& desktop
  , fs::path const& path_file_binary
  , std::ostream& os
)
{
  os << R"(<?xml version="1.0" encoding="UTF-8"?>)" << '\n';
  os << R"(<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">)" << '\n';
  os << std::format(R"(  <mime-type type="application/flatimage_{}">)", desktop.get_name()) << '\n';
  os << R"(    <comment>FlatImage Application</comment>)" << '\n';
  os << std::format(R"(    <glob weight="100" pattern="{}"/>)", path_file_binary.filename().string()) << '\n';
  os << R"(    <sub-class-of type="application/x-executable"/>)" << '\n';
  os << R"(    <generic-icon name="application-flatimage"/>)" << '\n';
  os << R"(  </mime-type>)" << '\n';
  os << R"(</mime-info>)";
  return {};
}

/**
 * @brief Generates the flatimage app specific mime package
 * 
 * @param desktop The desktop object
 * @param path_file_binary The path to the flatimage binary
 */
[[nodiscard]] inline Value<void> integrate_mime_database(ns_db::ns_desktop::Desktop const& desktop
  , fs::path const& path_file_binary
)
{
  // Get application specific mimetype location
  fs::path path_file_xml = Pop(get_path_file_mimetype(desktop));
  // Create parent directories
  fs::path path_dir_xml = path_file_xml.parent_path();
  Pop(ns_fs::create_directories(path_dir_xml)
    , "E::Could not create upper mimetype directories: {}", path_dir_xml
  );
  // Check if should update mime database
  if(is_update_mime_database(path_file_binary, path_file_xml) )
  {
    logger("I::Integrating mime database...");
  }
  else
  {
    logger("D::Skipping mime database update...");
    return {};
  }
  // Create application mimetype file
  std::ofstream file_xml(path_file_xml, std::ios::out | std::ios::trunc);
  qreturn_if(not file_xml.is_open(), Error("E::Could not open '{}'", path_file_xml));
  Pop(generate_mime_database(desktop,  path_file_binary, file_xml));
  Pop(integrate_mime_database_generic());
  Pop(update_mime_database());
  return {};
}

/**
 * @brief Integrates an svg icon for the flatimage mimetype
 * 
 * @param desktop The desktop entry object
 * @param path_file_icon The path to the icon file to integrate
 */
[[nodiscard]] inline Value<void> integrate_icons_svg(ns_db::ns_desktop::Desktop const& desktop, fs::path const& path_file_icon)
{
  // Path to mimetype icon
  auto [path_icon_mimetype, path_icon_app] = Pop(get_path_file_icon_svg(desktop.get_name()));
  Pop(ns_fs::create_directories(path_icon_mimetype.parent_path()));
  Pop(ns_fs::create_directories(path_icon_app.parent_path()));
  auto f_copy_icon = [](fs::path const& path_icon_src, fs::path const& path_icon_dst)
  {
    logger("D::Copy '{}' to '{}'", path_icon_src, path_icon_dst);
    std::error_code ec;
    if (not fs::exists(path_icon_dst, ec)
    and not fs::copy_file(path_icon_src, path_icon_dst, ec))
    {
      logger("E::Could not copy file '{}' to '{}': '{}'"
        , path_icon_src
        , path_icon_dst
        , (ec)? ec.message() : "Unknown error"
      );
    }
  };
  // Copy mimetype and app icons
  f_copy_icon(path_file_icon, path_icon_mimetype);
  f_copy_icon(path_file_icon, path_icon_app);
  return {};
}

/**
 * @brief Integrates PNG icons for the flatimage mimetype
 * 
 * @param desktop The desktop entry object
 * @param path_file_icon The path to the icon file to integrate
 */
[[nodiscard]] inline Value<void> integrate_icons_png(ns_db::ns_desktop::Desktop const& desktop, fs::path const& path_file_icon)
{
  std::error_code ec;
  auto f_create_parent = [](fs::path const& path_src) -> Value<void>
  {
    Pop(ns_fs::create_directories(path_src.parent_path()));
    return {};
  };
  for(auto&& size : arr_sizes)
  {
    // Path to mimetype and application icons
    auto [path_icon_mimetype,path_icon_app] = Pop(get_path_file_icon_png(desktop.get_name(), size));
    Pop(f_create_parent(path_icon_mimetype));
    Pop(f_create_parent(path_icon_app));
    // Avoid overwrite
    qcontinue_if (fs::exists(path_icon_mimetype, ec));
    // Copy icon with target widthXheight
    Pop(ns_image::resize(path_file_icon, path_icon_mimetype, size, size));
    // Duplicate icon to app directory
    if (not fs::copy_file(path_icon_mimetype, path_icon_app, fs::copy_options::skip_existing, ec))
    {
      logger("E::Could not copy file '{}': '{}'", path_icon_app, ec.message());
    }
  }
  return {};
}

/**
 * @brief Integrates the svg icon for the 'flatimage' mimetype
 * 
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Value<void> integrate_icon_flatimage()
{
  auto f_write_icon = [](fs::path const& path_src) -> Value<void>
  {
    Pop(ns_fs::create_directories(path_src.parent_path()));
    std::ofstream file_src(path_src, std::ios::out | std::ios::trunc);
    qreturn_if(not file_src.is_open(), Error("E::Failed to open file '{}'", path_src));
    file_src << ns_icon::FLATIMAGE;
    file_src.close();
    return {};
  };
  fs::path path_dir_xdg = Pop(ns_env::xdg_data_home<fs::path>());
  // Path to mimetype icon
  fs::path path_icon_mime = path_dir_xdg / fs::path{template_dir_mime_scalable} / "application-flatimage.svg";
  Pop(f_write_icon(path_icon_mime));
  // Path to app icon
  fs::path path_icon_app = path_dir_xdg / fs::path{template_dir_apps_scalable} / "flatimage.svg";
  Pop(f_write_icon(path_icon_app));
  return {};
}

/**
 * @brief Integrate flatimage icons in the current $HOME directory
 * 
 * @param config FlatImage configuration file
 * @param desktop Desktop object
 * @return Value<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> integrate_icons(ns_config::FlatimageConfig const& config, ns_db::ns_desktop::Desktop const& desktop)
{
  std::error_code ec;
  // Try to get a valid icon path to a png or svg file
  fs::path path_file_icon = ({
    fs::path path_file_icon_png = Pop(get_path_file_icon_png(desktop.get_name(), 64)).second;
    fs::path path_file_icon_svg = Pop(get_path_file_icon_svg(desktop.get_name())).second;
    fs::exists(path_file_icon_png, ec)? path_file_icon_png : path_file_icon_svg;
  });
  // Check for existing integration
  if (fs::exists(path_file_icon, ec))
  {
    logger("D::Icons are integrated, found {}", path_file_icon.string());
    return {};
  }
  qreturn_if(ec, Error("E::Could not check if icon exists: {}", ec.message()));
  // Read picture from flatimage binary
  ns_reserved::ns_icon::Icon icon = Pop(ns_reserved::ns_icon::read(config.path_file_binary));
  // Create temporary file to write icon to
  auto path_file_tmp_icon = config.path_dir_app / std::format("icon.{}", icon.m_ext);
  // Write icon to temporary file
  std::ofstream file_icon(path_file_tmp_icon, std::ios::out | std::ios::trunc);
  qreturn_if(not file_icon.is_open(), Error("E::Could not open temporary icon file for desktop integration"));
  file_icon.write(icon.m_data, icon.m_size);
  file_icon.close();
  // Create icons
  if ( path_file_tmp_icon.string().ends_with(".svg") )
  {
    Pop(integrate_icons_svg(desktop, path_file_tmp_icon));
  }
  else
  {
    Pop(integrate_icons_png(desktop, path_file_tmp_icon));
  }
  Pop(integrate_icon_flatimage());
  // Remove temporary file
  fs::remove(path_file_tmp_icon, ec);
  qreturn_if(ec, Error("E::Could not remove temporary icon file: {}", ec.message()));
  return {};
}

} // namespace

/**
 * @brief Integrates flatimage desktop data in current system
 * 
 * @param config Flatimage configuration object
 * @return Value<void> Nothing or success, or the respective error
 */
[[nodiscard]] inline Value<void> integrate(ns_config::FlatimageConfig const& config)
{
  // Deserialize json from binary
  auto str_raw_json = Pop(ns_reserved::ns_desktop::read(config.path_file_binary)
    , "E::Could not read desktop json from binary"
  );
  auto desktop = Pop(ns_db::ns_desktop::deserialize(str_raw_json)
    , "D::Missing or misconfigured desktop integration"
  );
  logger("D::Json desktop data: {}", str_raw_json);

  // Create desktop entry
  if(desktop.get_integrations().contains(IntegrationItem::ENTRY))
  {
    logger("I::Integrating desktop entry...");
    Pop(integrate_desktop_entry(desktop, config.path_file_binary));
  }
  // Create and update mime
  if(desktop.get_integrations().contains(IntegrationItem::MIMETYPE))
  {
    logger("I::Integrating mime database...");
    Pop(integrate_mime_database(desktop,  config.path_file_binary));
  }
  // Create desktop icons
  if(desktop.get_integrations().contains(IntegrationItem::ICON))
  {
    logger("I::Integrating desktop icons...");
    if(auto ret = integrate_icons(config, desktop); not ret)
    {
      logger("D::Could not integrate icons: '{}'", ret.error());
    }
  }
  // Check if should notify
  if (config.is_notify)
  {
    // Get bash binary
    fs::path path_file_binary_bash = Pop(ns_env::search_path("bash"));
    // Get possible icon paths
    fs::path path_file_icon = ({
      fs::path path_file_icon_png = Pop(get_path_file_icon_png(desktop.get_name(), 64)).second;
      fs::path path_file_icon_svg = Pop(get_path_file_icon_svg(desktop.get_name())).second;
      Try(fs::exists(path_file_icon_png))? path_file_icon_png : path_file_icon_svg;
    });
    // Path to mimetype icon
    using enum ns_subprocess::Stream;
    ns_subprocess::Subprocess(path_file_binary_bash)
      .with_args("-c", R"(notify-send "$@")", "--")
      .with_args("-i", path_file_icon, std::format("Started '{}' FlatImage", desktop.get_name()))
      .with_log_stdio()
      .wait()
      .discard("E::Failed to send notification");
  }
  else
  {
    logger("D::Notify is disabled");
  }
  
  return {};
}

/**
 * @brief Setup desktop integration in FlatImage
 * 
 * @param config FlatImage configuration object
 * @param path_file_json_src Path to the json which contains configuration data
 * @return Value<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> setup(ns_config::FlatimageConfig const& config, fs::path const& path_file_json_src)
{
  std::error_code ec;
  // Create desktop struct with input json
  std::ifstream file_json_src{path_file_json_src};
  qreturn_if(not file_json_src.is_open()
      , Error("E::Failed to open file '{}' for desktop integration", path_file_json_src)
  );
  auto desktop = Pop(ns_db::ns_desktop::deserialize(file_json_src), "E::Failed to deserialize json");
  qreturn_if(desktop.get_name().contains('/'), Error("E::Application name cannot contain the '/' character"));
  // Validate icon
  fs::path path_file_icon = Pop(desktop.get_path_file_icon(), "E::Could not retrieve icon path field from json");
  std::string str_ext = (path_file_icon.extension() == ".svg")? "svg"
    : (path_file_icon.extension() == ".png")? "png"
    : (path_file_icon.extension() == ".jpg" or path_file_icon.extension() == ".jpeg")? "jpg"
    : "";
  qreturn_if(str_ext.empty(), Error("E::Icon extension '{}' is not supported", path_file_icon.extension()));
  // Read icon into memory
  auto image_data = ({
    std::streamsize size_file_icon = fs::file_size(path_file_icon, ec);
    qreturn_if(ec, Error("E::Could not get size of file '{}': {}", path_file_icon, ec.message()));
    qreturn_if(static_cast<uint64_t>(size_file_icon) >= ns_reserved::FIM_RESERVED_OFFSET_ICON_END - ns_reserved::FIM_RESERVED_OFFSET_ICON_BEGIN
      , Error("E::File is too large, '{}' bytes", size_file_icon)
    );
    std::unique_ptr<char[]> ptr_data = std::make_unique<char[]>(size_file_icon);
    std::streamsize bytes = Pop(ns_reserved::read(path_file_icon, 0, ptr_data.get(), size_file_icon));
    qreturn_if(bytes != size_file_icon
      , Error("E::Icon read bytes '{}' do not match target size of '{}'", bytes, size_file_icon)
    );
    std::make_pair(std::move(ptr_data), bytes);
  });
  // Create icon struct
  ns_reserved::ns_icon::Icon icon;
  std::memset(icon.m_ext, 0, sizeof(icon.m_ext));
  std::memcpy(icon.m_ext, str_ext.data(), std::min(str_ext.size(), sizeof(icon.m_ext)-1));
  std::memcpy(icon.m_data, image_data.first.get(), image_data.second);
  icon.m_size = image_data.second;
  // Write icon struct to the flatimage binary
  Pop(ns_reserved::ns_icon::write(config.path_file_binary, icon), "E::Could not write image data");
  // Write json to flatimage binary, excluding the input icon path
  auto str_raw_json = Pop(ns_db::ns_desktop::serialize(desktop), "E::Failed to serialize desktop integration" );
  auto db = Pop(ns_db::from_string(str_raw_json), "E::Could not parse serialized json source");
  qreturn_if(not db.erase("icon"), Error("E::Could not erase icon field"));
  Pop(ns_reserved::ns_desktop::write(config.path_file_binary, Pop(db.dump())));
  // Print written json
  std::println("{}", Pop(db.dump()));
  return {};
}

/**
 * @brief Enables desktop integration for FlatImage 
 * 
 * @param config The FlatImage configuration object
 * @param set_integrations The set with integrations to enable
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Value<void> enable(ns_config::FlatimageConfig const& config, std::set<IntegrationItem> set_integrations)
{
  // Read json
  auto str_json = Pop(ns_reserved::ns_desktop::read(config.path_file_binary));
  // Deserialize json
  auto desktop = Pop(ns_db::ns_desktop::deserialize(str_json));
  // Update integrations value
  desktop.set_integrations(set_integrations);
  // Print to standard output
  for(auto const& str_integration : set_integrations)
  {
    std::println("{}", std::string{str_integration});
  }
  // Serialize json
  auto str_raw_json = Pop(ns_db::ns_desktop::serialize(desktop));
  // Write json
  Pop(ns_reserved::ns_desktop::write(config.path_file_binary, str_raw_json));
  return {};
}

/**
 * @brief Cleans desktop integration files
 * 
 * @param config The FlatImage configuration object
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Value<void> clean(ns_config::FlatimageConfig const& config)
{
  // Read json
  auto str_json = Pop(ns_reserved::ns_desktop::read(config.path_file_binary), "E::Failed to read from reserved space");
  // Deserialize json
  auto desktop = Pop(ns_db::ns_desktop::deserialize(str_json), "E::Failed to de-serialize desktop integration");
  // Get integrations
  auto integrations = desktop.get_integrations();
  auto f_try_erase = [](fs::path const& path)
  {
    std::error_code ec;
    fs::remove(path, ec);
    if(ec)
    {
      logger("E::Could not remove '{}': {}", path, ec.message());
    }
    else
    {
      logger("I::Removed file '{}'", path);
    }
  };
  // Remove entry
  if( integrations.contains(ns_db::ns_desktop::IntegrationItem::ENTRY))
  {
    f_try_erase(Pop(get_path_file_desktop(desktop)));
  }
  // Remove mimetype database
  if(integrations.contains(ns_db::ns_desktop::IntegrationItem::MIMETYPE))
  {
    f_try_erase(Pop(get_path_file_mimetype(desktop)));
    Pop(update_mime_database());
  }
  // Remove icons
  if(integrations.contains(ns_db::ns_desktop::IntegrationItem::ICON))
  {
    ns_reserved::ns_icon::Icon icon = Pop(ns_reserved::ns_icon::read(config.path_file_binary));
    if(std::string_view(icon.m_ext) == "png")
    {
      for(auto size : arr_sizes)
      {
        auto [path_icon_mime,path_icon_app] = Pop(get_path_file_icon_png(desktop.get_name(), size));
        f_try_erase(path_icon_mime);
        f_try_erase(path_icon_app);
      }
    }
    else
    {
      auto [path_icon_mime,path_icon_app] = Pop(get_path_file_icon_svg(desktop.get_name()));
      f_try_erase(path_icon_mime);
      f_try_erase(path_icon_app);
    }
  }
  return {};
}

/**
 * @brief Dumps the png or svg icon data to a file
 * 
 * @param config The FlatImage configuration object
 * @param path_file_dst The destination file to write the icon to
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Value<void> dump_icon(ns_config::FlatimageConfig const& config, fs::path path_file_dst)
{
  // Read icon data
  ns_reserved::ns_icon::Icon icon = Pop(ns_reserved::ns_icon::read(config.path_file_binary));
  // Make sure it has valid data
  qreturn_if(std::all_of(icon.m_data, icon.m_data+sizeof(icon.m_data), [](char c){ return c == 0; })
    , Error("E::Empty icon data");
  );
  // Get extension
  std::string_view ext = icon.m_ext;
  // Check if extension is valid
  qreturn_if(ext != "png" and ext != "svg", Error("E::Invalid file extension saved in desktop configuration"));
  // Append extension to output destination file
  if(not path_file_dst.extension().string().ends_with(ext))
  {
    path_file_dst = path_file_dst.parent_path() / (path_file_dst.filename().string() + std::format(".{}", ext));
  }
  // Open output file
  std::fstream file_dst(path_file_dst, std::ios::out | std::ios::trunc);
  qreturn_if(not file_dst.is_open(), Error("E::Could not open output file '{}'", path_file_dst));
  // Write data to output file
  file_dst.write(icon.m_data, icon.m_size);
  // Check bytes written
  qreturn_if(not file_dst
    , Error("E::Could not write all '{}' bytes to output file", icon.m_size)
  );
  return {};
}

/**
 * @brief Dumps the desktop entry if integration is configured
 * 
 * @param config The FlatImage configuration object
 * @return Value<std::string> The desktop entry or the respective error
 */
[[nodiscard]] inline Value<std::string> dump_entry(ns_config::FlatimageConfig const& config)
{
  // Get desktop object
  auto desktop = Pop(ns_db::ns_desktop::deserialize(
    Pop(ns_reserved::ns_desktop::read(config.path_file_binary))
  ));
  // Generate desktop entry
  std::stringstream ss;
  Pop(generate_desktop_entry(desktop, config.path_file_binary, ss));
  // Dump contents
  return ss.str();
}

/**
 * @brief Dumps the application mime type file if integration is configured
 * 
 * @param config The FlatImage configuration object
 * @return Value<std::string> The mime type data or the respective error
 */
[[nodiscard]] inline Value<std::string> dump_mimetype(ns_config::FlatimageConfig const& config)
{
  // Get desktop object
  auto desktop = Pop(ns_db::ns_desktop::deserialize(
    Pop(ns_reserved::ns_desktop::read(config.path_file_binary))
  ));
  // Generate mime database
  std::stringstream ss;
  Pop(generate_mime_database(desktop, config.path_file_binary, ss));
  // Dump contents
  return ss.str();
}

} // namespace ns_desktop

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/