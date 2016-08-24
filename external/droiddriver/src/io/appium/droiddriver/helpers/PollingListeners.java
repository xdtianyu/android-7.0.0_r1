package io.appium.droiddriver.helpers;

import io.appium.droiddriver.DroidDriver;
import io.appium.droiddriver.Poller.PollingListener;
import io.appium.droiddriver.exceptions.ElementNotFoundException;
import io.appium.droiddriver.finders.Finder;

/**
 * Static utility methods to create commonly used PollingListeners.
 */
public class PollingListeners {
  /**
   * Tries to find {@code watchFinder}, and clicks it if found.
   *
   * @param driver a DroidDriver instance
   * @param watchFinder Identifies the UI component to watch
   * @return whether {@code watchFinder} is found
   */
  public static boolean tryFindAndClick(DroidDriver driver, Finder watchFinder) {
    try {
      driver.find(watchFinder).click();
      return true;
    } catch (ElementNotFoundException enfe) {
      return false;
    }
  }

  /**
   * Returns a new {@code PollingListener} that will look for
   * {@code watchFinder}, then click {@code dismissFinder} to dismiss it.
   * <p>
   * Typically a {@code PollingListener} is used to dismiss "random" dialogs. If
   * you know the certain situation when a dialog is displayed, you should deal
   * with the dialog in the specific situation instead of using a
   * {@code PollingListener} because it is checked in all polling events, which
   * occur frequently.
   * </p>
   *
   * @param watchFinder Identifies the UI component, for example an AlertDialog
   * @param dismissFinder Identifies the UiElement to click on that will dismiss
   *        the UI component
   */
  public static PollingListener newDismissListener(final Finder watchFinder,
      final Finder dismissFinder) {
    return new PollingListener() {
      @Override
      public void onPolling(DroidDriver driver, Finder finder) {
        if (driver.has(watchFinder)) {
          driver.find(dismissFinder).click();
        }
      }
    };
  }

  private PollingListeners() {}
}
