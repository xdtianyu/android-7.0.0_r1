/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sles_allinclusive.h"
#include "android_prompts.h"
#include "channels.h"

#include <utils/String16.h>

#include <system/audio.h>
#include <SLES/OpenSLES_Android.h>

#include <android_runtime/AndroidRuntime.h>

#define KEY_RECORDING_SOURCE_PARAMSIZE  sizeof(SLuint32)
#define KEY_RECORDING_PRESET_PARAMSIZE  sizeof(SLuint32)

//-----------------------------------------------------------------------------
// Internal utility functions
//----------------------------

SLresult audioRecorder_setPreset(CAudioRecorder* ar, SLuint32 recordPreset) {
    SLresult result = SL_RESULT_SUCCESS;

    audio_source_t newRecordSource = AUDIO_SOURCE_DEFAULT;
    switch (recordPreset) {
    case SL_ANDROID_RECORDING_PRESET_GENERIC:
        newRecordSource = AUDIO_SOURCE_DEFAULT;
        break;
    case SL_ANDROID_RECORDING_PRESET_CAMCORDER:
        newRecordSource = AUDIO_SOURCE_CAMCORDER;
        break;
    case SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION:
        newRecordSource = AUDIO_SOURCE_VOICE_RECOGNITION;
        break;
    case SL_ANDROID_RECORDING_PRESET_VOICE_COMMUNICATION:
        newRecordSource = AUDIO_SOURCE_VOICE_COMMUNICATION;
        break;
    case SL_ANDROID_RECORDING_PRESET_UNPROCESSED:
            newRecordSource = AUDIO_SOURCE_UNPROCESSED;
            break;
    case SL_ANDROID_RECORDING_PRESET_NONE:
        // it is an error to set preset "none"
    default:
        SL_LOGE(ERROR_RECORDERPRESET_SET_UNKNOWN_PRESET);
        result = SL_RESULT_PARAMETER_INVALID;
    }

    // recording preset needs to be set before the object is realized
    // (ap->mAudioRecord is supposed to be 0 until then)
    if (SL_OBJECT_STATE_UNREALIZED != ar->mObject.mState) {
        SL_LOGE(ERROR_RECORDERPRESET_REALIZED);
        result = SL_RESULT_PRECONDITIONS_VIOLATED;
    } else {
        ar->mRecordSource = newRecordSource;
    }

    return result;
}


SLresult audioRecorder_getPreset(CAudioRecorder* ar, SLuint32* pPreset) {
    SLresult result = SL_RESULT_SUCCESS;

    switch (ar->mRecordSource) {
    case AUDIO_SOURCE_DEFAULT:
    case AUDIO_SOURCE_MIC:
        *pPreset = SL_ANDROID_RECORDING_PRESET_GENERIC;
        break;
    case AUDIO_SOURCE_VOICE_UPLINK:
    case AUDIO_SOURCE_VOICE_DOWNLINK:
    case AUDIO_SOURCE_VOICE_CALL:
        *pPreset = SL_ANDROID_RECORDING_PRESET_NONE;
        break;
    case AUDIO_SOURCE_VOICE_RECOGNITION:
        *pPreset = SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION;
        break;
    case AUDIO_SOURCE_CAMCORDER:
        *pPreset = SL_ANDROID_RECORDING_PRESET_CAMCORDER;
        break;
    case AUDIO_SOURCE_VOICE_COMMUNICATION:
        *pPreset = SL_ANDROID_RECORDING_PRESET_VOICE_COMMUNICATION;
        break;
    case AUDIO_SOURCE_UNPROCESSED:
        *pPreset = SL_ANDROID_RECORDING_PRESET_UNPROCESSED;
        break;
    default:
        *pPreset = SL_ANDROID_RECORDING_PRESET_NONE;
        result = SL_RESULT_INTERNAL_ERROR;
        break;
    }

    return result;
}


void audioRecorder_handleNewPos_lockRecord(CAudioRecorder* ar) {
    //SL_LOGV("received event EVENT_NEW_POS from AudioRecord");
    slRecordCallback callback = NULL;
    void* callbackPContext = NULL;

    interface_lock_shared(&ar->mRecord);
    callback = ar->mRecord.mCallback;
    callbackPContext = ar->mRecord.mContext;
    interface_unlock_shared(&ar->mRecord);

    if (NULL != callback) {
        // getting this event implies SL_RECORDEVENT_HEADATNEWPOS was set in the event mask
        (*callback)(&ar->mRecord.mItf, callbackPContext, SL_RECORDEVENT_HEADATNEWPOS);
    }
}


void audioRecorder_handleMarker_lockRecord(CAudioRecorder* ar) {
    //SL_LOGV("received event EVENT_MARKER from AudioRecord");
    slRecordCallback callback = NULL;
    void* callbackPContext = NULL;

    interface_lock_shared(&ar->mRecord);
    callback = ar->mRecord.mCallback;
    callbackPContext = ar->mRecord.mContext;
    interface_unlock_shared(&ar->mRecord);

    if (NULL != callback) {
        // getting this event implies SL_RECORDEVENT_HEADATMARKER was set in the event mask
        (*callback)(&ar->mRecord.mItf, callbackPContext, SL_RECORDEVENT_HEADATMARKER);
    }
}


void audioRecorder_handleOverrun_lockRecord(CAudioRecorder* ar) {
    //SL_LOGV("received event EVENT_OVERRUN from AudioRecord");
    slRecordCallback callback = NULL;
    void* callbackPContext = NULL;

    interface_lock_shared(&ar->mRecord);
    if (ar->mRecord.mCallbackEventsMask & SL_RECORDEVENT_HEADSTALLED) {
        callback = ar->mRecord.mCallback;
        callbackPContext = ar->mRecord.mContext;
    }
    interface_unlock_shared(&ar->mRecord);

    if (NULL != callback) {
        (*callback)(&ar->mRecord.mItf, callbackPContext, SL_RECORDEVENT_HEADSTALLED);
    }
}

//-----------------------------------------------------------------------------
SLresult android_audioRecorder_checkSourceSink(CAudioRecorder* ar) {

    const SLDataSource *pAudioSrc = &ar->mDataSource.u.mSource;
    const SLDataSink   *pAudioSnk = &ar->mDataSink.u.mSink;

    const SLuint32 sinkLocatorType = *(SLuint32 *)pAudioSnk->pLocator;
    const SLuint32 sinkFormatType = *(SLuint32 *)pAudioSnk->pFormat;

    const SLuint32 *df_representation = NULL; // pointer to representation field, if it exists

    // sink must be an Android simple buffer queue with PCM data format
    switch (sinkLocatorType) {
    case SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE: {
        switch (sinkFormatType) {
        case SL_ANDROID_DATAFORMAT_PCM_EX: {
            const SLAndroidDataFormat_PCM_EX *df_pcm =
                    (SLAndroidDataFormat_PCM_EX *) pAudioSnk->pFormat;
            // checkDataFormat() already checked representation
            df_representation = &df_pcm->representation;
        } // SL_ANDROID_DATAFORMAT_PCM_EX - fall through to next test.
        case SL_DATAFORMAT_PCM: {
            const SLDataFormat_PCM *df_pcm = (const SLDataFormat_PCM *) pAudioSnk->pFormat;
            // checkDataFormat already checked sample rate, channels, and mask
            ar->mNumChannels = df_pcm->numChannels;

            if (df_pcm->endianness != ar->mObject.mEngine->mEngine.mNativeEndianness) {
                SL_LOGE("Cannot create audio recorder: unsupported byte order %u",
                        df_pcm->endianness);
                return SL_RESULT_CONTENT_UNSUPPORTED;
            }

            ar->mSampleRateMilliHz = df_pcm->samplesPerSec; // Note: bad field name in SL ES
            SL_LOGV("AudioRecorder requested sample rate = %u mHz, %u channel(s)",
                    ar->mSampleRateMilliHz, ar->mNumChannels);

            // we don't support container size != sample depth
            if (df_pcm->containerSize != df_pcm->bitsPerSample) {
                SL_LOGE("Cannot create audio recorder: unsupported container size %u bits for "
                        "sample depth %u bits",
                        df_pcm->containerSize, (SLuint32)df_pcm->bitsPerSample);
                return SL_RESULT_CONTENT_UNSUPPORTED;
            }

            } break;
        default:
            SL_LOGE(ERROR_RECORDER_SINK_FORMAT_MUST_BE_PCM);
            return SL_RESULT_PARAMETER_INVALID;
        }   // switch (sourceFormatType)
        } break;    // case SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE
    default:
        SL_LOGE(ERROR_RECORDER_SINK_MUST_BE_ANDROIDSIMPLEBUFFERQUEUE);
        return SL_RESULT_PARAMETER_INVALID;
    }   // switch (sourceLocatorType)

    // Source check:
    // only input device sources are supported
    // check it's an IO device
    if (SL_DATALOCATOR_IODEVICE != *(SLuint32 *)pAudioSrc->pLocator) {
        SL_LOGE(ERROR_RECORDER_SOURCE_MUST_BE_IODEVICE);
        return SL_RESULT_PARAMETER_INVALID;
    } else {

        // check it's an input device
        SLDataLocator_IODevice *dl_iod = (SLDataLocator_IODevice *) pAudioSrc->pLocator;
        if (SL_IODEVICE_AUDIOINPUT != dl_iod->deviceType) {
            SL_LOGE(ERROR_RECORDER_IODEVICE_MUST_BE_AUDIOINPUT);
            return SL_RESULT_PARAMETER_INVALID;
        }

        // check it's the default input device, others aren't supported here
        if (SL_DEFAULTDEVICEID_AUDIOINPUT != dl_iod->deviceID) {
            SL_LOGE(ERROR_RECORDER_INPUT_ID_MUST_BE_DEFAULT);
            return SL_RESULT_PARAMETER_INVALID;
        }
    }

    return SL_RESULT_SUCCESS;
}
//-----------------------------------------------------------------------------
static void audioRecorder_callback(int event, void* user, void *info) {
    //SL_LOGV("audioRecorder_callback(%d, %p, %p) entering", event, user, info);

    CAudioRecorder *ar = (CAudioRecorder *)user;

    if (!android::CallbackProtector::enterCbIfOk(ar->mCallbackProtector)) {
        // it is not safe to enter the callback (the track is about to go away)
        return;
    }

    void * callbackPContext = NULL;

    switch (event) {
    case android::AudioRecord::EVENT_MORE_DATA: {
        slBufferQueueCallback callback = NULL;
        android::AudioRecord::Buffer* pBuff = (android::AudioRecord::Buffer*)info;

        // push data to the buffer queue
        interface_lock_exclusive(&ar->mBufferQueue);

        if (ar->mBufferQueue.mState.count != 0) {
            assert(ar->mBufferQueue.mFront != ar->mBufferQueue.mRear);

            BufferHeader *oldFront = ar->mBufferQueue.mFront;
            BufferHeader *newFront = &oldFront[1];

            size_t availSink = oldFront->mSize - ar->mBufferQueue.mSizeConsumed;
            size_t availSource = pBuff->size;
            size_t bytesToCopy = availSink < availSource ? availSink : availSource;
            void *pDest = (char *)oldFront->mBuffer + ar->mBufferQueue.mSizeConsumed;
            memcpy(pDest, pBuff->raw, bytesToCopy);

            if (bytesToCopy < availSink) {
                // can't consume the whole or rest of the buffer in one shot
                ar->mBufferQueue.mSizeConsumed += availSource;
                // pBuff->size is already equal to bytesToCopy in this case
            } else {
                // finish pushing the buffer or push the buffer in one shot
                pBuff->size = bytesToCopy;
                ar->mBufferQueue.mSizeConsumed = 0;
                if (newFront == &ar->mBufferQueue.mArray[ar->mBufferQueue.mNumBuffers + 1]) {
                    newFront = ar->mBufferQueue.mArray;
                }
                ar->mBufferQueue.mFront = newFront;

                ar->mBufferQueue.mState.count--;
                ar->mBufferQueue.mState.playIndex++;

                // data has been copied to the buffer, and the buffer queue state has been updated
                // we will notify the client if applicable
                callback = ar->mBufferQueue.mCallback;
                // save callback data
                callbackPContext = ar->mBufferQueue.mContext;
            }
        } else { // empty queue
            // no destination to push the data
            pBuff->size = 0;
        }

        interface_unlock_exclusive(&ar->mBufferQueue);

        // notify client
        if (NULL != callback) {
            (*callback)(&ar->mBufferQueue.mItf, callbackPContext);
        }
        }
        break;

    case android::AudioRecord::EVENT_OVERRUN:
        audioRecorder_handleOverrun_lockRecord(ar);
        break;

    case android::AudioRecord::EVENT_MARKER:
        audioRecorder_handleMarker_lockRecord(ar);
        break;

    case android::AudioRecord::EVENT_NEW_POS:
        audioRecorder_handleNewPos_lockRecord(ar);
        break;

    case android::AudioRecord::EVENT_NEW_IAUDIORECORD:
        // ignore for now
        break;

    default:
        SL_LOGE("Encountered unknown AudioRecord event %d for CAudioRecord %p", event, ar);
        break;
    }

    ar->mCallbackProtector->exitCb();
}


//-----------------------------------------------------------------------------
SLresult android_audioRecorder_create(CAudioRecorder* ar) {
    SL_LOGV("android_audioRecorder_create(%p) entering", ar);

    const SLDataSource *pAudioSrc = &ar->mDataSource.u.mSource;
    const SLDataSink *pAudioSnk = &ar->mDataSink.u.mSink;
    SLresult result = SL_RESULT_SUCCESS;

    const SLuint32 sourceLocatorType = *(SLuint32 *)pAudioSrc->pLocator;
    const SLuint32 sinkLocatorType = *(SLuint32 *)pAudioSnk->pLocator;

    //  the following platform-independent fields have been initialized in CreateAudioRecorder()
    //    ar->mNumChannels
    //    ar->mSampleRateMilliHz

    if ((SL_DATALOCATOR_IODEVICE == sourceLocatorType) &&
            (SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE == sinkLocatorType)) {
        // microphone to simple buffer queue
        ar->mAndroidObjType = AUDIORECORDER_FROM_MIC_TO_PCM_BUFFERQUEUE;
        ar->mAudioRecord.clear();
        ar->mCallbackProtector = new android::CallbackProtector();
        ar->mRecordSource = AUDIO_SOURCE_DEFAULT;
    } else {
        result = SL_RESULT_CONTENT_UNSUPPORTED;
    }

    return result;
}


//-----------------------------------------------------------------------------
SLresult android_audioRecorder_setConfig(CAudioRecorder* ar, const SLchar *configKey,
        const void *pConfigValue, SLuint32 valueSize) {

    SLresult result;

    assert(NULL != ar && NULL != configKey && NULL != pConfigValue);
    if (strcmp((const char*)configKey, (const char*)SL_ANDROID_KEY_RECORDING_PRESET) == 0) {

        // recording preset
        if (KEY_RECORDING_PRESET_PARAMSIZE > valueSize) {
            SL_LOGE(ERROR_CONFIG_VALUESIZE_TOO_LOW);
            result = SL_RESULT_BUFFER_INSUFFICIENT;
        } else {
            result = audioRecorder_setPreset(ar, *(SLuint32*)pConfigValue);
        }

    } else {
        SL_LOGE(ERROR_CONFIG_UNKNOWN_KEY);
        result = SL_RESULT_PARAMETER_INVALID;
    }

    return result;
}


//-----------------------------------------------------------------------------
SLresult android_audioRecorder_getConfig(CAudioRecorder* ar, const SLchar *configKey,
        SLuint32* pValueSize, void *pConfigValue) {

    SLresult result;

    assert(NULL != ar && NULL != configKey && NULL != pValueSize);
    if (strcmp((const char*)configKey, (const char*)SL_ANDROID_KEY_RECORDING_PRESET) == 0) {

        // recording preset
        if (NULL == pConfigValue) {
            result = SL_RESULT_SUCCESS;
        } else if (KEY_RECORDING_PRESET_PARAMSIZE > *pValueSize) {
            SL_LOGE(ERROR_CONFIG_VALUESIZE_TOO_LOW);
            result = SL_RESULT_BUFFER_INSUFFICIENT;
        } else {
            result = audioRecorder_getPreset(ar, (SLuint32*)pConfigValue);
        }
        *pValueSize = KEY_RECORDING_PRESET_PARAMSIZE;

    } else {
        SL_LOGE(ERROR_CONFIG_UNKNOWN_KEY);
        result = SL_RESULT_PARAMETER_INVALID;
    }

    return result;
}


//-----------------------------------------------------------------------------
SLresult android_audioRecorder_realize(CAudioRecorder* ar, SLboolean async) {
    SL_LOGV("android_audioRecorder_realize(%p) entering", ar);

    SLresult result = SL_RESULT_SUCCESS;

    // already checked in created and checkSourceSink
    assert(ar->mDataSink.mLocator.mLocatorType == SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE);

    const SLDataFormat_PCM *df_pcm = &ar->mDataSink.mFormat.mPCM;

    //  the following platform-independent fields have been initialized in CreateAudioRecorder()
    //    ar->mNumChannels
    //    ar->mSampleRateMilliHz

    uint32_t sampleRate = sles_to_android_sampleRate(df_pcm->samplesPerSec);

    // currently nothing analogous to canUseFastTrack() for recording
    audio_input_flags_t policy = AUDIO_INPUT_FLAG_FAST;

    SL_LOGV("Audio Record format: %dch(0x%x), %dbit, %dKHz",
            df_pcm->numChannels,
            df_pcm->channelMask,
            df_pcm->bitsPerSample,
            df_pcm->samplesPerSec / 1000000);

    // note that df_pcm->channelMask has already been validated during object creation.
    audio_channel_mask_t channelMask = sles_to_audio_input_channel_mask(df_pcm->channelMask);

    // To maintain backward compatibility with previous releases, ignore
    // channel masks that are not indexed.
    if (channelMask == AUDIO_CHANNEL_INVALID
            || audio_channel_mask_get_representation(channelMask)
                == AUDIO_CHANNEL_REPRESENTATION_POSITION) {
        channelMask = audio_channel_in_mask_from_count(df_pcm->numChannels);
        SL_LOGI("Emulating old channel mask behavior "
                "(ignoring positional mask %#x, using default mask %#x based on "
                "channel count of %d)", df_pcm->channelMask, channelMask,
                df_pcm->numChannels);
    }
    SL_LOGV("SLES channel mask %#x converted to Android mask %#x", df_pcm->channelMask, channelMask);

    // initialize platform-specific CAudioRecorder fields
    ar->mAudioRecord = new android::AudioRecord(
            ar->mRecordSource,     // source
            sampleRate,            // sample rate in Hertz
            sles_to_android_sampleFormat(df_pcm),               // format
            channelMask,           // channel mask
            android::String16(),   // app ops
            0,                     // frameCount
            audioRecorder_callback,// callback_t
            (void*)ar,             // user, callback data, here the AudioRecorder
            0,                     // notificationFrames
            AUDIO_SESSION_ALLOCATE,
            android::AudioRecord::TRANSFER_CALLBACK,
                                   // transfer type
            policy);               // audio_input_flags_t

    android::status_t status = ar->mAudioRecord->initCheck();
    if (android::NO_ERROR != status) {
        SL_LOGE("android_audioRecorder_realize(%p) error creating AudioRecord object; status %d",
                ar, status);
        // FIXME should return a more specific result depending on status
        result = SL_RESULT_CONTENT_UNSUPPORTED;
        ar->mAudioRecord.clear();
    }

    // If there is a JavaAudioRoutingProxy associated with this recorder, hook it up...
    JNIEnv* j_env = NULL;
    jclass clsAudioRecord = NULL;
    jmethodID midRoutingProxy_connect = NULL;
    if (ar->mAndroidConfiguration.mRoutingProxy != NULL &&
            (j_env = android::AndroidRuntime::getJNIEnv()) != NULL &&
            (clsAudioRecord = j_env->FindClass("android/media/AudioRecord")) != NULL &&
            (midRoutingProxy_connect =
                j_env->GetMethodID(clsAudioRecord, "deferred_connect", "(J)V")) != NULL) {
        j_env->ExceptionClear();
        j_env->CallVoidMethod(ar->mAndroidConfiguration.mRoutingProxy,
                              midRoutingProxy_connect,
                              ar->mAudioRecord.get());
        if (j_env->ExceptionCheck()) {
            SL_LOGE("Java exception releasing recorder routing object.");
            result = SL_RESULT_INTERNAL_ERROR;
        }
   }

    return result;
}


//-----------------------------------------------------------------------------
/**
 * Called with a lock on AudioRecorder, and blocks until safe to destroy
 */
void android_audioRecorder_preDestroy(CAudioRecorder* ar) {
    object_unlock_exclusive(&ar->mObject);
    if (ar->mCallbackProtector != 0) {
        ar->mCallbackProtector->requestCbExitAndWait();
    }
    object_lock_exclusive(&ar->mObject);
}


//-----------------------------------------------------------------------------
void android_audioRecorder_destroy(CAudioRecorder* ar) {
    SL_LOGV("android_audioRecorder_destroy(%p) entering", ar);

    if (ar->mAudioRecord != 0) {
        ar->mAudioRecord->stop();
        ar->mAudioRecord.clear();
    }
    // explicit destructor
    ar->mAudioRecord.~sp();
    ar->mCallbackProtector.~sp();
}


//-----------------------------------------------------------------------------
void android_audioRecorder_setRecordState(CAudioRecorder* ar, SLuint32 state) {
    SL_LOGV("android_audioRecorder_setRecordState(%p, %u) entering", ar, state);

    if (ar->mAudioRecord == 0) {
        return;
    }

    switch (state) {
     case SL_RECORDSTATE_STOPPED:
         ar->mAudioRecord->stop();
         break;
     case SL_RECORDSTATE_PAUSED:
         // Note that pausing is treated like stop as this implementation only records to a buffer
         //  queue, so there is no notion of destination being "opened" or "closed" (See description
         //  of SL_RECORDSTATE in specification)
         ar->mAudioRecord->stop();
         break;
     case SL_RECORDSTATE_RECORDING:
         ar->mAudioRecord->start();
         break;
     default:
         break;
     }

}


//-----------------------------------------------------------------------------
void android_audioRecorder_useRecordEventMask(CAudioRecorder *ar) {
    IRecord *pRecordItf = &ar->mRecord;
    SLuint32 eventFlags = pRecordItf->mCallbackEventsMask;

    if (ar->mAudioRecord == 0) {
        return;
    }

    if ((eventFlags & SL_RECORDEVENT_HEADATMARKER) && (pRecordItf->mMarkerPosition != 0)) {
        ar->mAudioRecord->setMarkerPosition((uint32_t)((((int64_t)pRecordItf->mMarkerPosition
                * sles_to_android_sampleRate(ar->mSampleRateMilliHz)))/1000));
    } else {
        // clear marker
        ar->mAudioRecord->setMarkerPosition(0);
    }

    if (eventFlags & SL_RECORDEVENT_HEADATNEWPOS) {
        SL_LOGV("pos update period %d", pRecordItf->mPositionUpdatePeriod);
         ar->mAudioRecord->setPositionUpdatePeriod(
                (uint32_t)((((int64_t)pRecordItf->mPositionUpdatePeriod
                * sles_to_android_sampleRate(ar->mSampleRateMilliHz)))/1000));
    } else {
        // clear periodic update
        ar->mAudioRecord->setPositionUpdatePeriod(0);
    }

    if (eventFlags & SL_RECORDEVENT_HEADATLIMIT) {
        // FIXME support SL_RECORDEVENT_HEADATLIMIT
        SL_LOGD("[ FIXME: IRecord_SetCallbackEventsMask(SL_RECORDEVENT_HEADATLIMIT) on an "
                    "SL_OBJECTID_AUDIORECORDER to be implemented ]");
    }

    if (eventFlags & SL_RECORDEVENT_HEADMOVING) {
        // FIXME support SL_RECORDEVENT_HEADMOVING
        SL_LOGD("[ FIXME: IRecord_SetCallbackEventsMask(SL_RECORDEVENT_HEADMOVING) on an "
                "SL_OBJECTID_AUDIORECORDER to be implemented ]");
    }

    if (eventFlags & SL_RECORDEVENT_BUFFER_FULL) {
        // nothing to do for SL_RECORDEVENT_BUFFER_FULL since this will not be encountered on
        // recording to buffer queues
    }

    if (eventFlags & SL_RECORDEVENT_HEADSTALLED) {
        // nothing to do for SL_RECORDEVENT_HEADSTALLED, callback event will be checked against mask
        // when AudioRecord::EVENT_OVERRUN is encountered

    }

}


//-----------------------------------------------------------------------------
void android_audioRecorder_getPosition(CAudioRecorder *ar, SLmillisecond *pPosMsec) {
    if ((NULL == ar) || (ar->mAudioRecord == 0)) {
        *pPosMsec = 0;
    } else {
        uint32_t positionInFrames;
        ar->mAudioRecord->getPosition(&positionInFrames);
        if (ar->mSampleRateMilliHz == UNKNOWN_SAMPLERATE) {
            *pPosMsec = 0;
        } else {
            *pPosMsec = ((int64_t)positionInFrames * 1000) /
                    sles_to_android_sampleRate(ar->mSampleRateMilliHz);
        }
    }
}
