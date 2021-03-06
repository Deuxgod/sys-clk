/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#include "file_utils.h"

static Mutex g_log_mutex;
static std::atomic_bool g_has_initialized = false;
static bool g_log_enabled = false;

static void _FileUtils_InitializeThreadFunc(void *args)
{
    FileUtils::Initialize();
    svcExitThread();
}

void FileUtils::WaitSDServices()
{
    Handle temporaryHandle = 0;
    std::vector<std::string> requiredServices = {"pcv", "gpio", "pinmux", "psc:c"};
    for (auto serviceName : requiredServices)
    {
        if (R_SUCCEEDED(smGetServiceOriginal(&temporaryHandle, smEncodeName(serviceName.c_str()))))
        {
            svcCloseHandle(temporaryHandle);
        }
    }
}

bool FileUtils::IsInitialized()
{
    return g_has_initialized;
}

void FileUtils::Log(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    if (g_has_initialized && g_log_enabled)
    {
        mutexLock(&g_log_mutex);
        FILE *file = fopen(FILE_LOG_FILE_PATH, "a");
        if (file)
        {
            va_start(args, format);
            vfprintf(file, format, args);
            fclose(file);
        }
        mutexUnlock(&g_log_mutex);
    }
    va_end(args);
}

void FileUtils::InitializeAsync()
{
    Thread initThread = {0};
    threadCreate(&initThread, _FileUtils_InitializeThreadFunc, NULL, 0x4000, 0x15, 0);
    threadStart(&initThread);
}

Result FileUtils::Initialize()
{
    Result rc = 0;

    mutexInit(&g_log_mutex);
    WaitSDServices();

    if (R_SUCCEEDED(rc))
    {
        rc = fsInitialize();
    }

    if (R_SUCCEEDED(rc))
    {
        rc = fsdevMountSdmc();
    }

    if (R_SUCCEEDED(rc))
    {
        FILE *file = fopen(FILE_LOG_FLAG_PATH, "r");
        if (file)
        {
            g_log_enabled = true;
            fclose(file);
        }

        g_has_initialized = true;

        FileUtils::Log("=== " TARGET " boot ===\n");
    }

    return rc;
}

void FileUtils::Exit()
{
    if (!g_has_initialized)
    {
        return;
    }

    g_has_initialized = false;
    g_log_enabled = false;

    fsdevUnmountAll();
    fsExit();
}
