# vim: set ft=javascript
{
# NOTE: 'module_name' and 'module_path' come from the 'binary' property in package.json
# node-pre-gyp handles passing them down to node-gyp when you build from source
  "targets": [
  {
    "target_name": "<(module_name)",
      "include_dirs" : [
        "include",
        "<!(node -e \"require('nan')\")"
      ],
      "sources": [ "src/binding.cpp" ],
      'conditions': [
      ['OS=="mac"', {
        'xcode_settings': {
          'GCC_ENABLE_CPP_RTTI': 'YES',
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
        }
      }],
      ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
        'cflags_cc!': ['-fno-rtti', '-fno-exceptions'],
        'cflags_cc+': ['-frtti', '-D_REENTRANT'],
        "libraries": [ "<(module_root_dir)/lib/libgfcppcache.so" ]
      }]
    ]
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
