#pragma once
#include "iter.hpp"
#include "types.hpp"

template <class Content>
class LinkedList {
public:
    struct Node {
        explicit Node(const Content& content)
            : m_next(this)
            , m_prev(this)
        {
            new (&m_content_space) Content(content);
        }

        const Content& contents() const { return *reinterpret_cast<const Content*>(&m_content_space); }
        Content& contents() { return *reinterpret_cast<Content*>(&m_content_space); }

        Node* next() { return m_next; }
        Node* prev() { return m_prev; }

    private:
        friend class LinkedList;

        Node* m_next;
        Node* m_prev;
        alignas(Content) u8 m_content_space[sizeof(Content)];
    };

    LinkedList() = default;

    Node* append(const Content& content)
    {
        auto* new_node_ptr = construct_node(content);
        append_node(new_node_ptr);
        return new_node_ptr;
    }

    void append_node(Node* new_node_ptr)
    {
        m_length++;
        if (!m_first_node_ptr) {
            new_node_ptr->m_next = new_node_ptr;
            new_node_ptr->m_prev = new_node_ptr;
            m_first_node_ptr = new_node_ptr;
            return;
        }

        auto* last_node_ptr = m_first_node_ptr->m_prev;
        if (last_node_ptr) {
            last_node_ptr->m_next = new_node_ptr;
            new_node_ptr->m_prev = last_node_ptr;
            new_node_ptr->m_next = m_first_node_ptr;
            m_first_node_ptr->m_prev = new_node_ptr;
        } else {
            m_first_node_ptr->m_next = new_node_ptr;
            m_first_node_ptr->m_prev = new_node_ptr;
            new_node_ptr->m_prev = m_first_node_ptr;
        }
    }

    void detach_node(Node* node)
    {
        m_length--;
        auto* old_next_ptr = m_first_node_ptr->m_next;
        if (node->m_next) {
            node->m_next->m_prev = node->m_prev;
            node->m_next = nullptr;
        }
        if (node->m_prev) {
            node->m_prev->m_next = old_next_ptr;
            node->m_prev = nullptr;
        }
        if (node == m_first_node_ptr)
            m_first_node_ptr = old_next_ptr != node ? old_next_ptr : nullptr;
    }

    size_t length() const { return m_length; }

    using Iter = CircularPtrIter<LinkedList, Node>;
    Iter begin() { return Iter::begin(m_first_node_ptr); }
    Iter end() { return Iter::end(m_first_node_ptr); }

private:
    Node* construct_node(const Content& content)
    {
        return new Node();
    }

    Node* m_first_node_ptr = nullptr;
    size_t m_length = 0;
};
