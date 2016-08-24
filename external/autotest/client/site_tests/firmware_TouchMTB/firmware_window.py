# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides GUI for touch device firmware test using GTK."""

import os
import re
import shutil

import gobject
import gtk
import gtk.gdk
import pango
import tempfile

import common_util
import firmware_utils
import test_conf as conf

from firmware_constants import TFK


TITLE = "Touch Firmware Test"


class BaseFrame(object):
    """A simple base frame class."""
    def __init__(self, label=None, size=None, aspect=False):
        # Create a regular/aspect frame
        self.frame = gtk.AspectFrame() if aspect else gtk.Frame()
        self.frame.set_shadow_type(gtk.SHADOW_ETCHED_OUT)
        self.size = size
        if label:
            self.frame.set_label(label)
            self.frame.set_label_align(0.0, 0.0)
            frame_label = self.frame.get_label_widget()
            markup_str = '<span foreground="%s" size="x-large">%s</span>'
            frame_label.set_markup(markup_str % ('black', label))
        if size:
            width, height = size
            self.frame.set_size_request(width, height)
            if aspect:
                self.frame.set(ratio=(float(width) / height))


class PromptFrame(BaseFrame):
    """A simple frame widget to display the prompt.

    It consists of:
      - A frame
      - a label showing the gesture name
      - a label showing the prompt
      - a label showing the keyboard interactions
    """

    def __init__(self, label=None, size=None):
        super(PromptFrame, self).__init__(label, size)

        # Create a vertical packing box.
        self.vbox = gtk.VBox(False, 0)
        self.frame.add(self.vbox)

        # Create a label to show the gesture name
        self.label_gesture = gtk.Label('Gesture Name')
        self.label_gesture.set_justify(gtk.JUSTIFY_LEFT)
        self.vbox.pack_start(self.label_gesture, True, True, 0)
        # Expand the lable to be wider and wrap the line if necessary.
        if self.size:
            _, label_height = self.label_gesture.get_size_request()
            width, _ = self.size
            label_width = int(width * 0.9)
            self.label_gesture.set_size_request(label_width, label_height)
        self.label_gesture.set_line_wrap(True)

        # Pack a horizontal separator
        self.vbox.pack_start(gtk.HSeparator(), True, True, 0)

        # Create a label to show the prompt
        self.label_prompt = gtk.Label('Prompt')
        self.label_prompt.set_justify(gtk.JUSTIFY_CENTER)
        self.vbox.pack_start(self.label_prompt, True, True, 0)

        # Create a label to show the choice
        self.label_choice = gtk.Label('')
        self.label_choice.set_justify(gtk.JUSTIFY_LEFT)
        self.vbox.pack_start(self.label_choice, True, True, 0)

        # Show all widgets added to this frame
        self.frame.show_all()

    def set_gesture_name(self, string, color='blue'):
        """Set the gesture name in label_gesture."""
        markup_str = '<b><span foreground="%s" size="xx-large"> %s </span></b>'
        self.label_gesture.set_markup(markup_str % (color, string))

    def set_prompt(self, string, color='black'):
        """Set the prompt in label_prompt."""
        markup_str = '<span foreground="%s" size="x-large"> %s </span>'
        self.label_prompt.set_markup(markup_str % (color, string))

    def set_choice(self, string):
        """Set the choice in label_choice."""
        self.label_choice.set_text(string)


class ResultFrame(BaseFrame):
    """A simple frame widget to display the test result.

    It consists of:
      - A frame
      - a scrolled window
      - a label showing the test result
    """
    SCROLL_STEP = 100.0

    def __init__(self, label=None, size=None):
        super(ResultFrame, self).__init__(label, size)

        # Create a scrolled window widget
        self.scrolled_window = gtk.ScrolledWindow()
        self.scrolled_window.set_policy(gtk.POLICY_AUTOMATIC,
                                        gtk.POLICY_AUTOMATIC)
        self.frame.add(self.scrolled_window)

        # Create a vertical packing box.
        self.vbox = gtk.VBox(False, 0)
        self.scrolled_window.add_with_viewport(self.vbox)

        # Create a label to show the gesture name
        self.result = gtk.Label()
        self.vbox.pack_start(self.result , False, False, 0)

        # Show all widgets added to this frame
        self.frame.show_all()

        # Get the vertical and horizontal adjustments
        self.vadj = self.scrolled_window.get_vadjustment()
        self.hadj = self.scrolled_window.get_hadjustment()

        self._scroll_func_dict = {TFK.UP: self._scroll_up,
                                  TFK.DOWN: self._scroll_down,
                                  TFK.LEFT: self._scroll_left,
                                  TFK.RIGHT: self._scroll_right}

    def _calc_result_font_size(self):
        """Calculate the font size so that it does not overflow."""
        label_width_in_px, _ = self.size
        font_size = int(float(label_width_in_px) / conf.num_chars_per_row *
                        pango.SCALE)
        return font_size

    def set_result(self, text, color='black'):
        """Set the text in the result label."""
        mod_text = re.sub('<', '&lt;', text)
        mod_text = re.sub('>', '&gt;', mod_text)
        markup_str = '<b><span foreground="%s" size="%d"> %s </span></b>'
        font_size = self._calc_result_font_size()
        self.result.set_markup(markup_str % (color, font_size, mod_text))

    def _calc_inc_value(self, adj):
        """Calculate new increased value of the specified adjustement object."""
        value = adj.get_value()
        new_value = min(value + self.SCROLL_STEP, adj.upper - adj.page_size)
        return new_value

    def _calc_dec_value(self, adj):
        """Calculate new decreased value of the specified adjustement object."""
        value = adj.get_value()
        new_value = max(value - self.SCROLL_STEP, adj.lower)
        return new_value

    def _scroll_down(self):
        """Scroll the scrolled_window down."""
        self.vadj.set_value(self._calc_inc_value(self.vadj))

    def _scroll_up(self):
        """Scroll the scrolled_window up."""
        self.vadj.set_value(self._calc_dec_value(self.vadj))

    def _scroll_right(self):
        """Scroll the scrolled_window to the right."""
        self.hadj.set_value(self._calc_inc_value(self.hadj))

    def _scroll_left(self):
        """Scroll the scrolled_window to the left."""
        self.hadj.set_value(self._calc_dec_value(self.hadj))

    def scroll(self, choice):
        """Scroll the result frame using the choice key."""
        scroll_method = self._scroll_func_dict.get(choice)
        if scroll_method:
            scroll_method()
        else:
            print 'Warning: the key choice "%s" is not legal!' % choice


class ImageFrame(BaseFrame):
    """A simple frame widget to display the mtplot window.

    It consists of:
      - An aspect frame
      - an image widget showing mtplot
    """

    def __init__(self, label=None, size=None):
        super(ImageFrame, self).__init__(label, size, aspect=True)

        # Use a fixed widget to display the image.
        self.fixed = gtk.Fixed()
        self.frame.add(self.fixed)

        # Create an image widget.
        self.image = gtk.Image()
        self.fixed.put(self.image, 0, 0)

        # Show all widgets added to this frame
        self.frame.show_all()

    def set_from_file(self, filename):
        """Set the image file."""
        self.image.set_from_file(filename)
        self.frame.show_all()


class FirmwareWindow(object):
    """A simple window class to display the touch firmware test window."""

    def __init__(self, size=None, prompt_size=None, result_size=None,
                 image_size=None):
        # Setup gtk environment correctly.
        self._setup_gtk_environment()

        # Create a new window
        self.win = gtk.Window(gtk.WINDOW_TOPLEVEL)
        if size:
            self.win_size = size
            self.win.resize(*size)
        self.win.set_title(TITLE)
        self.win.set_border_width(0)

        # Create the prompt frame
        self.prompt_frame = PromptFrame(TITLE, prompt_size)

        # Create the result frame
        self.result_frame = ResultFrame("Test results:", size=result_size)

        # Create the image frame for mtplot
        self.image_frame = ImageFrame(size=image_size)

        # Handle layout below
        self.box0 = gtk.VBox(False, 0)
        self.box1 = gtk.HBox(False, 0)
        # Arrange the layout about box0
        self.win.add(self.box0)
        self.box0.pack_start(self.prompt_frame.frame, True, True, 0)
        self.box0.pack_start(self.box1, True, True, 0)
        # Arrange the layout about box1
        self.box1.pack_start(self.image_frame.frame, True, True, 0)
        self.box1.pack_start(self.result_frame.frame, True, True, 0)

        # Capture keyboard events.
        self.win.add_events(gtk.gdk.KEY_PRESS_MASK | gtk.gdk.KEY_RELEASE_MASK)

        # Set a handler for delete_event that immediately exits GTK.
        self.win.connect("delete_event", self.delete_event)

        # Show all widgets.
        self.win.show_all()

    def _setup_gtk_environment(self):
        """Set up the gtk environment correctly."""

        def _warning(msg=None):
            print 'Warning: fail to setup gtk environment.'
            if msg:
                print '\t' + msg
            print '\tImage files would not be shown properly.'
            print '\tIt does not affect the test results though.'

        def _make_symlink(path, symlink):
            """Remove the symlink if exists. Create a new symlink to point to
            the given path.
            """
            if os.path.islink(symlink):
                os.remove(symlink)
            os.symlink(real_gtk_dir, self.gtk_symlink)
            self.new_symlink = True

        self.gtk_symlink = None
        self.tmp = tempfile.mkdtemp()
        self.moved_flag = False
        self.original_gtk_realpath = None
        self.new_symlink = False

        # Get LoaderDir:
        # The output of gdk-pixbuf-query-loaders looks like:
        #
        #   GdkPixbuf Image Loader Modules file
        #   Automatically generated file, do not edit
        #   Created by gdk-pixbuf-query-loaders from gtk+-2.20.1
        #
        #   LoaderDir = /usr/lib64/gtk-2.0/2.10.0/loaders
        loader_dir_str = common_util.simple_system_output(
                'gdk-pixbuf-query-loaders | grep LoaderDir')
        result = re.search('(/.*?)/(gtk-.*?)/', loader_dir_str)
        if result:
            prefix = result.group(1)
            self.gtk_version = result.group(2)
        else:
            _warning('Cannot derive gtk version from LoaderDir.')
            return

        # Verify the existence of the loaders file.
        gdk_pixbuf_loaders = ('/usr/local/etc/%s/gdk-pixbuf.loaders' %
                              self.gtk_version)
        if not os.path.isfile(gdk_pixbuf_loaders):
            msg = 'The loaders file "%s" does not exist.' % gdk_pixbuf_loaders
            _warning(msg)
            return

        # Setup the environment variable for GdkPixbuf Image Loader Modules file
        # so that gtk library could find it.
        os.environ['GDK_PIXBUF_MODULE_FILE'] = gdk_pixbuf_loaders

        # In the loaders file, it specifies the paths of various
        # sharable objects (.so) which are used to load images of corresponding
        # image formats. For example, for png loader, the path looks like
        #
        # "/usr/lib64/gtk-2.0/2.10.0/loaders/libpixbufloader-png.so"
        # "png" 5 "gtk20" "The PNG image format" "LGPL"
        # "image/png" ""
        # "png" ""
        # "\211PNG\r\n\032\n" "" 100
        #
        # However, the real path for the .so file is under
        # "/usr/local/lib64/..."
        # Hence, we would like to make a temporary symlink so that
        # gtk library could find the .so file correctly.
        self.gtk_symlink = os.path.join(prefix, self.gtk_version)
        prefix_list = prefix.split('/')
        prefix_list.insert(prefix_list.index('usr') + 1, 'local')
        real_gtk_dir = os.path.join('/', *(prefix_list + [self.gtk_version]))

        # Make sure that the directory of .so files does exist.
        if not os.path.isdir(real_gtk_dir):
            msg = 'The directory of gtk image loaders "%s" does not exist.'
            _warning(msg % real_gtk_dir)
            return

        # Take care of an existing symlink.
        if os.path.islink(self.gtk_symlink):
            # If the symlink does not point to the correct path,
            # save the real path of the symlink and re-create the symlink.
            if not os.path.samefile(self.gtk_symlink, real_gtk_dir):
                self.original_gtk_realpath = os.path.realpath(self.gtk_symlink)
                _make_symlink(real_gtk_dir, self.gtk_symlink)

        # Take care of an existing directory.
        elif os.path.isdir(self.gtk_symlink):
            # Move the directory only if it is not what we expect.
            if not os.path.samefile(self.gtk_symlink, real_gtk_dir):
                shutil.move(self.gtk_symlink, self.tmp)
                self.moved_flag = True
                _make_symlink(real_gtk_dir, self.gtk_symlink)

        # Take care of an existing file.
        # Such a file is not supposed to exist here. Move it anyway.
        elif os.path.isfile(self.gtk_symlink):
            shutil.move(self.gtk_symlink, self.tmp)
            self.moved_flag = True
            _make_symlink(real_gtk_dir, self.gtk_symlink)

        # Just create the temporary symlink since there is nothing here.
        else:
            _make_symlink(real_gtk_dir, self.gtk_symlink)

    def close(self):
        """Cleanup by restoring any symlink, file, or directory if necessary."""
        # Remove the symlink that the test created.
        if self.new_symlink:
            os.remove(self.gtk_symlink)

        # Restore the original symlink.
        if self.original_gtk_realpath:
            os.symlink(self.original_gtk_realpath, self.gtk_symlink)
        # Restore the original file or directory.
        elif self.moved_flag:
            tmp_gtk_path = os.path.join(self.tmp, self.gtk_version)
            if (os.path.isdir(tmp_gtk_path) or os.path.isfile(tmp_gtk_path)):
                shutil.move(tmp_gtk_path, os.path.dirname(self.gtk_symlink))
                self.moved_flag = False
                shutil.rmtree(self.tmp)

    def register_callback(self, event, callback):
        """Register a callback function for an event."""
        self.win.connect(event, callback)

    def register_timeout_add(self, callback, timeout):
        """Register a callback function for gobject.timeout_add."""
        return gobject.timeout_add(timeout, callback)

    def register_io_add_watch(self, callback, fd, data=None,
                              condition=gobject.IO_IN):
        """Register a callback function for gobject.io_add_watch."""
        if data:
            return gobject.io_add_watch(fd, condition, callback, data)
        else:
            return gobject.io_add_watch(fd, condition, callback)

    def create_key_press_event(self, keyval):
        """Create a key_press_event."""
        event = gtk.gdk.Event(gtk.gdk.KEY_PRESS)
        # Assign current time to the event
        event.time = 0
        event.keyval = keyval
        self.win.emit('key_press_event', event)

    def remove_event_source(self, tag):
        """Remove the registered callback."""
        gobject.source_remove(tag)

    def delete_event(self, widget, event, data=None):
        """A handler to exit the window."""
        self.stop()
        return False

    def set_gesture_name(self, string, color='blue'):
        """A helper method to set gesture name."""
        self.prompt_frame.set_gesture_name(string, color)

    def set_prompt(self, string, color='black'):
        """A helper method to set the prompt."""
        self.prompt_frame.set_prompt(string, color)

    def set_choice(self, string):
        """A helper method to set the choice."""
        self.prompt_frame.set_choice(string)

    def set_result(self, text):
        """A helper method to set the text in the result."""
        self.result_frame.set_result(text)

    def set_image(self, filename):
        """Set an image in the image frame."""
        self.image_frame.set_from_file(filename)

    def scroll(self, choice):
        """Scroll the result frame using the choice key."""
        self.result_frame.scroll(choice)

    def stop(self):
        """Quit the window."""
        self.close()
        gtk.main_quit()

    def main(self):
        """Main function of the window."""
        try:
            gtk.main()
        except KeyboardInterrupt:
            self.close()
