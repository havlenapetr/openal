/**
 * OpenAL cross platform audio library
 * Copyright (C) 2010 by Chris Robinson
 * Copyright (C) 2011 by Havlena Petr
 *
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "OpenAl"
#include <utils/Log.h>

#include "config.h"

#include <stdlib.h>
#include "alMain.h"
#include "AL/al.h"
#include "AL/alc.h"

#include <AudioTrack.h>

using namespace android;

#define RETURN_IF(value)                                \
    if(value != NO_ERROR) {                             \
        LOGE("ERR[%i] in %s line %d",                   \
            value, __func__, __LINE__);                 \
        return value;                                   \
    }

#define RETURN_FALSE_IF(value)                          \
    if(value != NO_ERROR) {                             \
        LOGE("ERR[%i] in %s line %d",                   \
            value, __func__, __LINE__);                 \
        return ALC_FALSE;                               \
    }

#define RETURN_FALSE_IF_NULL(value)                     \
    if(!value) {                                        \
        LOGE("ERR NULL in %s line %d",                  \
            __func__, __LINE__);                        \
        return ALC_FALSE;                               \
    }

#define CHECK_NULL(value)                               \
    if(!value) {                                        \
        LOGE("ERR NULL in %s line %d !!",               \
            __func__, __LINE__);                        \
    }

#define LOG_FUNC_START                                  \
    LOGV("%s - start", __func__);

#define LOG_FUNC_END                                    \
    LOGV("%s - end", __func__);

static const ALCchar android_device[] = "Android Default";

class AAudioBuffer {
public:
    AAudioBuffer(size_t size)
        : mData(malloc(size)),
          mSize(size) {
    }

    ~AAudioBuffer() {
        if(mData) {
            free(mData);
        }
        mSize = 0;
    }

    void* data() {
        return mData;
    }

    size_t size() {
        return mSize;
    }

private:
    void*       mData;
    size_t      mSize;
};

class APlaybackDevice {
public:
    APlaybackDevice(ALCdevice* device);
    ~APlaybackDevice();

    void            stop();
    ALCboolean      reset();
    void            close();
    bool            isPlaying();

private:
    status_t        open();
    bool            write(AAudioBuffer* buffer);
    int             handlePlayback();
    static ALuint   onPlayback(ALvoid *ptr);

    ALCdevice*      mDevice;
    AAudioBuffer*   mBuffer;
    AudioTrack      mAudioTrack;
    ALvoid*         mPlaybackThread;
    bool            mPlaybackEnabled;
};

APlaybackDevice::APlaybackDevice(ALCdevice* device)
    : mDevice(device),
      mBuffer(NULL),
      mPlaybackThread(NULL),
      mPlaybackEnabled(false)
{
}

APlaybackDevice::~APlaybackDevice()
{
    LOG_FUNC_START

    close();

    LOG_FUNC_END
}

bool APlaybackDevice::isPlaying()
{
    return mPlaybackEnabled && !mAudioTrack.stopped();
}

/* static */
ALuint APlaybackDevice::onPlayback(ALvoid *ptr)
{    
    SetRTPriority();

    APlaybackDevice* dev = (APlaybackDevice *) ptr;
    dev->mPlaybackEnabled = true;
    return dev->handlePlayback();
}

int APlaybackDevice::handlePlayback()
{
    int bufferSizeInSamples;

    bufferSizeInSamples = mBuffer->size() / aluFrameSizeFromFormat(mDevice->Format);

    mAudioTrack.start();

    while(mPlaybackEnabled) {
        aluMixData(mDevice, mBuffer->data(), bufferSizeInSamples);
        if(!write(mBuffer)) {
            LOGE("Can't write audio buffer into audio track");
            mPlaybackEnabled = false;
        }
    }

    mAudioTrack.stop();
    mAudioTrack.flush();
    return 0;
}

bool APlaybackDevice::write(AAudioBuffer* buffer)
{
    ssize_t length, size;

    length = 0;
    while(length < buffer->size()) {
        size = mAudioTrack.write(buffer->data() + length, buffer->size() - length);
        if(size < 0) {
            return false;
        }
        length += size;
    }
    return true;
}

status_t APlaybackDevice::open()
{
    status_t err;
    int sampleRateInHz;
    int channelConfig;
    int audioFormat;
    int bufferSizeInBytes;

    LOG_FUNC_START

    sampleRateInHz = mDevice->Frequency;
    channelConfig = aluChannelsFromFormat(mDevice->Format) == 1 ?
        AUDIO_CHANNEL_OUT_MONO : AUDIO_CHANNEL_OUT_STEREO;

    audioFormat = aluBytesFromFormat(mDevice->Format) == 1 ?
        AUDIO_FORMAT_PCM_8_BIT : AUDIO_FORMAT_PCM_16_BIT;

    err = AudioTrack::getMinFrameCount(&bufferSizeInBytes, audioFormat, sampleRateInHz);
    RETURN_IF(err);

    LOGV("rate(%i), channel(%i), format(%i), buffSize(%i), numUpdates(%i)",
        sampleRateInHz, channelConfig, audioFormat, bufferSizeInBytes, mDevice->NumUpdates);

    err = mAudioTrack.set(AUDIO_STREAM_MUSIC,
                          sampleRateInHz,
                          audioFormat,
                          channelConfig,
                          bufferSizeInBytes,     // frameCount
                          0,                     // flags
                          0, 0);                 // callback, callback data (user)
    RETURN_IF(err);

    err = mAudioTrack.initCheck();
    RETURN_IF(err);

    if(mBuffer) {
        delete mBuffer;
    }
    mBuffer = new AAudioBuffer(bufferSizeInBytes);

    LOG_FUNC_END

    return NO_ERROR;
}

void APlaybackDevice::stop()
{
    LOG_FUNC_START

    if(!isPlaying()) {
        LOGI("Stop called, but device isn't playing!");
        return;
    }

    mPlaybackEnabled = false;
    StopThread(mPlaybackThread);
    mPlaybackThread = NULL;

    LOG_FUNC_END
}

ALCboolean APlaybackDevice::reset()
{
    ALCboolean err;

    LOG_FUNC_START

    if (aluChannelsFromFormat(mDevice->Format) >= 2) {
        mDevice->Format = aluBytesFromFormat(mDevice->Format) >= 2 ?
            AL_FORMAT_STEREO16 : AL_FORMAT_STEREO8;
    } else {
        mDevice->Format = aluBytesFromFormat(mDevice->Format) >= 2 ?
            AL_FORMAT_MONO16 : AL_FORMAT_MONO8;
    }

    SetDefaultChannelOrder(mDevice);

    err = open();
    RETURN_FALSE_IF(err);

    mPlaybackThread = StartThread(APlaybackDevice::onPlayback, this);
    RETURN_FALSE_IF_NULL(mPlaybackThread);

    LOG_FUNC_END

    return ALC_TRUE;
}

void APlaybackDevice::close()
{
    LOG_FUNC_START

    if(isPlaying()) {
        stop();
    }

    if(mBuffer) {
        delete mBuffer;
        mBuffer = NULL;
    }

    LOG_FUNC_END
}

static ALCboolean android_open_playback(ALCdevice *device, const ALCchar *deviceName)
{
    LOGI("Opening OpenAl %s", ALSOFT_VERSION);

    if (!deviceName) {
        deviceName = android_device;
    } else if (strcmp(deviceName, android_device) != 0) {
        return ALC_FALSE;
    }

    device->szDeviceName = strdup(deviceName);
    device->ExtraData = new APlaybackDevice(device);
    return ALC_TRUE;
}

static void android_close_playback(ALCdevice *device)
{
    LOGI("Closing OpenAl %s", ALSOFT_VERSION);

    APlaybackDevice* adev = (APlaybackDevice*)device->ExtraData;
    CHECK_NULL(adev);

    adev->close();
    delete adev;
    device->ExtraData = NULL;
}

static ALCboolean android_reset_playback(ALCdevice *device)
{
    LOGV("%s -", __func__);

    APlaybackDevice* adev = (APlaybackDevice*)device->ExtraData;
    CHECK_NULL(adev);

    return adev->reset();
}

static void android_stop_playback(ALCdevice *device)
{
    LOGV("%s -", __func__);

    APlaybackDevice* adev = (APlaybackDevice*)device->ExtraData;
    CHECK_NULL(adev);

    adev->stop();
}

static ALCboolean android_open_capture(ALCdevice *pDevice, const ALCchar *deviceName)
{
    LOGV("%s -", __func__);

    (void)pDevice;
    (void)deviceName;
    return ALC_FALSE;
}

static void android_close_capture(ALCdevice *pDevice)
{
    LOGV("%s -", __func__);

    (void)pDevice;
}

static void android_start_capture(ALCdevice *pDevice)
{
    LOGV("%s -", __func__);

    (void)pDevice;
}

static void android_stop_capture(ALCdevice *pDevice)
{
    LOGV("%s -", __func__);

    (void)pDevice;
}

static void android_capture_samples(ALCdevice *pDevice, ALCvoid *pBuffer, ALCuint lSamples)
{
    LOGV("%s -", __func__);

    (void)pDevice;
    (void)pBuffer;
    (void)lSamples;
}

static ALCuint android_available_samples(ALCdevice *pDevice)
{
    LOGV("%s -", __func__);

    (void)pDevice;
    return 0;
}

extern "C" {
    
static const BackendFuncs android_funcs = {
    android_open_playback,
    android_close_playback,
    android_reset_playback,
    android_stop_playback,
    android_open_capture,
    android_close_capture,
    android_start_capture,
    android_stop_capture,
    android_capture_samples,
    android_available_samples
};

void alc_android_init(BackendFuncs *func_list)
{
    *func_list = android_funcs;
}

void alc_android_deinit(void)
{
    LOGV("%s -", __func__);
}

void alc_android_probe(int type)
{
    LOGV("%s -", __func__);

    if (type == DEVICE_PROBE) {
        AppendDeviceList(android_device);
    }
    else if (type == ALL_DEVICE_PROBE) {
        AppendAllDeviceList(android_device);
    }
}

} // end of extern C
