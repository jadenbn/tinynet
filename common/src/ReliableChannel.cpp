#include "ReliableChannel.h"
#include <algorithm>
#include <cstring>

namespace {
// ids are 16 bits and wrap, so "how far ahead" is done in 16 bit arithmetic
uint16_t Distance(uint16_t from, uint16_t to) {
  return static_cast<uint16_t>(to - from);
}
} // namespace

bool ReliableChannel::Send(const void *data, int size) {
  if (size <= 0 || size > MAX_MESSAGE_SIZE) {
    return false;
  }
  if (Distance(oldestUnacked, nextSendId) >= WINDOW) {
    return false;
  }

  Outgoing &message = outgoing[nextSendId % WINDOW];
  message = {};
  message.used = true;
  message.id = nextSendId;
  message.size = static_cast<uint8_t>(size);
  std::memcpy(message.data, data, size);
  nextSendId++;
  return true;
}

int ReliableChannel::Receive(void *out, int capacity) {
  Incoming &slot = incoming[nextReceiveId % WINDOW];
  if (!slot.used || slot.id != nextReceiveId) {
    return 0;
  }

  int size = std::min<int>(slot.size, capacity);
  std::memcpy(out, slot.data, size);
  slot.used = false;
  nextReceiveId++;
  messagesReceived++;
  return size;
}

void ReliableChannel::Write(Buffer &buff, uint32_t packetSequence,
                            Clock::time_point now, float rttMs) {
  // reserve the count byte and fill it in once we know
  int countAt = buff.index;
  packets::WriteChar(buff, 0);

  PacketRecord &record = packets[packetSequence % SENT_QUEUE_SIZE];
  record = {};

  // an ack takes about one rtt to come back, so resending sooner than that
  // just wastes bandwidth on messages that probably arrived
  auto resendDelay = std::max<Clock::duration>(
      MIN_RESEND_DELAY,
      std::chrono::duration_cast<Clock::duration>(
          std::chrono::duration<float, std::milli>(rttMs * 1.25f)));

  uint8_t count = 0;
  for (uint16_t id = oldestUnacked; id != nextSendId && count < MAX_PER_PACKET;
       id++) {
    Outgoing &message = outgoing[id % WINDOW];
    if (!message.used) {
      continue; // acked already, just not in order
    }
    if (message.sent && now - message.lastSent < resendDelay) {
      continue;
    }
    if (buff.capacity - buff.index < 3 + message.size) {
      break; // keep the rest for the next packet
    }

    packets::WriteShort(buff, id);
    packets::WriteChar(buff, message.size);
    for (int i = 0; i < message.size; i++) {
      packets::WriteChar(buff, message.data[i]);
    }

    if (message.sent) {
      messagesResent++;
    } else {
      messagesSent++;
    }
    message.sent = true;
    message.lastSent = now;
    record.ids[count++] = id;
  }

  buff.data[countAt] = count;
  if (count > 0) {
    record.used = true;
    record.sequence = packetSequence;
    record.count = count;
  }
}

bool ReliableChannel::Read(Buffer &buff) {
  // check the whole section before touching anything, so a bad packet can't
  // leave half its messages behind
  int index = buff.index;
  if (index < 0 || buff.length - index < 1) {
    return false;
  }
  uint8_t count = buff.data[index++];
  if (count > MAX_PER_PACKET) {
    return false;
  }
  for (int i = 0; i < count; i++) {
    if (buff.length - index < 3) {
      return false;
    }
    uint8_t size = buff.data[index + 2];
    index += 3;
    if (size == 0 || size > MAX_MESSAGE_SIZE || buff.length - index < size) {
      return false;
    }
    index += size;
  }

  packets::ReadChar(buff);
  for (int i = 0; i < count; i++) {
    uint16_t id = packets::ReadShort(buff);
    uint8_t size = packets::ReadChar(buff);

    // older than what we've delivered = a resend we already have. the window
    // check also throws out ids no honest sender could have sent yet
    if (Distance(nextReceiveId, id) < WINDOW) {
      Incoming &slot = incoming[id % WINDOW];
      if (!slot.used) {
        slot.used = true;
        slot.id = id;
        slot.size = size;
        std::memcpy(slot.data, buff.data + buff.index, size);
      }
    }
    buff.index += size;
  }

  return true;
}

void ReliableChannel::OnPacketAcked(uint32_t packetSequence) {
  PacketRecord &record = packets[packetSequence % SENT_QUEUE_SIZE];
  if (!record.used || record.sequence != packetSequence) {
    return;
  }

  for (int i = 0; i < record.count; i++) {
    Outgoing &message = outgoing[record.ids[i] % WINDOW];
    if (message.used && message.id == record.ids[i]) {
      message.used = false;
    }
  }
  record.used = false;

  while (oldestUnacked != nextSendId &&
         !outgoing[oldestUnacked % WINDOW].used) {
    oldestUnacked++;
  }
}

int ReliableChannel::Unacked() const {
  int unacked = 0;
  for (uint16_t id = oldestUnacked; id != nextSendId; id++) {
    unacked += outgoing[id % WINDOW].used ? 1 : 0;
  }
  return unacked;
}
