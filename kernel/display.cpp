#include "display.hpp"
#include "device/videocore/mailbox.hpp"
#include "panic.hpp"
#include "data/font.hpp"

#include <pine/errno.hpp>
#include <pine/c_string.hpp>

struct DisplayTagGetPhysicalDimensions {
    u32 tag = MAILBOX_TAG_SET_DISPLAY_PHYS_DIM;
    u32 in_size = 2 * sizeof(u32);
    u32 out_response_size = in_size;
    u32 in_out_width;
    u32 in_out_height;
};

struct DisplayTagGetVirtualDimensions {
    u32 tag = MAILBOX_TAG_SET_DISPLAY_VIRT_DIM;
    u32 in_size = 2 * sizeof(u32);
    u32 out_response_size = in_size;
    u32 in_out_width;
    u32 in_out_height;
};

struct DisplayTagSetDepth {
    u32 tag = MAILBOX_TAG_SET_DISPLAY_DEPTH;
    u32 in_size = 1 * sizeof(u32);
    u32 out_response_size = in_size;
    u32 in_out_depth;
};

struct DisplayTagSetVirtualOffset {
    u32 tag = MAILBOX_TAG_SET_VIRT_OFFSET;
    u32 in_size = 2 * sizeof(u32);
    u32 out_response_size = in_size;
    u32 out_x_offset = 0;
    u32 out_y_offset = 0;
};

struct DisplayTagAllocateBuffer {
    u32 tag = MAILBOX_TAG_ALLOCATE_BUFFER;
    u32 in_size = 2 * sizeof(u32);
    u32 out_response_size = in_size;
    u32 in_alignment_out_ptr = 0;
    u32 out_size = 0;
};

struct DisplayTagGetPitch {
    u32 tag = MAILBOX_TAG_GET_PITCH;
    u32 in_size = 1 * sizeof(u32);
    u32 out_response_size = in_size;
    u32 out_pitch = 0;
};

struct __attribute__((__packed__)) DisplayMailboxMessage {
    u32 size = sizeof(DisplayMailboxMessage);
    u32 type = MAILBOX_REQUEST;

    DisplayTagGetPhysicalDimensions phys_dim;
    DisplayTagGetVirtualDimensions virt_dim;
    DisplayTagSetDepth depth;
    DisplayTagSetVirtualOffset virt_offset;
    DisplayTagAllocateBuffer allocation;
    DisplayTagGetPitch pitch;

    u32 end_tag = MAILBOX_END_TAG;
};

static Display g_display;

Display& display()
{
    return g_display;
}

void display_init(unsigned width, unsigned height) {
    g_display.init(width, height);
}

void Display::init(unsigned int width, unsigned int height)
{
    // Hardcode and use 32-bit color values: 8-bit red, green, blue, with 8-bit transparency
    // Easier to work with...
    constexpr auto display_depth = sizeof(m_buffer) * CHAR_BIT;

    // The message must be aligned to 16 bits as only the upper 28 bits are
    // provided to the mailbox
    static __attribute__((aligned(16))) DisplayMailboxMessage message {
        .phys_dim = {
            .in_out_width = width,
            .in_out_height = height,
        },
        .virt_dim = {
            .in_out_width = width,
            .in_out_height = height,
        },
        .depth {
            .in_out_depth = display_depth,
        },
        .virt_offset {},
        .allocation {
            .in_alignment_out_ptr = PageSize,
        },
        .pitch {},
    };

    // We are safe to use u32 since our message is aligned to 32-bits
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
    auto* message_ptr = reinterpret_cast<u32*>(&message);
#pragma GCC diagnostic pop

    PANIC_IF(!mailbox_registers().send_in_property_channel(message_ptr));

    PANIC_IF(message.phys_dim.in_out_width != message.virt_dim.in_out_width);
    PANIC_IF(message.phys_dim.in_out_height != message.virt_dim.in_out_height);
    m_width = message.phys_dim.in_out_width;
    m_height = message.phys_dim.in_out_height;

    PANIC_IF(message.depth.in_out_depth != display_depth);
    m_depth = message.depth.in_out_depth;

    // Pitch is the width of the row in bytes
    m_pitch = message.pitch.out_pitch;

    // Convert from bus address to ARM accessible address
    m_buffer = reinterpret_cast<u32*>((message.allocation.in_alignment_out_ptr) & ~0xC0000000);
    m_size = message.allocation.out_size;
}

void Display::draw_string(pine::StringView string, unsigned x, unsigned y, u32 color)
{
    auto original_x = x;
    for (char ch : string) {
        if (ch == '\n') {
            x = original_x;
            y += font::height + 2;
            continue;
        }

        if (x + font::width > m_width) {
            continue;
        }
        if (y > m_width) {
            continue;
        }

        draw_character(ch, x, y, color);
        x += font::width + 2;
    }
}

void Display::draw_character(char ch, unsigned x, unsigned y, u32 color)
{
    for (unsigned font_y = 0; font_y < font::height; font_y++) {
        for (unsigned font_x = 0; font_x < font::width; font_x++) {
            if (!font::character_has_visible_pixel(ch, font_x, font_y))
                continue;

            draw_pixel(x + font_x, y + font_y, color);
        }
    }
}

void Display::draw_pixel(unsigned x, unsigned y, u32 color)
{
    auto bytes_per_pixel = m_depth / 8;
    u32 offset = x + ((y * m_pitch) / bytes_per_pixel);
    m_buffer[offset] = color;
}

ssize_t DisplayFile::read(char* buf, size_t at_most_bytes)
{
    if (m_displayed_string.empty())
        return 0;

    auto amount_copied = pine::strbufcopy(buf, at_most_bytes, m_displayed_string.c_str());
    return static_cast<ssize_t>(amount_copied);
}

ssize_t DisplayFile::write(char* buf, size_t bytes)
{
    m_displayed_string.clear();
    auto maybe_string = KString::try_create(kernel_allocator(), buf, bytes);
    if (!maybe_string)
        return -ENOMEM;

    m_displayed_string = pine::move(*maybe_string);
    display().draw_string(m_displayed_string, DISPLAY_X_INSET, DISPLAY_Y_INSET, 0x21dd7f);
    return bytes;
}
