// MIT License
//
// Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef HIPCUB_TEST_TEST_UTILS_SORT_COMPARATOR_HPP_
#define HIPCUB_TEST_TEST_UTILS_SORT_COMPARATOR_HPP_

#ifdef __HIP_PLATFORM_AMD__
#include <rocprim/type_traits.hpp>
#endif

namespace test_utils
{
// Original C++17 logic
//
// template<class Key, bool Descending, unsigned int StartBit, unsigned int EndBit>
// struct key_comparator
//{
//    bool operator()(const Key& lhs, const Key& rhs)
//    {
//        if constexpr (rocprim::is_unsigned<Key>::value)
//        {
//            if constexpr (StartBit == 0 && (EndBit == sizeof(Key) * 8))
//            {
//                return Descending ? (rhs < lhs) : (lhs < rhs);
//            }
//            else
//            {
//                auto mask = (1ull << (EndBit - StartBit)) - 1;
//                auto l = (static_cast<unsigned long long>(lhs) >> StartBit) & mask;
//                auto r = (static_cast<unsigned long long>(rhs) >> StartBit) & mask;
//                return Descending ? (r < l) : (l < r);
//            }
//        }
//        else
//        {
//            if constexpr (std::is_same_v<Key, rocprim::half>)
//            {
//                float l = static_cast<float>(lhs);
//                float r = static_cast<float>(rhs);
//                return Descending ? (r < l) : (l < r);
//            }
//            else
//            {
//                return Descending ? (rhs < lhs) : (lhs < rhs);
//            }
//        }
//    }
//};

// Faulty C++14 backported logic (consider fixing)
//
// template<class Key, bool Descending>
// bool generic_key_compare(const Key& lhs, const Key& rhs) { return Descending ? (rhs < lhs) : (lhs < rhs); }
//
// template<class Key, bool Descending, unsigned int StartBit, unsigned int EndBit>
// auto discriminate_bits(const Key& lhs, const Key& rhs) -> typename std::enable_if<StartBit == 0 && EndBit == sizeof(Key) * 8, bool>::type
//{
//    // TODO: pick adequately sized integral type (instead of 1ull) based on Key.
//    //       Needed to safely silence "'argument': conversion from 'unsigned __int64' to 'const Key', possible loss of data" auto mask = (1ull << (EndBit - StartBit)) - 1; auto l = (static_cast<unsigned long long>(lhs) >> StartBit) & mask; auto r = (static_cast<unsigned long long>(rhs) >> StartBit) & mask; return generic_key_compare<Key, Descending>(l, r);
//}
//
// template<class Key, bool Descending, unsigned int StartBit, unsigned int EndBit>
// auto discriminate_bits(const Key& lhs, const Key& rhs) -> typename std::enable_if<!(StartBit == 0 && EndBit == sizeof(Key) * 8), bool>::type
//{
//    return generic_key_compare<Key, Descending>(lhs, rhs);
//}
//
// template<class Key, bool Descending>
// auto discriminate_half(const Key& lhs, const Key& rhs) -> typename std::enable_if<std::is_same<Key, rocprim::half>::value, bool>::type
//{
//    // HIP's half doesn't have __host__ comparison operators, use floats instead
//    return generic_key_compare<float, Descending>((float)lhs, (float)rhs);
//}
//
// template<class Key, bool Descending>
// auto discriminate_half(const Key& lhs, const Key& rhs) -> typename std::enable_if<!std::is_same<Key, rocprim::half>::value, bool>::type
//{
//    return generic_key_compare<Key, Descending>(lhs, rhs);
//}
//
// template<class Key, bool Descending, unsigned int StartBit, unsigned int EndBit>
// auto discriminate_unsigned(const Key& lhs, const Key& rhs) -> typename std::enable_if<rocprim::is_unsigned<Key>::value, bool>::type
//{
//    return discriminate_bits<Key, Descending, StartBit, EndBit>(lhs, rhs);
//}
//
// template<class Key, bool Descending, unsigned int StartBit, unsigned int EndBit>
// auto discriminate_unsigned(const Key& lhs, const Key& rhs) -> typename std::enable_if<!rocprim::is_unsigned<Key>::value, bool>::type
//{
//    return discriminate_half<Key, Descending>(lhs, rhs);
//}
//
// template<class Key, bool Descending, unsigned int StartBit, unsigned int EndBit>
// struct key_comparator
//{
//    bool operator()(const Key& lhs, const Key& rhs)
//    {
//        return discriminate_unsigned<Key, Descending, StartBit, EndBit>(lhs, rhs);
//    }
//};
//
// template<class Key, class Value, bool Descending, unsigned int StartBit, unsigned int EndBit>
// struct key_value_comparator
//{
//    bool operator()(const std::pair<Key, Value>& lhs, const std::pair<Key, Value>& rhs)
//    {
//        return key_comparator<Key, Descending, StartBit, EndBit>()(lhs.first, rhs.first);
//    }
//};

// Original code with ISO-conforming overload control
//
// NOTE: ShiftLess helper is needed, because partial specializations cannot refer to the free template args.
//       See: https://stackoverflow.com/questions/2615905/c-template-nontype-parameter-arithmetic

template<class Key,
         bool         Descending,
         unsigned int StartBit,
         unsigned int EndBit,
         bool         ShiftLess = (StartBit == 0 && EndBit == sizeof(Key) * 8)>
struct key_comparator
{
#ifdef __HIP_PLATFORM_AMD__
    static_assert(rocprim::is_unsigned<Key>::value,
                  "Test supports start and end bits only for unsigned integers");
#endif

    bool operator()(const Key & lhs, const Key & rhs)
    {
        auto mask = (1ull << (EndBit - StartBit)) - 1;
        auto l    = (static_cast<unsigned long long>(lhs) >> StartBit) & mask;
        auto r    = (static_cast<unsigned long long>(rhs) >> StartBit) & mask;
        return Descending ? (r < l) : (l < r);
    }
};

template<class Key, bool Descending, unsigned int StartBit, unsigned int EndBit>
struct key_comparator<Key, Descending, StartBit, EndBit, true>
{
    bool operator()(const Key & lhs, const Key & rhs)
    {
        return Descending ? (rhs < lhs) : (lhs < rhs);
    }
};

template<class Key, class Value, bool Descending, unsigned int StartBit, unsigned int EndBit>
struct key_value_comparator
{
    bool operator()(const std::pair<Key, Value> & lhs, const std::pair<Key, Value> & rhs)
    {
        return key_comparator<Key, Descending, StartBit, EndBit>()(lhs.first, rhs.first);
    }
};

struct less
{
    template<typename T>
    HIPCUB_HOST_DEVICE inline constexpr bool operator()(const T & a, const T & b) const
    {
        return a < b;
    }
};

struct less_equal
{
    template<typename T>
    HIPCUB_HOST_DEVICE inline constexpr bool operator()(const T & a, const T & b) const
    {
        return a <= b;
    }
};

struct greater
{
    template<typename T>
    HIPCUB_HOST_DEVICE inline constexpr bool operator()(const T & a, const T & b) const
    {
        return a > b;
    }
};

struct greater_equal
{
    template<typename T>
    HIPCUB_HOST_DEVICE inline constexpr bool operator()(const T & a, const T & b) const
    {
        return a >= b;
    }
};

}
#endif // TEST_UTILS_SORT_COMPARATOR_HPP_
