/*
 * Copyright 2014 The Android Open Source Project
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

package android.media.cts;

import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.media.cts.CodecImage;
import android.media.Image;
import android.media.MediaCodecInfo;
import android.media.MediaCodecInfo.CodecCapabilities;
import android.media.MediaCodecList;
import android.util.Log;

import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.util.ArrayList;

public class CodecUtils  {
    private static final String TAG = "CodecUtils";

    /** Load jni on initialization */
    static {
        Log.i(TAG, "before loadlibrary");
        System.loadLibrary("ctsmediacodec_jni");
        Log.i(TAG, "after loadlibrary");
    }

    private static class ImageWrapper extends CodecImage {
        private final Image mImage;
        private final Plane[] mPlanes;

        private ImageWrapper(Image image) {
            mImage = image;
            Image.Plane[] planes = mImage.getPlanes();

            mPlanes = new Plane[planes.length];
            for (int i = 0; i < planes.length; i++) {
                mPlanes[i] = new PlaneWrapper(planes[i]);
            }
        }

        public static ImageWrapper createFromImage(Image image) {
            return new ImageWrapper(image);
        }

        @Override
        public int getFormat() {
            return mImage.getFormat();
        }

        @Override
        public int getWidth() {
            return mImage.getWidth();
        }

        @Override
        public int getHeight() {
            return mImage.getHeight();
        }

        @Override
        public long getTimestamp() {
            return mImage.getTimestamp();
        }

        @Override
        public Plane[] getPlanes() {
            return mPlanes;
        }

        @Override
        public void close() {
            mImage.close();
        }

        private static class PlaneWrapper extends CodecImage.Plane {
            private final Image.Plane mPlane;

            PlaneWrapper(Image.Plane plane) {
                mPlane = plane;
            }

            @Override
            public int getRowStride() {
                return mPlane.getRowStride();
            }

           @Override
            public int getPixelStride() {
               return mPlane.getPixelStride();
            }

            @Override
            public ByteBuffer getBuffer() {
                return mPlane.getBuffer();
            }
        }
    }

    /* two native image checksum functions */
    public native static int getImageChecksumAdler32(CodecImage image);
    public native static String getImageChecksumMD5(CodecImage image);

    public native static void copyFlexYUVImage(CodecImage target, CodecImage source);

    public static void copyFlexYUVImage(Image target, CodecImage source) {
        copyFlexYUVImage(ImageWrapper.createFromImage(target), source);
    }
    public static void copyFlexYUVImage(Image target, Image source) {
        copyFlexYUVImage(
                ImageWrapper.createFromImage(target),
                ImageWrapper.createFromImage(source));
    }

    public native static void fillImageRectWithYUV(
            CodecImage image, Rect area, int y, int u, int v);

    public static void fillImageRectWithYUV(Image image, Rect area, int y, int u, int v) {
        fillImageRectWithYUV(ImageWrapper.createFromImage(image), area, y, u, v);
    }

    public native static long[] getRawStats(CodecImage image, Rect area);

    public static long[] getRawStats(Image image, Rect area) {
        return getRawStats(ImageWrapper.createFromImage(image), area);
    }

    public native static float[] getYUVStats(CodecImage image, Rect area);

    public static float[] getYUVStats(Image image, Rect area) {
        return getYUVStats(ImageWrapper.createFromImage(image), area);
    }

    public native static float[] Raw2YUVStats(long[] rawStats);

    public static String getImageMD5Checksum(Image image) throws Exception {
        int format = image.getFormat();
        if (ImageFormat.YUV_420_888 != format) {
            Log.w(TAG, "unsupported image format");
            return "";
        }

        MessageDigest md = MessageDigest.getInstance("MD5");

        int imageWidth = image.getWidth();
        int imageHeight = image.getHeight();

        Image.Plane[] planes = image.getPlanes();
        for (int i = 0; i < planes.length; ++i) {
            ByteBuffer buf = planes[i].getBuffer();

            int width, height, rowStride, pixelStride, x, y;
            rowStride = planes[i].getRowStride();
            pixelStride = planes[i].getPixelStride();
            if (i == 0) {
                width = imageWidth;
                height = imageHeight;
            } else {
                width = imageWidth / 2;
                height = imageHeight /2;
            }
            // local contiguous pixel buffer
            byte[] bb = new byte[width * height];
            if (buf.hasArray()) {
                byte b[] = buf.array();
                int offs = buf.arrayOffset();
                if (pixelStride == 1) {
                    for (y = 0; y < height; ++y) {
                        System.arraycopy(bb, y * width, b, y * rowStride + offs, width);
                    }
                } else {
                    // do it pixel-by-pixel
                    for (y = 0; y < height; ++y) {
                        int lineOffset = offs + y * rowStride;
                        for (x = 0; x < width; ++x) {
                            bb[y * width + x] = b[lineOffset + x * pixelStride];
                        }
                    }
                }
            } else { // almost always ends up here due to direct buffers
                int pos = buf.position();
                if (pixelStride == 1) {
                    for (y = 0; y < height; ++y) {
                        buf.position(pos + y * rowStride);
                        buf.get(bb, y * width, width);
                    }
                } else {
                    // local line buffer
                    byte[] lb = new byte[rowStride];
                    // do it pixel-by-pixel
                    for (y = 0; y < height; ++y) {
                        buf.position(pos + y * rowStride);
                        // we're only guaranteed to have pixelStride * (width - 1) + 1 bytes
                        buf.get(lb, 0, pixelStride * (width - 1) + 1);
                        for (x = 0; x < width; ++x) {
                            bb[y * width + x] = lb[x * pixelStride];
                        }
                    }
                }
                buf.position(pos);
            }
            md.update(bb, 0, width * height);
        }

        return convertByteArrayToHEXString(md.digest());
    }

    private static String convertByteArrayToHEXString(byte[] ba) throws Exception {
        StringBuilder result = new StringBuilder();
        for (int i = 0; i < ba.length; i++) {
            result.append(Integer.toString((ba[i] & 0xff) + 0x100, 16).substring(1));
        }
        return result.toString();
    }
}

