#pragma once

#include <string>
#include <functional>
#include <memory>

namespace helpers {

    namespace impl {

        template <typename... T>
        class Defer {
        public:
            Defer(std::tuple<T...> fns) : deferred(fns){}
            ~Defer(){
                std::apply([](T... args){call_deferred(args...);}, deferred);
            }
        private:
            std::tuple<T...> deferred;
            template <typename Fn, typename... Rest>
            static void call_deferred(Fn fn, Rest... rest){
                fn();
                call_deferred(rest...);
            }
            static void call_deferred() {}
        };

    } // impl::

    ///////////////////////////////////////////////////////////////////////////
    // Type alias for a spp::sparse_hash_map with hashed strings as keys
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    using hashed_string_map = spp::sparse_hash_map<entt::hashed_string::hash_type, T, entt::identity>;

    ///////////////////////////////////////////////////////////////////////////
    // Utility to make a variant visitor out of lambdas, using the *overloaded
    // pattern* as describped in cppreference:
    //  https://en.cppreference.com/w/cpp/utility/variant/visit).
    //
    // Taken from Lager, MIT Licensed
    //  Copyright (C) 2017 Juan Pedro Bolivar Puente
    //  https://github.com/arximboldi/lager/blob/master/lager/util.hpp
    ///////////////////////////////////////////////////////////////////////////

    template <class... Ts>
    struct visitor : Ts...
    {
        using Ts::operator()...;
    };

    template <class... Ts>
    visitor(Ts...)->visitor<Ts...>;

    ///////////////////////////////////////////////////////////////////////////
    // Defer calls to be run on scope exit
    ///////////////////////////////////////////////////////////////////////////
    template <typename... T>
    auto defer (T... fns) {
        return impl::Defer<T...>(std::make_tuple(fns...));
    }
    struct exit_scope_obj {
        template <typename Lambda>
        exit_scope_obj(Lambda& f) : func(f) {}
        template <typename Lambda>
        exit_scope_obj(Lambda&& f) : func(std::move(f)) {}
        ~exit_scope_obj() {func();}
    private:
        std::function<void()> func;
    };
    #define CONCAT_IDENTIFIERS_(a,b) a ## b
    #define ON_SCOPE_EXIT_(name,num) helpers::exit_scope_obj name ## num
    #define on_scope_exit ON_SCOPE_EXIT_(exit_scope_obj_, __LINE__)
    #define defer_calls auto CONCAT_IDENTIFIERS_(deferred_calls, __LINE__) = helpers::defer

    ///////////////////////////////////////////////////////////////////////////
    // Remove 'index' from container by swapping it with the last item and then shrinking the container by one
    ///////////////////////////////////////////////////////////////////////////
    template <typename ContainerType>
    void remove(ContainerType& container, std::size_t index)
    {
        auto it = container.begin() + index;
        auto last = container.end() - 1;
        if (it != last) {
            // If not the last item, move the last into this element
            *it = std::move(*last);
        }
        // Remove the last item in the container
        container.pop_back();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Remove items for which pred(item) is true and place into the removed container. Remove by swapping with last item and shrinking
    ///////////////////////////////////////////////////////////////////////////
    template <typename ContainerType, typename Predicate>
    void remove_into(ContainerType& container, ContainerType& removed, Predicate pred)
    {
        auto last = container.end() - 1;
        for (auto it = last; it >= container.begin(); --it) {
            if (pred(*it)) {
                // Move the item to remove into the 'removed' container
                removed.push_back(std::move(*it));
                // If not the last item, move the last into this element
                if (it != last) {
                    *it = std::move(*last);    
                }
                // Remove the last item in the container
                container.pop_back();
                --last;
            }
        }        
    }

    ///////////////////////////////////////////////////////////////////////////
    // Move item at 'index' to back of the container and move 'data' into index
    ///////////////////////////////////////////////////////////////////////////
    template <typename ContainerType>
    void move_back_and_replace(ContainerType& container, std::size_t index, typename ContainerType::reference&& data)
    {
        container.push_back(std::move(container[index]));
        container[index] = std::move(data);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Move items from 'in' container to 'out' container using push_back
    ///////////////////////////////////////////////////////////////////////////
    // If elements in 'in' have a deleted copy ctor, then _inserter may not work when compiling with EASTL, this is a workaround
    template <typename InputContainer, typename OutputContainer>
    void move_back (InputContainer& in, OutputContainer& out)
    {
        for (auto& item : in) {
            out.push_back(std::move(item));
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Pad a container with values until it contains the desired number of items
    ///////////////////////////////////////////////////////////////////////////
    template <typename ContainerType>
    void pad_with (ContainerType& container, std::size_t size, typename ContainerType::value_type value)
    {
        if (container.size() < size) {
            auto required = container.size() - size;
            for (auto i=required; i; --i) {
                container.push_back(value);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Binary search a container for an item, returning its index.
    // WARNING: Item MUST exist and container must be sorted
    ///////////////////////////////////////////////////////////////////////////
    template <typename ContainerType>
    std::size_t binary_search (ContainerType& container, typename ContainerType::value_type* item)
    {
        std::size_t front = 0;
        std::size_t back = container.size() - 1;
        std::size_t prev = front;
        do {
            std::size_t midpoint = front + ((back - front) >> 1);
            if (midpoint == prev) {
                ++midpoint;
            }
            typename ContainerType::value_type* midpoint_item = &container[midpoint];
            if (item > midpoint_item) {
                front = midpoint;
            } else if (item < midpoint_item) {
                back = midpoint;
            } else {
                return midpoint;
            }
            prev = midpoint;
        } while (true);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Read a file to a string, from PhysicsFS
    ///////////////////////////////////////////////////////////////////////////
    std::string readToString(const std::string& filename);

    ///////////////////////////////////////////////////////////////////////////
    // Unique pointer creation from constructor and destructor functions
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Ctor, typename Dtor>
    struct unique_maker_obj {
        explicit unique_maker_obj (Ctor ctor, Dtor dtor) : ctor(ctor), dtor(dtor) {}

        template <typename... Args>
        std::unique_ptr<T, Dtor> construct (Args... args) {
            return std::unique_ptr<T, Dtor>(ctor(args...), dtor);
        }
    private:
        Ctor ctor;
        Dtor dtor;
    };

    template <typename T, typename Ctor>
    unique_maker_obj<T, Ctor, void(*)(T*)> ptr (Ctor ctor) {
        return unique_maker_obj<T, Ctor, void(*)(T*)>(ctor, [](T* p){ delete p; });
    }
    template <typename T, typename Ctor, typename Dtor>
    unique_maker_obj<T, Ctor, void(*)(T*)> ptr (Ctor ctor, Dtor dtor) {
        return unique_maker_obj<T, Ctor, void(*)(T*)>(ctor, dtor);
    }
    template <typename T>
    std::unique_ptr<T> ptr (T* raw) {
        return std::unique_ptr<T>(raw);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Pointer alignment
    ///////////////////////////////////////////////////////////////////////////
    template <typename T = char>
    inline T* align(void* pointer, const uintptr_t bytes_alignment) {
        intptr_t value = reinterpret_cast<intptr_t>(pointer);
        value += (-value) & (bytes_alignment - 1);
        return reinterpret_cast<T*>(value);
    }

    inline intptr_t align(intptr_t pointer, const uintptr_t bytes_alignment) {
        return pointer + ((-pointer) & (bytes_alignment - 1));
    }

    ///////////////////////////////////////////////////////////////////////////
    // Number rounding
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    inline T roundDown(T n, T m) {
        return n >= 0 ? (n / m) * m : ((n - m + 1) / m) * m;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Reverse iterator
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct reversion_wrapper { T& iterable; };

    template <typename T>
    auto begin (reversion_wrapper<T> w) { return std::rbegin(w.iterable); }

    template <typename T>
    auto end (reversion_wrapper<T> w) { return std::rend(w.iterable); }

    template <typename T>
    reversion_wrapper<T> reverse (T&& iterable) { return { iterable }; }

    ///////////////////////////////////////////////////////////////////////////
    // In-place string search and replace
    ///////////////////////////////////////////////////////////////////////////
    void string_replace_inplace (std::string& input, const std::string& search, const std::string& replace);

    ///////////////////////////////////////////////////////////////////////////
    // Identity callable, for use with spp::sparse_hash_map when the key
    // is already hashed (eg when using entt::hashed_string::hash_value as keys)
    ///////////////////////////////////////////////////////////////////////////
    struct Identity {
        template <typename T> T operator()(T k) const { return k; }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Return (copy of) value from container or 'default_value' if not found
    ///////////////////////////////////////////////////////////////////////////
    template <typename Container>
    typename Container::mapped_type find_or (const Container& container, typename Container::key_type key, typename Container::mapped_type default_value) {
        auto it = container.find(key);
        if (it != container.end()) {
            return it->second;
        }
        return default_value;
    }

    ///////////////////////////////////////////////////////////////////////////
    // A runtime array whose values are inline in memory after the object
    // itself. Must be constructed into a buffer large enough to hold the array
    // count and all of its elements.
    ///////////////////////////////////////////////////////////////////////////

    // WARNING: Its up to the caller to ensure that this object exists in a buffer with enough size for all items
    template <typename T>
    class InlineArray {
    public:
        InlineArray () : m_size(0) {}
        InlineArray (const std::vector<T>& items) {
            fill(items);
        }
        InlineArray (std::vector<T>&& items) {
            fill(items);
        }

        void fill (const std::vector<T>& items) {
            m_size = items.size();
            T* next = m_items;
            for (const auto& item : items) {
                *next++ = item;
            }
        }

        void fill (std::vector<T>&& items) {
            m_size = items.size();
            T* next = m_items;
            for (const auto&& item : items) {
                *next++ = std::move(item);
            }
        }

        const std::uint32_t size () const {
            return m_size;
        }
        T& operator[] (std::size_t i) {
            assert(i < m_size);
            return m_items[i];
        }
    private:
        std::uint32_t m_size;
        T m_items[];
    };

    ///////////////////////////////////////////////////////////////////////////
    // Default allocator that delegates to new/delete
    ///////////////////////////////////////////////////////////////////////////

    struct DefaultAllocator {
        static std::byte* allocate (std::size_t length) {
            return new std::byte[length];
        }
        static void deallocate (std::byte* buffer) {
            delete [] buffer;
        }

        struct Object {
            std::byte* allocate (std::size_t length) {
                return DefaultAllocator::allocate(length);
            }
            void deallocate (std::byte* buffer) {
                DefaultAllocator::deallocate(buffer);
            }
        };
        static Object object () {
            return {};
        }
    };

} // helpers::
