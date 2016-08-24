/*
 * Copyright (C) 2016 The Android Open Source Project
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
package android.transition.cts;

import android.app.Activity;
import android.app.Instrumentation;
import android.test.ActivityInstrumentationTestCase2;
import android.test.suitebuilder.annotation.MediumTest;
import android.transition.Slide;
import android.transition.TransitionManager;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;

import org.junit.Test;

import java.util.concurrent.TimeUnit;

@MediumTest
public class SlideEdgeTest extends ActivityInstrumentationTestCase2<TransitionActivity>  {
    private static final Object[][] sSlideEdgeArray = {
            { Gravity.START, "START" },
            { Gravity.END, "END" },
            { Gravity.LEFT, "LEFT" },
            { Gravity.TOP, "TOP" },
            { Gravity.RIGHT, "RIGHT" },
            { Gravity.BOTTOM, "BOTTOM" },
    };

    public SlideEdgeTest() {
        super(TransitionActivity.class);
    }

    public void testSetSide() throws Throwable {
        for (int i = 0; i < sSlideEdgeArray.length; i++) {
            int slideEdge = (Integer) (sSlideEdgeArray[i][0]);
            String edgeName = (String) (sSlideEdgeArray[i][1]);
            Slide slide = new Slide(slideEdge);
            assertEquals("Edge not set properly in constructor " + edgeName,
                    slideEdge, slide.getSlideEdge());

            slide = new Slide();
            slide.setSlideEdge(slideEdge);
            assertEquals("Edge not set properly with setter " + edgeName,
                    slideEdge, slide.getSlideEdge());
        }
    }

    public void testSlideOut() throws Throwable {
        for (int i = 0; i < sSlideEdgeArray.length; i++) {
            final int slideEdge = (Integer) (sSlideEdgeArray[i][0]);
            final Slide slide = new Slide(slideEdge);
            final SimpleTransitionListener listener = new SimpleTransitionListener();
            slide.addListener(listener);

            final Instrumentation instrumentation = getInstrumentation();
            final Activity activity = getActivity();
            instrumentation.runOnMainSync(new Runnable() {
                @Override
                public void run() {
                    activity.setContentView(R.layout.scene1);
                }
            });
            instrumentation.waitForIdleSync();

            final View redSquare = activity.findViewById(R.id.redSquare);
            final View greenSquare = activity.findViewById(R.id.greenSquare);
            final View hello = activity.findViewById(R.id.hello);
            final ViewGroup sceneRoot = (ViewGroup) activity.findViewById(R.id.holder);

            instrumentation.runOnMainSync(new Runnable() {
                @Override
                public void run() {
                    TransitionManager.beginDelayedTransition(sceneRoot, slide);
                    redSquare.setVisibility(View.INVISIBLE);
                    greenSquare.setVisibility(View.INVISIBLE);
                    hello.setVisibility(View.INVISIBLE);
                }
            });
            assertTrue(listener.startLatch.await(1, TimeUnit.SECONDS));
            assertEquals(1, listener.endLatch.getCount());
            assertEquals(View.VISIBLE, redSquare.getVisibility());
            assertEquals(View.VISIBLE, greenSquare.getVisibility());
            assertEquals(View.VISIBLE, hello.getVisibility());

            float redStartX = redSquare.getTranslationX();
            float redStartY = redSquare.getTranslationY();

            Thread.sleep(200);
            assertTranslation(slideEdge, redSquare);
            assertTranslation(slideEdge, greenSquare);
            assertTranslation(slideEdge, hello);

            final float redMidX = redSquare.getTranslationX();
            final float redMidY = redSquare.getTranslationY();

            switch (slideEdge) {
                case Gravity.LEFT:
                case Gravity.START:
                    assertTrue(
                            "isn't sliding out to left. Expecting " + redStartX + " > " + redMidX,
                            redStartX > redMidX);
                    break;
                case Gravity.RIGHT:
                case Gravity.END:
                    assertTrue(
                            "isn't sliding out to right. Expecting " + redStartX + " < " + redMidX,
                            redStartX < redMidX);
                    break;
                case Gravity.TOP:
                    assertTrue("isn't sliding out to top. Expecting " + redStartY + " > " + redMidY,
                            redStartY > redSquare.getTranslationY());
                    break;
                case Gravity.BOTTOM:
                    assertTrue(
                            "isn't sliding out to bottom. Expecting " + redStartY + " < " + redMidY,
                            redStartY < redSquare.getTranslationY());
                    break;
            }
            assertTrue(listener.endLatch.await(1, TimeUnit.SECONDS));
            instrumentation.waitForIdleSync();

            assertNoTranslation(redSquare);
            assertNoTranslation(greenSquare);
            assertNoTranslation(hello);
            assertEquals(View.INVISIBLE, redSquare.getVisibility());
            assertEquals(View.INVISIBLE, greenSquare.getVisibility());
            assertEquals(View.INVISIBLE, hello.getVisibility());
        }
    }

    public void testSlideIn() throws Throwable {
        for (int i = 0; i < sSlideEdgeArray.length; i++) {
            final int slideEdge = (Integer) (sSlideEdgeArray[i][0]);
            final Slide slide = new Slide(slideEdge);
            final SimpleTransitionListener listener = new SimpleTransitionListener();
            slide.addListener(listener);

            final Instrumentation instrumentation = getInstrumentation();
            final Activity activity = getActivity();

            instrumentation.runOnMainSync(new Runnable() {
                @Override
                public void run() {
                    activity.setContentView(R.layout.scene1);
                }
            });
            instrumentation.waitForIdleSync();

            final View redSquare = activity.findViewById(R.id.redSquare);
            final View greenSquare = activity.findViewById(R.id.greenSquare);
            final View hello = activity.findViewById(R.id.hello);
            final ViewGroup sceneRoot = (ViewGroup) activity.findViewById(R.id.holder);

            instrumentation.runOnMainSync(new Runnable() {
                @Override
                public void run() {
                    redSquare.setVisibility(View.INVISIBLE);
                    greenSquare.setVisibility(View.INVISIBLE);
                    hello.setVisibility(View.INVISIBLE);
                }
            });
            instrumentation.waitForIdleSync();

            // now slide in
            instrumentation.runOnMainSync(new Runnable() {
                @Override
                public void run() {
                    TransitionManager.beginDelayedTransition(sceneRoot, slide);
                    redSquare.setVisibility(View.VISIBLE);
                    greenSquare.setVisibility(View.VISIBLE);
                    hello.setVisibility(View.VISIBLE);
                }
            });
            assertTrue(listener.startLatch.await(1, TimeUnit.SECONDS));

            assertEquals(1, listener.endLatch.getCount());
            assertEquals(View.VISIBLE, redSquare.getVisibility());
            assertEquals(View.VISIBLE, greenSquare.getVisibility());
            assertEquals(View.VISIBLE, hello.getVisibility());

            final float redStartX = redSquare.getTranslationX();
            final float redStartY = redSquare.getTranslationY();

            Thread.sleep(200);
            assertTranslation(slideEdge, redSquare);
            assertTranslation(slideEdge, greenSquare);
            assertTranslation(slideEdge, hello);
            final float redMidX = redSquare.getTranslationX();
            final float redMidY = redSquare.getTranslationY();

            switch (slideEdge) {
                case Gravity.LEFT:
                case Gravity.START:
                    assertTrue(
                            "isn't sliding in from left. Expecting " + redStartX + " < " + redMidX,
                            redStartX < redMidX);
                    break;
                case Gravity.RIGHT:
                case Gravity.END:
                    assertTrue(
                            "isn't sliding in from right. Expecting " + redStartX + " > " + redMidX,
                            redStartX > redMidX);
                    break;
                case Gravity.TOP:
                    assertTrue(
                            "isn't sliding in from top. Expecting " + redStartY + " < " + redMidY,
                            redStartY < redSquare.getTranslationY());
                    break;
                case Gravity.BOTTOM:
                    assertTrue("isn't sliding in from bottom. Expecting " + redStartY + " > "
                                    + redMidY,
                            redStartY > redSquare.getTranslationY());
                    break;
            }
            assertTrue(listener.endLatch.await(1, TimeUnit.SECONDS));
            instrumentation.waitForIdleSync();

            assertNoTranslation(redSquare);
            assertNoTranslation(greenSquare);
            assertNoTranslation(hello);
            assertEquals(View.VISIBLE, redSquare.getVisibility());
            assertEquals(View.VISIBLE, greenSquare.getVisibility());
            assertEquals(View.VISIBLE, hello.getVisibility());
        }
    }

    private void assertTranslation(int slideEdge, View view) {
        switch (slideEdge) {
            case Gravity.LEFT:
            case Gravity.START:
                assertTrue(view.getTranslationX() < 0);
                assertEquals(0f, view.getTranslationY(), 0.01f);
                break;
            case Gravity.RIGHT:
            case Gravity.END:
                assertTrue(view.getTranslationX() > 0);
                assertEquals(0f, view.getTranslationY(), 0.01f);
                break;
            case Gravity.TOP:
                assertTrue(view.getTranslationY() < 0);
                assertEquals(0f, view.getTranslationX(), 0.01f);
                break;
            case Gravity.BOTTOM:
                assertTrue(view.getTranslationY() > 0);
                assertEquals(0f, view.getTranslationX(), 0.01f);
                break;
        }
    }

    private void assertNoTranslation(View view) {
        assertEquals(0f, view.getTranslationX(), 0.01f);
        assertEquals(0f, view.getTranslationY(), 0.01f);
    }
}

