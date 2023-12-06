#pragma once

#if defined(__clang__)
    #define PRAGMA_DISABLE_WARNINGS \
        _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Weverything\"")

    #define PRAGMA_ENABLE_WARNINGS \
        _Pragma("clang diagnostic pop")
#elif defined(_MSC_VER)
    #define PRAGMA_DISABLE_WARNINGS \
        _Pragma("warning(push, 0)")
    
    #define PRAGMA_ENABLE_WARNINGS \
        _Pragma("warning(pop)")
#endif