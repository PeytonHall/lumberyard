/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef AZSTD_UNITTEST_USERTYPES_H
#define AZSTD_UNITTEST_USERTYPES_H

// enable checked iterators (in debug) to test. we can't really do this with AZCore since the lib is compiled without AZSTD_CHECKED_ITERATORS by default
// define AZSTD_CHECKED_ITERATORS in the lib and then test
//#define AZSTD_CHECKED_ITERATORS 1

#ifdef _DEBUG
#define AZSTD_DEBUG_HEAP_IMPLEMENTATION
#endif

#include "../TestTypes.h"
#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/typetraits.h>

//for the temporary "lock-free" allocator, remove when we have a real one
#include <AzCore/std/allocator.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/containers/vector.h>

#include <AzCore/Memory/SystemAllocator.h>

namespace UnitTestInternal
{
    /**
     * Examples test class with an aligned int data member.
     */
    class MyClass
    {
    public:
        MyClass(int data = 10)
            : m_data(data)
            , m_isMoved(false) {}
        MyClass(int data, bool, float)
            : m_data(data) {}
        MyClass(const MyClass& rhs)
            : m_data(rhs.m_data)
            , m_isMoved(false)  {}
#if defined(AZ_HAS_RVALUE_REFS)
        MyClass(MyClass&& rhs)
        {
            m_isMoved = true;
            m_data = rhs.m_data;
        }
#endif // AZ_HAS_RVALUE_REFS
        virtual ~MyClass() {}
        virtual void make_polymorphic() {}

        MyClass& operator=(const MyClass& rhs)
        {
            m_data = rhs.m_data;
            m_isMoved = rhs.m_isMoved;
            return *this;
        }

        // We use this class on the stack often, so alignment more than 16 bytes will not work on all platforms.
        AZ_ALIGN(int m_data, 16);
        bool m_isMoved;
    };

    AZ_FORCE_INLINE bool operator==(const MyClass& a, const MyClass& b) { return a.m_data == b.m_data; }
    AZ_FORCE_INLINE bool operator!=(const MyClass& a, const MyClass& b) { return a.m_data != b.m_data; }
    AZ_FORCE_INLINE bool operator<(const MyClass& a, const MyClass& b)  { return a.m_data < b.m_data; }
    AZ_FORCE_INLINE bool operator>(const MyClass& a, const MyClass& b)  { return a.m_data > b.m_data; }

    struct MyNoCopyClass
    {
        // Allowed construction mechanisms
        MyNoCopyClass() = default;
        MyNoCopyClass(int i, bool b, float f)
            : m_int(i)
            , m_bool(b)
            , m_float(f) { }
        MyNoCopyClass(MyNoCopyClass&& rhs)
            : m_int(rhs.m_int)
            , m_bool(rhs.m_bool)
            , m_float(rhs.m_float)
        {
            rhs = { };
        }
        MyNoCopyClass& operator=(MyNoCopyClass&& rhs)
        {
            m_int = rhs.m_int;
            m_bool = rhs.m_bool;
            m_float = rhs.m_float;

            rhs.m_int = 0;
            rhs.m_bool = false;
            rhs.m_float = 0.0f;

            return *this;
        }

        // Disallowed construction mechanisms
        MyNoCopyClass(const MyNoCopyClass&) = delete;
        MyNoCopyClass& operator=(const MyNoCopyClass&) = delete;

        int m_int = 0;
        bool m_bool = false;
        float m_float = 0.0f;
    };

    /**
     * Example Interface class.
     */
    class MyInterface
    {
    public:
        virtual ~MyInterface() {}
        virtual int Add() = 0;
        virtual void Remove(int i) = 0;
    };

    /**
     * Example if POD struct.
     */
    struct MyStruct
    {
        int foo()   { return 0; }

        int m_value;
    };

    struct MyEmptyStruct
    {};

    enum MyEnum
    {
        zero = 0,
        one,
        two
    };

    union MyUnion
    {
        int     m_int;
        long    m_long;
    };

    class MyLockFreeAllocator
        : public AZStd::allocator
    {
        struct DelayedFreeItem
        {
            pointer_type m_ptr;
            size_type m_byteSize;
            size_type m_alignment;
        };
    public:
        bool is_lock_free()             { return true; } //ahem
        bool is_stale_read_allowed()    { return true; }
        bool is_delayed_recycling()     { return true; }

        ~MyLockFreeAllocator()
        {
            do_delayed_frees();
        }

        void deallocate(pointer_type ptr, size_type byteSize, size_type alignment)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            DelayedFreeItem item;
            item.m_ptr = ptr;
            item.m_byteSize = byteSize;
            item.m_alignment = alignment;
            m_delayedFreeItems.push_back(item);
        }

        void do_delayed_frees()
        {
            for (unsigned int i = 0; i < m_delayedFreeItems.size(); ++i)
            {
                DelayedFreeItem& item = m_delayedFreeItems[i];
                AZStd::allocator::deallocate(item.m_ptr, item.m_byteSize, item.m_alignment);
            }
            m_delayedFreeItems.clear();
        }

    private:
        AZStd::mutex m_mutex;
        AZStd::vector<DelayedFreeItem> m_delayedFreeItems;
    };
}

// Without compiler help we should help a little. If the compiler support TR1 this will not be necessary.
AZSTD_DECLARE_POD_TYPE(UnitTestInternal::MyStruct)
AZSTD_DECLARE_UNION(UnitTestInternal::MyUnion)

#endif // AZSTD_UNITTEST_USERTYPES_H
#pragma once