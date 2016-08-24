/*
 * Copyright (C) 2014 The Android Open Source Project
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
package android.uirendering.cts.bitmapverifiers;

/**
 * Used when the tester wants to find the opposite result from a Verifier
 */
public class InvertVerifier extends BitmapVerifier {
    private BitmapVerifier mBitmapVerifier;

    public InvertVerifier(BitmapVerifier bitmapVerifier) {
        mBitmapVerifier = bitmapVerifier;
    }

    @Override
    public boolean verify(int[] bitmap, int offset, int stride, int width, int height) {
        boolean success = mBitmapVerifier.verify(bitmap, offset, stride, width, height);
        mDifferenceBitmap = mBitmapVerifier.getDifferenceBitmap();
        return !success;
    }
}
