#pragma once

// Macro to declare an event struct that has an ID static constexpr field
#define DECLARE_EVENT(obj, name) struct Event_ID__ ## obj { static constexpr entt::hashed_string ID = entt::hashed_string{name}; }; struct __attribute__((packed)) obj : public Event_ID__ ## obj

// Macro to generate constexpr function hasMember_`Function`<T>() which returns true if T::`Function``Args` exists, otherwise false
#define HAS_MEMBER_FUNCTION(Function, Args)                                         \
namespace supports_detail {                                                         \
  template<typename U>                                                              \
  struct hasMember_ ## Function {                                                   \
  private:                                                                          \
    template<typename>                                                              \
    static constexpr std::false_type test(...);                                     \
    template<typename T = U>                                                        \
    static decltype((std::declval<T>().Function Args), std::true_type{}) test(int); \
  public:                                                                           \
    static constexpr bool value = decltype(test<U>(0))::value;                      \
  }; } template<typename T> constexpr bool hasMember_ ## Function ()                \
  {return supports_detail::hasMember_ ## Function<T>::value;}

#define HAS_MEMBER_FUNCTION_N(Function, Suffix, Args)                               \
namespace supports_detail {                                                         \
  template<typename U>                                                              \
  struct hasMember_ ## Function ## Suffix {                                         \
  private:                                                                          \
    template<typename>                                                              \
    static constexpr std::false_type test(...);                                     \
    template<typename T = U>                                                        \
    static decltype((std::declval<T>().Function Args), std::true_type{}) test(int); \
  public:                                                                           \
    static constexpr bool value = decltype(test<U>(0))::value;                      \
  }; } template<typename T> constexpr bool hasMember_ ## Function ## Suffix ()      \
  {return supports_detail::hasMember_ ## Function ## Suffix<T>::value;}


///////////////////////////////////////////////////////////////////////////////
// Macro for declaring module class
///////////////////////////////////////////////////////////////////////////////

// Add module class boilerplate
#define MM_MODULE_NAME(ClassName) ClassName : public mm_module::Module<ClassName>
#define MM_MODULE_CLASS(ClassName) public: ClassName(const std::string& name) : mm_module::Module<ClassName>(name) {} virtual ~ClassName() {} private:

// Declare module
#define MM_MODULE(ClassName) MM_MODULE_INIT(mm_module::) { MM_REGISTER_COMPONENTS return mm->createModule<ClassName>(name); }