package android.view.cts;

import android.os.Parcel;
import android.test.InstrumentationTestCase;
import android.view.KeyEvent;
import android.view.KeyboardShortcutGroup;
import android.view.KeyboardShortcutInfo;

import com.google.android.collect.Lists;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests for {@link android.view.KeyboardShortcutGroup}.
 */
public class KeyboardShortcutGroupTest extends InstrumentationTestCase {
    private static final CharSequence TEST_LABEL = "Test Group Label";
    private final List<KeyboardShortcutInfo> TEST_ITEMS = Lists.newArrayList(
            new KeyboardShortcutInfo("Item 1", KeyEvent.KEYCODE_U, KeyEvent.META_CTRL_ON),
            new KeyboardShortcutInfo("Item 2", KeyEvent.KEYCODE_F, KeyEvent.META_CTRL_ON));

    public void testConstructor() {
        KeyboardShortcutGroup group = new KeyboardShortcutGroup(TEST_LABEL, TEST_ITEMS);

        assertEquals(TEST_LABEL, group.getLabel());
        assertEquals(TEST_ITEMS, group.getItems());
        assertFalse(group.isSystemGroup());
        assertEquals(0, group.describeContents());
    }

    public void testShortConstructor() {
        KeyboardShortcutGroup group = new KeyboardShortcutGroup(TEST_LABEL);

        assertEquals(TEST_LABEL, group.getLabel());
        assertNotNull(group.getItems());
        assertFalse(group.isSystemGroup());
        assertEquals(0, group.describeContents());
    }

    public void testSystemConstructor() {
        KeyboardShortcutGroup group = new KeyboardShortcutGroup(TEST_LABEL, TEST_ITEMS, true);

        assertEquals(TEST_LABEL, group.getLabel());
        assertEquals(TEST_ITEMS, group.getItems());
        assertTrue(group.isSystemGroup());
        assertEquals(0, group.describeContents());
    }

    public void testSystemShortConstructor() {
        KeyboardShortcutGroup group = new KeyboardShortcutGroup(TEST_LABEL, true);

        assertEquals(TEST_LABEL, group.getLabel());
        assertNotNull(group.getItems());
        assertTrue(group.isSystemGroup());
        assertEquals(0, group.describeContents());
    }

    public void testConstructorChecksList() {
        try {
            KeyboardShortcutGroup group = new KeyboardShortcutGroup(TEST_LABEL, null);
        } catch (NullPointerException expected) {
            return;
        }
        fail();
    }

    public void testAddItem() {
        KeyboardShortcutGroup group = new KeyboardShortcutGroup(TEST_LABEL, TEST_ITEMS);

        group.addItem(new KeyboardShortcutInfo(
                "Additional item", KeyEvent.KEYCODE_P, KeyEvent.META_CTRL_ON));

        final int newSize = group.getItems().size();
        assertEquals(TEST_ITEMS.size() + 1, newSize);
        assertEquals("Additional item", group.getItems().get(newSize - 1).getLabel());
    }

    public void testWriteToParcelAndRead() {
        Parcel dest = Parcel.obtain();
        KeyboardShortcutGroup group = new KeyboardShortcutGroup(TEST_LABEL, TEST_ITEMS, true);
        group.writeToParcel(dest, 0);

        dest.setDataPosition(0);
        KeyboardShortcutGroup result = KeyboardShortcutGroup.CREATOR.createFromParcel(dest);

        assertEquals(TEST_LABEL, result.getLabel());
        assertEquals(TEST_ITEMS.size(), result.getItems().size());
        assertEquals(TEST_ITEMS.get(0).getLabel(), result.getItems().get(0).getLabel());
        assertEquals(TEST_ITEMS.get(1).getLabel(), result.getItems().get(1).getLabel());
        assertEquals(TEST_ITEMS.get(0).getKeycode(), result.getItems().get(0).getKeycode());
        assertEquals(TEST_ITEMS.get(1).getKeycode(), result.getItems().get(1).getKeycode());
        assertEquals(TEST_ITEMS.get(0).getModifiers(), result.getItems().get(0).getModifiers());
        assertEquals(TEST_ITEMS.get(1).getModifiers(), result.getItems().get(1).getModifiers());
        assertTrue(result.isSystemGroup());
    }
}
