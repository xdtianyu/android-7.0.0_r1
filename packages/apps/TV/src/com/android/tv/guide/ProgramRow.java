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

package com.android.tv.guide;

import android.content.Context;
import android.graphics.Rect;
import android.support.v7.widget.LinearLayoutManager;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.ViewParent;
import android.view.ViewTreeObserver;

import com.android.tv.data.Channel;
import com.android.tv.guide.ProgramManager.TableEntry;
import com.android.tv.util.Utils;

import java.util.concurrent.TimeUnit;

public class ProgramRow extends TimelineGridView {
    private static final String TAG = "ProgramRow";
    private static final boolean DEBUG = false;

    private static final long ONE_HOUR_MILLIS = TimeUnit.HOURS.toMillis(1);
    private static final long HALF_HOUR_MILLIS = ONE_HOUR_MILLIS / 2;

    private ProgramManager mProgramManager;

    private boolean mKeepFocusToCurrentProgram;
    private ChildFocusListener mChildFocusListener;

    interface ChildFocusListener {
        /**
         * Is called after focus is moved. Only children to {@code ProgramRow} will be passed.
         * See {@code ProgramRow#setChildFocusListener(ChildFocusListener)}.
         */
        void onChildFocus(View oldFocus, View newFocus);
    }

    private final ViewTreeObserver.OnGlobalFocusChangeListener mGlobalFocusChangeListener =
            new ViewTreeObserver.OnGlobalFocusChangeListener() {
                @Override
                public void onGlobalFocusChanged(View oldFocus, View newFocus) {
                    updateCurrentFocus(oldFocus, newFocus);
                }
            };

    /**
     * Used only for debugging.
     */
    private Channel mChannel;

    public ProgramRow(Context context) {
        this(context, null);
    }

    public ProgramRow(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ProgramRow(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    /**
     * Registers a listener focus events occurring on children to the {@code ProgramRow}.
     */
    public void setChildFocusListener(ChildFocusListener childFocusListener) {
        mChildFocusListener = childFocusListener;
    }

    @Override
    public void onScrolled(int dx, int dy) {
        super.onScrolled(dx, dy);
        int childCount = getChildCount();
        if (DEBUG) {
            Log.d(TAG, "onScrolled by " + dx);
            Log.d(TAG, "channelId=" + mChannel.getId() + ", childCount=" + childCount);
            Log.d(TAG, "ProgramRow {" + Utils.toRectString(this) + "}");
        }
        for (int i = 0; i < childCount; ++i) {
            ProgramItemView child = (ProgramItemView) getChildAt(i);
            if (getLeft() <= child.getRight() && child.getLeft() <= getRight()) {
                child.layoutVisibleArea(getLayoutDirection() == LAYOUT_DIRECTION_LTR
                        ? getLeft() - child.getLeft() : child.getRight() - getRight());
            }
        }
    }

    /**
     * Moves focus to the current program.
     */
    public void focusCurrentProgram() {
        View currentProgram = getCurrentProgramView();
        if (currentProgram == null) {
            currentProgram = getChildAt(0);
        }
        updateCurrentFocus(null, currentProgram);
    }

    private void updateCurrentFocus(View oldFocus, View newFocus) {
        if (mChildFocusListener == null) {
            return;
        }

        mChildFocusListener.onChildFocus(isChild(oldFocus) ? oldFocus : null,
                isChild(newFocus) ? newFocus : null);
    }

    private boolean isChild(View view) {
        if (view == null) {
            return false;
        }

        for (ViewParent p = view.getParent(); p != null; p = p.getParent()) {
            if (p == this) {
                return true;
            }
        }
        return false;
    }

    // Call this API after RTL is resolved. (i.e. View is measured.)
    private boolean isDirectionStart(int direction) {
        return getLayoutDirection() == LAYOUT_DIRECTION_LTR
                ? direction == View.FOCUS_LEFT : direction == View.FOCUS_RIGHT;
    }

    // Call this API after RTL is resolved. (i.e. View is measured.)
    private boolean isDirectionEnd(int direction) {
        return getLayoutDirection() == LAYOUT_DIRECTION_LTR
                ? direction == View.FOCUS_RIGHT : direction == View.FOCUS_LEFT;
    }

    @Override
    public View focusSearch(View focused, int direction) {
        TableEntry focusedEntry = ((ProgramItemView) focused).getTableEntry();
        long fromMillis = mProgramManager.getFromUtcMillis();
        long toMillis = mProgramManager.getToUtcMillis();

        if (isDirectionStart(direction) || direction == View.FOCUS_BACKWARD) {
            if (focusedEntry.entryStartUtcMillis < fromMillis) {
                // The current entry starts outside of the view; Align or scroll to the left.
                scrollByTime(Math.max(-ONE_HOUR_MILLIS,
                        focusedEntry.entryStartUtcMillis - fromMillis));
                return focused;
            }
        } else if (isDirectionEnd(direction) || direction == View.FOCUS_FORWARD) {
            if (focusedEntry.entryEndUtcMillis > toMillis + ONE_HOUR_MILLIS) {
                // The current entry ends outside of the view; Scroll to the right.
                scrollByTime(ONE_HOUR_MILLIS);
                return focused;
            }
        }

        View target = super.focusSearch(focused, direction);
        if (!(target instanceof ProgramItemView)) {
            if (isDirectionEnd(direction) || direction == View.FOCUS_FORWARD) {
                if (focusedEntry.entryEndUtcMillis != toMillis) {
                    // The focused entry is the last entry; Align to the right edge.
                    scrollByTime(focusedEntry.entryEndUtcMillis - mProgramManager.getToUtcMillis());
                    return focused;
                }
            }
            return target;
        }

        TableEntry targetEntry = ((ProgramItemView) target).getTableEntry();

        if (isDirectionStart(direction) || direction == View.FOCUS_BACKWARD) {
            if (targetEntry.entryStartUtcMillis < fromMillis &&
                    targetEntry.entryEndUtcMillis < fromMillis + HALF_HOUR_MILLIS) {
                // The target entry starts outside the view; Align or scroll to the left.
                scrollByTime(Math.max(-ONE_HOUR_MILLIS,
                        targetEntry.entryStartUtcMillis - fromMillis));
            }
        } else if (isDirectionEnd(direction) || direction == View.FOCUS_FORWARD) {
            if (targetEntry.entryStartUtcMillis > fromMillis + ONE_HOUR_MILLIS + HALF_HOUR_MILLIS) {
                // The target entry starts outside the view; Align or scroll to the right.
                scrollByTime(Math.min(ONE_HOUR_MILLIS,
                        targetEntry.entryStartUtcMillis - fromMillis - ONE_HOUR_MILLIS));
            }
        }

        return target;
    }

    private void scrollByTime(long timeToScroll) {
        if (DEBUG) {
            Log.d(TAG, "scrollByTime(timeToScroll="
                    + TimeUnit.MILLISECONDS.toMinutes(timeToScroll) + "min)");
        }
        mProgramManager.shiftTime(timeToScroll);
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        getViewTreeObserver().addOnGlobalFocusChangeListener(mGlobalFocusChangeListener);
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        getViewTreeObserver().removeOnGlobalFocusChangeListener(mGlobalFocusChangeListener);
    }

    @Override
    public void onChildDetachedFromWindow(View child) {
        if (child.hasFocus()) {
            // Focused view can be detached only if it's updated.
            TableEntry entry = ((ProgramItemView) child).getTableEntry();
            if (entry.isCurrentProgram()) {
                if (DEBUG) Log.d(TAG, "Keep focus to the current program");
                // Current program is visible in the guide.
                // Updated entries including current program's will be attached again soon
                // so give focus back in onChildAttachedToWindow().
                mKeepFocusToCurrentProgram = true;
            }
            // TODO: Try to keep focus for non-current program.
        }
        super.onChildDetachedFromWindow(child);
    }

    @Override
    public void onChildAttachedToWindow(View child) {
        super.onChildAttachedToWindow(child);
        if (mKeepFocusToCurrentProgram) {
            TableEntry entry = ((ProgramItemView) child).getTableEntry();
            if (entry.isCurrentProgram()) {
                post(new Runnable() {
                    @Override
                    public void run() {
                        requestFocus();
                    }
                });
                mKeepFocusToCurrentProgram = false;
            }
        }
    }

    @Override
    public boolean onRequestFocusInDescendants(int direction, Rect previouslyFocusedRect) {
        // Give focus to the current program by default.
        // Note that this logic is used only if requestFocus() is called to the ProgramRow,
        // so focus finding logic will not be blocked by this.
        View currentProgram = getCurrentProgramView();
        if (currentProgram != null) {
            return currentProgram.requestFocus();
        }

        if (DEBUG) Log.d(TAG, "onRequestFocusInDescendants");

        boolean result = super.onRequestFocusInDescendants(direction, previouslyFocusedRect);
        if (!result) {
            // The default focus search logic of LeanbackLibrary is sometimes failed.
            // As a fallback solution, we request focus to the first focusable view.
            for (int i = 0; i < getChildCount(); ++i) {
                View child = getChildAt(i);
                if (child.isShown() && child.hasFocusable()) {
                    return child.requestFocus();
                }
            }
        }
        return result;
    }

    private View getCurrentProgramView() {
        for (int i = 0; i < getChildCount(); ++i) {
            TableEntry entry = ((ProgramItemView) getChildAt(i)).getTableEntry();
            if (entry.isCurrentProgram()) {
                return getChildAt(i);
            }
        }
        return null;
    }

    public void setChannel(Channel channel) {
        mChannel = channel;
    }

    /**
     * Sets the instance of {@link ProgramManager}
     */
    public void setProgramManager(ProgramManager programManager) {
        mProgramManager = programManager;
    }

    /**
     * Resets the scroll with the initial offset {@code scrollOffset}.
     */
    public void resetScroll(int scrollOffset) {
        long startTime = GuideUtils.convertPixelToMillis(scrollOffset)
                + mProgramManager.getStartTime();
        int position = mChannel == null ? -1 : mProgramManager.getProgramIndexAtTime(
                mChannel.getId(), startTime);
        if (position < 0) {
            getLayoutManager().scrollToPosition(0);
        } else {
            TableEntry entry = mProgramManager.getTableEntry(mChannel.getId(), position);
            int offset = GuideUtils.convertMillisToPixel(
                    mProgramManager.getStartTime(), entry.entryStartUtcMillis) - scrollOffset;
            ((LinearLayoutManager) getLayoutManager())
                    .scrollToPositionWithOffset(position, offset);
        }
    }
}
