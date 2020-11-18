#pragma once
#include <cstdint>
#include <functional>

#define ws_coroutine_begin(coroutine) \
    if(!coroutine.__is_waiting()) { \
        auto& __coroutine = coroutine; \
        switch(__coroutine.__current()) { \
            case 0: {

#define ws_coroutine_label(label) \
                __coroutine.__jump(-__hash(label)); \
            } break; \
            case -__hash(label): {

#define ws_coroutine_yield \
            __coroutine.__jump(__LINE__); \
            } break;

#define ws_coroutine_wait(time) \
                __coroutine.__jump(__LINE__); \
                __coroutine.__wait(time); \
            } break; \
            case __LINE__: {

#define ws_coroutine_end \
            __coroutine.__end(); \
            } break; \
        } \
    }

#define ws_coroutine_jump(label) __coroutine.__jump(-__hash(label));
#define ws_coroutine_repeat __coroutine.__jump(__coroutine.__current());

struct ws_coroutine_t {
    using ref = ws_coroutine_t&;
    using coroutine_function = std::function<void(ref)>;

    bool __running = true;
    int32_t __current_state = 0;
    float __wait_until = 0.0f;
    coroutine_function __function;
    ws_coroutine_t() = default;
    ws_coroutine_t(coroutine_function function);
    int __current() const;
    bool __is_waiting();
    void __jump(uint32_t hash_line);
    void __wait(float wait_length);
    void __end();
    void run();
};

constexpr int32_t __hash(char const* s, std::size_t count)
{
    return ((count ? __hash(s, count - 1) : 2166136261) ^ s[count]) * 16777619;
}