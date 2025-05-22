#include "sundog.h"
#include "pixilang.h"

const char* g_app_name = "Pixilang " PIXILANG_VERSION_STR " (" __DATE__ ")";
const char* g_app_name_short = "Pixilang";
const char* g_app_config[] = { "1:/pixilang_config.ini", "2:/pixilang_config.ini", 0 };
#if defined(OS_ANDROID) || defined(OS_WIN) || defined(OS_MACOS)
    const char* g_app_log = "1:/pixilang_log.txt";
#else
    const char* g_app_log = "3:/pixilang_log.txt";
#endif
int g_app_window_xsize = 480;
int g_app_window_ysize = 320;
uint g_app_window_flags = WIN_INIT_FLAG_SCALABLE | WIN_INIT_FLAG_SHRINK_DESKTOP_TO_SAFE_AREA;
app_option g_app_options[] = { { NULL } };
const char* g_app_usage =
"Pixilang " PIXILANG_VERSION_STR " (" __DATE__ ") / WarmPlace.ru\n"
"Usage: pixilang [options] [filename] [arg]\n"
"Options:\n"
" -?            show help\n"
" clearall      reset all settings\n"
" -ncl          disable logging to console\n"
" -nfl          disable logging to file\n"
" -cfg <config> apply additional configuration in the following format:\n"
"               \"OPTION1=VALUE|OPTION2=VALUE|...\"\n"
" -c            generate bytecode; filename.pixicode file will be produced\n"
"Filename: source (UTF-8 text file) or bytecode (*.pixicode).\n"
"Additional arguments (arg): some options for the Pixilang program.\n";
