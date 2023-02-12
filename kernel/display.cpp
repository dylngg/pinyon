#include "display.hpp"
#include "mailbox.hpp"
#include "panic.hpp"

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

    PANIC_IF(!mailbox_registers().send_in_property_channel(reinterpret_cast<u32*>(&message)));

    PANIC_IF(message.phys_dim.in_out_width != message.virt_dim.in_out_width);
    PANIC_IF(message.phys_dim.in_out_height != message.virt_dim.in_out_height);
    m_width = message.phys_dim.in_out_width;
    m_height = message.phys_dim.in_out_height;

    PANIC_IF(message.depth.in_out_depth != display_depth);
    m_depth = message.depth.in_out_depth;

    // Convert from bus address to ARM accessible address
    m_buffer = reinterpret_cast<u32*>((message.allocation.in_alignment_out_ptr) & ~0xC0000000);
    m_size = message.allocation.out_size;

    mmu::page_allocator().reserve_region(PageRegion::from_ptr(m_buffer, m_size), mmu::PageAllocator::Backing::Identity);

    m_buffer[0] = 1;
}