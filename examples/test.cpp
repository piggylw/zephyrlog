#include "zephyrlog/zephyrlog.h"

int main()
{
    using namespace zephyrlog;

    ZDEBUG << "This is a debug message";
    ZINFO << "This is an info message";
    ZWARN << "This is a warning message";
    ZERROR << "This is an error message";
    ZCRIT << "This is a critical message";

    ZINFO << "Logging an integer: " << 42;
    ZINFO << "Logging a double: " << 3.14159;

    return 0;
}
