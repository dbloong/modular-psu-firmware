/*
 * EEZ Modular Firmware
 * Copyright (C) 2015-present, Envox d.o.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <eez/modules/psu/temperature.h>
#include <eez/modules/psu/io_pins.h>
#include <eez/modules/psu/sd_card.h>

#define PROFILE_EXT ".profile"

namespace eez {
namespace psu {
/// PSU configuration profiles (save, recall, ...).
namespace profile {

#define MAX_CHANNEL_PARAMETERS_SIZE 200

/// Channel parameters stored in profile.
struct ChannelParameters {
    uint16_t moduleType;
    uint16_t moduleRevision;
    bool parametersAreValid;
    uint32_t parameters[MAX_CHANNEL_PARAMETERS_SIZE  / 4];
};

/// Channel binary flags stored in profile.
struct ProfileFlags {
    unsigned isValid: 1;
    unsigned powerIsUp: 1;
    unsigned couplingType : 3;
    unsigned triggerContinuousInitializationEnabled: 1;
    unsigned reserverd : 10;
};

enum LoadStatus {
    LOAD_STATUS_ONLY_NAME,
    LOAD_STATUS_LOADING,
    LOAD_STATUS_LOADED
};

/// Profile parameters.
struct Parameters {
    LoadStatus loadStatus;
    ProfileFlags flags;
    char name[PROFILE_NAME_MAX_LENGTH + 1];
    ChannelParameters channels[CH_MAX];
    temperature::ProtectionConfiguration tempProt[temp_sensor::MAX_NUM_TEMP_SENSORS];
    uint16_t triggerSource;
    float triggerDelay;
    io_pins::IOPin ioPins[4];
};

void init();
void tick();

void onAfterSdCardMounted();

void shutdownSave();

Parameters *getProfileParameters(int location);

enum {
    RECALL_OPTION_FORCE_DISABLE_OUTPUT = 0x01,
    RECALL_OPTION_IGNORE_POWER = 0x02
};

bool recallFromLocation(int location);
bool recallFromLocation(int location, int recallOptions, bool showProgress, int *err);
bool recallFromFile(const char *filePath, int recallOptions, bool showProgress, int *err);

void recallStateFromPsuThread();

bool saveToLocation(int location);
bool saveToLocation(int location, const char *name, bool showProgress, int *err);
bool saveToFile(const char *filePath, bool showProgress, int *err);

bool importFileToLocation(const char *filePath, int location, bool showProgress, int *err);
bool exportLocationToFile(int location, const char *filePath, bool showProgress, int *err);

bool deleteLocation(int location, bool showProgress, int *err);
bool deleteAllLocations(int *err);

bool isLoaded(int location);
bool isValid(int location);

void getSaveName(int location, char *name);

bool setName(int location, const char *name, bool showProgress, int *err);
void getName(int location, char *name, int count);

bool getFreezeState();
void setFreezeState(bool value);

void loadProfileParametersToCache(int location);

class WriteContext {
public:
    WriteContext(File &file_);

    bool group(const char *groupName);
    bool group(const char *groupNamePrefix, unsigned int index);

    bool property(const char *propertyName, int value);
    bool property(const char *propertyName, unsigned int value);
    bool property(const char *propertyName, float value);
    bool property(const char *propertyName, const char *str);
    bool property(
        const char *propertyName,
        float *dwellList, uint16_t &dwellListLength,
        float *voltageList, uint16_t &voltageListLength,
        float *currentList, uint16_t &currentListLength
        );

    bool flush();

private:
    sd_card::BufferedFileWrite file;
};

#define WRITE_GROUP(p) if (!ctx.group(name)) return false
#define WRITE_PROPERTY(p1, p2) if (!ctx.property(p1, p2)) return false
#define WRITE_LIST_PROPERTY(p1, p2, p3, p4, p5, p6, p7) if (!ctx.property(p1, p2, p3, p4, p5, p6, p7)) return false

class ReadContext {
public:
    ReadContext(File &file_);

    bool doRead(bool(*callback)(ReadContext &ctx, Parameters &parameters, List *lists), Parameters &parameters, List *lists, int options, bool showProgress);

    bool matchGroup(const char *groupName);
    bool matchGroup(const char *groupNamePrefix, int &index);

    bool property(const char *name, unsigned int &value);
    bool property(const char *name, uint16_t &value);
    bool property(const char *name, bool &value);

    bool property(const char *name, float &value);
    bool property(const char *name, char *str, unsigned int strLength);

    bool listProperty(const char *name, int channelIndex, List *lists);

    void skipPropertyValue();

    bool result;

private:
    sd_card::BufferedFileRead file;
    char groupName[100];
    char propertyName[100];
};

#define READ_FLAG(name, value) \
    auto name = value; \
    if (ctx.property(#name, name)) { \
        value = name; \
        return true; \
    }

#define READ_PROPERTY(name, value) \
    if (ctx.property(#name, value)) { \
        return true; \
    }

#define READ_STRING_PROPERTY(name, str, strLength) \
    if (ctx.property(#name, str, strLength)) { \
        return true; \
    }

#define READ_LIST_PROPERTY(name, channelIndex, lists) \
    if (ctx.listProperty(#name, channelIndex, lists)) { \
        return true; \
    }

#define SKIP_PROPERTY(name) \
    if (ctx.skipProperty(#name)) { \
        return true; \
    }

}
}
} // namespace eez::psu::profile
