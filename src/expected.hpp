/*
 * At the moment CodeCrafters offers only a C++20 "package".
 * Until they offer C++23, I'll need to include my own std::expected.
 * This is a simplified version of the one included in GNU C++'s library.
 */

#ifndef __MY_EXPECTED
#define __MY_EXPECTED

#include <version>

#ifdef __cpp_lib_expected
#include <expected>
namespace std {
    template<typename _Er>
    using unexpected_23 = std::unexpected<_Er>;
}
#else

#include <cassert>
#include <memory>
#include <type_traits>

namespace _local {
    // Taken from GNU stdlibc++ 12's <type_traits>
    using true_type = std::integral_constant<bool, true>;
    using false_type = std::integral_constant<bool, false>;

    template<bool _Cond, typename _If, typename _Else>
        using __conditional_t = typename std::conditional<_Cond, _If, _Else>;

    template<typename...>
        struct __or_;

    template<>
        struct __or_<>
        : public false_type
        { };

    template<typename _B1>
        struct __or_<_B1>    
        : public _B1
        { };

    template<typename _B1, typename _B2>
        struct __or_<_B1, _B2>
        : public __conditional_t<_B1::value, _B1, _B2>
        { };

    template<typename _B1, typename _B2, typename _B3, typename... _Bn>
        struct __or_<_B1, _B2, _B3, _Bn...>
        : public __conditional_t<_B1::value, _B1, __or_<_B2, _B3, _Bn...>>
        { };

    template<typename...>
        struct __and_;

    template<>
        struct __and_<>
        : public true_type
        { };

    template<typename _B1>
        struct __and_<_B1>   
        : public _B1
        { };

    template<typename _B1, typename _B2>
        struct __and_<_B1, _B2>
        : public __conditional_t<_B1::value, _B2, _B1>
        { };

    template<typename _B1, typename _B2, typename _B3, typename... _Bn>
        struct __and_<_B1, _B2, _B3, _Bn...>
        : public __conditional_t<_B1::value, __and_<_B2, _B3, _Bn...>, _B1>
        { };

    template<typename _Pp>
        struct __not_
        : public std::bool_constant<!bool(_Pp::value)>
        { };

    template<typename... _Bn>
        inline constexpr bool __or_v = __or_<_Bn...>::value;

    template<typename... _Bn>
        inline constexpr bool __and_v = __and_<_Bn...>::value;
}

namespace std {
    template<typename _Tp, typename _Er>
        class expected;

    template<typename _Er>
        class unexpected_23;

namespace __expected {
    template<typename _Tp>
        constexpr bool __is_expected = false;
    template<typename _Tp, typename _Er>
        constexpr bool __is_expected<expected<_Tp, _Er>> = true;

    template<typename _Tp>
        constexpr bool __is_unexpected = false;
    template<typename _Tp>
        constexpr bool __is_unexpected<unexpected_23<_Tp>> = true;

    template<typename _Er>
        concept __can_be_unexpected
          = is_object_v<_Er> && (!is_array_v<_Er>)
              && (!__expected::__is_unexpected<_Er>)
              && (!is_const_v<_Er>) && (!is_volatile_v<_Er>);
}

    template<typename _Er>
        class unexpected_23 {
            static_assert( __expected::__can_be_unexpected<_Er> );
        public:
            constexpr unexpected_23(const unexpected_23&) = default;
            constexpr unexpected_23(unexpected_23&&) = default;

            template<typename _Err = _Er>
                requires (!is_same_v<remove_cvref_t<_Err>, unexpected_23>)
                    && (!is_same_v<remove_cvref_t<_Err>, in_place_t>)
                    && is_constructible_v<_Er, _Err>
                constexpr explicit
                unexpected_23(_Err&& __e)
                noexcept(is_nothrow_constructible_v<_Er, _Err>)
                : _M_unexpected(std::forward<_Err>(__e))
                { }

            template<typename... _Args>
                requires is_constructible_v<_Er, _Args...>
                constexpr explicit
                unexpected_23(in_place_t, _Args&&... __args)
                noexcept(is_nothrow_constructible_v<_Er, _Args...>)
                : _M_unexpected(std::forward<_Args>(__args)...)
                { }

            template<typename _Up, typename... _Args>
                requires is_constructible_v<_Er, initializer_list<_Up>>
                constexpr explicit
                unexpected_23(in_place_t, initializer_list<_Up> __il, _Args&&... __args)
                : _M_unexpected(__il, std::forward<_Args>(__args)...)
                { }

            constexpr unexpected_23& operator=(const unexpected_23&) = default;
            constexpr unexpected_23& operator=(unexpected_23&&) = default;

            [[nodiscard]]
            constexpr const _Er&
            error() const & noexcept { return _M_unexpected; }
        private:
            _Er _M_unexpected;
        };

    template<typename _Er> unexpected_23(_Er) -> unexpected_23<_Er>;

    template<typename _Tp, typename _Er>
        class expected {
        public:
            using value_type = _Tp;
            using error_type = _Er;
            using unexpected_type = unexpected_23<_Er>;

            template<typename _Up, typename _Err, typename _Unex = unexpected_23<_Er>>
                static constexpr bool __cons_from_expected
                = _local::__or_v<
                is_constructible<_Tp, expected<_Up, _Err>&>,
                is_constructible<_Tp, expected<_Up, _Err>>,
                is_constructible<_Tp, const expected<_Up, _Err>&>,
                is_constructible<_Tp, const expected<_Up, _Err>>,
                is_convertible<expected<_Up, _Err>&, _Tp>,
                is_convertible<expected<_Up, _Err>, _Tp>,
                is_convertible<const expected<_Up, _Err>&, _Tp>,
                is_convertible<const expected<_Up, _Err>, _Tp>,
                is_constructible<_Unex, expected<_Up, _Err>&>,
                is_constructible<_Unex, expected<_Up, _Err>>,
                is_constructible<_Unex, const expected<_Up, _Err>&>,
                is_constructible<_Unex, const expected<_Up, _Err>>
                    >;

            template<typename _Up, typename _Err>
                constexpr static bool __explicit_conv
                = _local::__or_v<_local::__not_<is_convertible<_Up, _Tp>>,
                _local::__not_<is_convertible<_Err, _Er>>
                    >;

            constexpr
            expected()
            noexcept(is_nothrow_constructible_v<_Tp>)
            requires is_default_constructible_v<_Tp>
            : _M_value(), _M_has_value(true)
            { }

            expected(const expected&) = default;

            constexpr
            expected(const expected& __x)
            noexcept(_local::__and_v<is_nothrow_copy_constructible<_Tp>,
                                     is_nothrow_copy_constructible<_Er>>)
            requires is_copy_constructible_v<_Tp> && is_copy_constructible_v<_Er>
            && (!is_trivially_copy_constructible_v<_Tp>
                || !is_trivially_copy_constructible_v<_Er>)
            : _M_has_value(__x._M_has_value)
            {
                if (_M_has_value)
                    std::construct_at(std::addressof(_M_value), __x._M_value);
                else
                    std::construct_at(std::addressof(_M_unexpected), __x._M_unexpected);
            }

            expected(expected&&) = default;

            constexpr
            expected(expected&& __x)
            noexcept(is_nothrow_move_constructible_v<_Tp> && is_nothrow_move_constructible_v<_Er>)
            requires is_move_constructible_v<_Tp> && is_move_constructible_v<_Er>
            && (!is_trivially_move_constructible_v<_Tp>
                || !is_trivially_move_constructible_v<_Er>)
            : _M_has_value(__x._M_has_value)
            {
                if (_M_has_value)
                    std::construct_at(std::addressof(_M_value), std::move(__x)._M_value);
                else
                    std::construct_at(std::addressof(_M_unexpected), std::move(__x)._M_unexpected);
            }

            template<typename _Up, typename _Gr>
                requires is_constructible_v<_Tp, const _Up&>
                      && is_constructible_v<_Er, const _Gr&>
                      && (!__cons_from_expected<_Up, _Gr>)
                constexpr explicit(__explicit_conv<const _Up&, const _Gr&>)
                expected(const expected<_Up, _Gr>& __x)
                noexcept(_local::__and_v<is_nothrow_constructible<_Tp, const _Up&>,
                                         is_nothrow_constructible<_Er, const _Gr&>>)
                : _M_has_value(__x._M_has_value)
                {
                    if (_M_has_value)
                        std::construct_at(__builtin_addressof(_M_value), __x._M_value);
                    else
                        std::construct_at(__builtin_addressof(_M_unexpected), __x._M_unexpected);
                }

            template<typename _Up, typename _Gr>
                requires is_constructible_v<_Tp, _Up>
                      && is_constructible_v<_Er, _Gr>
                      && (!__cons_from_expected<_Up, _Gr>)
                constexpr explicit(__explicit_conv<_Up, _Gr>)
                expected(expected<_Up, _Gr>&& __x)
                noexcept(_local::__and_v<is_nothrow_constructible<_Tp, _Up>,
                                         is_nothrow_constructible<_Er, _Gr>>)
                : _M_has_value(__x._M_has_value)
                {
                    if (_M_has_value)
                        std::construct_at(__builtin_addressof(_M_value),
                                std::move(__x)._M_value);
                    else
                        std::construct_at(__builtin_addressof(_M_unexpected),
                                std::move(__x)._M_unexpected);
                }

            template<typename _Up = _Tp>
                requires (!is_same_v<remove_cvref_t<_Up>, expected>)
                    && (!is_same_v<remove_cvref_t<_Up>, in_place_t>)
                    && (!__expected::__is_unexpected<remove_cvref_t<_Up>>)
                    && is_constructible_v<_Tp, _Up>
                constexpr explicit(!is_convertible_v<_Up, _Tp>)
                expected(_Up&& __v)
                noexcept(is_nothrow_constructible_v<_Tp, _Up>)
                : _M_value(std::forward<_Up>(__v)), _M_has_value(true)
                { }

            template<typename _Gr = _Er>
                requires is_constructible_v<_Er, const _Gr&>
                constexpr explicit(!is_convertible_v<const _Gr&, _Er>)
                expected(const unexpected_23<_Gr>& __u)
                noexcept(is_nothrow_constructible_v<_Er, const _Gr&>)
                : _M_unexpected(__u.error()), _M_has_value(false)
                { }

            template<typename _Gr = _Er>
                requires is_constructible_v<_Er, _Gr>
                constexpr explicit(!is_convertible_v<_Gr, _Er>)
                expected(unexpected_23<_Gr>&& __u)
                noexcept(is_nothrow_constructible_v<_Er, _Gr>)
                : _M_unexpected(std::move(__u).error()), _M_has_value(false)
                { }

            // Observers
            [[nodiscard]]
            constexpr const _Tp&
            operator*() const & noexcept
            {
                assert(_M_has_value);
                return _M_value;
            }

            constexpr const _Er&
            error() const & noexcept
            {
                assert(!_M_has_value);
                return _M_unexpected;
            }

            constexpr const _Er&&
            error() const && noexcept
            {
                assert(!_M_has_value);
                return std::move(_M_unexpected);
            }

            [[nodiscard]]
            constexpr explicit
            operator bool() const noexcept { return _M_has_value; }

            [[nodiscard]]
            constexpr bool has_value() const noexcept { return _M_has_value; }

        private:
            union {
                _Tp _M_value;
                _Er _M_unexpected;
            };

            bool _M_has_value;
        };
}

#endif // __cpp_lib_expected


#endif // __MY_EXPECTED
