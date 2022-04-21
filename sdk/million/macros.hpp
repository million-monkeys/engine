#pragma once

#define DECLARE_EVENT(obj, name) struct Event_ID__ ## obj { static constexpr entt::hashed_string EventID = entt::hashed_string{name}; }; struct __attribute__((packed)) obj : public Event_ID__ ## obj
