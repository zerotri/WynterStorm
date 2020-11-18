#include <WynterStorm.h>
#include <iostream>

ws_coroutine_t::ws_coroutine_t(ws_coroutine_t::coroutine_function function)
: __function(function)
{
}

int ws_coroutine_t::__current() const
{
    return __current_state;
}

void ws_coroutine_t::__jump(uint32_t hash_line)
{
    __current_state = hash_line;
    __wait_until = 0.0f;
}

void ws_coroutine_t::__end()
{
    __running = false;
}

void ws_coroutine_t::run()
{
    if(__running)
        __function(*this);
}

bool ws_coroutine_t::__is_waiting()
{
    if(__wait_until != 0.0f)
    {
        if(ws_time_current() > __wait_until)
        {
            __wait_until = 0.0f;
            return false;
        }
        return true;
    }
    return false;
}

void ws_coroutine_t::__wait(float wait_length)
{
    __wait_until = ws_time_current() + wait_length;
    // std::cout << "Waiting until " << __wait_until << "currently " << ws_time_current() << std::endl;
}