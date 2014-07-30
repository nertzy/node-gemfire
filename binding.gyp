# vim: set ft=javascript
{
# NOTE: 'module_name' and 'module_path' come from the 'binary' property in package.json
# node-pre-gyp handles passing them down to node-gyp when you build from source
  "targets": [
    {
      "target_name": "<(module_name)",
      "include_dirs" : [
        "$(GFCPP)/include",
        "<!(node -e \"require('nan')\")"
      ],
      'cflags_cc!': ['-fno-rtti', '-fno-exceptions'],
      'cflags_cc+': ['-frtti', '-D_REENTRANT'],
      "libraries": [ "$(GFCPP)/lib/libgfcppcache.so" ],
      "sources": [ "src/binding.cpp" ]
    },
    {
      "target_name": "action_after_build",
      "type": "none",
      "dependencies": [ "<(module_name)" ],
      "copies": [
        {
          "files": [ "<(PRODUCT_DIR)/<(module_name).node" ],
          "destination": "<(module_path)"
        }
      ]
    }
  ]
}
