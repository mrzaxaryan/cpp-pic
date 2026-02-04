/**
 * pool.h - Generic Pool Templates for PIL (Position Independent Language)
 *
 * Provides reusable pool templates to eliminate code duplication.
 * Position-independent, no .rdata dependencies, no dynamic allocation.
 *
 * Part of RAL (Runtime Abstraction Layer).
 *
 * Two pool types:
 * - Pool<T, N>       - For simple/trivial types (direct array storage)
 * - ObjectPool<T, N> - For complex types requiring placement new
 */

#pragma once

#include "primitives.h"

// ============================================================================
// PLACEMENT NEW OPERATORS (if not already defined)
// ============================================================================

#ifndef PIL_PLACEMENT_NEW_DEFINED
#define PIL_PLACEMENT_NEW_DEFINED
inline void* operator new(USIZE, void* ptr) noexcept { return ptr; }
inline void operator delete(void*, void*) noexcept { }
#endif

namespace PIL
{

// ============================================================================
// SIMPLE POOL TEMPLATE
// ============================================================================

/**
 * Pool - Fixed-size pool for simple/trivial types.
 *
 * Use this for types that:
 * - Have trivial default constructor
 * - Have trivial destructor (or destructor does nothing special)
 * - Can be copied/assigned directly
 *
 * @tparam T       Type of items in the pool
 * @tparam MaxSize Maximum number of items
 */
template<typename T, USIZE MaxSize>
class Pool
{
private:
    T m_items[MaxSize];
    BOOL m_inUse[MaxSize];

public:
    Pool() noexcept
    {
        for (USIZE i = 0; i < MaxSize; i++)
        {
            m_inUse[i] = FALSE;
        }
    }

    ~Pool() noexcept
    {
        // Don't call CloseAll() automatically - let derived pools handle this
        // to avoid double-close if user explicitly closes items
    }

    /**
     * Allocate a handle from the pool.
     * @return Handle (0 to MaxSize-1) or -1 if pool exhausted
     */
    NOINLINE INT32 Alloc() noexcept
    {
        for (USIZE i = 0; i < MaxSize; i++)
        {
            if (!m_inUse[i])
            {
                m_inUse[i] = TRUE;
                return (INT32)i;
            }
        }
        return -1;
    }

    /**
     * Get item by handle.
     * @param handle Valid handle from Alloc()
     * @return Pointer to item or nullptr if invalid
     */
    FORCE_INLINE T* Get(INT32 handle) noexcept
    {
        if (handle < 0 || (USIZE)handle >= MaxSize || !m_inUse[handle])
        {
            return nullptr;
        }
        return &m_items[handle];
    }

    FORCE_INLINE const T* Get(INT32 handle) const noexcept
    {
        if (handle < 0 || (USIZE)handle >= MaxSize || !m_inUse[handle])
        {
            return nullptr;
        }
        return &m_items[handle];
    }

    /**
     * Set item at handle (move assignment).
     * @param handle Valid handle from Alloc()
     * @param item   Item to move into pool
     * @return TRUE on success, FALSE if invalid handle
     */
    NOINLINE BOOL Set(INT32 handle, T&& item) noexcept
    {
        if (handle < 0 || (USIZE)handle >= MaxSize || !m_inUse[handle])
        {
            return FALSE;
        }
        m_items[handle] = static_cast<T&&>(item);
        return TRUE;
    }

    /**
     * Free a handle (marks as not in use).
     * Does NOT call Close() - use FreeWithClose() for that.
     * @param handle Handle to free
     */
    FORCE_INLINE void Free(INT32 handle) noexcept
    {
        if (handle >= 0 && (USIZE)handle < MaxSize)
        {
            m_inUse[handle] = FALSE;
        }
    }

    /**
     * Check if handle is valid.
     * @param handle Handle to check
     * @return TRUE if valid and in use
     */
    FORCE_INLINE BOOL IsValid(INT32 handle) const noexcept
    {
        return handle >= 0 && (USIZE)handle < MaxSize && m_inUse[handle];
    }

    /**
     * Reset pool (marks all as not in use).
     * Does NOT call destructors or Close().
     */
    FORCE_INLINE void Reset() noexcept
    {
        for (USIZE i = 0; i < MaxSize; i++)
        {
            m_inUse[i] = FALSE;
        }
    }

    /**
     * Get number of items currently in use.
     */
    NOINLINE USIZE GetCount() const noexcept
    {
        USIZE count = 0;
        for (USIZE i = 0; i < MaxSize; i++)
        {
            if (m_inUse[i]) count++;
        }
        return count;
    }

    /**
     * Get maximum capacity.
     */
    static constexpr USIZE GetCapacity() noexcept
    {
        return MaxSize;
    }
};

// ============================================================================
// CLOSEABLE POOL TEMPLATE (calls T::Close() on Free)
// ============================================================================

/**
 * CloseablePool - Pool that calls T::Close() when freeing items.
 *
 * Use this for types like File, Socket that have a Close() method.
 *
 * @tparam T       Type of items in the pool (must have Close() method)
 * @tparam MaxSize Maximum number of items
 */
template<typename T, USIZE MaxSize>
class CloseablePool
{
private:
    T m_items[MaxSize];
    BOOL m_inUse[MaxSize];

public:
    CloseablePool() noexcept
    {
        for (USIZE i = 0; i < MaxSize; i++)
        {
            m_inUse[i] = FALSE;
        }
    }

    ~CloseablePool() noexcept
    {
        CloseAll();
    }

    /**
     * Allocate a handle from the pool.
     * @return Handle (0 to MaxSize-1) or -1 if pool exhausted
     */
    NOINLINE INT32 Alloc() noexcept
    {
        for (USIZE i = 0; i < MaxSize; i++)
        {
            if (!m_inUse[i])
            {
                m_inUse[i] = TRUE;
                return (INT32)i;
            }
        }
        return -1;
    }

    /**
     * Get item by handle.
     * @param handle Valid handle from Alloc()
     * @return Pointer to item or nullptr if invalid
     */
    FORCE_INLINE T* Get(INT32 handle) noexcept
    {
        if (handle < 0 || (USIZE)handle >= MaxSize || !m_inUse[handle])
        {
            return nullptr;
        }
        return &m_items[handle];
    }

    FORCE_INLINE const T* Get(INT32 handle) const noexcept
    {
        if (handle < 0 || (USIZE)handle >= MaxSize || !m_inUse[handle])
        {
            return nullptr;
        }
        return &m_items[handle];
    }

    /**
     * Set item at handle (move assignment).
     * @param handle Valid handle from Alloc()
     * @param item   Item to move into pool
     * @return TRUE on success, FALSE if invalid handle
     */
    NOINLINE BOOL Set(INT32 handle, T&& item) noexcept
    {
        if (handle < 0 || (USIZE)handle >= MaxSize || !m_inUse[handle])
        {
            return FALSE;
        }
        m_items[handle] = static_cast<T&&>(item);
        return TRUE;
    }

    /**
     * Free a handle (calls Close() and marks as not in use).
     * @param handle Handle to free
     */
    NOINLINE void Free(INT32 handle) noexcept
    {
        if (handle >= 0 && (USIZE)handle < MaxSize && m_inUse[handle])
        {
            m_items[handle].Close();
            m_inUse[handle] = FALSE;
        }
    }

    /**
     * Close all items in use.
     */
    NOINLINE void CloseAll() noexcept
    {
        for (USIZE i = 0; i < MaxSize; i++)
        {
            if (m_inUse[i])
            {
                m_items[i].Close();
                m_inUse[i] = FALSE;
            }
        }
    }

    /**
     * Check if handle is valid.
     * @param handle Handle to check
     * @return TRUE if valid and in use
     */
    FORCE_INLINE BOOL IsValid(INT32 handle) const noexcept
    {
        return handle >= 0 && (USIZE)handle < MaxSize && m_inUse[handle];
    }

    /**
     * Get number of items currently in use.
     */
    NOINLINE USIZE GetCount() const noexcept
    {
        USIZE count = 0;
        for (USIZE i = 0; i < MaxSize; i++)
        {
            if (m_inUse[i]) count++;
        }
        return count;
    }

    /**
     * Get maximum capacity.
     */
    static constexpr USIZE GetCapacity() noexcept
    {
        return MaxSize;
    }
};

// ============================================================================
// OBJECT POOL TEMPLATE (for types requiring placement new)
// ============================================================================

/**
 * ObjectPool - Fixed-size pool for complex types using placement new.
 *
 * Use this for types that:
 * - Have non-trivial constructor taking arguments
 * - Need explicit destructor calls
 * - Have non-default copy/move operations
 *
 * @tparam T       Type of items in the pool
 * @tparam MaxSize Maximum number of items
 */
template<typename T, USIZE MaxSize>
class ObjectPool
{
private:
    T* m_ptrs[MaxSize];
    BOOL m_inUse[MaxSize];
    alignas(T) CHAR m_storage[MaxSize][sizeof(T)];

public:
    ObjectPool() noexcept
    {
        for (USIZE i = 0; i < MaxSize; i++)
        {
            m_inUse[i] = FALSE;
            m_ptrs[i] = nullptr;
        }
    }

    ~ObjectPool() noexcept
    {
        CloseAll();
    }

    /**
     * Allocate a handle from the pool.
     * Note: Does NOT construct the object - call Init() after.
     * @return Handle (0 to MaxSize-1) or -1 if pool exhausted
     */
    NOINLINE INT32 Alloc() noexcept
    {
        for (USIZE i = 0; i < MaxSize; i++)
        {
            if (!m_inUse[i])
            {
                m_inUse[i] = TRUE;
                m_ptrs[i] = nullptr;  // Not constructed yet
                return (INT32)i;
            }
        }
        return -1;
    }

    /**
     * Initialize object at handle with single argument.
     * @param handle Valid handle from Alloc()
     * @param arg    Constructor argument
     * @return TRUE on success, FALSE if invalid handle
     */
    template<typename Arg>
    NOINLINE BOOL Init(INT32 handle, Arg arg) noexcept
    {
        if (handle < 0 || (USIZE)handle >= MaxSize || !m_inUse[handle])
        {
            return FALSE;
        }
        // Destroy existing object if any
        if (m_ptrs[handle])
        {
            m_ptrs[handle]->~T();
        }
        // Placement new to construct
        m_ptrs[handle] = ::new (m_storage[handle]) T(arg);
        return TRUE;
    }

    /**
     * Initialize object at handle with two arguments.
     */
    template<typename Arg1, typename Arg2>
    NOINLINE BOOL Init(INT32 handle, Arg1 arg1, Arg2 arg2) noexcept
    {
        if (handle < 0 || (USIZE)handle >= MaxSize || !m_inUse[handle])
        {
            return FALSE;
        }
        if (m_ptrs[handle])
        {
            m_ptrs[handle]->~T();
        }
        m_ptrs[handle] = ::new (m_storage[handle]) T(arg1, arg2);
        return TRUE;
    }

    /**
     * Get item by handle.
     * @param handle Valid handle from Alloc()
     * @return Pointer to item or nullptr if invalid/not initialized
     */
    FORCE_INLINE T* Get(INT32 handle) noexcept
    {
        if (handle < 0 || (USIZE)handle >= MaxSize || !m_inUse[handle])
        {
            return nullptr;
        }
        return m_ptrs[handle];
    }

    FORCE_INLINE const T* Get(INT32 handle) const noexcept
    {
        if (handle < 0 || (USIZE)handle >= MaxSize || !m_inUse[handle])
        {
            return nullptr;
        }
        return m_ptrs[handle];
    }

    /**
     * Free a handle (calls destructor and Close() if initialized).
     * @param handle Handle to free
     */
    NOINLINE void Free(INT32 handle) noexcept
    {
        if (handle >= 0 && (USIZE)handle < MaxSize && m_inUse[handle])
        {
            if (m_ptrs[handle])
            {
                m_ptrs[handle]->Close();
                m_ptrs[handle]->~T();
                m_ptrs[handle] = nullptr;
            }
            m_inUse[handle] = FALSE;
        }
    }

    /**
     * Close all items in use.
     */
    NOINLINE void CloseAll() noexcept
    {
        for (USIZE i = 0; i < MaxSize; i++)
        {
            if (m_inUse[i])
            {
                if (m_ptrs[i])
                {
                    m_ptrs[i]->Close();
                    m_ptrs[i]->~T();
                    m_ptrs[i] = nullptr;
                }
                m_inUse[i] = FALSE;
            }
        }
    }

    /**
     * Check if handle is valid.
     * @param handle Handle to check
     * @return TRUE if valid and in use
     */
    FORCE_INLINE BOOL IsValid(INT32 handle) const noexcept
    {
        return handle >= 0 && (USIZE)handle < MaxSize && m_inUse[handle];
    }

    /**
     * Check if handle is valid and initialized.
     * @param handle Handle to check
     * @return TRUE if valid, in use, and object constructed
     */
    FORCE_INLINE BOOL IsInitialized(INT32 handle) const noexcept
    {
        return handle >= 0 && (USIZE)handle < MaxSize && m_inUse[handle] && m_ptrs[handle] != nullptr;
    }

    /**
     * Get number of items currently in use.
     */
    NOINLINE USIZE GetCount() const noexcept
    {
        USIZE count = 0;
        for (USIZE i = 0; i < MaxSize; i++)
        {
            if (m_inUse[i]) count++;
        }
        return count;
    }

    /**
     * Get maximum capacity.
     */
    static constexpr USIZE GetCapacity() noexcept
    {
        return MaxSize;
    }
};

} // namespace PIL
