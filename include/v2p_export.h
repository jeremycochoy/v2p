#ifndef EXPORT_H_
#define EXPORT_H_

#if defined _WIN32 || defined _WIN64 || defined __CYGWIN__
  #ifdef BUILDING_DLL
    #ifdef __GNUC__
      #define SYMPH_API __attribute__ ((dllexport))
    #else
      #define SYMPH_API __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #elif defined LINKING_DLL
    #ifdef __GNUC__
      #define SYMPH_API __attribute__ ((dllimport))
    #else
      #define SYMPH_API __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #endif
#else
  #define SYMPH_API
#endif

#endif /* !EXPORT_H_ */
