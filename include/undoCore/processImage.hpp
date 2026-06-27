/**
 * @file processImage.hpp
 * @brief Process Image for IEC 61131-3 PLC execution
 * @author Salvatore Bamundo
 * @date June 2026
 * @brief
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: Copyright (c) 2026 undoRT
 * 
 * This header provides the processImage basic class that will be used by undoPLC and undoBUS.
 */

#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace undoCore {

/**
 * @brief Class handler for Process Image
 * @details 
 * Provides a double-buffered Process Image split into:
 *   - PII (Process Image Inputs)  : AT %I* — written by undoBUS, read by undoPLC
 *   - PIQ (Process Image Outputs) : AT %Q* — written by undoPLC, read by undoBUS
 *   - PIM (Process Image Markers) : AT %M* — internal PLC memory
 *
 * Memory layout assumption: little-endian host (x86/x86_64).
 * EtherCAT PDO data is little-endian — no conversion required on x86.
 *
 * Sync contract:
 *   undoBUS calls copyIn()  after writing PDO RX data  → start of PLC cycle
 *   undoBUS calls copyOut() before reading PDO TX data → end of PLC cycle
 *   The timing of these calls is managed by undoPLC/undoBUS coordination,
 *   not by this class.
 *
 * @tparam InputBytes  Size of the input process image in bytes
 * @tparam OutputBytes Size of the output process image in bytes
 * @tparam MarkerBytes Size of the marker area in bytes (default: 0)
 */
template<size_t InputBytes, size_t OutputBytes, size_t MarkerBytes = 0>
class ProcessImage
{
   static_assert(InputBytes > 0, "InputBytes must be greater than 0");
   static_assert(OutputBytes > 0, "OutputBytes must be greater than 0");

public:
   // Size constants
   static constexpr size_t inputBytes = InputBytes;
   static constexpr size_t outputBytes = OutputBytes;
   static constexpr size_t markerBytes = MarkerBytes;

   // Constructor
   ProcessImage()
   {
      _busInputs.fill(0);
      _plcInputs.fill(0);
      _plcOutputs.fill(0);
      _busOutputs.fill(0);
      if constexpr (MarkerBytes > 0) {
         _markers.fill(0);
      }
   }

   // Bus-side interface (used by undoBUS)

   /**
     * @brief Raw pointer to the bus input buffer.
     * undoBUS writes PDO RX data here before calling copyIn().
     */
   uint8_t* busInputPtr() noexcept { return _busInputs.data(); }

   /**
     * @brief Raw pointer to the bus output buffer.
     * undoBUS reads PDO TX data from here after copyOut().
     */
   const uint8_t* busOutputPtr() const noexcept { return _busOutputs.data(); }

   /**
     * @brief Copy-in: freeze bus inputs into PLC-side input snapshot.
     * Called by undoBUS (or MasterTask) at the start of each PLC cycle.
     */
   void copyIn() noexcept { std::memcpy(_plcInputs.data(), _busInputs.data(), InputBytes); }

   /**
     * @brief Copy-out: push PLC outputs to bus-side output buffer.
     * Called by undoBUS (or MasterTask) at the end of each PLC cycle.
     */
   void copyOut() noexcept { std::memcpy(_busOutputs.data(), _plcOutputs.data(), OutputBytes); }

   // PLC-side input accessors — AT %I*  (read-only for PLC)

   /**
     * @brief Read input bit: AT %IX<byteOffset>.<bitOffset>
     * @param byteOffset Byte offset in the input image
     * @param bitOffset  Bit offset within the byte [0..7]
     */
   bool readInputBit(size_t byteOffset, uint8_t bitOffset) const
   {
      checkInputBounds(byteOffset);
      checkBitOffset(bitOffset);
      return (_plcInputs[byteOffset] >> bitOffset) & 0x01;
   }

   /** @brief Read input byte: AT %IB<byteOffset> */
   uint8_t readInputByte(size_t byteOffset) const
   {
      checkInputBounds(byteOffset);
      return _plcInputs[byteOffset];
   }

   /** @brief Read input word (16-bit): AT %IW<byteOffset> */
   uint16_t readInputWord(size_t byteOffset) const
   {
      checkInputBounds(byteOffset + 1);
      uint16_t value;
      std::memcpy(&value, &_plcInputs[byteOffset], sizeof(uint16_t));
      return value;
   }

   /** @brief Read input dword (32-bit): AT %ID<byteOffset> */
   uint32_t readInputDword(size_t byteOffset) const
   {
      checkInputBounds(byteOffset + 3);
      uint32_t value;
      std::memcpy(&value, &_plcInputs[byteOffset], sizeof(uint32_t));
      return value;
   }

   /** @brief Read input lword (64-bit): AT %IL<byteOffset> */
   uint64_t readInputLword(size_t byteOffset) const
   {
      checkInputBounds(byteOffset + 7);
      uint64_t value;
      std::memcpy(&value, &_plcInputs[byteOffset], sizeof(uint64_t));
      return value;
   }

   // =========================================================================
   // PLC-side output accessors — AT %Q*  (read/write for PLC)
   // =========================================================================

   /** @brief Write output bit: AT %QX<byteOffset>.<bitOffset> */
   void writeOutputBit(size_t byteOffset, uint8_t bitOffset, bool value)
   {
      checkOutputBounds(byteOffset);
      checkBitOffset(bitOffset);
      if (value) {
         _plcOutputs[byteOffset] |= (uint8_t(1) << bitOffset);
      } else {
         _plcOutputs[byteOffset] &= ~(uint8_t(1) << bitOffset);
      }
   }

   /** @brief Read output bit (PLC reads back its own output) */
   bool readOutputBit(size_t byteOffset, uint8_t bitOffset) const
   {
      checkOutputBounds(byteOffset);
      checkBitOffset(bitOffset);
      return (_plcOutputs[byteOffset] >> bitOffset) & 0x01;
   }

   /** @brief Write output byte: AT %QB<byteOffset> */
   void writeOutputByte(size_t byteOffset, uint8_t value)
   {
      checkOutputBounds(byteOffset);
      _plcOutputs[byteOffset] = value;
   }

   /** @brief Read output byte */
   uint8_t readOutputByte(size_t byteOffset) const
   {
      checkOutputBounds(byteOffset);
      return _plcOutputs[byteOffset];
   }

   /** @brief Write output word (16-bit): AT %QW<byteOffset> */
   void writeOutputWord(size_t byteOffset, uint16_t value)
   {
      checkOutputBounds(byteOffset + 1);
      std::memcpy(&_plcOutputs[byteOffset], &value, sizeof(uint16_t));
   }

   /** @brief Read output word */
   uint16_t readOutputWord(size_t byteOffset) const
   {
      checkOutputBounds(byteOffset + 1);
      uint16_t value;
      std::memcpy(&value, &_plcOutputs[byteOffset], sizeof(uint16_t));
      return value;
   }

   /** @brief Write output dword (32-bit): AT %QD<byteOffset> */
   void writeOutputDword(size_t byteOffset, uint32_t value)
   {
      checkOutputBounds(byteOffset + 3);
      std::memcpy(&_plcOutputs[byteOffset], &value, sizeof(uint32_t));
   }

   /** @brief Read output dword */
   uint32_t readOutputDword(size_t byteOffset) const
   {
      checkOutputBounds(byteOffset + 3);
      uint32_t value;
      std::memcpy(&value, &_plcOutputs[byteOffset], sizeof(uint32_t));
      return value;
   }

   /** @brief Write output lword (64-bit): AT %QL<byteOffset> */
   void writeOutputLword(size_t byteOffset, uint64_t value)
   {
      checkOutputBounds(byteOffset + 7);
      std::memcpy(&_plcOutputs[byteOffset], &value, sizeof(uint64_t));
   }

   /** @brief Read output lword */
   uint64_t readOutputLword(size_t byteOffset) const
   {
      checkOutputBounds(byteOffset + 7);
      uint64_t value;
      std::memcpy(&value, &_plcOutputs[byteOffset], sizeof(uint64_t));
      return value;
   }

   // Marker accessors — AT %M*  (read/write, PLC internal)

   /** @brief Write marker bit: AT %MX<byteOffset>.<bitOffset> */
   void writeMarkerBit(size_t byteOffset, uint8_t bitOffset, bool value)
   {
      static_assert(MarkerBytes > 0, "Marker area is disabled (MarkerBytes == 0)");
      checkMarkerBounds(byteOffset);
      checkBitOffset(bitOffset);
      if (value) {
         _markers[byteOffset] |= (uint8_t(1) << bitOffset);
      } else {
         _markers[byteOffset] &= ~(uint8_t(1) << bitOffset);
      }
   }

   /** @brief Read marker bit */
   bool readMarkerBit(size_t byteOffset, uint8_t bitOffset) const
   {
      static_assert(MarkerBytes > 0, "Marker area is disabled (MarkerBytes == 0)");
      checkMarkerBounds(byteOffset);
      checkBitOffset(bitOffset);
      return (_markers[byteOffset] >> bitOffset) & 0x01;
   }

   /** @brief Write marker byte: AT %MB<byteOffset> */
   void writeMarkerByte(size_t byteOffset, uint8_t value)
   {
      static_assert(MarkerBytes > 0, "Marker area is disabled (MarkerBytes == 0)");
      checkMarkerBounds(byteOffset);
      _markers[byteOffset] = value;
   }

   /** @brief Read marker byte */
   uint8_t readMarkerByte(size_t byteOffset) const
   {
      static_assert(MarkerBytes > 0, "Marker area is disabled (MarkerBytes == 0)");
      checkMarkerBounds(byteOffset);
      return _markers[byteOffset];
   }

private:
   // Memory buffers — cache-line aligned to avoid false sharing
   alignas(64) std::array<uint8_t, InputBytes> _busInputs;
   alignas(64) std::array<uint8_t, InputBytes> _plcInputs;
   alignas(64) std::array<uint8_t, OutputBytes> _plcOutputs;
   alignas(64) std::array<uint8_t, OutputBytes> _busOutputs;
   alignas(64) std::array<uint8_t, (MarkerBytes > 0 ? MarkerBytes : 1)> _markers;

   // Bounds checking helpers

   void checkInputBounds(size_t byteOffset) const
   {
      if (byteOffset >= InputBytes) {
         throw std::out_of_range("ProcessImage: input byte offset out of range");
      }
   }

   void checkOutputBounds(size_t byteOffset) const
   {
      if (byteOffset >= OutputBytes) {
         throw std::out_of_range("ProcessImage: output byte offset out of range");
      }
   }

   void checkMarkerBounds(size_t byteOffset) const
   {
      if (byteOffset >= MarkerBytes) {
         throw std::out_of_range("ProcessImage: marker byte offset out of range");
      }
   }

   void checkBitOffset(uint8_t bitOffset) const
   {
      if (bitOffset > 7) {
         throw std::out_of_range("ProcessImage: bit offset must be in [0..7]");
      }
   }
};

} // namespace undoCore