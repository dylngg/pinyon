#include "mailbox.hpp"

#include <kernel/panic.hpp>

#include <pine/twomath.hpp>

MailboxRegisters& mailbox_registers()
{
    static auto* g_mailbox_registers = reinterpret_cast<MailboxRegisters*>(MAILBOX_BASE);
    return *g_mailbox_registers;
}

bool MailboxRegisters::send_in_property_channel(u32* message_contents)
{
    // See https://github.com/raspberrypi/firmware/wiki/Accessing-mailboxes
    auto message_addr = reinterpret_cast<PtrData>(message_contents);
    PANIC_IF(!pine::is_aligned_two(message_addr, 16u));

    // upper 28 msb are the message address, lower 4 lsb are the channel
    auto message = message_addr | static_cast<PtrData>(MailboxChannel::PropertyTagsSend);

    while (status & MAILBOX_FULL) {};

    write = message;

    for (;;) {
        if (status & MAILBOX_EMPTY)
            continue;

        // check if response to our message
        while (read == message) {
            if (message_contents[1] == MAILBOX_RESPONSE)
                return true;
            if (message_contents[1] == MAILBOX_ERROR)
                return false;
        }
    }
    return false;
}

pine::Maybe<u64> try_retrieve_serial_num_from_mailbox()
{
    // The message must be aligned to 16 bits as only the upper 28 bits are
    // provided to the mailbox
    alignas(16) u32 message_contents[8];

    message_contents[0] = sizeof(message_contents);     // length of buffer
    message_contents[1] = MAILBOX_REQUEST;              // mark as request

    message_contents[2] = MAILBOX_TAG_GETSERIAL;
    message_contents[3] = 8;    // buffer size in bytes
    message_contents[4] = 8;    // response size in bytes

    // 5:6 is the message contents

    message_contents[7] = MAILBOX_END_TAG;

    if (!mailbox_registers().send_in_property_channel(&message_contents[0]))
        return {};

    return static_cast<u64>(message_contents[5]) | (static_cast<u64>(message_contents[6]) << 32);
}
