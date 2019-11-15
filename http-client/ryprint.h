#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>

#define COLOR_STR_NONE          "\033[0m"
#define COLOR_STR_BLACK         "\033[0;30m"
#define COLOR_STR_LIGHT_GRAY    "\033[0;37m"
#define COLOR_STR_DARK_GRAY     "\033[1;30m"
#define COLOR_STR_BLUE          "\033[0;34m"
#define COLOR_STR_LIGHT_BLUE    "\033[1;34m"
#define COLOR_STR_GREEN         "\033[0;32m"
#define COLOR_STR_LIGHT_GREEN   "\033[1;32m"
#define COLOR_STR_CYAN          "\033[0;36m"
#define COLOR_STR_LIGHT_CYAN    "\033[1;36m"
#define COLOR_STR_RED           "\033[0;31m"
#define COLOR_STR_LIGHT_RED     "\033[1;31m"
#define COLOR_STR_PURPLE        "\033[0;35m"
#define COLOR_STR_LIGHT_PURPLE  "\033[1;35m"
#define COLOR_STR_BROWN         "\033[0;33m"
#define COLOR_STR_YELLOW        "\033[1;33m"
#define COLOR_STR_WHITE         "\033[1;37m"
#define TIME_STR        "[%04d-%02d-%02d %02d:%02d:%02d]"

#define filename(x) strrchr(x,'/')?strrchr(x,'/')+1:x

#define ryDbg(str, args...)     do {struct timeval tm_tmp; struct tm *ptm = NULL; gettimeofday(&tm_tmp, NULL); ptm = gmtime(&tm_tmp.tv_sec); \
    printf(COLOR_STR_GREEN " " TIME_STR " %s>: FUNCTION: %s, LINE: %d, " COLOR_STR_NONE " " str, \
    ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, filename(__FILE__), __FUNCTION__, __LINE__, ## args); } while (0)

#define ryErr(str, args...)     do {struct timeval tm_tmp; struct tm *ptm = NULL; gettimeofday(&tm_tmp, NULL); ptm = gmtime(&tm_tmp.tv_sec); \
    printf(COLOR_STR_RED " " TIME_STR " %s:%d, %s> " COLOR_STR_NONE " " str, \
    ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, __FILE__, __LINE__, __FUNCTION__, ## args); } while (0)


#define assert_param(expect) \
do { \
    if (!(expect)) \
    { \
        ryErr("assert failed, expect(%s)\n", #expect); \
        return; \
    } \
} while (0)

#define assert_param_return(expect, ret) \
do { \
    if (!(expect)) \
    { \
        ryErr("assert failed, expect \"%s\"\n", #expect); \
        return ret; \
    } \
} while (0)




#ifdef __cplusplus
}
#endif
