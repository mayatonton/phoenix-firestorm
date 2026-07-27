/**
 * @file llpacketring.h
 * @brief definition of LLPacketRing class for implementing a resend,
 * drop, or delay in packet transmissions
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#pragma once

#include <mutex>
#include <vector>

#include "llhost.h"
#include "llpacketbuffer.h"
#include "llthrottle.h"


class LLPacketRing
{
public:
    LLPacketRing();
    ~LLPacketRing();

    // receive one packet: either buffered or from the socket
    S32  receivePacket (S32 socket, char *datap);

    // send one packet
    bool sendPacket(int h_socket, const char * send_buffer, S32 buf_size, LLHost host);

    // drains packets from socket and returns final mNumBufferedPackets
    S32 drainSocket(S32 socket);

    // blocks up to timeout_ms waiting for socket readability, then drains
    S32 waitAndDrain(S32 socket, S32 timeout_ms);

    void setDrainThreadActive(bool active) { mDrainThreadActive = active; }
    bool getDrainThreadActive() const { return mDrainThreadActive; }

    void dropPackets(U32);
    void setDropPercentage (F32 percent_to_drop);

    inline LLHost getLastSender() const;
    inline LLHost getLastReceivingInterface() const;

    S32 getActualInBytes() const { std::lock_guard<std::mutex> lock(mRingMutex); return mActualBytesIn; }
    S32 getActualOutBytes() const { return mActualBytesOut; }
    S32 getAndResetActualInBits()   { std::lock_guard<std::mutex> lock(mRingMutex); S32 bits = mActualBytesIn * 8; mActualBytesIn = 0; return bits;}
    S32 getAndResetActualOutBits()  { S32 bits = mActualBytesOut * 8; mActualBytesOut = 0; return bits;}

    S32 getNumBufferedPackets() const { std::lock_guard<std::mutex> lock(mRingMutex); return (S32)(mNumBufferedPackets); }
    S32 getNumBufferedBytes() const { std::lock_guard<std::mutex> lock(mRingMutex); return mNumBufferedBytes; }
    S32 getNumDroppedPackets() const { std::lock_guard<std::mutex> lock(mRingMutex); return mNumDroppedPacketsTotal + mNumDroppedPackets; }

    F32 getBufferLoadRate() const; // from 0 to 4 (0 - empty, 1 - default size is full)
    void dumpPacketRingStats();
protected:
    // returns 'true' if we should intentionally drop a packet
    bool computeDrop();

    // returns packet_size of received packet, zero or less if no packet found
    S32 receiveOrDropPacket(S32 socket, char *datap, bool drop);
    S32 receiveOrDropBufferedPacket(char *datap, bool drop);

    // returns packet_size of packet buffered
    S32 bufferInboundPacket(S32 socket);

    // returns 'true' if ring was expanded
    bool expandRing();

protected:
    mutable std::mutex mRingMutex;
    std::vector<LLPacketBuffer*> mPacketRing;
    S16 mHeadIndex { 0 };
    S16 mNumBufferedPackets { 0 };
    S32 mNumDroppedPackets { 0 };
    S32 mNumDroppedPacketsTotal { 0 };
    S32 mNumBufferedBytes { 0 };
    bool mDrainThreadActive { false };

    S32 mActualBytesIn { 0 };
    S32 mActualBytesOut { 0 };
    F32 mDropPercentage { 0.0f };   // % of inbound packets to drop
    U32 mPacketsToDrop { 0 };       // drop next inbound n packets

    // These are the sender and receiving_interface for the last packet delivered by receivePacket()
    LLHost mLastSender;
    LLHost mLastReceivingIF;
};


inline LLHost LLPacketRing::getLastSender() const
{
    return mLastSender;
}

inline LLHost LLPacketRing::getLastReceivingInterface() const
{
    return mLastReceivingIF;
}
