/*
 * Copyright (C) 2015 DroidDriver committers
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

/**
 * Helper classes for writing an Android UI test framework using DroidDriver.
 *
 * <h2>UI test framework design principles</h2>
 *
 * A UI test framework should model the UI of the AUT in a hierarchical way to maximize code reuse.
 * Common interactions should be abstracted as methods of page objects. Uncommon interactions may
 * not be abstracted, but carried out using "driver" directly.
 * <p>
 * The organization of the entities (pages, components) does not need to strictly follow the AUT
 * structure. The UI model can be greatly simplified to make it easy to use.
 * <p>
 * In general the framework should follow these principles:
 * <ul>
 *   <li>Layered abstraction: at the highest level, methods completely abstract the implementation
 *       detail. This kind of methods carry out a complex action, usually involving multiple steps.
 *       At a lower level, methods can expose some details, e.g. clickInstallButton(), which does a
 *       single action and returns a dialog instance it opens, and let the caller decide how to
 *       further interact with it. Lastly at the lowest level, you can always use "driver" to access
 *       any elements if no higher-level methods are available.</li>
 *   <li>Instance methods of a page object assume the page is currently shown.</li>
 *   <li>If a method opens another page, it should return that page on a best-effort basis. There
 *       could be exceptions where we let callers determine the type of the new page, but that
 *       should be considered hacks and be clearly documented.</li>
 *   <li>The page object constructors are public so that it's easy to hack as mentioned above, but
 *       don't abuse it -- typically callers should acquire page objects by calling methods of other
 *       page objects. The root is the home page of the AUT.</li>
 *   <li>Simple dialogs may not merit their own top-level classes, and can be nested as static
 *       subclasses.</li>
 *   <li>Define constants that use values generated from Android resources instead of using string
 *       literals. For example, call {@link android.content.Context#getResources} to get the
 *       Resources instance, then call {@link android.content.res.Resources#getResourceName} to get
 *       the string representation of a resource id, or call {@link
 *       android.content.res.Resources#getString} to get the localized string of a string resource.
 *       This gives you compile-time check over incompatible changes.</li>
 *   <li>Avoid public constants. Typically clients of a page object are interested in what can be
 *       done on the page (the content or actions), not how to achieve that (which is an
 *       implementation detail). The constants used by the page object hence should be encapsulated
 *       (declared private). Another reason for this item is that the constants may not be real
 *       constants. Instead they are generated from resources and acquiring the values requires the
 *       {@link android.content.Context}, which is not available until setUp() is called. If those
 *       are referenced in static fields of a test class, they will be initialized at class loading
 *       time and result in a crash.</li>
 *   <li>There are cases that exposing public constants is arguably desired. For example, when the
 *       interaction is trivial (e.g. clicking a button that does not open a new page), and there
 *       are many similar elements on the page, thus adding distinct methods for them will bloat the
 *       page object class. In these cases you may define public constants, with a warning that
 *       "Don't use them in static fields of tests".</li>
 * </ul>
 *
 * <h2>Common pitfalls</h2>
 * <ul>
 *   <li>UI elements are generally views. Users can get attributes and perform actions. Note that
 *       actions often update a UiElement, so users are advised not to store instances of UiElement
 *       for later use - the instances could become stale. In other words, UiElement represents a
 *       dynamic object, while Finder represents a static object. Don't declare fields of the type
 *       UiElement; use Finder instead.</li>
 *   <li>{@link android.test.ActivityInstrumentationTestCase2#getActivity} calls
 *       {@link android.test.InstrumentationTestCase#launchActivityWithIntent}, which may hang in
 *       {@link android.app.Instrumentation#waitForIdleSync}. You can call
 *       {@link android.content.Context#startActivity} directly.</li>
 *   <li>startActivity does not wait until the new Activity is shown. This may cause problem when
 *       the old Activity on screen contains UiElements that match what are expected on the new
 *       Activity - interaction with the UiElements fails because the old Activity is closing.
 *       Sometimes it shows as a test passes when run alone but fails when run with other tests.
 *       The work-around is to add a delay after calling startActivity.</li>
 *   <li>Error "android.content.res.Resources$NotFoundException: Unable to find resource ID ..."?
 *       <br>
 *       This may occur if you reference the AUT's resource in tests, and the two APKs are out of
 *       sync. Solution: build and install both AUT and tests together.</li>
 *   <li>"You said the test runs on older devices as well as API18 devices, but mine is broken on
 *       X (e.g. GingerBread)!"
 *       <br>
 *       This may occur if your AUT has different implementations on older devices. In this case,
 *       your tests have to match the different execution paths of AUT, which requires insight into
 *       the implementation of the AUT. A tip for testing older devices: uiautomatorviewer does not
 *       work on ore-API16 devices (the "Device screenshot" button won't work), but you can use it
 *       with dumps from DroidDriver (use to-uiautomator.xsl to convert the format).</li>
 *   <li>"com.android.launcher has stopped unexpectedly" and logcat says OutOfMemoryError
 *       <br>
 *       This is sometimes seen on GingerBread or other low-memory and slow devices. GC is not fast
 *       enough to reclaim memory on those devices. A work-around: call gc more aggressively and
 *       sleep to let gc run, e.g.
 *       <pre>       
public void setUp() throws Exception {
  super.setUp();
  if (Build.VERSION.SDK_INT &lt;= Build.VERSION_CODES.GINGERBREAD_MR1) {
    Runtime.getRuntime().gc();
    SystemClock.sleep(1000L);
  }
}
</pre></li>
 * </ul>
 */
package io.appium.droiddriver.helpers;
