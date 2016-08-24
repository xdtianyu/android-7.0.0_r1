package io.appium.droiddriver.manualtest;

import android.app.Activity;

import io.appium.droiddriver.finders.By;
import io.appium.droiddriver.finders.Finder;
import io.appium.droiddriver.helpers.BaseDroidDriverTest;
import io.appium.droiddriver.helpers.DroidDrivers;
import io.appium.droiddriver.helpers.DroidDriversInitializer;
import io.appium.droiddriver.uiautomation.UiAutomationDriver;

/**
 * This is for manually testing DroidDriver. It is not meant for continuous
 * testing. Instead it is used for debugging failures. It assumes the device is
 * in a condition that is ready to reproduce a failure. For example,
 * {@link #testSetTextForPassword} assumes the password_edit field is displayed
 * on screen.
 * <p>
 * Run it as (optionally with -e debug true)
 *
 * <pre>
 * adb shell am instrument -w io.appium.droiddriver.manualtest/io.appium.droiddriver.runner.TestRunner
 * </pre>
 */
public class ManualTest extends BaseDroidDriverTest<Activity> {
  public ManualTest() {
    super(Activity.class);
  }

  // This does not instrument a certain AUT, so InstrumentationDriver won't work
  protected void classSetUp() {
    DroidDrivers.checkUiAutomation();
    DroidDriversInitializer.get(new UiAutomationDriver(getInstrumentation())).singleRun();
  }

  public void testSetTextForPassword() {
    Finder password_edit = By.resourceId("com.google.android.gsf.login:id/password_edit");
    String oldPassword = "A fake password that is not empty and needs to be cleared by setText";
    String newPassword = "1";
    driver.on(password_edit).setText(oldPassword);
    driver.on(password_edit).setText(newPassword);
    // This won't work because password_edit does not reveal text to
    // Accessibility service. But you can see the length changed on screen.
    // assertEquals(newPassword, driver.on(password_edit).getText());
    assertEquals(null, driver.on(password_edit).getText());
  }
}
