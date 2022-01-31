#pragma once
#include "iter.hpp"
#include "types.hpp"
#include "utility.hpp"

template <class Content>
class LinkedList {
public:
    struct Node {
        explicit Node(const Content& content)
            : m_next(nullptr)
            , m_prev(nullptr)
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

    struct NodeIter {
        // prefix increment
        NodeIter operator++()
        {
            m_node_ptr = m_node_ptr->next();
            return *this;
        }
        // postfix increment
        NodeIter operator++(int)
        {
            return ++(*this);
        }

        constexpr bool at_end() const { return m_node_ptr == nullptr; }

        constexpr Node*& operator*() { return m_node_ptr; };
        constexpr const Node*& operator*() const { return m_node_ptr; };

        constexpr bool operator==(const NodeIter& other) const { return m_node_ptr == other.m_node_ptr; }
        constexpr bool operator!=(const NodeIter& other) const { return !(*this == other); }

    private:
        NodeIter() = default;
        NodeIter(Node* ptr)
            : m_node_ptr(ptr) {};
        friend class LinkedList;

        Node* m_node_ptr = nullptr;
    };

    LinkedList() = default;

    void append(Node* new_node_ptr)
    {
        m_length++;
        if (!m_head) {
            m_head = m_tail = new_node_ptr;
            return;
        }

        m_tail->m_next = new_node_ptr;
        new_node_ptr->m_prev = m_tail;
        m_tail = new_node_ptr;
    }

    void detach(Node* node_ptr)
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

    size_t length() const { return m_length; }

    NodeIter begin() { return NodeIter(m_head); }
    NodeIter end() { return NodeIter(); }

private:
    Node* m_head = nullptr;
    Node* m_tail = nullptr;
    size_t m_length = 0;
};