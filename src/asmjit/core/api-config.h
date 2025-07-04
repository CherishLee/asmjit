// This file is part of AsmJit project <https://asmjit.com>
//
// See <asmjit/core.h> or LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifndef ASMJIT_CORE_API_CONFIG_H_INCLUDED
#define ASMJIT_CORE_API_CONFIG_H_INCLUDED

// AsmJit Library & ABI Version
// ============================

//! \addtogroup asmjit_core
//! \{

//! Makes a 32-bit integer that represents AsmJit version in `(major << 16) | (minor << 8) | patch` form.
#define ASMJIT_LIBRARY_MAKE_VERSION(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

//! AsmJit library version, see \ref ASMJIT_LIBRARY_MAKE_VERSION for a version format reference.
#define ASMJIT_LIBRARY_VERSION ASMJIT_LIBRARY_MAKE_VERSION(1, 17, 0)

//! \def ASMJIT_ABI_NAMESPACE
//!
//! AsmJit ABI namespace is an inline namespace within \ref asmjit namespace.
//!
//! It's used to make sure that when user links to an incompatible version of AsmJit, it won't link. It has also
//! some additional properties as well. When `ASMJIT_ABI_NAMESPACE` is defined by the user it would override the
//! AsmJit default, which makes it possible to use multiple AsmJit libraries within a single project, totally
//! controlled by users. This is useful especially in cases in which some of such library comes from third party.
#if !defined(ASMJIT_ABI_NAMESPACE)
  #define ASMJIT_ABI_NAMESPACE v1_17
#endif // !ASMJIT_ABI_NAMESPACE

//! \}

// Global Dependencies
// ===================

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h> // We really want std types as globals, not under 'std' namespace.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <initializer_list>
#include <limits>
#include <type_traits>
#include <utility>

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
  #include <pthread.h>
#endif

// Build Options
// =============

// NOTE: Doxygen cannot document macros that are not defined, that's why we have to define them and then undefine
// them immediately, so it won't use the macros with its own preprocessor.
#ifdef _DOXYGEN
namespace asmjit {

//! \addtogroup asmjit_build
//! \{

//! Asmjit is embedded, implies \ref ASMJIT_STATIC.
#define ASMJIT_EMBED

//! Enables static-library build.
#define ASMJIT_STATIC

//! Defined when AsmJit's build configuration is 'Debug'.
//!
//! \note Can be defined explicitly to bypass auto-detection.
#define ASMJIT_BUILD_DEBUG

//! Defined when AsmJit's build configuration is 'Release'.
//!
//! \note Can be defined explicitly to bypass auto-detection.
#define ASMJIT_BUILD_RELEASE

//! Disables deprecated API at compile time (deprecated API won't be available).
#define ASMJIT_NO_DEPRECATED

//! Disables the use of an inline ABI namespace within asmjit namespace (the inline namespace is used as an ABI tag).
#define ASMJIT_NO_ABI_NAMESPACE

//! Disables X86/X64 backends.
#define ASMJIT_NO_X86

//! Disables AArch64 backend.
#define ASMJIT_NO_AARCH64

//! Disables the use of `shm_open` on all targets even when it's supported.
#define ASMJIT_NO_SHM_OPEN

//! Disables JIT memory management and \ref asmjit::JitRuntime.
#define ASMJIT_NO_JIT

//! Disables \ref asmjit::Logger and \ref asmjit::Formatter.
#define ASMJIT_NO_LOGGING

//! Disables everything that contains text.
#define ASMJIT_NO_TEXT

//! Disables instruction validation API.
#define ASMJIT_NO_VALIDATION

//! Disables instruction introspection API.
#define ASMJIT_NO_INTROSPECTION

//! Disables non-host backends entirely (useful for JIT compilers to minimize the library size).
#define ASMJIT_NO_FOREIGN

//! Disables \ref asmjit_builder functionality completely.
#define ASMJIT_NO_BUILDER

//! Disables \ref asmjit_compiler functionality completely.
#define ASMJIT_NO_COMPILER

// Avoid doxygen preprocessor using feature-selection definitions.
#undef ASMJIT_BUILD_EMBED
#undef ASMJIT_BUILD_STATIC
#undef ASMJIT_BUILD_DEBUG
#undef ASMJIT_BUILD_RELEASE

// (keep ASMJIT_NO_DEPRECATED defined, we don't document deprecated APIs).
#undef ASMJIT_NO_ABI_NAMESPACE

#undef ASMJIT_NO_X86
#undef ASMJIT_NO_AARCH64
#undef ASMJIT_NO_FOREIGN

#undef ASMJIT_NO_JIT
#undef ASMJIT_NO_LOGGING
#undef ASMJIT_NO_TEXT
#undef ASMJIT_NO_VALIDATION
#undef ASMJIT_NO_INTROSPECTION
#undef ASMJIT_NO_BUILDER
#undef ASMJIT_NO_COMPILER
#undef ASMJIT_NO_UJIT

//! \}

} // {asmjit}
#endif // _DOXYGEN

// ASMJIT_NO_BUILDER implies ASMJIT_NO_COMPILER.
#if defined(ASMJIT_NO_BUILDER) && !defined(ASMJIT_NO_COMPILER)
  #define ASMJIT_NO_COMPILER
#endif

// ASMJIT_NO_COMPILER implies ASMJIT_NO_UJIT.
#if defined(ASMJIT_NO_COMPILER) && !defined(ASMJIT_NO_UJIT)
  #define ASMJIT_NO_UJIT
#endif

// Prevent compile-time errors caused by misconfiguration.
#if defined(ASMJIT_NO_TEXT) && !defined(ASMJIT_NO_LOGGING)
  #pragma message("'ASMJIT_NO_TEXT' can only be defined when 'ASMJIT_NO_LOGGING' is defined.")
  #undef ASMJIT_NO_TEXT
#endif

#if defined(ASMJIT_NO_INTROSPECTION) && !defined(ASMJIT_NO_COMPILER)
  #pragma message("'ASMJIT_NO_INTROSPECTION' can only be defined when 'ASMJIT_NO_COMPILER' is defined")
  #undef ASMJIT_NO_INTROSPECTION
#endif

// Build Mode
// ==========

// Detect ASMJIT_BUILD_DEBUG and ASMJIT_BUILD_RELEASE if not defined.
#if !defined(ASMJIT_BUILD_DEBUG) && !defined(ASMJIT_BUILD_RELEASE)
  #if !defined(NDEBUG)
    #define ASMJIT_BUILD_DEBUG
  #else
    #define ASMJIT_BUILD_RELEASE
  #endif
#endif

// Target Architecture Detection
// =============================

//! \addtogroup asmjit_core
//! \{

//! \def ASMJIT_ARCH_X86
//!
//! Defined to either 0, 32, or 64 depending on whether the target CPU is X86 (32) or X86_64 (64).

//! \def ASMJIT_ARCH_ARM
//!
//! Defined to either 0, 32, or 64 depending on whether the target CPU is ARM (32) or AArch64 (64).

//! \def ASMJIT_ARCH_MIPS
//!
//! Defined to either 0, 32, or 64 depending on whether the target CPU is MIPS (32) or MISP64 (64).

//! \def ASMJIT_ARCH_RISCV
//!
//! Defined to either 0, 32, or 64 depending on whether the target CPU is RV32 (32) or RV64 (64).

//! \def ASMJIT_ARCH_BITS
//!
//! Defined to either 32 or 64 depending on the target.

//! \def ASMJIT_ARCH_LE
//!
//! Defined to 1 if the target architecture is little endian.

//! \def ASMJIT_ARCH_BE
//!
//! Defined to 1 if the target architecture is big endian.

//! \def ASMJIT_HAS_HOST_BACKEND
//!
//! Defined when AsmJit is built with the target architecture backend.
//!
//! For example if AsmJit is building for x86 or x86_64 architectures and `ASMJIT_NO_X86` is not defined,
//! it would define `ASMJIT_HAS_HOST_BACKEND` when `<asmjit/code.h>` or ``<asmjit/host.h>` is included.

//! \}

//! \cond NONE

#if defined(_M_X64) || defined(__x86_64__)
  #define ASMJIT_ARCH_X86 64
#elif defined(_M_IX86) || defined(__X86__) || defined(__i386__)
  #define ASMJIT_ARCH_X86 32
#else
  #define ASMJIT_ARCH_X86 0
#endif

#if defined(_M_ARM64) || defined(__arm64__) || defined(__aarch64__)
# define ASMJIT_ARCH_ARM 64
#elif defined(_M_ARM) || defined(_M_ARMT) || defined(__arm__) || defined(__thumb__) || defined(__thumb2__)
  #define ASMJIT_ARCH_ARM 32
#else
  #define ASMJIT_ARCH_ARM 0
#endif

#if defined(_MIPS_ARCH_MIPS64) || defined(__mips64)
  #define ASMJIT_ARCH_MIPS 64
#elif defined(_MIPS_ARCH_MIPS32) || defined(_M_MRX000) || defined(__mips__)
  #define ASMJIT_ARCH_MIPS 32
#else
  #define ASMJIT_ARCH_MIPS 0
#endif

// NOTE `__riscv` is the correct macro in this case as specified by "RISC-V Toolchain Conventions".
#if (defined(__riscv) || defined(__riscv__)) && defined(__riscv_xlen)
  #define ASMJIT_ARCH_RISCV __riscv_xlen
#else
  #define ASMJIT_ARCH_RISCV 0
#endif

#define ASMJIT_ARCH_BITS (ASMJIT_ARCH_X86 | ASMJIT_ARCH_ARM | ASMJIT_ARCH_MIPS | ASMJIT_ARCH_RISCV)
#if ASMJIT_ARCH_BITS == 0
  #undef ASMJIT_ARCH_BITS
  #if defined(__LP64__) || defined(_LP64)
    #define ASMJIT_ARCH_BITS 64
  #else
    #define ASMJIT_ARCH_BITS 32
  #endif
#endif

#if (defined(__ARMEB__))  || \
    (defined(__MIPSEB__)) || \
    (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
  #define ASMJIT_ARCH_LE 0
  #define ASMJIT_ARCH_BE 1
#else
  #define ASMJIT_ARCH_LE 1
  #define ASMJIT_ARCH_BE 0
#endif

#if defined(ASMJIT_NO_FOREIGN)
  #if !ASMJIT_ARCH_X86 && !defined(ASMJIT_NO_X86)
    #define ASMJIT_NO_X86
  #endif

  #if ASMJIT_ARCH_ARM != 64 && !defined(ASMJIT_NO_AARCH64)
    #define ASMJIT_NO_AARCH64
  #endif
#endif

#if ASMJIT_ARCH_X86 != 0 && !defined(ASMJIT_NO_X86)
  #define ASMJIT_HAS_HOST_BACKEND
#endif

#if ASMJIT_ARCH_ARM == 64 && !defined(ASMJIT_NO_AARCH64)
  #define ASMJIT_HAS_HOST_BACKEND
#endif

#if !defined(ASMJIT_NO_UJIT)
  #if !defined(ASMJIT_NO_X86) && ASMJIT_ARCH_X86 != 0
    #define ASMJIT_UJIT_X86
  #elif !defined(ASMJIT_NO_AARCH64) && ASMJIT_ARCH_ARM == 64
    #define ASMJIT_UJIT_AARCH64
  #else
    #define ASMJIT_NO_UJIT
  #endif
#endif

//! \endcond

// C++ Compiler and Features Detection
// ===================================

#if defined(__GNUC__) && defined(__has_attribute)
  #define ASMJIT_CXX_HAS_ATTRIBUTE(NAME, CHECK) (__has_attribute(NAME))
#else
  #define ASMJIT_CXX_HAS_ATTRIBUTE(NAME, CHECK) (!(!(CHECK)))
#endif // !ASMJIT_CXX_HAS_ATTRIBUTE

// API Decorators & C++ Extensions
// ===============================

//! \addtogroup asmjit_core
//! \{

//! \def ASMJIT_API
//!
//! A decorator that is used to decorate API that AsmJit exports when built as a shared library.

//! \def ASMJIT_VIRTAPI
//!
//! This is basically a workaround. When using MSVC and marking class as DLL export everything gets exported, which
//! is unwanted in most projects. MSVC automatically exports typeinfo and vtable if at least one symbol of the class
//! is exported. However, GCC has some strange behavior that even if one or more symbol is exported it doesn't export
//! typeinfo unless the class itself is decorated with "visibility(default)" (i.e. ASMJIT_API).

//! \def ASMJIT_INLINE
//!
//! Decorator to force inlining of functions, uses either `__attribute__((__always_inline__))` or __forceinline,
//! depending on C++ compiler.

//! \def ASMJIT_INLINE_NODEBUG
//!
//! Like \ref ASMJIT_INLINE, but uses additionally `__nodebug__` or `__artificial__` attribute to make the
//! debugging of some AsmJit functions easier, especially getters and one-line abstractions where usually you don't
//! want to step in.

//! \def ASMJIT_INLINE_CONSTEXPR
//!
//! Like \ref ASMJIT_INLINE_NODEBUG, but having an additional `constexpr` attribute.

//! \def ASMJIT_NOINLINE
//!
//! Decorator to avoid inlining of functions, uses either `__attribute__((__noinline__))` or `__declspec(noinline)`
//! depending on C++ compiler.

//! \def ASMJIT_CDECL
//!
//! CDECL function attribute - either `__attribute__((__cdecl__))` or `__cdecl`.

//! \def ASMJIT_STDCALL
//!
//! STDCALL function attribute - either `__attribute__((__stdcall__))` or `__stdcall`.
//!
//! \note This expands to nothing on non-x86 targets as STDCALL is X86 specific.

//! \def ASMJIT_FASTCALL
//!
//! FASTCALL function attribute - either `__attribute__((__fastcall__))` or `__fastcall`.
//!
//! \note Expands to nothing on non-x86 targets as FASTCALL is X86 specific.

//! \def ASMJIT_REGPARM(N)
//!
//! Expands to `__attribute__((__regparm__(N)))` when compiled by GCC or clang, nothing otherwise.

//! \def ASMJIT_VECTORCALL
//!
//! VECTORCALL function attribute - either `__attribute__((__vectorcall__))` or `__vectorcall`.
//!
//! \note Expands to nothing on non-x86 targets as VECTORCALL is X86 specific.

//! \}

// API (Export / Import).
#if !defined(ASMJIT_STATIC)
  #if defined(_WIN32) && (defined(_MSC_VER) || defined(__MINGW32__))
    #ifdef ASMJIT_EXPORTS
      #define ASMJIT_API __declspec(dllexport)
    #else
      #define ASMJIT_API __declspec(dllimport)
    #endif
  #elif defined(_WIN32) && defined(__GNUC__)
    #ifdef ASMJIT_EXPORTS
      #define ASMJIT_API __attribute__((__dllexport__))
    #else
      #define ASMJIT_API __attribute__((__dllimport__))
    #endif
  #elif defined(__GNUC__)
    #define ASMJIT_API __attribute__((__visibility__("default")))
  #endif
#endif

#if !defined(ASMJIT_API)
  #define ASMJIT_API
#endif

#if !defined(ASMJIT_VARAPI)
  #define ASMJIT_VARAPI extern ASMJIT_API
#endif

#if defined(__GNUC__) && !defined(_WIN32)
  #define ASMJIT_VIRTAPI ASMJIT_API
#else
  #define ASMJIT_VIRTAPI
#endif

// Function attributes.
#if !defined(ASMJIT_BUILD_DEBUG) && defined(__GNUC__)
  #define ASMJIT_INLINE inline __attribute__((__always_inline__))
#elif !defined(ASMJIT_BUILD_DEBUG) && defined(_MSC_VER)
  #define ASMJIT_INLINE __forceinline
#else
  #define ASMJIT_INLINE inline
#endif


#if defined(__clang__)
  #define ASMJIT_INLINE_NODEBUG inline __attribute__((__always_inline__, __nodebug__))
#elif defined(__GNUC__)
  #define ASMJIT_INLINE_NODEBUG inline __attribute__((__always_inline__, __artificial__))
#else
  #define ASMJIT_INLINE_NODEBUG inline
#endif

#define ASMJIT_INLINE_CONSTEXPR constexpr ASMJIT_INLINE_NODEBUG

#if defined(__GNUC__)
  #define ASMJIT_NOINLINE __attribute__((__noinline__))
#elif defined(_MSC_VER)
  #define ASMJIT_NOINLINE __declspec(noinline)
#else
  #define ASMJIT_NOINLINE
#endif

// Calling conventions.
#if ASMJIT_ARCH_X86 == 32 && defined(__GNUC__)
  #define ASMJIT_CDECL __attribute__((__cdecl__))
  #define ASMJIT_STDCALL __attribute__((__stdcall__))
  #define ASMJIT_FASTCALL __attribute__((__fastcall__))
  #define ASMJIT_REGPARM(N) __attribute__((__regparm__(N)))
#elif ASMJIT_ARCH_X86 == 32 && defined(_MSC_VER)
  #define ASMJIT_CDECL __cdecl
  #define ASMJIT_STDCALL __stdcall
  #define ASMJIT_FASTCALL __fastcall
  #define ASMJIT_REGPARM(N)
#else
  #define ASMJIT_CDECL
  #define ASMJIT_STDCALL
  #define ASMJIT_FASTCALL
  #define ASMJIT_REGPARM(N)
#endif

#if ASMJIT_ARCH_X86 && defined(_WIN32) && defined(_MSC_VER)
  #define ASMJIT_VECTORCALL __vectorcall
#elif ASMJIT_ARCH_X86 && defined(_WIN32)
  #define ASMJIT_VECTORCALL __attribute__((__vectorcall__))
#else
  #define ASMJIT_VECTORCALL
#endif

// Type alignment (not allowed by C++17 'alignas' keyword).
#if defined(__GNUC__)
  #define ASMJIT_ALIGN_TYPE(N, ...) __attribute__((__aligned__(N))) __VA_ARGS__
#elif defined(_MSC_VER)
  #define ASMJIT_ALIGN_TYPE(N, ...) __declspec(align(N)) __VA_ARGS__
#else
  #define ASMJIT_ALIGN_TYPE(N, ...) __VA_ARGS__
#endif

//! \def ASMJIT_MAY_ALIAS
//!
//! Expands to `__attribute__((__may_alias__))` if supported.
#if defined(__GNUC__)
  #define ASMJIT_MAY_ALIAS __attribute__((__may_alias__))
#else
  #define ASMJIT_MAY_ALIAS
#endif

#if defined(__clang__) && !defined(_DOXYGEN)
  // NOTE: Clang allows to apply this attribute to function arguments, which is what we want. Once GCC decides
  // to support this use, we will enable it for GCC as well. However, until that, it will be clang only, which
  // is what we need for static analysis.
  #define ASMJIT_NONNULL(FUNCTION_ARGUMENT) FUNCTION_ARGUMENT __attribute__((__nonnull__))
#else
  #define ASMJIT_NONNULL(FUNCTION_ARGUMENT) FUNCTION_ARGUMENT
#endif

//! \def ASMJIT_ASSUME(...)
//!
//! Macro that tells the C/C++ compiler that the expression `...` evaluates to true.
//!
//! This macro has two purposes:
//!
//!   1. Enable optimizations that would not be possible without the assumption.
//!   2. Hint static analysis tools that a certain condition is true to prevent false positives.
#if defined(__clang__)
  #define ASMJIT_ASSUME(...) __builtin_assume(__VA_ARGS__)
#elif defined(__GNUC__)
  #define ASMJIT_ASSUME(...) do { if (!(__VA_ARGS__)) __builtin_unreachable(); } while (0)
#elif defined(_MSC_VER)
  #define ASMJIT_ASSUME(...) __assume(__VA_ARGS__)
#else
  #define ASMJIT_ASSUME(...) (void)0
#endif

//! \def ASMJIT_LIKELY(...)
//!
//! Condition is likely to be taken (mostly error handling and edge cases).

//! \def ASMJIT_UNLIKELY(...)
//!
//! Condition is unlikely to be taken (mostly error handling and edge cases).
#if defined(__GNUC__)
  #define ASMJIT_LIKELY(...) __builtin_expect(!!(__VA_ARGS__), 1)
  #define ASMJIT_UNLIKELY(...) __builtin_expect(!!(__VA_ARGS__), 0)
#else
  #define ASMJIT_LIKELY(...) (__VA_ARGS__)
  #define ASMJIT_UNLIKELY(...) (__VA_ARGS__)
#endif

// Utilities.
#define ASMJIT_OFFSET_OF(STRUCT, MEMBER) ((int)(intptr_t)((const char*)&((const STRUCT*)0x100)->MEMBER) - 0x100)
#define ASMJIT_ARRAY_SIZE(X) uint32_t(sizeof(X) / sizeof(X[0]))

#if ASMJIT_CXX_HAS_ATTRIBUTE(no_sanitize, 0)
  #define ASMJIT_ATTRIBUTE_NO_SANITIZE_UNDEF __attribute__((__no_sanitize__("undefined")))
#elif defined(__GNUC__)
  #define ASMJIT_ATTRIBUTE_NO_SANITIZE_UNDEF __attribute__((__no_sanitize_undefined__))
#else
  #define ASMJIT_ATTRIBUTE_NO_SANITIZE_UNDEF
#endif

// Diagnostic Macros
// ======================================

#if defined(_MSC_VER) && !defined(__clang__) && !defined(_DOXYGEN)
  #define ASMJIT_BEGIN_DIAGNOSTIC_SCOPE                                        \
    __pragma(warning(push))                                                    \
    __pragma(warning(disable: 4127))  /* conditional expression is const */    \
    __pragma(warning(disable: 4201))  /* nameless struct/union */
  #define ASMJIT_END_DIAGNOSTIC_SCOPE                                          \
    __pragma(warning(pop))
#else
  #define ASMJIT_BEGIN_DIAGNOSTIC_SCOPE
  #define ASMJIT_END_DIAGNOSTIC_SCOPE
#endif

// Begin-Namespace & End-Namespace Macros
// ======================================

#if !defined(ASMJIT_NO_ABI_NAMESPACE) && !defined(_DOXYGEN)
  #define ASMJIT_BEGIN_NAMESPACE                                               \
    ASMJIT_BEGIN_DIAGNOSTIC_SCOPE                                              \
    namespace asmjit {                                                         \
    inline namespace ASMJIT_ABI_NAMESPACE {
  #define ASMJIT_END_NAMESPACE                                                 \
    }}                                                                         \
    ASMJIT_END_DIAGNOSTIC_SCOPE
#else
  #define ASMJIT_BEGIN_NAMESPACE                                               \
    ASMJIT_BEGIN_DIAGNOSTIC_SCOPE                                              \
    namespace asmjit {
  #define ASMJIT_END_NAMESPACE                                                 \
    }                                                                          \
    ASMJIT_END_DIAGNOSTIC_SCOPE
#endif

#define ASMJIT_BEGIN_SUB_NAMESPACE(NAMESPACE) ASMJIT_BEGIN_NAMESPACE namespace NAMESPACE {
#define ASMJIT_END_SUB_NAMESPACE } ASMJIT_END_NAMESPACE

// C++ Utilities
// =============

#define ASMJIT_NONCOPYABLE(Type)                                               \
    Type(const Type& other) = delete;                                          \
    Type& operator=(const Type& other) = delete;

#define ASMJIT_NONCONSTRUCTIBLE(Type)                                          \
    Type() = delete;                                                           \
    Type(const Type& other) = delete;                                          \
    Type& operator=(const Type& other) = delete;

//! \def ASMJIT_DEFINE_ENUM_FLAGS(T)
//!
//! Defines bit operations for enumeration flags.
#ifdef _DOXYGEN
  #define ASMJIT_DEFINE_ENUM_FLAGS(T)
#else
  #define ASMJIT_DEFINE_ENUM_FLAGS(T)                                          \
    static ASMJIT_INLINE_CONSTEXPR T operator~(T a) noexcept {                 \
      return T(~std::underlying_type_t<T>(a));                                 \
    }                                                                          \
                                                                               \
    static ASMJIT_INLINE_CONSTEXPR T operator|(T a, T b) noexcept {            \
      return T(std::underlying_type_t<T>(a) | std::underlying_type_t<T>(b));   \
    }                                                                          \
    static ASMJIT_INLINE_CONSTEXPR T operator&(T a, T b) noexcept {            \
      return T(std::underlying_type_t<T>(a) & std::underlying_type_t<T>(b));   \
    }                                                                          \
    static ASMJIT_INLINE_CONSTEXPR T operator^(T a, T b) noexcept {            \
      return T(std::underlying_type_t<T>(a) ^ std::underlying_type_t<T>(b));   \
    }                                                                          \
                                                                               \
    static ASMJIT_INLINE_CONSTEXPR T& operator|=(T& a, T b) noexcept {         \
      a = T(std::underlying_type_t<T>(a) | std::underlying_type_t<T>(b));      \
      return a;                                                                \
    }                                                                          \
    static ASMJIT_INLINE_CONSTEXPR T& operator&=(T& a, T b) noexcept {         \
      a = T(std::underlying_type_t<T>(a) & std::underlying_type_t<T>(b));      \
      return a;                                                                \
    }                                                                          \
    static ASMJIT_INLINE_CONSTEXPR T& operator^=(T& a, T b) noexcept {         \
      a = T(std::underlying_type_t<T>(a) ^ std::underlying_type_t<T>(b));      \
      return a;                                                                \
    }
#endif

//! \def ASMJIT_DEFINE_ENUM_COMPARE(T)
//!
//! Defines comparison operations for enumeration flags.
#if defined(_DOXYGEN) || (defined(_MSC_VER) && _MSC_VER <= 1900)
  #define ASMJIT_DEFINE_ENUM_COMPARE(T)
#else
  #define ASMJIT_DEFINE_ENUM_COMPARE(T)                                        \
    static ASMJIT_INLINE_CONSTEXPR bool operator<(T a, T b) noexcept {         \
      return (std::underlying_type_t<T>)(a) < (std::underlying_type_t<T>)(b);  \
    }                                                                          \
    static ASMJIT_INLINE_CONSTEXPR bool operator<=(T a, T b) noexcept {        \
      return (std::underlying_type_t<T>)(a) <= (std::underlying_type_t<T>)(b); \
    }                                                                          \
    static ASMJIT_INLINE_CONSTEXPR bool operator>(T a, T b) noexcept {         \
      return (std::underlying_type_t<T>)(a) > (std::underlying_type_t<T>)(b);  \
    }                                                                          \
    static ASMJIT_INLINE_CONSTEXPR bool operator>=(T a, T b) noexcept {        \
      return (std::underlying_type_t<T>)(a) >= (std::underlying_type_t<T>)(b); \
    }
#endif

//! Defines a strong type `C` that wraps a value of `T`.
#define ASMJIT_DEFINE_STRONG_TYPE(C, T)                                                   \
struct C {                                                                                \
  T v;                                                                                    \
                                                                                          \
  ASMJIT_INLINE_NODEBUG C() = default;                                                    \
  ASMJIT_INLINE_CONSTEXPR explicit C(T x) noexcept : v(x) {}                              \
  ASMJIT_INLINE_CONSTEXPR C(const C& other) noexcept = default;                           \
                                                                                          \
  ASMJIT_INLINE_CONSTEXPR T value() const noexcept { return v; }                          \
                                                                                          \
  ASMJIT_INLINE_CONSTEXPR T* valuePtr() noexcept { return &v; }                           \
  ASMJIT_INLINE_CONSTEXPR const T* valuePtr() const noexcept { return &v; }               \
                                                                                          \
  ASMJIT_INLINE_CONSTEXPR C& operator=(T x) noexcept { v = x; return *this; };            \
  ASMJIT_INLINE_CONSTEXPR C& operator=(const C& x) noexcept { v = x.v; return *this; }    \
                                                                                          \
  ASMJIT_INLINE_CONSTEXPR C operator+(T x) const noexcept { return C(v + x); }            \
  ASMJIT_INLINE_CONSTEXPR C operator-(T x) const noexcept { return C(v - x); }            \
  ASMJIT_INLINE_CONSTEXPR C operator*(T x) const noexcept { return C(v * x); }            \
  ASMJIT_INLINE_CONSTEXPR C operator/(T x) const noexcept { return C(v / x); }            \
                                                                                          \
  ASMJIT_INLINE_CONSTEXPR C operator+(const C& x) const noexcept { return C(v + x.v); }   \
  ASMJIT_INLINE_CONSTEXPR C operator-(const C& x) const noexcept { return C(v - x.v); }   \
  ASMJIT_INLINE_CONSTEXPR C operator*(const C& x) const noexcept { return C(v * x.v); }   \
  ASMJIT_INLINE_CONSTEXPR C operator/(const C& x) const noexcept { return C(v / x.v); }   \
                                                                                          \
  ASMJIT_INLINE_CONSTEXPR C& operator+=(T x) noexcept { v += x; return *this; }           \
  ASMJIT_INLINE_CONSTEXPR C& operator-=(T x) noexcept { v -= x; return *this; }           \
  ASMJIT_INLINE_CONSTEXPR C& operator*=(T x) noexcept { v *= x; return *this; }           \
  ASMJIT_INLINE_CONSTEXPR C& operator/=(T x) noexcept { v /= x; return *this; }           \
                                                                                          \
  ASMJIT_INLINE_CONSTEXPR C& operator+=(const C& x) noexcept { v += x.v; return *this; }  \
  ASMJIT_INLINE_CONSTEXPR C& operator-=(const C& x) noexcept { v -= x.v; return *this; }  \
  ASMJIT_INLINE_CONSTEXPR C& operator*=(const C& x) noexcept { v *= x.v; return *this; }  \
  ASMJIT_INLINE_CONSTEXPR C& operator/=(const C& x) noexcept { v /= x.v; return *this; }  \
                                                                                          \
  ASMJIT_INLINE_CONSTEXPR bool operator==(T x) const noexcept { return v == x; }          \
  ASMJIT_INLINE_CONSTEXPR bool operator!=(T x) const noexcept { return v != x; }          \
  ASMJIT_INLINE_CONSTEXPR bool operator> (T x) const noexcept { return v >  x; }          \
  ASMJIT_INLINE_CONSTEXPR bool operator>=(T x) const noexcept { return v >= x; }          \
  ASMJIT_INLINE_CONSTEXPR bool operator< (T x) const noexcept { return v <  x; }          \
  ASMJIT_INLINE_CONSTEXPR bool operator<=(T x) const noexcept { return v <= x; }          \
                                                                                          \
  ASMJIT_INLINE_CONSTEXPR bool operator==(const C& x) const noexcept { return v == x.v; } \
  ASMJIT_INLINE_CONSTEXPR bool operator!=(const C& x) const noexcept { return v != x.v; } \
  ASMJIT_INLINE_CONSTEXPR bool operator> (const C& x) const noexcept { return v >  x.v; } \
  ASMJIT_INLINE_CONSTEXPR bool operator>=(const C& x) const noexcept { return v >= x.v; } \
  ASMJIT_INLINE_CONSTEXPR bool operator< (const C& x) const noexcept { return v <  x.v; } \
  ASMJIT_INLINE_CONSTEXPR bool operator<=(const C& x) const noexcept { return v <= x.v; } \
};

#endif // ASMJIT_CORE_API_CONFIG_H_INCLUDED
