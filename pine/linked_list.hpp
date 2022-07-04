#pragma once

#include <initializer_list>  // comes with -ffreestanding

#include "iter.hpp"
#include "types.hpp"
#include "utility.hpp"

namespace pine {

template <class Content>
class ManualLinkedList {
public:
    struct Node {
        explicit Node(const Content& content)
            : m_next(nullptr)
            , m_prev(nullptr)
        {
            new (&m_content_space) Content(content);
        }
        template <typename... Args>
        explicit Node(Args&&... args)
            : m_next(nullptr)
            , m_prev(nullptr)
        {
            new (&m_content_space) Content(forward<Args>(args)...);
        }

        const Content& contents() const { return *reinterpret_cast<const Content*>(&m_content_space); }
        Content& contents() { return *reinterpret_cast<Content*>(&m_content_space); }

        Node* next() { return m_next; }
        Node* prev() { return m_prev; }
        const Node* next() const { return m_next; }
        const Node* prev() const { return m_prev; }

    private:
        friend class ManualLinkedList;

        Node* m_next;
        Node* m_prev;
        alignas(Content) u8 m_content_space[sizeof(Content)];
    };

    /*
     * Less than comparator for Node* contents.
     */
    struct Less {
        bool operator()(const Node* first, const Content& second)
        {
            return first->contents() < second;
        }
    };


    ManualLinkedList() = default;
    // FIXME: Implement these + destructor
    ManualLinkedList(const ManualLinkedList&) = delete;
    ManualLinkedList(ManualLinkedList&&) = delete;

    void insert(Node* after_node_ptr, Node& new_node)
    {
        m_length++;
        if (!after_node_ptr)
            after_node_ptr = m_tail;

        auto new_node_ptr = &new_node;
        if (!m_head) {  // either m_head and m_tail are non-null or both are null
            m_head = m_tail = new_node_ptr;
            return;
        }

        new_node_ptr->m_prev = after_node_ptr;
        Node* next = exchange(after_node_ptr->m_next, new_node_ptr);
        if (next) {
            new_node_ptr->m_next = next;
            next->m_prev = new_node_ptr;
        }
        else {
            m_tail = new_node_ptr;
        }
    }
    void append(Node& new_node)
    {
        insert(m_tail, new_node);
    }
    void remove(Node* node_ptr)
    {
        m_length--;
        auto* prev = exchange(node_ptr->m_prev, nullptr);
        auto* next = exchange(node_ptr->m_next, nullptr);

        if (prev)
            prev->m_next = next;
        if (next)
            next->m_prev = prev;

        if (node_ptr == m_head)
            m_head = next;
        if (node_ptr == m_tail)
            m_tail = prev ? prev : next;
    }

    [[nodiscard]] size_t length() const { return m_length; }

    using Iter = PtrIter<ManualLinkedList<Content>, Node*>;
    Iter begin() { return Iter::begin(m_head, m_tail); }
    Iter end() { return Iter::end(m_tail); }

    using ConstIter = PtrIter<ManualLinkedList<Content>, const Node*>;
    ConstIter begin() const { return Iter::begin(m_head, m_tail); }
    ConstIter end() const { return Iter::end(m_tail); }

protected:

private:
    Node* m_head = nullptr;
    Node* m_tail = nullptr;
    size_t m_length = 0;
};

}
