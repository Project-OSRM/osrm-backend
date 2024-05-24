{
  "includes": [
      "common.gypi"
  ],
  "targets": [
    {
      "target_name": "tests",
      "type": "executable",
      "sources": [
        "test/unit.cpp",
        "test/t/binary_visitor_1.cpp",
        "test/t/binary_visitor_2.cpp",
        "test/t/binary_visitor_3.cpp",
        "test/t/binary_visitor_4.cpp",
        "test/t/binary_visitor_5.cpp",
        "test/t/binary_visitor_6.cpp",
        "test/t/issue21.cpp",
        "test/t/mutating_visitor.cpp",
        "test/t/optional.cpp",
        "test/t/recursive_wrapper.cpp",
        "test/t/sizeof.cpp",
        "test/t/unary_visitor.cpp",
        "test/t/variant.cpp"
      ],
      "xcode_settings": {
        "SDKROOT": "macosx",
        "SUPPORTED_PLATFORMS":["macosx"]
      },
      "include_dirs": [
          "./include",
          "test/include"
      ]
    }
  ]
}
