/*
 * Copyright 2016, The Android Open Source Project
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

package com.android.managedprovisioning.model;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import java.util.Objects;

import com.android.internal.annotations.Immutable;

/**
 * Stores the WiFi configuration which is used in managed provisioning.
 */
@Immutable
public final class WifiInfo implements Parcelable {
    public static final boolean DEFAULT_WIFI_HIDDEN = false;
    public static final int DEFAULT_WIFI_PROXY_PORT = 0;

    public static final Parcelable.Creator<WifiInfo> CREATOR
            = new Parcelable.Creator<WifiInfo>() {
        @Override
        public WifiInfo createFromParcel(Parcel in) {
            return new WifiInfo(in);
        }

        @Override
        public WifiInfo[] newArray(int size) {
            return new WifiInfo[size];
        }
    };

    /** Ssid of the wifi network. */
    public final String ssid;
    /** Wifi network in {@link #ssid} is hidden or not. */
    public final boolean hidden;
    /** Security type of the wifi network in {@link #ssid}. */
    @Nullable
    public final String securityType;
    /** Password of the wifi network in {@link #ssid}. */
    @Nullable
    public final String password;
    /** Proxy host for the wifi network in {@link #ssid}. */
    @Nullable
    public final String proxyHost;
    /** Proxy port for the wifi network in {@link #ssid}. */
    public final int proxyPort;
    /** The proxy bypass for the wifi network in {@link #ssid}. */
    @Nullable
    public final String proxyBypassHosts;
    /** The proxy bypass list for the wifi network in {@link #ssid}. */
    @Nullable
    public final String pacUrl;

    private WifiInfo(Builder builder) {
        ssid = builder.mSsid;
        hidden = builder.mHidden;
        securityType = builder.mSecurityType;
        password = builder.mPassword;
        proxyHost = builder.mProxyHost;
        proxyPort = builder.mProxyPort;
        proxyBypassHosts = builder.mProxyBypassHosts;
        pacUrl = builder.mPacUrl;

        validateFields();
    }

    private WifiInfo(Parcel in) {
        ssid = in.readString();
        hidden = in.readInt() == 1;
        securityType = in.readString();
        password = in.readString();
        proxyHost = in.readString();
        proxyPort = in.readInt();
        proxyBypassHosts = in.readString();
        pacUrl = in.readString();

        validateFields();
    }

    private void validateFields() {
        if (TextUtils.isEmpty(ssid)) {
            throw new IllegalArgumentException("Ssid must not be empty!");
        }
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        out.writeString(ssid);
        out.writeInt(hidden ? 1 : 0);
        out.writeString(securityType);
        out.writeString(password);
        out.writeString(proxyHost);
        out.writeInt(proxyPort);
        out.writeString(proxyBypassHosts);
        out.writeString(pacUrl);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }
        WifiInfo that = (WifiInfo) o;
        return hidden == that.hidden
                && proxyPort == that.proxyPort
                && Objects.equals(ssid, that.ssid)
                && Objects.equals(securityType, that.securityType)
                && Objects.equals(password, that.password)
                && Objects.equals(proxyHost, that.proxyHost)
                && Objects.equals(proxyBypassHosts, that.proxyBypassHosts)
                && Objects.equals(pacUrl, that.pacUrl);
    }

    public final static class Builder {
        private String mSsid;
        private boolean mHidden = DEFAULT_WIFI_HIDDEN;
        private String mSecurityType;
        private String mPassword;
        private String mProxyHost;
        private int mProxyPort = DEFAULT_WIFI_PROXY_PORT;
        private String mProxyBypassHosts;
        private String mPacUrl;

        public Builder setSsid(String ssid) {
            mSsid = ssid;
            return this;
        }

        public Builder setHidden(boolean hidden) {
            mHidden = hidden;
            return this;
        }

        public Builder setSecurityType(String securityType) {
            mSecurityType = securityType;
            return this;
        }

        public Builder setPassword(String password) {
            mPassword = password;
            return this;
        }

        public Builder setProxyHost(String proxyHost) {
            mProxyHost = proxyHost;
            return this;
        }

        public Builder setProxyPort(int proxyPort) {
            mProxyPort = proxyPort;
            return this;
        }

        public Builder setProxyBypassHosts(String proxyBypassHosts) {
            mProxyBypassHosts = proxyBypassHosts;
            return this;
        }

        public Builder setPacUrl(String pacUrl) {
            mPacUrl = pacUrl;
            return this;
        }

        public WifiInfo build() {
            return new WifiInfo(this);
        }

        public static Builder builder() {
            return new Builder();
        }
    }
}
