#include "region_shortcuts.hpp"
#include <string>

using namespace gemfire;

namespace node_gemfire {

RegionShortcut getRegionShortcut(const std::string & regionShortcutName) {
  if (regionShortcutName == "PROXY") {
    return PROXY;
  } else if (regionShortcutName == "CACHING_PROXY") {
    return CACHING_PROXY;
  } else if (regionShortcutName == "CACHING_PROXY_ENTRY_LRU") {
    return CACHING_PROXY_ENTRY_LRU;
  } else if (regionShortcutName == "LOCAL") {
    return LOCAL;
  } else if (regionShortcutName == "LOCAL_ENTRY_LRU") {
    return LOCAL_ENTRY_LRU;
  }

  return invalidRegionShortcut;
}

RegionShortcut invalidRegionShortcut = (RegionShortcut) -1;

}  // namespace node_gemfire
