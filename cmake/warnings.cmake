include (CheckCXXCompilerFlag)
include (CheckCCompilerFlag)

# Try to add -Wflag if compiler supports it (GCC/Clang)
macro (add_warning flag)
    string(REPLACE "-" "_" underscored_flag ${flag})
    string(REPLACE "+" "x" underscored_flag ${underscored_flag})

    check_cxx_compiler_flag("-W${flag}" SUPPORTS_CXXFLAG_${underscored_flag})
    check_c_compiler_flag("-W${flag}" SUPPORTS_CFLAG_${underscored_flag})

    if (SUPPORTS_CXXFLAG_${underscored_flag})
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W${flag}")
    else()
        message (STATUS "Flag -W${flag} is unsupported")
    endif()

    if (SUPPORTS_CFLAG_${underscored_flag})
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W${flag}")
    else()
        message(STATUS "Flag -W${flag} is unsupported")
    endif()
endmacro()

# MSVC warning management macros
macro (msvc_warning_level level)
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W${level}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W${level}")
    endif()
endmacro()

# Disable specific MSVC warning by code (e.g., 4711)
macro (msvc_disable_warning code)
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd${code}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd${code}")
    endif()
endmacro()

# Enable specific MSVC warning by code
macro (msvc_enable_warning code)
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w1${code}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /w1${code}")
    endif()
endmacro()

# Disable MSVC warning for specific target
macro (target_msvc_disable_warning target code)
    if (MSVC)
        target_compile_options(${target} PRIVATE "/wd${code}")
    endif()
endmacro()

# Try to add -Wno flag if compiler supports it
macro (no_warning flag)
    add_warning(no-${flag})
endmacro ()


# The same but only for specified target.
macro (target_add_warning target flag)
    string (REPLACE "-" "_" underscored_flag ${flag})
    string (REPLACE "+" "x" underscored_flag ${underscored_flag})

    check_cxx_compiler_flag("-W${flag}" SUPPORTS_CXXFLAG_${underscored_flag})

    if (SUPPORTS_CXXFLAG_${underscored_flag})
        target_compile_options (${target} PRIVATE "-W${flag}")
    else ()
        message (STATUS "Flag -W${flag} is unsupported")
    endif ()
endmacro ()

macro (target_no_warning target flag)
    target_add_warning(${target} no-${flag})
endmacro ()

add_warning(all)
add_warning(extra)
add_warning(pedantic)
add_warning(error) # treat all warnings as errors
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_warning(strict-overflow=1)
endif()
add_warning(suggest-override)
add_warning(suggest-destructor-override)
add_warning(unused)
add_warning(unreachable-code)
add_warning(delete-incomplete)
add_warning(duplicated-cond)
add_warning(disabled-optimization)
add_warning(init-self)
add_warning(bool-compare)
add_warning(logical-not-parentheses)
add_warning(logical-op)
add_warning(misleading-indentation)
# `no-` prefix is part of warning name(i.e. doesn't mean we are disabling it)
add_warning(no-return-local-addr)
add_warning(odr)
add_warning(pointer-arith)
add_warning(redundant-decls)
add_warning(reorder)
add_warning(shift-negative-value)
add_warning(sizeof-array-argument)
add_warning(switch-bool)
add_warning(tautological-compare)
add_warning(trampolines)
# these warnings are not enabled by default
# no_warning(name-of-warning)
no_warning(deprecated-comma-subscript)
no_warning(comma-subscript)
no_warning(ambiguous-reversed-operator)
no_warning(restrict)
no_warning(free-nonheap-object)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  no_warning(stringop-overflow)
endif()

# MSVC-specific warning configuration
if(MSVC)
  # Set warning level 3 (default level with reasonable warnings)
  msvc_warning_level(3)

  # Disable excessive informational warnings that don't indicate bugs
  msvc_disable_warning(4711)  # Function selected for automatic inline expansion
  msvc_disable_warning(4710)  # Function not inlined
  msvc_disable_warning(4514)  # Unreferenced inline function removed
  msvc_disable_warning(4820)  # Padding added to struct/class

  # Disable implicit special member function warnings (often unavoidable in modern C++)
  msvc_disable_warning(4626)  # Assignment operator implicitly deleted
  msvc_disable_warning(4625)  # Copy constructor implicitly deleted
  msvc_disable_warning(5027)  # Move assignment operator implicitly deleted
  msvc_disable_warning(5026)  # Move constructor implicitly deleted
  msvc_disable_warning(4623)  # Default constructor implicitly deleted

  # Disable other low-value warnings
  msvc_disable_warning(5219)  # Implicit conversion from T to bool
  msvc_disable_warning(5045)  # Spectre mitigation
  msvc_disable_warning(5246)  # Aggregate initialization
  msvc_disable_warning(4686)  # Template specialization
  msvc_disable_warning(5266)  # const qualifier discarded
  msvc_disable_warning(4800)  # Implicit conversion to bool
  msvc_disable_warning(4868)  # Left-to-right evaluation order
  msvc_disable_warning(4371)  # Layout change from previous compiler version
  msvc_disable_warning(4127)  # Conditional expression is constant
  msvc_disable_warning(4355)  # 'this' used in member initializer list
  msvc_disable_warning(4668)  # Preprocessor macro not defined
  msvc_disable_warning(5039)  # Exception specification issue
  msvc_disable_warning(4777)  # Format string mismatch
  msvc_disable_warning(5264)  # Variable declared but not used

  # KEEP these warnings enabled - they indicate potential bugs:
  # C4365: Signed/unsigned mismatch
  # C4267: Conversion with possible loss of data
  # C4244: Data conversion with possible loss
  # C4242: Data conversion with possible loss
  # C4458: Declaration hides class member
  # C4245: Signed/unsigned mismatch in conversion
  # C4389: Signed/unsigned mismatch in equality
  # C4457: Declaration hides function parameter
  # C4146: Unary minus applied to unsigned type
  # C4456: Declaration hides previous local declaration
  # C4996: Deprecated function usage
  # C4100: Unreferenced formal parameter
  # C4101: Unreferenced local variable
  # C4061: Switch statement case not handled

  message(STATUS "MSVC warning configuration applied - suppressed informational warnings, kept bug-indicating warnings")
endif()