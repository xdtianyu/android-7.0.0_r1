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
 * limitations under the License
 */

package android.graphics.drawable.cts;

import android.graphics.cts.R;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.cts.ImageViewCtsActivity;
import android.graphics.drawable.Icon;
import android.graphics.drawable.Drawable;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.os.Parcel;
import android.test.ActivityInstrumentationTestCase2;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;

public class IconTest extends ActivityInstrumentationTestCase2<ImageViewCtsActivity> {
    static final long TIMEOUT = 1000;

    Activity mActivity;
    Instrumentation mInstrumentation;
    Icon mIcon;

    MockOnDrawableLoadedListener mListener;
    MockRunner mRunner;

    public IconTest() {
        super("android.graphics.cts", ImageViewCtsActivity.class);
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
        mInstrumentation = getInstrumentation();
    }

    public void testBitmapIcon() {
        checkIconValidity(
                Icon.createWithBitmap(Bitmap.createBitmap(16, 16, Bitmap.Config.ARGB_8888)));
    }

    public void testDataIcon() {
        byte[] data = new byte[4];
        data[0] = data[1] = data[2] = data[3] = (byte)255;
        checkIconValidity(Icon.createWithData(data, 0, 4));
    }

    public void testFileIcon() throws IOException {
        File file = new File(mActivity.getFilesDir(), "testimage.jpg");
        try {
            writeSampleImage(file);
            assertTrue(file.exists());

            checkIconValidity(Icon.createWithFilePath(file.getPath()));

            checkIconValidity(Icon.createWithContentUri(Uri.fromFile(file)));

            checkIconValidity(Icon.createWithContentUri(file.toURI().toString()));
        } finally {
            file.delete();
        }
    }

    public void testResourceIcon() {
        checkIconValidity(Icon.createWithResource(mActivity, R.drawable.bmp_test));

        checkIconValidity(Icon.createWithResource(mActivity.getPackageName(), R.drawable.bmp_test));
    }

    public void testLoadDrawableAsync() {
        mIcon = Icon.createWithBitmap(Bitmap.createBitmap(16, 16, Bitmap.Config.ARGB_8888));

        mListener = new MockOnDrawableLoadedListener();
        mInstrumentation.runOnMainSync(new Runnable() {
            @Override
            public void run() {
                mIcon.loadDrawableAsync(mActivity, mListener, new Handler());
            }
        });
        sleep(TIMEOUT);

        assertEquals(1, mListener.getLoadedCount());
    }

    public void testLoadDrawableAsyncWithMessage() {
        mIcon = Icon.createWithBitmap(Bitmap.createBitmap(16, 16, Bitmap.Config.ARGB_8888));

        mRunner = new MockRunner();
        mInstrumentation.runOnMainSync(new Runnable() {
            @Override
            public void run() {
                mIcon.loadDrawableAsync(mActivity, Message.obtain(new Handler(), mRunner));
            }
        });
        sleep(TIMEOUT);

        assertEquals(1, mRunner.getRunCount());
    }

    class MockOnDrawableLoadedListener implements Icon.OnDrawableLoadedListener {
        int mLoadedCount;

        @Override
        public void onDrawableLoaded(Drawable d) {
            assertNotNull(d);
            ++mLoadedCount;
        }

        int getLoadedCount() { return mLoadedCount; }
    }

    class MockRunner implements Runnable {
        int mRun;

        @Override
        public void run() {
            ++mRun;
        }

        int getRunCount() { return mRun; }
    };

    private void sleep(long time) {
        try {
            Thread.sleep(time);
        } catch (InterruptedException e) {
            fail(e.getMessage());
        }
    }

    private void writeSampleImage(File imagefile) throws IOException {
        InputStream source = null;
        OutputStream target = null;

        try {
            source = mActivity.getResources().openRawResource(R.drawable.testimage);
            target = new FileOutputStream(imagefile);

            byte[] buffer = new byte[1024];
            for (int len = source.read(buffer); len >= 0; len = source.read(buffer)) {
                target.write(buffer, 0, len);
            }
        } finally {
            if (target != null) {
                target.close();
            }

            if (source != null) {
                source.close();
            }
        }
    }

    // Check if the created icon is valid and doesn't cause crashes for the public methods.
    private void checkIconValidity(Icon icon) {
        assertNotNull(icon);

        // tint properties.
        icon.setTint(Color.BLUE);
        icon.setTintList(ColorStateList.valueOf(Color.RED));
        icon.setTintMode(PorterDuff.Mode.XOR);

        // Parcelable methods.
        icon.describeContents();
        Parcel parcel = Parcel.obtain();
        icon.writeToParcel(parcel, 0);

        parcel.setDataPosition(0);
        assertNotNull(Icon.CREATOR.createFromParcel(parcel));

        // loading drawable synchronously.
        assertNotNull(icon.loadDrawable(mActivity));
    }
}
