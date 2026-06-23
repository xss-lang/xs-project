#ifndef XS_COMPILER_CHECK_H
#define XS_COMPILER_CHECK_H

#if defined(__GNUC__) && !defined(__clang__)
#error "GCC is not supported; use Clang."
#endif

#if !defined(__clang__)
#error "Clang is required."
#endif

#if !defined(__STRICT_ANSI__)
#error "GNU C extensions are enabled; compile with strict ISO C mode, e.g. -std=c23, not -std=gnu23."
#endif

#endif
