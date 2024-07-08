#ifndef CPU_H_
#define CPU_H_

#include <sys/types.h>
#include <stdint.h>


#if defined(__APPLE__) && defined(__MACH__)
  #define CP_USER 0
  #define CP_SYS  1
  #define CP_IDLE 2
  #define CP_NICE 3
  #define CP_STATES 4
#else
  #define CP_USER   0
  #define CP_NICE   1
  #define CP_SYS    2

  #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    // *BSD or OSX
    #define CP_INTR   3
    #define CP_IDLE   4
    #define CP_STATES 5
  #else
    //linux
    #define CP_IDLE 3
    #define CP_STATES 4
  #endif
#endif

float cpu_percentage( unsigned );
uint32_t get_cpu_count();

/** CPU percentage output mode.
 *
 * Examples:
 *
 * CPU_MODE_DEFAULT: 100%
 * CPU_MODE_THREADS: 800% (8 cores, fully loaded)
 */
enum CPU_MODE
{
  CPU_MODE_DEFAULT,
  CPU_MODE_THREADS
};

#endif

