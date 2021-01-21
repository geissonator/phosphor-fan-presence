/**
 * Copyright © 2021 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include "power_state.hpp"
#include "types.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

namespace sensor::monitor
{

/**
 * @class ShutdownAlarmMonitor
 *
 * This class finds all instances of the D-Bus interfaces
 *   xyz.openbmc_project.Sensor.Threshold.SoftShutdown
 *   xyz.openbmc_project.Sensor.Threshold.HardShutdown
 *
 * and then watches the high and low alarm properties.  If they trip
 * the code will start a timer, at the end of which the system will
 * be shut down.  The timer values can be modified with configure.ac
 * options.  If the alarm is cleared before the timer expires, then
 * the timer is stopped.
 *
 * Event logs are also created when the alarms trip and clear.
 *
 * Note that the SoftShutdown alarm code actually implements a hard shutdown.
 * This is because in the system this is being written for, the host is
 * driving the shutdown process (i.e. doing a soft shutdown) based on an alert
 * it receives via another channel.  If the soft shutdown timer expires, it
 * means that the host didn't do a soft shutdown in the time allowed and
 * now a hard shutdown is required.  This behavior could be modified with
 * compile flags if anyone needs a different behavior in the future.
 *
 * It currently uses the PGoodState class to check for power state.
 * If a different property is ever desired, a new class can be
 * derived from PowerState and a compile option can be used.
 *
 */
class ShutdownAlarmMonitor
{
  public:
    ShutdownAlarmMonitor() = delete;
    ~ShutdownAlarmMonitor() = default;
    ShutdownAlarmMonitor(const ShutdownAlarmMonitor&) = delete;
    ShutdownAlarmMonitor& operator=(const ShutdownAlarmMonitor&) = delete;
    ShutdownAlarmMonitor(ShutdownAlarmMonitor&&) = delete;
    ShutdownAlarmMonitor& operator=(ShutdownAlarmMonitor&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] bus - The sdbusplus bus object
     * @param[in] event - The sdeventplus event object
     */
    ShutdownAlarmMonitor(sdbusplus::bus::bus& bus, sdeventplus::Event& event);

  private:
    /**
     * @brief The propertiesChanged handled for the shutdown interfaces.
     *
     * If the power is on, the new alarm values will be checked to see
     * if the shutdown timer needs to be started or stopped.
     */
    void propertiesChanged(sdbusplus::message::message& message);

    /**
     * @brief Checks an alarm value to see if a shutdown timer needs
     *        to be started or stopped.
     *
     * If the alarm is on and the timer isn't running, start it.
     * If the alarm is off and the timer is running, stop it.
     *
     * @param[in] value - The alarm property value
     * @param[in] alarmKey - The alarm key
     */
    void checkAlarm(bool value, const AlarmKey& alarmKey);

    /**
     * @brief Checks all currently known alarm properties on D-Bus.
     *
     * May result in starting or stopping shutdown timers.
     */
    void checkAlarms();

    /**
     * @brief Finds all shutdown alarm interfaces currently on
     *        D-Bus and adds them to the alarms map.
     */
    void findAlarms();

    /**
     * @brief The power state changed handler.
     *
     * Checks alarms when power is turned on, and clears
     * any running timers on a power off.
     *
     * @param[in] powerStateOn - If the power is now on or off.
     */
    void powerStateChanged(bool powerStateOn);

    /**
     * @brief The sdbusplus bus object
     */
    sdbusplus::bus::bus& bus;

    /**
     * @brief The sdeventplus Event object
     */
    sdeventplus::Event& event;

    /**
     * @brief The match for properties changing on the HardShutdown
     *        interface.
     */
    sdbusplus::bus::match::match hardShutdownMatch;

    /**
     * @brief The match for properties changing on the SoftShutdown
     *        interface.
     */
    sdbusplus::bus::match::match softShutdownMatch;

    /**
     * @brief The PowerState object to track power state changes.
     */
    std::unique_ptr<phosphor::fan::PowerState> _powerState;

    /**
     * @brief The map of alarms.
     */
    std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                           sdeventplus::ClockId::Monotonic>>>
        alarms;
};

} // namespace sensor::monitor
