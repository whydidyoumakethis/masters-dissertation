#pragma once
#include <entt/entt.hpp>
#include <functional>
#include <memory>
template<typename... Args>
class Event {
public:

    //template<auto Fn>
    /*void AddListener() {
        entt::sink{ _sig }.connect<Fn>();
    }*/

    /*template<auto MemberFn, typename T>
    void AddListener(T* instance) {
        entt::sink{ _sig }.connect<MemberFn>(instance);
    }*/
};