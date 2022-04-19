
#include <game.hpp>

#include <iterator> 

namespace engine {
    class System {

    };
}

namespace events {

    using ID = entt::hashed_string;

    template <typename T>
    struct EventNameRegistry {
        static constexpr events::ID EventID;
    };

    namespace internal {
        void declare (ID name, entt::id_type type_id, std::uint8_t size)
        {
#ifdef DEBUG_BUILD
            if (size > 62) {
                spdlog::error("Event {} is {} bytes in size, must not exceed 62 bytes", name.data(), size);
                throw std::logic_error("Declared event struct is too large");
            }
#endif
        }
        std::byte* emit (entt::id_type event_type, std::uint8_t size)
        {
            return nullptr;
        }
        std::byte* post (entt::id_type event_type, std::uint8_t size, entt::entity entity)
        {
            return nullptr;
        }
    }
    
    template <typename T, typename... Rest>
    void declare ()
    {
        internal::declare(EventNameRegistry<T>::EventID, entt::type_index<T>::value(), sizeof(T));
        if constexpr (sizeof...(Rest) > 0) {
            declare<Rest...>();
        }
    }

    template <typename T>
    [[maybe_unused]] T& emit ()
    {
        return *(new (internal::emit(entt::type_index<T>::value(), sizeof(T))) T{});
    }

    template <typename T, typename... Args>
    [[maybe_unused]] T& emit (Args... args)
    {
        return *(new (internal::emit(entt::type_index<T>::value(), sizeof(T))) T{args...});
    }

    template <typename T>
    [[maybe_unused]] T& post (entt::entity receiver)
    {
        return *(new (internal::post(entt::type_index<T>::value(), sizeof(T), receiver)) T{});
    }

    template <typename T, typename... Args>
    [[maybe_unused]] T& post (entt::entity receiver, Args... args)
    {
        return *(new (internal::post(entt::type_index<T>::value(), sizeof(T), receiver)) T{args...});
    }

    struct EventIterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = int;
        using pointer           = value_type*;
        using reference         = value_type&;
    };
    struct EventIterable {
        const EventIterator begin() const { return m_begin;}
        const EventIterator end() const { return m_end;}
    private:
        const EventIterator m_begin;
        const EventIterator m_end;
    };

}

#define DECLARE_EVENT(obj, name)  template<> constexpr events::ID events::EventNameRegistry<struct obj>::EventID = events::ID{name}; struct __attribute__((packed)) obj

DECLARE_EVENT(MyEvent, "my_event") {
    int foo;
};
DECLARE_EVENT(TestEvent, "test_event") {
    int parameter;
};
DECLARE_EVENT(AnotherEvent, "another_event") {
    float value1;
    float value2;
};

class MySystem : public engine::System
{
public:

    MySystem () : engine::System()
    {
        events::declare<TestEvent, AnotherEvent, MyEvent>();
    }

    void run_system ()
    {
        // Emit an event
        events::emit<TestEvent>(4);
        // Emit an event, receiving reference for setting parameters
        auto& ev = events::emit<AnotherEvent>();
        ev.value1 = 1.0f;
        ev.value2 = 2.0f;
        // Post event to specific entity 
        events::post<MyEvent>(an_entity, 100);
    }

private:
    entt::entity an_entity = entt::null;
};
