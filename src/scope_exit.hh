#ifndef SCOPE_EXIT_HH
#define SCOPE_EXIT_HH

// the followins SCOPE_EXIT magic comes from
// http://the-witness.net/news/2012/11/scopeexit-in-c11/
template <typename F>
struct ScopeExit {
    ScopeExit(F code) : f(code) {}
    ~ScopeExit() { f(); }

    ScopeExit(const ScopeExit& other) = default;

    F f;
};

template <typename F>
inline ScopeExit<F> MakeScopeExit(F f) {
    return ScopeExit<F>(f);
};

#define DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2
#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
#define SCOPE_EXIT(code) \
     auto STRING_JOIN2(scope_exit_, __LINE__) = MakeScopeExit([=](){code;})

#define SCOPE_EXIT_BY_REF(code) \
     auto STRING_JOIN2(scope_exit_, __LINE__) = MakeScopeExit([&](){code;})


#endif
