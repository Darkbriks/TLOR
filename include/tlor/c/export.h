/**
 * @file export.h
 * @brief Symbol export macro of the TLOR shared library.
 */
#ifndef TLOR_C_EXPORT_H
#define TLOR_C_EXPORT_H

#if defined(_WIN32)
#if defined(TLOR_BUILDING_LIBRARY)
#define TLOR_API __declspec(dllexport)
#else
#define TLOR_API __declspec(dllimport)
#endif
#else
#define TLOR_API __attribute__((visibility("default")))
#endif

#endif
