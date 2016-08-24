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

package io.appium.droiddriver.base;

import android.os.SystemClock;

import java.util.Collection;
import java.util.LinkedList;

import io.appium.droiddriver.DroidDriver;
import io.appium.droiddriver.Poller;
import io.appium.droiddriver.exceptions.NoRunningActivityException;
import io.appium.droiddriver.exceptions.TimeoutException;
import io.appium.droiddriver.finders.Finder;

/**
 * Default implementation of a {@link Poller}.
 */
public class DefaultPoller implements Poller {
  private final Collection<TimeoutListener> timeoutListeners = new LinkedList<TimeoutListener>();
  private final Collection<PollingListener> pollingListeners = new LinkedList<PollingListener>();
  private long timeoutMillis = 10000;
  private long intervalMillis = 500;

  @Override
  public long getIntervalMillis() {
    return intervalMillis;
  }

  @Override
  public void setIntervalMillis(long intervalMillis) {
    this.intervalMillis = intervalMillis;
  }

  @Override
  public long getTimeoutMillis() {
    return timeoutMillis;
  }

  @Override
  public void setTimeoutMillis(long timeoutMillis) {
    this.timeoutMillis = timeoutMillis;
  }

  @Override
  public <T> T pollFor(DroidDriver driver, Finder finder, ConditionChecker<T> checker) {
    return pollFor(driver, finder, checker, timeoutMillis);
  }

  @Override
  public <T> T pollFor(DroidDriver driver, Finder finder, ConditionChecker<T> checker,
      long timeoutMillis) {
    long end = SystemClock.uptimeMillis() + timeoutMillis;
    while (true) {
      try {
        try {
          driver.refreshUiElementTree();
        } catch (NoRunningActivityException nrae) {
          if (checker == GONE) {
            return null;
          }
          throw nrae;
        }
        return checker.check(driver, finder);
      } catch (UnsatisfiedConditionException uce) {
        // fall through to poll
      }

      for (PollingListener pollingListener : pollingListeners) {
        pollingListener.onPolling(driver, finder);
      }

      long remainingMillis = end - SystemClock.uptimeMillis();
      if (remainingMillis < 0) {
        for (TimeoutListener timeoutListener : timeoutListeners) {
          timeoutListener.onTimeout(driver, finder);
        }
        throw new TimeoutException(String.format(
            "Timed out after %d milliseconds waiting for %s %s", timeoutMillis, finder, checker));
      }
      SystemClock.sleep(Math.min(intervalMillis, remainingMillis));
    }
  }

  @Override
  public ListenerRemover addListener(final TimeoutListener timeoutListener) {
    timeoutListeners.add(timeoutListener);
    return new ListenerRemover() {
      @Override
      public void remove() {
        timeoutListeners.remove(timeoutListener);
      }
    };
  }

  @Override
  public ListenerRemover addListener(final PollingListener pollingListener) {
    pollingListeners.add(pollingListener);
    return new ListenerRemover() {
      @Override
      public void remove() {
        pollingListeners.remove(pollingListener);
      }
    };
  }
}
