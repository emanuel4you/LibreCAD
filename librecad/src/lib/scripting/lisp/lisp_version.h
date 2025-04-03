#ifndef LISP_VERSION_H
#define LISP_VERSION_H

#ifdef DEVELOPER

#define LISP_STR_HELPER(x) #x
#define LISP_STR(x) LISP_STR_HELPER(x)

#ifndef _WIN64
#define HOST "linux"
#else
#define HOST "windows"
#endif

#if defined(__clang__)
#define USED_COMPILER "[Clang " __clang_version__ "]"
#elif defined(__GNUC__)
#define USED_COMPILER "[C++ " __VERSION__ "]"
#elif defined(_MSC_VER)
    #if _MSC_VER < 1910
        #define USED_COMPILER "[MSVC++ 2015]"
    #elif _MSC_VER < 1920
        #define USED_COMPILER "[MSVC++ 2017]"
    #elif _MSC_VER < 1930
        #define USED_COMPILER "[MSVC++ 2019]"
    #elif _MSC_VER >= 1930
        #define USED_COMPILER "[MSVC++ 2022]"
    #else
        #define USED_COMPILER "[MSVC++]"
    #endif
#else
#define USED_COMPILER "[C++]"
#endif

// __LISP__
#ifndef __LISP__
#define LISP_MAJOR_VER  1
#define LISP_MINOR_VER  2
#define LISP_PATCHLEVEL 11
#define LISP_BUILD "devel"

#define LISP_VERSION LISP_MAJOR_VER * 10000 \
    + LISP_MINOR_VER * 100 \
    + LISP_PATCHLEVEL

#define __LISP__ \
    LISP_STR(LISP_MAJOR_VER) \
    "." \
    LISP_STR(LISP_MINOR_VER) \
    "." \
    LISP_STR(LISP_PATCHLEVEL)

#define LISP_VERSION_STR_HELPER(rel, build, date, time) \
    "LibreLisp " rel " (" build ", " date ", " time ") " USED_COMPILER " on " HOST

#define LISP_VERSION_STR \
    LISP_VERSION_STR_HELPER(__LISP__, LISP_BUILD, __DATE__, __TIME__)

#define LISP_COPYRIGHT \
    "\nType \"help\", \"copyright\", \"credits\" or \"license\" " \
    "for more information."
#endif

#endif // DEVELOPER

#endif // LISP_VERSION_H
