#pragma once

// Symbol visibility for shared library builds.
// Users consuming the SDK do NOT define ZEPHYRLOG_BUILD — classes/methods
// marked ZEPHYRLOG_EXPORT will be imported from the shared library.

#if defined(_WIN32) || defined(__CYGWIN__)
  #ifdef ZEPHYRLOG_BUILD
    #define ZEPHYRLOG_EXPORT __declspec(dllexport)
  #else
    #define ZEPHYRLOG_EXPORT __declspec(dllimport)
  #endif
#elif defined(__GNUC__) || defined(__clang__)
  #define ZEPHYRLOG_EXPORT __attribute__((visibility("default")))
#else
  #define ZEPHYRLOG_EXPORT
#endif
