/*
 * pstate-frequency Easier control of the Intel p-state driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * For questions please contact pyamsoft at pyam(dot)soft(at)gmail(dot)com
 */

#include <iostream>
#include <sstream>

#include "psfreq_color.h"
#include "psfreq_log.h"
#include "psfreq_values.h"

namespace psfreq {

const std::string Values::BAD_READ = std::string();
const int Values::ACTION_NULL = -1;
const int Values::ACTION_SET = 1;
const int Values::ACTION_GET = 0;
const int Values::REQUESTED_CURRENT = 0;
const int Values::REQUESTED_REAL = 1;
const int Values::POWER_PLAN_NONE = -1;
const int Values::POWER_PLAN_POWERSAVE = 1;
const int Values::POWER_PLAN_PERFORMANCE = 2;
const int Values::POWER_PLAN_MAX_PERFORMANCE = 3;
const int Values::POWER_PLAN_AUTO = 0;
const int Values::AUTO_NONE = 0;
const int Values::CPUFREQ_NO_TURBO = 0;
const int Values::CPUFREQ_TURBO = 1;
const int Values::PSTATE_NO_TURBO = 1;
const int Values::PSTATE_TURBO = 0;
const int Values::TURBO_INSANE = -1;
const int Values::MIN_POSSIBLE_FREQ = 0;
const int Values::MAX_POSSIBLE_FREQ = 100;
const int Values::UNINITIALIZED = -1;
const bool Values::SLEEP = true;
const bool Values::NO_SLEEP = false;
const unsigned int Values::CUSTOM_NUMBER = 4;

bool Values::shouldSleep() const
{
        return doSleep;
}

void Values::noSleep()
{
        doSleep = NO_SLEEP;
}

bool Values::isInitialized() const
{
        return hasAction() && (max != ACTION_NULL
                        || min != ACTION_NULL
                        || turbo != ACTION_NULL
                        || governor != BAD_READ);
}

bool Values::hasAction() const
{
        return action != ACTION_NULL;
}

bool Values::hasPlan() const
{
        return plan != POWER_PLAN_NONE;
}

bool Values::isActionNull() const
{
        return action == ACTION_NULL;
}

bool Values::isActionGet() const
{
        return action == ACTION_GET;
}

bool Values::isActionSet() const
{
        return action == ACTION_SET;
}

/*
 * If the governor to set is a valid governor,
 * continue with normal operation, otherwise
 * exit the program from main.
 */
bool Values::setGovernor(const std::string& newGovernor)
{
        if (newGovernor != BAD_READ) {
                governor = newGovernor;
                return true;
        } else {
                return false;
        }
}

void Values::setAction(const int newAction)
{
        action = newAction;
}

bool Values::setMax(const int newMax)
{
        if (newMax != BAD_NUMBER) {
                max = newMax;
                return true;
        } else {
                return false;
        }
}

bool Values::setMin(const int newMin)
{
        if (newMin != BAD_NUMBER) {
                min = newMin;
                return true;
        } else {
                return false;
        }
}

bool Values::setTurbo(const int newTurbo)
{
        if (newTurbo != TURBO_INSANE) {
                turbo = newTurbo;
                return true;
        } else {
                return false;
        }
}

void Values::setRequested(const int newRequest)
{
        requested = newRequest;
}

const std::string Values::getGovernor() const
{
        return governor;
}

int Values::getAction() const
{
        return action;
}

int Values::getMax() const
{
        return max;
}

int Values::getMin() const
{
        return min;
}

int Values::getTurbo() const
{
        return turbo;
}

int Values::getRequested() const
{
        return requested;
}

/*
 * If the plan to set is a valid plan,
 * continue with normal operation, otherwise
 * exit the program from main.
 */
bool Values::setPlan(const int powerPlan) {
        if (powerPlan != POWER_PLAN_NONE) {
                plan = powerPlan;
                return true;
        } else {
                return false;
        }
}

/*
 * If the plan has not been requesed, return true and continue,
 * otherwise make sure that the plan requested does infact exist,
 * if not return false and exit the program from main.
 */
bool Values::runPlan()
{
        bool result = true;
        unsigned int autoPlan = AUTO_NONE;
        switch(plan) {
        case POWER_PLAN_NONE:
                if (Log::isDebug()) {
                        std::cout << "[Debug] no power plan" << std::endl;
                }
                break;
        case POWER_PLAN_POWERSAVE:
                if (Log::isDebug()) {
                        std::cout << "[Debug] power plan: powersave"
                                << std::endl;
                }
                setPlanPowersave();
                break;
        case POWER_PLAN_PERFORMANCE:
                if (Log::isDebug()) {
                        std::cout << "[Debug] power plan: performance"
                                << std::endl;
                }
                setPlanPerformance();
                break;
        case POWER_PLAN_MAX_PERFORMANCE:
                if (Log::isDebug()) {
                        std::cout << "[Debug] power plan: max-performance"
                                << std::endl;
                }
                setPlanMaxPerformance();
                break;
        case POWER_PLAN_AUTO:
                if (Log::isDebug()) {
                        std::cout << "[Debug] power plan: auto"
                                << std::endl;
                }
                autoPlan = setPlanAuto();
                if (autoPlan == AUTO_NONE) {
                        if (!Log::isAllQuiet()) {
                                std::cerr << Color::boldRed()
                                        << "[Error] Failed to decide an "
                                        << "automatic plan."
                                        << Color::reset()
                                        << std::endl;
                        }
                        result = false;
                        break;
                } else if (autoPlan == Cpu::POWER_SOURCE_BAT) {
                        setPlanAutoBat();
                } else {
                        setPlanAutoAC();
                }
                break;
        default:
                result = false;
        }
        if (Log::isDebug() && autoPlan == AUTO_NONE) {
                std::cout << "[Debug] autoPlan unused" << std::endl;
        }
        return result;
}

void Values::setPlanAutoAC()
{
#ifdef AUTO_POWER_PLAN_AC
        const std::string acString = std::string(AUTO_POWER_PLAN_AC);
        if (stringStartsWith(acString, "powersave")
                        || stringStartsWith(acString, "0")) {
                if (Log::isDebug()) {
                        std::cout << "[Debug] AC power plan: powersave "
                                << "from auto" << std::endl;
                }
                setPlanPowersave();
        } else if (stringStartsWith(acString, "performance")
                        || stringStartsWith(acString, "1")) {
                if (Log::isDebug()) {
                        std::cout << "[Debug] AC power plan: performance "
                                << "from auto" << std::endl;
                }
                setPlanPerformance();
        } else if (stringStartsWith(acString, "max-performance")
                        || stringStartsWith(acString, "2")) {
                if (Log::isDebug()) {
                        std::cout << "[Debug] AC power plan: max-performance "
                                << "from auto" << std::endl;
                }
                setPlanMaxPerformance();
        }
#else
        if (Log::isDebug()) {
                std::cout << "[Debug] AC power plan: default performance "
                        << "from auto" << std::endl;
        }
        setPlanPerformance();
#endif
}

void Values::setPlanCustom(const std::vector<std::string> &ac)
{
        const unsigned int items = ac.size();
        if (items != CUSTOM_NUMBER) {
                if (!Log::isAllQuiet()) {
                        std::cout << "[Debug] power plan: custom "
                                << "incorrect size. Excepted "
                                << CUSTOM_NUMBER << ", got "
                                << items << std::endl;
                        return;
                }
        }

        // assign the values
        const unsigned int newMax = stringToNumber(ac.at(0));
        const unsigned int newMin = stringToNumber(ac.at(1));
        const unsigned int newTurbo = stringToNumber(ac.at(2));
        const std::string newGov = ac.at(3);
        if (Log::isDebug()) {
                std::cout << "[Debug] power plan: custom " << std::endl
                        << "new max: " << newMax << std::endl
                        << "new min: " << newMin << std::endl
                        << "new turbo: " << newTurbo << std::endl
                        << "new gov: " << newGov << std::endl;
        }
        setMax(newMax);
        setMin(newMin);
        setTurbo(newTurbo);
        setGovernor(newGov);
}

void Values::setPlanAutoBat()
{
#ifdef AUTO_POWER_PLAN_BAT
        const std::string acString = std::string(AUTO_POWER_PLAN_BAT);
        if (stringStartsWith(acString, "powersave")
                        || stringStartsWith(acString, "0")) {
                if (Log::isDebug()) {
                        std::cout << "[Debug] BAT power plan: powersave "
                                << "from auto" << std::endl;
                }
                setPlanPowersave();
        } else if (stringStartsWith(acString, "performance")
                        || stringStartsWith(acString, "1")) {
                if (Log::isDebug()) {
                        std::cout << "[Debug] BAT power plan: performance "
                                << "from auto" << std::endl;
                }
                setPlanPerformance();
        } else if (stringStartsWith(acString, "max-performance")
                        || stringStartsWith(acString, "2")) {
                if (Log::isDebug()) {
                        std::cout << "[Debug] BAT power plan: max-performance "
                                << "from auto" << std::endl;
                }
                setPlanMaxPerformance();
        }
#else
        if (Log::isDebug()) {
                std::cout << "[Debug] BAT power plan: default powersave "
                        << "from auto" << std::endl;
        }
        setPlanPowersave();
#endif
}

void Values::setPlanPowersave()
{
#ifdef PRESET_POWER_PLAN_POWERSAVE
        const std::string preset = std::string(PRESET_POWER_PLAN_POWERSAVE);
        const std::vector<std::string> ac = splitString(preset, ' ');
        setPlanCustom(ac);
#else
        max = MIN_POSSIBLE_FREQ;
        min = MIN_POSSIBLE_FREQ;
        turbo = cpu.hasPstate() ? PSTATE_NO_TURBO : CPUFREQ_NO_TURBO;
        governor = "powersave";
#endif
}

void Values::setPlanPerformance()
{
#ifdef PRESET_POWER_PLAN_PERFORMANCE
        const std::string preset = std::string(PRESET_POWER_PLAN_PERFORMANCE);
        const std::vector<std::string> ac = splitString(preset, ' ');
        setPlanCustom(ac);
#else
        max = MAX_POSSIBLE_FREQ;
        min = MIN_POSSIBLE_FREQ;
        turbo = cpu.hasPstate() ? PSTATE_NO_TURBO : CPUFREQ_NO_TURBO;
        governor = cpu.hasPstate() ? "powersave" : "ondemand";
#endif
}

void Values::setPlanMaxPerformance()
{
#ifdef PRESET_POWER_PLAN_MAX_PERFORMANCE
        const std::string preset = std::string(PRESET_POWER_PLAN_MAX_PERFORMANCE);
        const std::vector<std::string> ac = splitString(preset, ' ');
        setPlanCustom(ac);
#else
        max = MAX_POSSIBLE_FREQ;
        min = MAX_POSSIBLE_FREQ;
        turbo = cpu.hasPstate() ? PSTATE_TURBO : CPUFREQ_TURBO;
        governor = "performance";
#endif
}

/*
 * When requested, this plan will search the power_supply sysfs to
 * find which interface is considered the 'Mains' type adapter.
 * Once found, it will set a power plan based on the online status
 * on the 'Mains' adapter.
 */
unsigned int Values::setPlanAuto()
{
        const char *const dirName = "/sys/class/power_supply/";
        DIR *const directory = opendir(dirName);
        if (Log::isDebug()) {
                std::cout << "[Debug] open dir: " << dirName << std::endl;
        }
        if (!directory) {
                if (!Log::isAllQuiet()) {
                        std::cerr << Color::boldRed()
                                << "[Error] Could not open directory: "
                                << dirName << Color::reset()
                                << std::endl;
                }
                return AUTO_NONE;
        } else {
                unsigned int result = getPowerSourceDirectory(directory,
                                                              dirName);
                if (closedir(directory)) {
                        if (!Log::isAllQuiet()) {
                                std::cerr << Color::boldRed()
                                        << "[Error] Failed to close "
                                        << "directory: " << dirName
                                        << Color::reset()
                                        << std::endl;
                        }
                        return AUTO_NONE;
                } else {
                        return result;
                }
        }
}

unsigned int Values::getPowerSourceDirectory(DIR *const directory,
                const char *const dirName)
{
        struct dirent *entry =  readdir(directory);
        unsigned int result = AUTO_NONE;
        while(entry) {
                const std::string entryName = entry->d_name;
                if (!hideDirectory(entryName)) {
                        std::ostringstream oss;
                        oss << dirName << entryName << "/";
                        const std::string fullPath = oss.str();
                        if (Log::isDebug()) {
                                std::cout << "[Debug] power supply "
                                        << "path: '" << fullPath
                                        << "'" << std::endl;
                        }
                        if (fullPath.length() < fullPath.max_size()) {
                                result = cpu.getPowerSupply(fullPath);
                                if (Log::isDebug()) {
                                        std::cout << "[Debug] power supply "
                                                << "result: '" << result
                                                << "'" << std::endl;
                                }
                                if (result > AUTO_NONE) {
                                        if (Log::isDebug()) {
                                                std::cout << "[Debug] power "
                                                        << "supply result is "
                                                        << "valid. Break."
                                                        << std::endl;
                                        }
                                        break;
                                }
                        } else {
                                if (!Log::isAllQuiet()) {
                                        std::cerr << Color::boldRed()
                                                << "[Error] Path is larger "
                                                << "than max allowed."
                                                << Color::reset()
                                                << std::endl;
                                }
                                /*
                                 * Breaking here is safe, as the result should
                                 * still be equivalent to AUTO_NONE, meaning
                                 * that the final return will still work
                                 */
                                break;
                        }
                }
                if (Log::isDebug()) {
                        std::cout << "[Debug] read the next dir in directory"
                                << std::endl;
                }
                entry = readdir(directory);
        }
        return result;
}


/*
 * Return a boolean to decide whether to display the directory entry
 * based on whether or not the entry is the current . or parent .. directory.
 */
bool Values::hideDirectory(const std::string &entryName)
{
        if (Log::isDebug()) {
                std::cout << "[Debug] entryName: '" << entryName
                        << "'" << std::endl;
        }
        return (entryName.compare("..") == 0 || entryName.compare(".") == 0);
}

}

