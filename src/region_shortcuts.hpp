#ifndef __REGION_SHORTCUTS_HPP__
#define __REGION_SHORTCUTS_HPP__

#include <gfcpp/RegionShortcut.hpp>
#include <string>

namespace node_gemfire {

gemfire::RegionShortcut getRegionShortcut(const std::string & regionShortcutName);

extern gemfire::RegionShortcut invalidRegionShortcut;

}  // namespace node_gemfire

#endif
