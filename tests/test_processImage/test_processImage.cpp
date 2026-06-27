/**
 * @file test_processImage.cpp
 * @brief Unit tests for undoCore::ProcessImage
 * @author Salvatore Bamundo
 * @date June 2026
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: Copyright (c) 2026 undoRT
 */

#include <iostream>
#include <functional>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdint>

#include "undoCore/undoCore.hpp"

// ============================================================================
// Minimal test runner — zero external dependencies
// ============================================================================

struct TestResult
{
   std::string name;
   bool passed;
   std::string failMessage;
};

static std::vector<TestResult> g_results;

#define TEST(name, body) \
   g_results.push_back([&]() -> TestResult { \
      TestResult r{name, true, ""}; \
      try { \
         body \
      } catch (const std::exception& e) { \
         r.passed = false; \
         r.failMessage = std::string("Exception: ") + e.what(); \
      } catch (...) { \
         r.passed = false; \
         r.failMessage = "Unknown exception"; \
      } \
      return r; \
   }())

#define EXPECT_EQ(a, b) \
   if ((a) != (b)) { \
      r.passed = false; \
      r.failMessage = std::string("EXPECT_EQ failed at line ") + std::to_string(__LINE__) + ": expected " + std::to_string(b) + ", got " \
                      + std::to_string(a); \
      return r; \
   }

#define EXPECT_TRUE(expr) \
   if (!(expr)) { \
      r.passed = false; \
      r.failMessage = std::string("EXPECT_TRUE failed at line ") + std::to_string(__LINE__) + ": " #expr; \
      return r; \
   }

#define EXPECT_FALSE(expr) \
   if ((expr)) { \
      r.passed = false; \
      r.failMessage = std::string("EXPECT_FALSE failed at line ") + std::to_string(__LINE__) + ": " #expr; \
      return r; \
   }

#define EXPECT_THROW(expr, exc_type) \
   { \
      bool threw = false; \
      try { \
         (void) (expr); \
      } catch (const exc_type&) { \
         threw = true; \
      } catch (...) { \
      } \
      if (!threw) { \
         r.passed = false; \
         r.failMessage = std::string("EXPECT_THROW: ") + #expr + " did not throw " #exc_type + " at line " + std::to_string(__LINE__); \
         return r; \
      } \
   }

// ============================================================================
// Fixtures — PI sizes representative of a small EtherCAT network
// ============================================================================

// 16 input bytes, 16 output bytes, 32 marker bytes
using SmallPI = undoCore::ProcessImage<16, 16, 32>;

// Minimal PI without markers
using NoMarkerPI = undoCore::ProcessImage<8, 8>;

// ============================================================================
// Tests
// ============================================================================

int main()
{
   // ------------------------------------------------------------------------
   // 1. Static size constants
   // ------------------------------------------------------------------------
   TEST("static_constants", {
      EXPECT_EQ(SmallPI::inputBytes, 16u);
      EXPECT_EQ(SmallPI::outputBytes, 16u);
      EXPECT_EQ(SmallPI::markerBytes, 32u);

      EXPECT_EQ(NoMarkerPI::inputBytes, 8u);
      EXPECT_EQ(NoMarkerPI::outputBytes, 8u);
      EXPECT_EQ(NoMarkerPI::markerBytes, 0u);
   });

   // ------------------------------------------------------------------------
   // 2. Zero-initialization on construction
   // ------------------------------------------------------------------------
   TEST("zero_init_inputs", {
      SmallPI pi;
      for (size_t i = 0; i < 16; ++i) {
         EXPECT_EQ(pi.readInputByte(i), 0u);
      }
   });

   TEST("zero_init_outputs", {
      SmallPI pi;
      for (size_t i = 0; i < 16; ++i) {
         EXPECT_EQ(pi.readOutputByte(i), 0u);
      }
   });

   TEST("zero_init_markers", {
      SmallPI pi;
      for (size_t i = 0; i < 32; ++i) {
         EXPECT_EQ(pi.readMarkerByte(i), 0u);
      }
   });

   // ------------------------------------------------------------------------
   // 3. copyIn: busInputs → plcInputs
   // ------------------------------------------------------------------------
   TEST("copyIn_transfers_data", {
      SmallPI pi;

      // Write known pattern directly into bus-side buffer
      uint8_t* bus = pi.busInputPtr();
      for (size_t i = 0; i < 16; ++i) {
         bus[i] = static_cast<uint8_t>(i + 1); // 1..16
      }

      // Before copyIn, PLC-side must still see zeros
      EXPECT_EQ(pi.readInputByte(0), 0u);

      pi.copyIn();

      // After copyIn, PLC-side must see the written values
      for (size_t i = 0; i < 16; ++i) {
         EXPECT_EQ(pi.readInputByte(i), static_cast<uint8_t>(i + 1));
      }
   });

   TEST("copyIn_does_not_modify_bus_buffer", {
      SmallPI pi;
      uint8_t* bus = pi.busInputPtr();
      bus[0] = 0xAB;
      pi.copyIn();
      // Bus buffer must be untouched
      EXPECT_EQ(bus[0], 0xABu);
   });

   // ------------------------------------------------------------------------
   // 4. copyOut: plcOutputs → busOutputs
   // ------------------------------------------------------------------------
   TEST("copyOut_transfers_data", {
      SmallPI pi;

      // PLC writes outputs
      for (size_t i = 0; i < 16; ++i) {
         pi.writeOutputByte(i, static_cast<uint8_t>(0x10 + i));
      }

      // Before copyOut, bus-side must see zeros
      const uint8_t* busOut = pi.busOutputPtr();
      EXPECT_EQ(busOut[0], 0u);

      pi.copyOut();

      // After copyOut, bus-side sees PLC outputs
      for (size_t i = 0; i < 16; ++i) {
         EXPECT_EQ(busOut[i], static_cast<uint8_t>(0x10 + i));
      }
   });

   TEST("copyOut_does_not_modify_plc_outputs", {
      SmallPI pi;
      pi.writeOutputByte(3, 0x77);
      pi.copyOut();
      EXPECT_EQ(pi.readOutputByte(3), 0x77u);
   });

   // ------------------------------------------------------------------------
   // 5. Input bit accessors
   // ------------------------------------------------------------------------
   TEST("readInputBit_all_bits", {
      SmallPI pi;
      uint8_t* bus = pi.busInputPtr();
      bus[0] = 0b10110101; // bits: 7=1 6=0 5=1 4=1 3=0 2=1 1=0 0=1
      pi.copyIn();

      EXPECT_TRUE(pi.readInputBit(0, 0));
      EXPECT_FALSE(pi.readInputBit(0, 1));
      EXPECT_TRUE(pi.readInputBit(0, 2));
      EXPECT_FALSE(pi.readInputBit(0, 3));
      EXPECT_TRUE(pi.readInputBit(0, 4));
      EXPECT_TRUE(pi.readInputBit(0, 5));
      EXPECT_FALSE(pi.readInputBit(0, 6));
      EXPECT_TRUE(pi.readInputBit(0, 7));
   });

   TEST("readInputBit_isolation", {
      // Setting byte 1 must not affect reads on byte 0
      SmallPI pi;
      uint8_t* bus = pi.busInputPtr();
      bus[0] = 0x00;
      bus[1] = 0xFF;
      pi.copyIn();

      for (uint8_t b = 0; b < 8; ++b) {
         EXPECT_FALSE(pi.readInputBit(0, b));
      }
      for (uint8_t b = 0; b < 8; ++b) {
         EXPECT_TRUE(pi.readInputBit(1, b));
      }
   });

   // ------------------------------------------------------------------------
   // 6. Input multi-byte accessors
   // ------------------------------------------------------------------------
   TEST("readInputWord_little_endian", {
      SmallPI pi;
      uint8_t* bus = pi.busInputPtr();
      bus[2] = 0x34;
      bus[3] = 0x12;
      pi.copyIn();

      // Little-endian: low byte at lower address → 0x1234
      EXPECT_EQ(pi.readInputWord(2), static_cast<undoCore::WORD>(0x1234));
   });

   TEST("readInputDword_little_endian", {
      SmallPI pi;
      uint8_t* bus = pi.busInputPtr();
      bus[4] = 0x78;
      bus[5] = 0x56;
      bus[6] = 0x34;
      bus[7] = 0x12;
      pi.copyIn();

      EXPECT_EQ(pi.readInputDword(4), 0x12345678u);
   });

   TEST("readInputLword_little_endian", {
      SmallPI pi;
      uint8_t* bus = pi.busInputPtr();
      bus[0] = 0x08;
      bus[1] = 0x07;
      bus[2] = 0x06;
      bus[3] = 0x05;
      bus[4] = 0x04;
      bus[5] = 0x03;
      bus[6] = 0x02;
      bus[7] = 0x01;
      pi.copyIn();

      EXPECT_EQ(pi.readInputLword(0), 0x0102030405060708ULL);
   });

   // ------------------------------------------------------------------------
   // 7. Output bit accessors
   // ------------------------------------------------------------------------
   TEST("writeOutputBit_set", {
      SmallPI pi;
      pi.writeOutputBit(0, 3, true);
      EXPECT_TRUE(pi.readOutputBit(0, 3));
      // Adjacent bits must be untouched
      EXPECT_FALSE(pi.readOutputBit(0, 2));
      EXPECT_FALSE(pi.readOutputBit(0, 4));
   });

   TEST("writeOutputBit_clear", {
      SmallPI pi;
      pi.writeOutputByte(0, 0xFF);
      pi.writeOutputBit(0, 5, false);
      EXPECT_FALSE(pi.readOutputBit(0, 5));
      // Other bits must remain set
      EXPECT_TRUE(pi.readOutputBit(0, 4));
      EXPECT_TRUE(pi.readOutputBit(0, 6));
   });

   TEST("writeOutputBit_idempotent_set", {
      SmallPI pi;
      pi.writeOutputBit(1, 2, true);
      pi.writeOutputBit(1, 2, true); // second set must not corrupt byte
      EXPECT_EQ(pi.readOutputByte(1), 0x04u);
   });

   TEST("writeOutputBit_idempotent_clear", {
      SmallPI pi;
      pi.writeOutputByte(1, 0xFF);
      pi.writeOutputBit(1, 2, false);
      pi.writeOutputBit(1, 2, false);
      EXPECT_EQ(pi.readOutputByte(1), 0xFBu);
   });

   // ------------------------------------------------------------------------
   // 8. Output multi-byte accessors
   // ------------------------------------------------------------------------
   TEST("writeReadOutputByte", {
      SmallPI pi;
      pi.writeOutputByte(7, 0xDE);
      EXPECT_EQ(pi.readOutputByte(7), 0xDEu);
   });

   TEST("writeReadOutputWord", {
      SmallPI pi;
      pi.writeOutputWord(2, 0xBEEF);
      EXPECT_EQ(pi.readOutputWord(2), 0xBEEFu);
   });

   TEST("writeReadOutputDword", {
      SmallPI pi;
      pi.writeOutputDword(4, 0xDEADBEEFu);
      EXPECT_EQ(pi.readOutputDword(4), 0xDEADBEEFu);
   });

   TEST("writeReadOutputLword", {
      SmallPI pi;
      pi.writeOutputLword(0, 0xCAFEBABEDEADBEEFULL);
      EXPECT_EQ(pi.readOutputLword(0), 0xCAFEBABEDEADBEEFULL);
   });

   // ------------------------------------------------------------------------
   // 9. Marker accessors
   // ------------------------------------------------------------------------
   TEST("writeReadMarkerByte", {
      SmallPI pi;
      pi.writeMarkerByte(10, 0x42);
      EXPECT_EQ(pi.readMarkerByte(10), 0x42u);
   });

   TEST("writeMarkerBit_set_clear", {
      SmallPI pi;
      pi.writeMarkerBit(0, 6, true);
      EXPECT_TRUE(pi.readMarkerBit(0, 6));
      pi.writeMarkerBit(0, 6, false);
      EXPECT_FALSE(pi.readMarkerBit(0, 6));
   });

   // ------------------------------------------------------------------------
   // 10. Isolation between areas
   //     Writing inputs must not affect outputs or markers, and vice versa
   // ------------------------------------------------------------------------
   TEST("areas_are_independent", {
      SmallPI pi;

      // Flood bus inputs
      uint8_t* bus = pi.busInputPtr();
      for (size_t i = 0; i < 16; ++i) {
         bus[i] = 0xFF;
      }
      pi.copyIn();

      // Outputs and markers must still be zero
      for (size_t i = 0; i < 16; ++i) {
         EXPECT_EQ(pi.readOutputByte(i), 0u);
      }
      for (size_t i = 0; i < 32; ++i) {
         EXPECT_EQ(pi.readMarkerByte(i), 0u);
      }

      // Flood outputs
      for (size_t i = 0; i < 16; ++i) {
         pi.writeOutputByte(i, 0xFF);
      }
      pi.copyOut();

      // Inputs and markers must still be unaffected
      for (size_t i = 0; i < 16; ++i) {
         EXPECT_EQ(pi.readInputByte(i), 0xFFu); // from copyIn above
      }
      for (size_t i = 0; i < 32; ++i) {
         EXPECT_EQ(pi.readMarkerByte(i), 0u);
      }
   });

   // ------------------------------------------------------------------------
   // 11. Bounds checking
   // ------------------------------------------------------------------------
   TEST("bounds_input_byte_out_of_range", {
      SmallPI pi;
      pi.copyIn();
      EXPECT_THROW(pi.readInputByte(16), std::out_of_range);
   });

   TEST("bounds_output_byte_out_of_range", {
      SmallPI pi;
      EXPECT_THROW(pi.writeOutputByte(16, 0xFF), std::out_of_range);
      EXPECT_THROW(pi.readOutputByte(16), std::out_of_range);
   });

   TEST("bounds_marker_byte_out_of_range", {
      SmallPI pi;
      EXPECT_THROW(pi.writeMarkerByte(32, 0xFF), std::out_of_range);
      EXPECT_THROW(pi.readMarkerByte(32), std::out_of_range);
   });

   TEST("bounds_bit_offset_out_of_range", {
      SmallPI pi;
      pi.copyIn();
      EXPECT_THROW(pi.readInputBit(0, 8), std::out_of_range);
      EXPECT_THROW(pi.writeOutputBit(0, 8, true), std::out_of_range);
   });

   TEST("bounds_word_straddle_end", {
      // Reading a WORD at last valid byte must throw (would straddle boundary)
      SmallPI pi;
      pi.copyIn();
      EXPECT_THROW(pi.readInputWord(15), std::out_of_range);
   });

   TEST("bounds_dword_straddle_end", {
      SmallPI pi;
      pi.copyIn();
      EXPECT_THROW(pi.readInputDword(13), std::out_of_range);
   });

   // ------------------------------------------------------------------------
   // 12. NoMarkerPI: marker area disabled
   //     static_assert prevents compilation if markers are called,
   //     so we only verify the PI works correctly without them.
   // ------------------------------------------------------------------------
   TEST("no_marker_pi_inputs_outputs", {
      NoMarkerPI pi;
      uint8_t* bus = pi.busInputPtr();
      bus[0] = 0xAB;
      pi.copyIn();
      EXPECT_EQ(pi.readInputByte(0), 0xABu);

      pi.writeOutputByte(0, 0xCD);
      pi.copyOut();
      EXPECT_EQ(pi.busOutputPtr()[0], 0xCDu);
   });

   // =========================================================================
   // Results
   // =========================================================================

   int passed = 0, failed = 0;
   std::cout << "\n=== undoCore::ProcessImage test results ===\n\n";

   for (const auto& r : g_results) {
      if (r.passed) {
         std::cout << "  [PASS] " << r.name << "\n";
         ++passed;
      } else {
         std::cout << "  [FAIL] " << r.name << "\n"
                   << "         " << r.failMessage << "\n";
         ++failed;
      }
   }

   std::cout << "\n"
             << "  Passed: " << passed << "\n"
             << "  Failed: " << failed << "\n"
             << "  Total:  " << (passed + failed) << "\n\n";

   return failed > 0 ? 1 : 0;
}