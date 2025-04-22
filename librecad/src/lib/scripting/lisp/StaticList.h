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

#ifndef INCLUDE_STATICLIST_H
#define INCLUDE_STATICLIST_H

#ifdef DEVELOPER

template<typename T>
class StaticList
{
public:
    StaticList() : m_head(NULL) { }

    class Iterator;
    Iterator begin() { return Iterator(m_head); }
    Iterator end()   { return Iterator(NULL);   }

    class Node {
    public:
        Node(StaticList<T>& list, T item)
        : m_item(item), m_next(list.m_head) {
            list.m_head = this;
        }

    private:
        friend class Iterator;
        T m_item;
        Node* m_next;
    };

    class Iterator {
    public:
        Iterator& operator ++ () {
            m_node = m_node->m_next;
            return *this;
        }

        T& operator * () { return m_node->m_item; }
        bool operator != (const Iterator& that) {
            return m_node != that.m_node;
        }

    private:
        friend class StaticList<T>;
        Iterator(Node* node) : m_node(node) { }
        Node* m_node;
    };

private:
    friend class Node;
    Node*  m_head;
};

#endif // DEVELOPER

#endif // INCLUDE_STATICLIST_H
