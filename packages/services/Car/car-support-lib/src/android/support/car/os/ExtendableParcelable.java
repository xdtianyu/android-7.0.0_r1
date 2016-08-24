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

package android.support.car.os;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.car.annotation.VersionDef;

/**
 * ExtendableParcelable helps extending Parcelable in future in safe way.
 * Basic idea is to write version code and payload length when writing to Parcel.
 * Reader side reads version and do following actions:
 *   - if writer version <= reader version: Reader should know up to which element is supported by
 *     writer and read only up to that portion.
 *   - if writer version > reader version: Reader reads all members it knows and throw away remaing
 *     data so that additional Parcel reading can be done safely.
 *
 * For reader side:
 *  - In constructor with Parcel, call super(Parcel) first so that version number and payload length
 *    is read properly.
 *  - Call {@link #readHeader(Parcel)}.
 *  - After finishing all recognized or available members, call
 *    {@link #completeReading(Parcel, int)} with second argument set to return value from
 *    {@link #readHeader(Parcel)}. This will throw away any unread data.
 *
 * For writer side, when implementing writeToParcel:
 *   - call {@link #writeHeader(Parcel)} before writing anything else.
 *   - call {@link #completeWriting(Parcel, int)} with second argument set to return value from
 *     {@link #writeHeader(Parcel)}.
 */
public abstract class ExtendableParcelable implements Parcelable {
    /**
     * Version of this Parcelable. Reader side needs to read only up to the version written.
     * This does not represent class version but contents version. For example, original
     * Parcelable (=V1) passed to V2 supporting Parcelable will have version 1 even if
     * V2 supporting Parcelable may have additional member variables added in V2.
     */
    @VersionDef(version = 1)
    public final int version;

    /**
     * Constructor for reading parcel. Always call this first before reading anything else.
     * @param in
     * @param maxVersion
     */
    protected ExtendableParcelable(Parcel in, int version) {
        int writerVersion = in.readInt();
        if (version < writerVersion) { // version limited by reader
            this.version = version;
        } else { // version limited by writer
            this.version = writerVersion;
        }
    }

    /**
     * Constructor for writer side. Version should be always set.
     * @param version
     */
    protected ExtendableParcelable(int version) {
        this.version = version;
    }

    /**
     * Read header of Parcelable from Pacel. This should be done after super(Parcel, int) and
     * before reading any Parcel. After all reading is done, {@link #completeReading(Parcel, int)}
     * should be called.
     *
     * @param in
     * @return last position. This should be passed to {@link #completeReading(Parcel, int)}.
     */
    protected int readHeader(Parcel in) {
        int payloadLength = in.readInt();
        int startingPosition = in.dataPosition();
        return startingPosition + payloadLength;
    }

    /**
     * Complete reading and safely throw away any unread data.
     * @param in
     * @param lastPosition Last position of of this Parcelable in the passed Parcel.
     *                     The value is passed from {@link #readHeader(Parcel)}.
     */
    protected void completeReading(Parcel in, int lastPosition) {
        in.setDataPosition(lastPosition);
    }

    /**
     * Write header for writing to Parcel. This should be done before writing anything else.
     * Code to use ths can look like:
     *   int pos = writeHeader(dest);
     *   dest.writeInt(0); // whatever relevant data
     *   ...
     *   completeWrite(dest, pos);
     *
     * @param dest
     * @return startingPosition which should be passed when calling completeWrite.
     */
    protected int writeHeader(Parcel dest) {
        dest.writeInt(version);
        // temporary value for playload length. will be replaced in completeWrite
        dest.writeInt(0);
        // previous int is 4 bytes before this.
        return dest.dataPosition();
    }

    /**
     * Complete writing the current Parcelable. No more write to Parcel should be done after
     * this call.
     * @param dest
     * @param startingPosition startingPosition returned from writeHeader
     */
    protected void completeWriting(Parcel dest, int startingPosition) {
        int currentPosition = dest.dataPosition();
        dest.setDataPosition(startingPosition - 4);
        int payloadLength = currentPosition - startingPosition;
        dest.writeInt(payloadLength);
        dest.setDataPosition(currentPosition);
    }
}
