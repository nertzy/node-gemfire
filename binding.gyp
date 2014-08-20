# vim: set ft=javascript
{
  "target_defaults": {
    "include_dirs" : [
      "$(GFCPP)/include",
      "<!(node -e \"require('nan')\")"
    ],
    'cflags_cc!': [
      '-fno-rtti', 
      '-fno-exceptions'
    ],
    'cflags_cc+': [
      '-std=c++0x',
      '-frtti'
    ],
    "defines": [
      "_REENTRANT"
    ],
    "libraries": [ 
      "$(GFCPP)/lib/libgfcppcache.so" 
    ],
    "sources": [
      "src/binding.cpp",
      "src/NodeCacheListener.cpp",
      "src/exceptions.cpp",
      "src/conversions.cpp",
      "src/cache.cpp",
      "src/region.cpp",
    ]
  },
  "targets": [
    {
      "target_name": "<(module_name)",
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
    },
    {
      "target_name": "test",
      "libraries": [
        "/usr/lib64/libgtest.so"
      ],
      "sources": [
        "spec/cpp/test.cpp",
      ]
    }
  ]
}
