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
    bool operator!=(const MutationState& other_state)
    {
        return !(*this == other_state);
    }

    friend std::ostream& operator<<(std::ostream&, const MutationState&);
};

std::ostream& operator<<(std::ostream& os, const MutationState& state)
{
    os << "MutationState { copies: " << state.total_copies << ", destructions: " << state.total_destructions << ", moves: " << state.total_moves << " }";
    return os;
}

class MutationTrackerNoMove {
public:
    MutationTrackerNoMove() = default;
    ~MutationTrackerNoMove()
    {
        m_shared_state->total_destructions++;
    }
    MutationTrackerNoMove(const MutationTrackerNoMove& other)
        : m_shared_state(other.m_shared_state)
    {
        m_shared_state->total_copies++;
    };
    MutationTrackerNoMove(MutationTrackerNoMove&&) = delete;
    MutationTrackerNoMove& operator=(const MutationTrackerNoMove& other)
    {
        if (this != &other) {
            m_shared_state = other.m_shared_state;
            m_shared_state->total_copies++;
        }
        return *this;
    };
    MutationTrackerNoMove& operator=(MutationTrackerNoMove&&) = delete;

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

class MutationTracker : public MutationTrackerNoMove {
public:
    using MutationTrackerNoMove::MutationTrackerNoMove;

    MutationTracker(MutationTracker&& other)
    {
        m_shared_state = other.m_shared_state;  // copy; we need to mark destructor (will leak)
        m_shared_state->total_moves++;
    }
    MutationTracker& operator=(MutationTracker&& other)
    {
        if (this != &other) {
            m_shared_state = other.m_shared_state;  // copy; we need to mark destructor (will leak)
            m_shared_state->total_moves++;
        }
        return *this;
    }
};

class MutationTrackerNoCopy : public MutationTracker {
public:
    using MutationTracker::MutationTracker;

    MutationTrackerNoCopy(const MutationTrackerNoCopy& other) = delete;
    MutationTrackerNoCopy(MutationTrackerNoCopy&& other) = default;
    MutationTrackerNoCopy& operator=(const MutationTrackerNoCopy& other) = delete;
    MutationTrackerNoCopy& operator=(MutationTrackerNoCopy&& other) = default;
};

#define ASSERT_EXPECTED_STATE(t, e) \
    assert_expected_state((t), (e), __PRETTY_FUNCTION__, __FILE__, __LINE__)

template <typename T>
inline void assert_expected_state(const T& t, MutationState expected, const char* function, const char* filename, int line)
{
    auto actual = t.state();
    if (actual != expected) {
        std::cerr << filename << ":" << line << " " << function << "\tExpected " << expected << ", but got " << actual << std::endl;
        assert(false);
    }
}

void maybe_copy_assignment()
{
    auto tracker = MutationTrackerNoMove {};
    auto m = Maybe<MutationTrackerNoMove> { tracker };
    assert(m.has_value());

    MutationState expected_state {};
    expected_state.total_copies = 1;
    expected_state.total_destructions = 0;
    ASSERT_EXPECTED_STATE(m.value(), expected_state);
}

void maybe_copy_constructor()
{
    auto tracker = MutationTrackerNoMove {};
    Maybe<MutationTrackerNoMove> m1 { tracker };
    Maybe<MutationTrackerNoMove> m2 { m1 };
    assert(m2.has_value());

    MutationState expected_state {};
    // first copy of default constructed Tracker, then copy into m2
    expected_state.total_copies = 2;
    expected_state.total_destructions = 0;
    ASSERT_EXPECTED_STATE(m2.value(), expected_state);
}

void maybe_move_assignment()
{
    Maybe<MutationTrackerNoCopy> m1 {{}};
    {
        assert(m1.has_value());
        Maybe<MutationTrackerNoCopy> m2 = std::move(m1);  // destroy old m1 value after move
        assert(!m1.has_value());
        assert(m2.has_value());

        MutationState expected_state {};
        // move and destruct of m1, then another move of m1 into m2
        expected_state.total_destructions = 1;
        expected_state.total_moves = 2;
        ASSERT_EXPECTED_STATE(m2.value(), expected_state);
    }

    assert(!m1.has_value());

    MutationState expected_state {};
    expected_state.total_moves = 2;
    expected_state.total_destructions = 2;
    ASSERT_EXPECTED_STATE(m1.value(), expected_state);
}

void maybe_move_constructor()
{
    Maybe<MutationTrackerNoCopy> m1 {{}};
    {
        assert(m1.has_value());
        Maybe<MutationTrackerNoCopy> m2 { std::move(m1) };  // destroy old m1 value after move
        assert(!m1.has_value());
        assert(m2.has_value());

        MutationState expected_state {};
        expected_state.total_destructions = 1;
        expected_state.total_moves = 2;
        ASSERT_EXPECTED_STATE(m2.value(), expected_state);
    }
    assert(!m1.has_value());

    MutationState expected_state {};
    expected_state.total_moves = 2;
    expected_state.total_destructions = 2;
    ASSERT_EXPECTED_STATE(m1.value(), expected_state);
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
