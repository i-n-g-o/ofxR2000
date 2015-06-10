// Copyright (c) 2014, Pepperl+Fuchs GmbH, Mannheim
// Copyright (c) 2014, Denis Dillenberger
// All rights reserved.
//
// Use, modification, and distribution is subject to the
// 3-clause BSD license ("Revised BSD License",
// "New BSD License", or "Modified BSD License")
// You should have received a copy of this license
// in a file named COPYING or LICENSE.

#ifndef PACKET_STRUCTURE_H
#define PACKET_STRUCTURE_H
#include <tr1/cstdint>
#include <vector>

namespace pepperl_fuchs {

#pragma pack(1)
//! \struct PacketHeader
//! \brief Header of a TCP or UDP data packet from the scanner
struct PacketHeader
{
    //! Magic bytes, must be  5C A2 (hex)
    std::tr1::uint16_t magic;

    //! Packet type, must be 43 00 (hex)
    std::tr1::uint16_t packet_type;

    //! Overall packet size (header+payload), 1404 bytes with maximum payload
    std::tr1::uint32_t packet_size;

    //! Header size, defaults to 60 bytes
    std::tr1::uint16_t header_size;

    //! Sequence for scan (incremented for every scan, starting with 0, overflows)
    std::tr1::uint16_t scan_number;

    //! Sequence number for packet (counting packets of a particular scan, starting with 1)
    std::tr1::uint16_t packet_number;

    //! Raw timestamp of internal clock in NTP time format
    std::tr1::uint64_t timestamp_raw;

    //! With an external NTP server synced Timestamp  (currenty not available and and set to zero)
    std::tr1::uint64_t timestamp_sync;

    //! Status flags
    std::tr1::uint32_t status_flags;

    //! Frequency of scan-head rotation in mHz (Milli-Hertz)
    std::tr1::uint32_t scan_frequency;

    //! Total number of scan points (samples) within complete scan
    std::tr1::uint16_t num_points_scan;

    //! Total number of scan points within this packet
    std::tr1::uint16_t num_points_packet;

    //! Index of first scan point within this packet
    std::tr1::uint16_t first_index;

    //! Absolute angle of first scan point within this packet in 1/10000°
    std::tr1::int32_t first_angle;

    //! Delta between two succeding scan points 1/10000°
    std::tr1::int32_t angular_increment;

    //! Output status
    std::tr1::uint32_t output_status;

    //! Field status
    std::tr1::uint32_t field_status;

    //! Possible padding to align header size to 32bit boundary
    //std::uint8 padding[0];
};

//! \struct PacketTypeC
//! \brief Structure of a UDP or TCP data packet from the laserscanner
struct PacketTypeC
{
    PacketHeader header;
    std::tr1::uint32_t distance_amplitude_payload; // distance 20 bit, amplitude 12 bit
};
#pragma pack()

//! \struct ScanData
//! \brief Normally contains one complete laserscan (a full rotation of the scanner head)
struct ScanData
{
    //! Distance data in polar form in millimeter
    std::vector<std::tr1::uint32_t> distance_data;

    //! Amplitude data in the range 32-4095, values lower than 32 indicate an error or undefined values
    std::vector<std::tr1::uint32_t> amplitude_data;

    //! Header received with the distance and amplitude data
    std::vector<PacketHeader> headers;
};

}

#endif // PACKET_STRUCTURE_H
