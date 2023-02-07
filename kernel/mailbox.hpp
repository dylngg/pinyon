#pragma once
#include <pine/types.hpp>
#include <pine/bit.hpp>
#include <pine/maybe.hpp>

#define MAILBOX_BASE    0x3f00b880
#define MAILBOX_REQUEST    0

enum class MailboxChannel : u32 {
    Power = 0,
    FrameBuffer = 0,
    VirtualUART = 2,
    VCHIQ = 3,
    LEDs = 4,
    Buttons = 5,
    TouchScreen = 6,
    Unused = 7,
    PropertyTagsSend = 8,
    PropertyTagsRecieve = 9,
};

/* tags */
#define MAILBOX_TAG_GETSERIAL      0x10004
#define MAILBOX_END_TAG            0

/* responses */
#define MAILBOX_RESPONSE   0x80000000
#define MAILBOX_ERROR      0x80000001
#define MAILBOX_FULL       0x80000000
#define MAILBOX_EMPTY      0x40000000

class MailboxRegisters;
MailboxRegisters& mailbox_registers();

struct MailboxRegisters {
    bool send_in_property_channel(u32* message_contents);

    volatile u32 read;
    volatile u32 unused1;
    volatile u32 unused2;
    volatile u32 unused3;
    volatile u32 peek;
    volatile u32 sender;
    volatile u32 status;
    volatile u32 config;
    volatile u32 write;
};

static_assert(offsetof(MailboxRegisters, write) == 0x20);

pine::Maybe<u64> try_retrieve_serial_num_from_mailbox();
