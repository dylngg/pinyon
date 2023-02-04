#pragma once

class Waitable {
public:
    virtual ~Waitable() = default;
    virtual bool is_finished() const = 0;
};