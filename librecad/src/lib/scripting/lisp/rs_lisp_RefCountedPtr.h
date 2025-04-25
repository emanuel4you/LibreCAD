/*******************************************************************************
*
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 emanuel

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#ifndef INCLUDE_RS_LISP_REFCOUNTEDPTR_H
#define INCLUDE_RS_LISP_REFCOUNTEDPTR_H

#ifdef DEVELOPER

#include "rs_lisp_debug.h"

#include <cstddef>

class RefCounted {
public:
    RefCounted() : m_refCount(0) { }
    virtual ~RefCounted() { }

    const RefCounted* acquire() const { m_refCount++; return this; }
    int release() const { return --m_refCount; }
    int refCount() const { return m_refCount; }

private:
    RefCounted(const RefCounted&); // no copy ctor
    RefCounted& operator = (const RefCounted&); // no assignments

    mutable int m_refCount;
};

template<class T>
class RefCountedPtr {
public:
    RefCountedPtr() : m_object(0) { }

    RefCountedPtr(T* object) : m_object(0)
    { acquire(object); }

    RefCountedPtr(const RefCountedPtr& rhs) : m_object(0)
    { acquire(rhs.m_object); }

    const RefCountedPtr& operator = (const RefCountedPtr& rhs) {
        acquire(rhs.m_object);
        return *this;
    }

    bool operator == (const RefCountedPtr& rhs) const {
        return m_object == rhs.m_object;
    }

    bool operator != (const RefCountedPtr& rhs) const {
        return m_object != rhs.m_object;
    }

    operator bool () const {
        return m_object != NULL;
    }

    ~RefCountedPtr() {
        release();
    }

    T* operator -> () const { return m_object; }
    T* ptr() const { return m_object; }

private:
    void acquire(T* object) {
        if (object != NULL) {
            object->acquire();
        }
        release();
        m_object = object;
    }

    void release() {
        if ((m_object != NULL) && (m_object->release() == 0)) {
            delete m_object;
        }
    }

    T* m_object;
};

#endif // DEVELOPER

#endif // INCLUDE_RS_LISP_REFCOUNTEDPTR_H
