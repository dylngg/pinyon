#pragma once
#include <pine/types.hpp>
#include <pine/string_view.hpp>

#define MAILBOX_TAG_ALLOCATE_BUFFER      0x40001
#define MAILBOX_TAG_SET_DISPLAY_PHYS_DIM 0x48003
#define MAILBOX_TAG_SET_DISPLAY_VIRT_DIM 0x48004
#define MAILBOX_TAG_SET_DISPLAY_DEPTH    0x48005
#define MAILBOX_TAG_GET_PITCH            0x40008
#define MAILBOX_TAG_SET_VIRT_OFFSET      0x48009

class Display {
public:
    void init(unsigned width, unsigned height);

private:
    void draw_string(pine::StringView string, unsigned x, unsigned y, u32 color);
    void draw_character(char ch, unsigned x, unsigned y, u32 color);
    void draw_pixel(unsigned x, unsigned y, u32 color);

    unsigned m_width = 0;
    unsigned m_height = 0;
    unsigned m_depth = 0;
    unsigned m_pitch = 0;
    u32* m_buffer = nullptr;
    size_t m_size = 0;
};

void display_init(unsigned width, unsigned height);

Display& display();