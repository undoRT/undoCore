/**
 * @file ioBus.hpp
 * @brief Abstract interface for fieldbus masters in the undoRT ecosystem
 * @author Salvatore Bamundo
 * @date June 2026
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: Copyright (c) 2026 undoRT
 */

#pragma once

#include <cstdint>

namespace undoCore {

/**
 * @brief Fieldbus cycle synchronization protocol
 *
 * Implemented by undoBUS (EtherCAT, Profibus, simulated, etc.)
 * Consumed by undoPLC MasterTask to drive the execution cycle.
 * 
 * It defines the contract between undoPLC (consumer) and any
 * fieldbus master implementation undoBUS (producer).
 *
 * The synchronization protocol is:
 *
 *   [Bus]  cycleStart()  → copyIn() → notifyPlc()
 *   [PLC]                                          → execute() → notifyBus()
 *   [Bus]  copyOut() → sendFrame()
 *
 * undoPLC calls waitCycle() to block until the bus signals a new cycle.
 * undoPLC calls notifyDone() when all tasks have completed execution.
 * The bus drives timing — PLC is always the follower.
 */
class IoBus
{
public:
   virtual ~IoBus() = default;

   // Lifecycle

   /**
   * @brief Initialize and start the fieldbus master.
   * After start(), the bus begins generating cycles.
   * @return true if started successfully
   */
   virtual bool start() = 0;

   /**
   * @brief Stop the fieldbus master gracefully.
   * Must be safe to call from a signal handler context.
   */
   virtual void stop() = 0;

   /**
   * @brief Query whether the bus is currently running.
   */
   virtual bool isRunning() const = 0;

   // Synchronization — called by undoPLC MasterTask

   /**
   * @brief Block until the bus signals the start of a new PLC cycle.
   *
   * The implementation MUST call processImage.copyIn()
   * before unblocking this call. When waitCycle() returns, plcInputs
   * are already frozen and safe to read by PLC tasks.
   *
   * @param timeoutUs Watchdog timeout in microseconds. 0 = infinite.
   * @return true if cycle arrived within timeout, false otherwise.
   */
   virtual bool waitCycle(uint32_t timeoutUs = 0) = 0;

   /**
   * @brief Notify the bus that PLC execution has completed for this cycle.
   *
   * The bus will proceed to copyOut() and send the EtherCAT frame
   * only after this call. Must be called exactly once per waitCycle().
   */
   virtual void notifyDone() = 0;

   // Diagnostics

   /**
   * @brief Cycle counter since start().
   * Monotonically increasing. Useful for watchdog and diagnostics.
   */
   virtual uint64_t cycleCount() const = 0;

   /**
   * @brief Last measured cycle time in microseconds.
   * Reflects actual bus cycle, not the configured nominal period.
   */
   virtual uint32_t lastCycleTimeUs() const = 0;

   /**
   * @brief Configured nominal cycle period in microseconds.
   * Set at construction time from ENI/configuration.
   */
   virtual uint32_t nominalCycleTimeUs() const = 0;

   /**
    * @brief Returns the last code error if a method goes wrong, 0 if no error detected.
    * @return The error code if an error is present, otherwise 0.
    */
   virtual int32_t lastError() const = 0;

   // Non-copyable, non-movable
   // Fieldbus masters own hardware resources — copying makes no sense

   IoBus(const IoBus&) = delete;
   IoBus& operator=(const IoBus&) = delete;
   IoBus(IoBus&&) = delete;
   IoBus& operator=(IoBus&&) = delete;

protected:
   IoBus() = default;
};

} // namespace undoCore