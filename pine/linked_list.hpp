#pragma once

#include "iter.hpp"
#include "types.hpp"
#include "utility.hpp"
#include "print.hpp"
#include "units.hpp"

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
        ~Node()
        {
            auto* prev = exchange(m_prev, nullptr);
            auto* next = exchange(m_next, nullptr);

            if (prev)
                prev->m_next = next;
            if (next)
                next->m_prev = prev;

            contents().~Content();
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
        // Needed for allocators
    };

    static_assert(offsetof(Node, m_content_space) % Alignment == 0);

    /*
     * Comparators for Node* contents.
     */
    struct Less {
        template <typename Second>
        bool operator()(const Node* first, const Second& second)
        {
            return first->contents() < second;
        }
    };
    struct Greater {
        template <typename Second>
        bool operator()(const Node* first, const Second& second)
        {
            return first->contents() > second;
        }
    };

    ManualLinkedList() = default;
    ~ManualLinkedList()
    {
        // Cannot use range-based for loop since we are destructing while we
        // are iterating (setting m_next to null)
        auto next = m_head;
        while (next != nullptr) {
            auto curr = next;
            next = curr->m_next;
            curr->~Node();
        }
    }
    // Because the user is manually dealing with memory we cannot have copy
    // semantics
    ManualLinkedList(const ManualLinkedList&) = delete;
    ManualLinkedList(ManualLinkedList&&) = delete;

    void insert_after(Node* after_node_ptr, Node& new_node)
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
        insert_after(m_tail, new_node);
    }
    void remove(Node* node_ptr)
    {
        m_length--;
        auto prev = node_ptr->m_prev;
        auto next = node_ptr->m_next;
        node_ptr->~Node();

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
    ConstIter cbegin() const { return ConstIter::begin(m_head, m_tail); }
    ConstIter cend() const { return ConstIter::end(m_tail); }

protected:
    friend void print_with(Printer& printer, const ManualLinkedList<Content>& list)
    {
        auto it = list.cbegin();
        print_with(printer, "[");
        while (it != list.cend()) {
            print_with(printer, (*it)->contents());
            if (++it != list.cend())
                print_with(printer, " <-> ");
        }
        print_with(printer, "]");
    }

private:
    Node* m_head = nullptr;
    Node* m_tail = nullptr;
    size_t m_length = 0;
};

}
