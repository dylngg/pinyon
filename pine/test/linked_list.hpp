#pragma once

#include <pine/alien/malloc.hpp>
#include <pine/alien/print.hpp>

#include <pine/iter.hpp>
#include <pine/linked_list.hpp>

#include <cassert>

void manual_linked_list_create()
{
    pine::ManualLinkedList<int> list {};
    assert(list.length() == 0);
    assert(list.begin() == list.end());
}

void manual_linked_list_iterate()
{
    using Node = pine::ManualLinkedList<int>::Node;
    pine::ManualLinkedList<int> list {};
    auto it = list.begin();
    auto end = list.end();
    assert(it == end);

    int items[3] = { 1, 2, 3 };
    pine::ManualLinkedList<int> list2 {};
    for (int i = 0; i < 3; i++)
        list2.append(new (alien::malloc<Node>()) Node(items[i]));

    assert(list2.begin() != list2.end());

    // ++ iterator
    size_t i = 0;
    Node* prev = nullptr;
    it = list2.begin();
    auto it2 = it; // copy
    assert(it2 == list2.begin());

    end = list2.end();
    while (it != end) {
        auto node = *it;
        auto node2 = *it2;
        assert(node);
        assert(node == node2);

        assert(node->prev() == prev);
        if (i != 0) {
            assert(prev);
            assert(prev->next() == node);
        }

        assert(node->contents() == items[i]);

        prev = node;
        ++i;
        ++it;
        it2++;
    }

    assert(i == 3);
    assert(list2.length() == 3);

    // -- iterator
    Node* next = nullptr;
    it = --list2.end();
    it2 = it; // copy
    assert(it2 != list2.end());

    auto begin = list2.begin();
    for (i = list2.length(); i > 0; i--) {
        auto node = *it;
        auto node2 = *it2;
        assert(node);
        assert(node == node2);

        assert(it == pine::next(begin, i - 1));

        assert(node->next() == next);
        if (i != list2.length()) {
            assert(next);
            assert(next->prev() == node);
        }

        assert(node->contents() == items[i - 1]);

        next = node;
        --it;
        it2--;
    }

    assert(i == 0);
    assert(list2.length() == 3);
}

void manual_linked_list_append()
{
    using Node = pine::ManualLinkedList<int>::Node;
    pine::ManualLinkedList<int> list {};

    Node* prev = nullptr;
    int added[5] { 1, 2, 3, 4, 5 };
    for (size_t i = 0; i < 5; i++) {
        assert(list.length() == i);
        list.append(new (alien::malloc<Node>()) Node(added[i]));

        auto node = *next(list.begin(), i);
        assert(node->prev() == prev);

        assert(node->contents() == added[i]);
        for (size_t j = 0; j < i; j++)
            assert((*next(list.begin(), j))->contents() == added[j]);

        assert(list.length() == i + 1);

        prev = node;
    }
}

void manual_linked_list_remove()
{
    using Node = pine::ManualLinkedList<int>::Node;
    pine::ManualLinkedList<int> list;
    for (int i = 1; i <= 2; i++)
        list.append(new (alien::malloc<Node>()) Node(i));

    assert(list.length() == 2);

    list.remove(*list.begin());
    assert(list.length() == 1);

    assert((*list.begin())->contents() == 2);
    assert((*--list.end())->contents() == 2);

    list.remove(*list.begin());
    assert(list.length() == 0);
}
