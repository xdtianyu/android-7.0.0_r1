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

import android.transition.ChangeTransform;
import android.transition.TransitionManager;
import android.view.View;
import android.view.ViewGroup;

public class ChangeTransformTest extends BaseTransitionTest {
    ChangeTransform mChangeTransform;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        resetChangeBoundsTransition();
    }

    private void resetChangeBoundsTransition() {
        mChangeTransform = new ChangeTransform();
        mTransition = mChangeTransform;
        resetListener();
    }

    public void testTranslation() throws Throwable {
        enterScene(R.layout.scene1);

        final View redSquare = mActivity.findViewById(R.id.redSquare);

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                TransitionManager.beginDelayedTransition(mSceneRoot, mChangeTransform);
                redSquare.setTranslationX(500);
                redSquare.setTranslationY(600);
            }
        });
        waitForStart();

        assertEquals(1, mListener.endLatch.getCount()); // still running
        // There is no way to validate the intermediate matrix because it uses
        // hidden properties of the View to execute.
        waitForEnd(400);
        assertEquals(500f, redSquare.getTranslationX());
        assertEquals(600f, redSquare.getTranslationY());
    }

    public void testRotation() throws Throwable {
        enterScene(R.layout.scene1);

        final View redSquare = mActivity.findViewById(R.id.redSquare);

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                TransitionManager.beginDelayedTransition(mSceneRoot, mChangeTransform);
                redSquare.setRotation(45);
            }
        });
        waitForStart();

        assertEquals(1, mListener.endLatch.getCount()); // still running
        // There is no way to validate the intermediate matrix because it uses
        // hidden properties of the View to execute.
        waitForEnd(400);
        assertEquals(45f, redSquare.getRotation());
    }

    public void testScale() throws Throwable {
        enterScene(R.layout.scene1);

        final View redSquare = mActivity.findViewById(R.id.redSquare);

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                TransitionManager.beginDelayedTransition(mSceneRoot, mChangeTransform);
                redSquare.setScaleX(2f);
                redSquare.setScaleY(3f);
            }
        });
        waitForStart();

        assertEquals(1, mListener.endLatch.getCount()); // still running
        // There is no way to validate the intermediate matrix because it uses
        // hidden properties of the View to execute.
        waitForEnd(400);
        assertEquals(2f, redSquare.getScaleX());
        assertEquals(3f, redSquare.getScaleY());
    }

    public void testReparent() throws Throwable {
        assertEquals(true, mChangeTransform.getReparent());
        enterScene(R.layout.scene5);
        startTransition(R.layout.scene9);
        assertEquals(1, mListener.endLatch.getCount()); // still running
        waitForEnd(400);

        resetListener();
        mChangeTransform.setReparent(false);
        assertEquals(false, mChangeTransform.getReparent());
        startTransition(R.layout.scene5);
        waitForEnd(0); // no transition to run because reparent == false
    }

    public void testReparentWithOverlay() throws Throwable {
        assertEquals(true, mChangeTransform.getReparentWithOverlay());
        enterScene(R.layout.scene5);
        startTransition(R.layout.scene9);
        assertEquals(1, mListener.endLatch.getCount()); // still running
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                View view = new View(mActivity);
                view.setRight(100);
                view.setBottom(100);
                mSceneRoot.getOverlay().add(view);
                ViewGroup container = (ViewGroup) view.getParent();
                assertEquals(2, container.getChildCount());
                mSceneRoot.getOverlay().remove(view);
                assertTrue(mActivity.findViewById(R.id.text).getVisibility() != View.VISIBLE);
            }
        });
        waitForEnd(400);

        mChangeTransform.setReparentWithOverlay(false);
        assertEquals(false, mChangeTransform.getReparentWithOverlay());
        resetListener();
        startTransition(R.layout.scene5);
        assertEquals(1, mListener.endLatch.getCount()); // still running
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                View view = new View(mActivity);
                view.setRight(100);
                view.setBottom(100);
                mSceneRoot.getOverlay().add(view);
                ViewGroup container = (ViewGroup) view.getParent();
                assertEquals(1, container.getChildCount());
                mSceneRoot.getOverlay().remove(view);
                assertEquals(View.VISIBLE, mActivity.findViewById(R.id.text).getVisibility());
            }
        });
        waitForEnd(400);
    }
}

