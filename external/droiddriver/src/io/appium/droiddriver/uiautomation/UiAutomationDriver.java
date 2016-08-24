/*
 * Copyright (C) 2013 DroidDriver committers
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

package io.appium.droiddriver.uiautomation;

import android.annotation.TargetApi;
import android.app.Instrumentation;
import android.app.UiAutomation;
import android.content.Context;
import android.os.SystemClock;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeInfo;

import io.appium.droiddriver.actions.InputInjector;
import io.appium.droiddriver.base.BaseDroidDriver;
import io.appium.droiddriver.exceptions.TimeoutException;
import io.appium.droiddriver.uiautomation.UiAutomationContext.UiAutomationCallable;
import io.appium.droiddriver.util.Logs;

/**
 * Implementation of DroidDriver that gets attributes via the Accessibility API
 * and is acted upon via synthesized events.
 */
@TargetApi(18)
public class UiAutomationDriver extends BaseDroidDriver<AccessibilityNodeInfo, UiAutomationElement> {
  // This is a magic const copied from UiAutomator.
  /**
   * This value has the greatest bearing on the appearance of test execution
   * speeds. This value is used as the minimum time to wait before considering
   * the UI idle after each action.
   */
  private static final long QUIET_TIME_TO_BE_CONSIDERD_IDLE_STATE = 500;// ms
  private static long idleTimeoutMillis = QUIET_TIME_TO_BE_CONSIDERD_IDLE_STATE;

  /** Sets the {@code idleTimeoutMillis} argument for calling {@link UiAutomation#waitForIdle} */
  public static void setIdleTimeoutMillis(long idleTimeoutMillis) {
    UiAutomationDriver.idleTimeoutMillis = idleTimeoutMillis;
  }

  private final UiAutomationContext context;
  private final InputInjector injector;
  private final UiAutomationUiDevice uiDevice;
  private AccessibilityNodeInfoCacheClearer clearer =
      new WindowStateAccessibilityNodeInfoCacheClearer();

  public UiAutomationDriver(Instrumentation instrumentation) {
    context = new UiAutomationContext(instrumentation, this);
    injector = new UiAutomationInputInjector(context);
    uiDevice = new UiAutomationUiDevice(context);
  }

  @Override
  public InputInjector getInjector() {
    return injector;
  }

  @Override
  protected UiAutomationElement newRootElement() {
    return context.newRootElement(getRootNode());
  }

  @Override
  protected UiAutomationElement newUiElement(AccessibilityNodeInfo rawElement,
      UiAutomationElement parent) {
    return new UiAutomationElement(context, rawElement, parent);
  }

  private AccessibilityNodeInfo getRootNode() {
    final long timeoutMillis = getPoller().getTimeoutMillis();
    context.callUiAutomation(new UiAutomationCallable<Void>() {
      @Override
      public Void call(UiAutomation uiAutomation) {
        try {
          uiAutomation.waitForIdle(idleTimeoutMillis, timeoutMillis);
          return null;
        } catch (java.util.concurrent.TimeoutException e) {
          throw new TimeoutException(e);
        }
      }
    });

    long end = SystemClock.uptimeMillis() + timeoutMillis;
    while (true) {
      AccessibilityNodeInfo root =
          context.callUiAutomation(new UiAutomationCallable<AccessibilityNodeInfo>() {
            @Override
            public AccessibilityNodeInfo call(UiAutomation uiAutomation) {
              return uiAutomation.getRootInActiveWindow();
            }
          });
      if (root != null) {
        return root;
      }
      long remainingMillis = end - SystemClock.uptimeMillis();
      if (remainingMillis < 0) {
        throw new TimeoutException(
            String.format("Timed out after %d milliseconds waiting for root AccessibilityNodeInfo",
                timeoutMillis));
      }
      SystemClock.sleep(Math.min(250, remainingMillis));
    }
  }

  /**
   * Some widgets fail to trigger some AccessibilityEvent's after actions,
   * resulting in stale AccessibilityNodeInfo's. As a work-around, force to
   * clear the AccessibilityNodeInfoCache.
   */
  public void clearAccessibilityNodeInfoCache() {
    Logs.call(this, "clearAccessibilityNodeInfoCache");
    clearer.clearAccessibilityNodeInfoCache(this);
  }

  public interface AccessibilityNodeInfoCacheClearer {
    void clearAccessibilityNodeInfoCache(UiAutomationDriver driver);
  }

  /**
   * Clears AccessibilityNodeInfoCache by turning screen off then on.
   */
  public static class ScreenOffAccessibilityNodeInfoCacheClearer implements
      AccessibilityNodeInfoCacheClearer {
    public void clearAccessibilityNodeInfoCache(UiAutomationDriver driver) {
      driver.getUiDevice().sleep();
      driver.getUiDevice().wakeUp();
    }
  }

  /**
   * Clears AccessibilityNodeInfoCache by exploiting an implementation detail of
   * AccessibilityNodeInfoCache. This is a hack; use it at your own discretion.
   */
  public static class WindowStateAccessibilityNodeInfoCacheClearer implements
      AccessibilityNodeInfoCacheClearer {
    public void clearAccessibilityNodeInfoCache(UiAutomationDriver driver) {
      AccessibilityManager accessibilityManager =
          (AccessibilityManager) driver.context.getInstrumentation().getTargetContext()
              .getSystemService(Context.ACCESSIBILITY_SERVICE);
      accessibilityManager.sendAccessibilityEvent(AccessibilityEvent
          .obtain(AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED));
    }
  }

  public void setAccessibilityNodeInfoCacheClearer(AccessibilityNodeInfoCacheClearer clearer) {
    this.clearer = clearer;
  }

  @Override
  public UiAutomationUiDevice getUiDevice() {
    return uiDevice;
  }
}
