/*
 * Copyright (C) 2015 The Android Open Source Project
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

// Test program for audio_utils FIFO library.
// This only tests the single-threaded aspects, not the barriers.

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <audio_utils/fifo.h>
#include <audio_utils/sndfile.h>

int main(int argc, char **argv)
{
    size_t frameCount = 256;
    size_t maxFramesPerRead = 1;
    size_t maxFramesPerWrite = 1;
    int i;
    for (i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (arg[0] != '-')
            break;
        switch (arg[1]) {
        case 'c':   // FIFO frame count
            frameCount = atoi(&arg[2]);
            break;
        case 'r':   // maximum frame count per read from FIFO
            maxFramesPerRead = atoi(&arg[2]);
            break;
        case 'w':   // maximum frame count per write to FIFO
            maxFramesPerWrite = atoi(&arg[2]);
            break;
        default:
            fprintf(stderr, "%s: unknown option %s\n", argv[0], arg);
            goto usage;
        }
    }

    if (argc - i != 2) {
usage:
        fprintf(stderr, "usage: %s [-c#] in.wav out.wav\n", argv[0]);
        return EXIT_FAILURE;
    }
    char *inputFile = argv[i];
    char *outputFile = argv[i+1];

    SF_INFO sfinfoin;
    memset(&sfinfoin, 0, sizeof(sfinfoin));
    SNDFILE *sfin = sf_open(inputFile, SFM_READ, &sfinfoin);
    if (sfin == NULL) {
        perror(inputFile);
        return EXIT_FAILURE;
    }
    // sf_readf_short() does conversion, so not strictly necessary to check the file format.
    // But I want to do "cmp" on input and output files afterwards,
    // and it is easier if they are all the same format.
    // Enforcing that everything is 16-bit is convenient for this.
    if ((sfinfoin.format & (SF_FORMAT_TYPEMASK | SF_FORMAT_SUBMASK)) !=
            (SF_FORMAT_WAV | SF_FORMAT_PCM_16)) {
        fprintf(stderr, "%s: unsupported format\n", inputFile);
        sf_close(sfin);
        return EXIT_FAILURE;
    }
    size_t frameSize = sizeof(short) * sfinfoin.channels;
    short *inputBuffer = new short[sfinfoin.frames * sfinfoin.channels];
    sf_count_t actualRead = sf_readf_short(sfin, inputBuffer, sfinfoin.frames);
    if (actualRead != sfinfoin.frames) {
        fprintf(stderr, "%s: unexpected EOF or error\n", inputFile);
        sf_close(sfin);
        return EXIT_FAILURE;
    }
    sf_close(sfin);

    short *outputBuffer = new short[sfinfoin.frames * sfinfoin.channels];
    size_t framesWritten = 0;
    size_t framesRead = 0;
    struct audio_utils_fifo fifo;
    short *fifoBuffer = new short[frameCount * sfinfoin.channels];
    audio_utils_fifo_init(&fifo, frameCount, frameSize, fifoBuffer);
    int fifoWriteCount = 0, fifoReadCount = 0;
    int fifoFillLevel = 0, minFillLevel = INT_MAX, maxFillLevel = INT_MIN;
    for (;;) {
        size_t framesToWrite = sfinfoin.frames - framesWritten;
        size_t framesToRead = sfinfoin.frames - framesRead;
        if (framesToWrite == 0 && framesToRead == 0) {
            break;
        }

        if (framesToWrite > maxFramesPerWrite) {
            framesToWrite = maxFramesPerWrite;
        }
        framesToWrite = rand() % (framesToWrite + 1);
        ssize_t actualWritten = audio_utils_fifo_write(&fifo,
                &inputBuffer[framesWritten * sfinfoin.channels], framesToWrite);
        if (actualWritten < 0 || (size_t) actualWritten > framesToWrite) {
            fprintf(stderr, "write to FIFO failed\n");
            break;
        }
        framesWritten += actualWritten;
        if (actualWritten > 0) {
            fifoWriteCount++;
        }
        fifoFillLevel += actualWritten;
        if (fifoFillLevel > maxFillLevel) {
            maxFillLevel = fifoFillLevel;
            if (maxFillLevel > (int) frameCount)
                abort();
        }

        if (framesToRead > maxFramesPerRead) {
            framesToRead = maxFramesPerRead;
        }
        framesToRead = rand() % (framesToRead + 1);
        ssize_t actualRead = audio_utils_fifo_read(&fifo,
                &outputBuffer[framesRead * sfinfoin.channels], framesToRead);
        if (actualRead < 0 || (size_t) actualRead > framesToRead) {
            fprintf(stderr, "read from FIFO failed\n");
            break;
        }
        framesRead += actualRead;
        if (actualRead > 0) {
            fifoReadCount++;
        }
        fifoFillLevel -= actualRead;
        if (fifoFillLevel < minFillLevel) {
            minFillLevel = fifoFillLevel;
            if (minFillLevel < 0)
                abort();
        }
    }
    printf("FIFO non-empty writes: %d, non-empty reads: %d\n", fifoWriteCount, fifoReadCount);
    printf("fill=%d, min=%d, max=%d\n", fifoFillLevel, minFillLevel, maxFillLevel);
    audio_utils_fifo_deinit(&fifo);
    delete[] fifoBuffer;

    SF_INFO sfinfoout;
    memset(&sfinfoout, 0, sizeof(sfinfoout));
    sfinfoout.samplerate = sfinfoin.samplerate;
    sfinfoout.channels = sfinfoin.channels;
    sfinfoout.format = sfinfoin.format;
    SNDFILE *sfout = sf_open(outputFile, SFM_WRITE, &sfinfoout);
    if (sfout == NULL) {
        perror(outputFile);
        return EXIT_FAILURE;
    }
    sf_count_t actualWritten = sf_writef_short(sfout, outputBuffer, framesRead);
    delete[] inputBuffer;
    delete[] outputBuffer;
    delete[] fifoBuffer;
    if (actualWritten != (sf_count_t) framesRead) {
        fprintf(stderr, "%s: unexpected error\n", outputFile);
        sf_close(sfout);
        return EXIT_FAILURE;
    }
    sf_close(sfout);
    return EXIT_SUCCESS;
}
