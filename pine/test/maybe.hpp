#pragma once

#include <cassert>
#include <ostream>
#include <iostream>
#include <memory>

#include <pine/maybe.hpp>
#include <pine/types.hpp>

void maybe_basic()
{
    Maybe<int> m { 0 };
    assert(m.has_value());
    assert(m.value() == 0);
    assert(*m == 0);

    Maybe<int> m1 {};
    assert(!m1.has_value());
    assert(!m1);

    Maybe<float> m2 { 1.0f };
    assert(m2.value() == 1.0f);
    assert(m2);
}

struct MutationState {
    int total_copies = 0;
    int total_destructions = 0;
    int total_moves = 0;

    bool operator==(const MutationState& other_state)
    {
        return (
            total_copies == other_state.total_copies
            && total_destructions == other_state.total_destructions
            && total_moves == other_state.total_moves
        );
    }

    friend std::ostream& operator<<(std::ostream&, const MutationState&);
};

std::ostream& operator<<(std::ostream& os, const MutationState& state)
{
    os << "MutationState { copies: " << state.total_copies << ", destructions: " << state.total_destructions << ", moves: " << state.total_moves << " }";
    return os;
}

class MutationTrackerDefaultMove {
public:
    MutationTrackerDefaultMove() = default;
    ~MutationTrackerDefaultMove()
    {
        m_shared_state->total_destructions++;
    }
    MutationTrackerDefaultMove(const MutationTrackerDefaultMove& other)
        : m_shared_state(other.m_shared_state)
    {
        m_shared_state->total_copies++;
    };
    MutationTrackerDefaultMove& operator=(const MutationTrackerDefaultMove& other)
    {
        if (this != &other) {
            m_shared_state = other.m_shared_state;
            m_shared_state->total_copies++;
        }
        return *this;
    };

    bool has_expected_state(MutationState expected_state) const
    {
        return *m_shared_state == expected_state;
    }

    MutationState state() const
    {
        return *m_shared_state;
    }

protected:
    std::shared_ptr<MutationState> m_shared_state = std::make_shared<MutationState>();
};

class MutationTracker : public MutationTrackerDefaultMove {
public:
    using MutationTrackerDefaultMove::MutationTrackerDefaultMove;

    MutationTracker(const MutationTracker&& other)
    {
        m_shared_state = std::move(other.m_shared_state);
        m_shared_state->total_moves++;
    }
    MutationTracker& operator=(const MutationTracker&& other)
    {
        if (this != &other) {
            m_shared_state = std::move(other.m_shared_state);
            m_shared_state->total_moves++;
        }
        return *this;
    }
};

class MutationTrackerNoCopies : public MutationTracker {
public:
    MutationTrackerNoCopies() = default;
    MutationTrackerNoCopies(const MutationTrackerNoCopies& other) = delete;
    MutationTrackerNoCopies(MutationTrackerNoCopies&& other) = default;
    MutationTrackerNoCopies& operator=(MutationTrackerNoCopies&& other)
    {
        return static_cast<MutationTrackerNoCopies&>(MutationTracker::operator=(std::move(other)));
    };
    MutationTrackerNoCopies& operator=(const MutationTrackerNoCopies& other) = delete;
};

void maybe_copy_assignment()
{
    auto m = Maybe<MutationTrackerDefaultMove> {{}};
    assert(m.has_value());

    MutationState expected_state {};
    expected_state.total_copies = 1;
    expected_state.total_destructions = 1;
    assert(m.value().has_expected_state(expected_state));
}

void maybe_copy_constructor()
{
    Maybe<MutationTrackerDefaultMove> m1 {{}};
    Maybe<MutationTrackerDefaultMove> m2 { m1 };
    assert(m2.has_value());

    MutationState expected_state {};
    // first copy of default constructed Tracker, then copy into m2
    expected_state.total_copies = 2;
    expected_state.total_destructions = 1;
    assert(m2.value().has_expected_state(expected_state));
}

void maybe_move_assignemnt()
{
    Maybe<MutationTracker> m1 {{}};
    {
        assert(m1.has_value());
        Maybe<MutationTracker> m2 = std::move(m1);
        assert(!m1.has_value());
        assert(m2.has_value());

        MutationState expected_state {};
        // move and destruct of m1, then another move of m1 into m2
        expected_state.total_destructions = 1;
        expected_state.total_moves = 2;
        assert(m2.value().has_expected_state(expected_state));
    }

    assert(!m1.has_value());

    MutationState expected_state {};
    expected_state.total_moves = 2;
    expected_state.total_destructions = 2;
    assert(m1.value().has_expected_state(expected_state));
}

void maybe_move_constructor()
{
    Maybe<MutationTrackerNoCopies> m1 {{}};
    {
        assert(m1.has_value());
        Maybe<MutationTrackerNoCopies> m2 { std::move(m1) };
        assert(!m1.has_value());
        assert(m2.has_value());

        MutationState expected_state {};
        expected_state.total_destructions = 1;
        expected_state.total_moves = 2;
        assert(m2.value().has_expected_state(expected_state));
    }
    assert(!m1.has_value());

    MutationState expected_state {};
    expected_state.total_moves = 2;
    expected_state.total_destructions = 2;
    assert(m1.value().has_expected_state(expected_state));
}

class NonDefaultConstructible {
public:
    NonDefaultConstructible(int) {};
};

void maybe_non_default_constructible_type()
{
    // will compile
    Maybe<NonDefaultConstructible> m1 {};
}