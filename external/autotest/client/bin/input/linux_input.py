# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Description:
#
# Python version of include/linux/input.h
#  as of kernel v2.6.39

from linux_ioctl import *

# The event structure itself
#   struct input_event {
#       struct timeval time;
#       __u16 type;
#       __u16 code;
#       __s32 value;
#   };
#
#   struct timeval {
#       __kernel_time_t         tv_sec;         /* seconds */
#       __kernel_suseconds_t    tv_usec;        /* microseconds */
#   };
input_event_t = 'LLHHi'


# Protocol version.
EV_VERSION = 0x010001

# IOCTLs (0x00 - 0x7f)

#   struct input_id {
#       __u16 bustype;
#       __u16 vendor;
#       __u16 product;
#       __u16 version;
#   };
input_id_t = 'HHHH'

# struct input_absinfo - used by EVIOCGABS/EVIOCSABS ioctls
# @value: latest reported value for the axis.
# @minimum: specifies minimum value for the axis.
# @maximum: specifies maximum value for the axis.
# @fuzz: specifies fuzz value used to filter noise from the event stream.
# @flat: values within this value are reported as 0.
# @resolution: specifies resolution for the values reported for the axis.
#
# Note that input core does not clamp reported values to the
# [minimum, maximum] limits, such task is left to userspace.
#
# Resolution for main axes (ABS_X, ABS_Y, ABS_Z) is reported in
# units per millimeter (units/mm), resolution for rotational axes
# (ABS_RX, ABS_RY, ABS_RZ) is reported in units per radian.
#
#   struct input_absinfo {
#       __s32 value;
#       __s32 minimum;
#       __s32 maximum;
#       __s32 fuzz;
#       __s32 flat;
#       __s32 resolution;
#   };
input_absinfo_t = 'iiiiii'

# struct input_keymap_entry - used by EVIOCGKEYCODE/EVIOCSKEYCODE ioctls
# @scancode: scancode represented in machine-endian form.
# @len: length of the scancode that resides in @scancode buffer.
# @index: index in the keymap, may be used instead of scancode
# @flags: allows to specify how kernel should handle the request. For
# example, setting INPUT_KEYMAP_BY_INDEX flag indicates that kernel
# should perform lookup in keymap by @index instead of @scancode
# @keycode: key code assigned to this scancode
#
# The structure is used to retrieve and modify keymap data. Users have
# option of performing lookup either by @scancode itself or by @index
# in keymap entry. EVIOCGKEYCODE will also return scancode or index
# (depending on which element was used to perform lookup).
#
#   struct input_keymap_entry {
#       __u8  flags;
#       __u8  len;
#       __u16 index;
#       __u32 keycode;
#       __u8  scancode[32];
#   };
input_keymap_entry_t = 'BBHI32B'

EVIOCGVERSION           = IOR('E', 0x01, 'i')           # get driver version
EVIOCGID                = IOR('E', 0x02, input_id_t)    # get device ID
EVIOCGREP               = IOR('E', 0x03, '2I')          # get repeat settings
EVIOCSREP               = IOW('E', 0x03, '2I')          # set repeat settings

EVIOCGKEYCODE           = IOR('E', 0x04, '2I')          # get keycode
EVIOCGKEYCODE_V2        = IOR('E', 0x04, input_keymap_entry_t)
EVIOCSKEYCODE           = IOW('E', 0x04, '2I')          # set keycode
EVIOCSKEYCODE_V2        = IOW('E', 0x04, input_keymap_entry_t)

def EVIOCGNAME(length):
    return IOC(IOC_READ, 'E', 0x06, length)  # get device name

def EVIOCGPHYS(length):
    return IOC(IOC_READ, 'E', 0x07, length)  # get physical location

def EVIOCGUNIQ(length):
    return IOC(IOC_READ, 'E', 0x08, length)  # get unique identifier

def EVIOCGPROP(length):
    return IOC(IOC_READ, 'E', 0x09, length)  # get device properties

def EVIOCGMTSLOTS(length):
    return IOC(IOC_READ, 'E', 0x0a, length)  # get mt slot values

def EVIOCGKEY(length):
    return IOC(IOC_READ, 'E', 0x18, length)  # get global key state

def EVIOCGLED(length):
    return IOC(IOC_READ, 'E', 0x19, length)  # get all LEDs

def EVIOCGSND(length):
    return IOC(IOC_READ, 'E', 0x1a, length)  # get all sounds status

def EVIOCGSW(length):
    return IOC(IOC_READ, 'E', 0x1b, length)  # get all switch states

def EVIOCGBIT(ev,length):
    return IOC(IOC_READ, 'E', 0x20 + ev, length)  # get event bits

def EVIOCGABS(axis):
    return IOR('E', 0x40 + axis, input_absinfo_t)  # get abs value/limits

def EVIOCSABS(axis):
    return IOW('E', 0xc0 + axis, input_absinfo_t)  # set abs value/limits

# send a force effect to a force feedback device
"""  TODO: Support force feedback. """
#EVIOCSFF                = IOW('E', 0x80, ff_effect_t)
# Erase a force effect
EVIOCRMFF               = IOW('E', 0x81, 'i')
# Report number of effects playable at the same time
EVIOCGEFFECTS           = IOR('E', 0x84, 'i')
# Grab/Release device
EVIOCGRAB               = IOW('E', 0x90, 'i')

# Device properties and quirks
INPUT_PROP_POINTER   = 0x00 # needs a pointer
INPUT_PROP_DIRECT    = 0x01 # direct input devices
INPUT_PROP_BUTTONPAD = 0x02 # has button(s) under pad
INPUT_PROP_SEMI_MT   = 0x03 # touch rectangle only

INPUT_PROP_MAX       = 0x1f
INPUT_PROP_CNT       = (INPUT_PROP_MAX + 1)

# Event types
EV_SYN          = 0x00
EV_KEY          = 0x01
EV_REL          = 0x02
EV_ABS          = 0x03
EV_MSC          = 0x04
EV_SW           = 0x05
EV_LED          = 0x11
EV_SND          = 0x12
EV_REP          = 0x14
EV_FF           = 0x15
EV_PWR          = 0x16
EV_FF_STATUS    = 0x17
EV_MAX          = 0x1f
EV_CNT          = (EV_MAX + 1)

# Synchronization events.

SYN_REPORT      = 0
SYN_CONFIG      = 1
SYN_MT_REPORT   = 2
SYN_DROPPED     = 3

"""
 * Keys and buttons
 *
 * Most of the keys/buttons are modeled after USB HUT 1.12
 * (see http://www.usb.org/developers/hidpage).
 * Abbreviations in the comments:
 * AC - Application Control
 * AL - Application Launch Button
 * SC - System Control
"""

KEY_RESERVED    =  0
KEY_ESC         =  1
KEY_1           =  2
KEY_2           =  3
KEY_3           =  4
KEY_4           =  5
KEY_5           =  6
KEY_6           =  7
KEY_7           =  8
KEY_8           =  9
KEY_9           = 10
KEY_0           = 11
KEY_MINUS       = 12
KEY_EQUAL       = 13
KEY_BACKSPACE   = 14
KEY_TAB         = 15
KEY_Q           = 16
KEY_W           = 17
KEY_E           = 18
KEY_R           = 19
KEY_T           = 20
KEY_Y           = 21
KEY_U           = 22
KEY_I           = 23
KEY_O           = 24
KEY_P           = 25
KEY_LEFTBRACE   = 26
KEY_RIGHTBRACE  = 27
KEY_ENTER       = 28
KEY_LEFTCTRL    = 29
KEY_A           = 30
KEY_S           = 31
KEY_D           = 32
KEY_F           = 33
KEY_G           = 34
KEY_H           = 35
KEY_J           = 36
KEY_K           = 37
KEY_L           = 38
KEY_SEMICOLON   = 39
KEY_APOSTROPHE  = 40
KEY_GRAVE       = 41
KEY_LEFTSHIFT   = 42
KEY_BACKSLASH   = 43
KEY_Z           = 44
KEY_X           = 45
KEY_C           = 46
KEY_V           = 47
KEY_B           = 48
KEY_N           = 49
KEY_M           = 50
KEY_COMMA       = 51
KEY_DOT         = 52
KEY_SLASH       = 53
KEY_RIGHTSHIFT  = 54
KEY_KPASTERISK  = 55
KEY_LEFTALT     = 56
KEY_SPACE       = 57
KEY_CAPSLOCK    = 58
KEY_F1          = 59
KEY_F2          = 60
KEY_F3          = 61
KEY_F4          = 62
KEY_F5          = 63
KEY_F6          = 64
KEY_F7          = 65
KEY_F8          = 66
KEY_F9          = 67
KEY_F10         = 68
KEY_NUMLOCK     = 69
KEY_SCROLLLOCK  = 70
KEY_KP7         = 71
KEY_KP8         = 72
KEY_KP9         = 73
KEY_KPMINUS     = 74
KEY_KP4         = 75
KEY_KP5         = 76
KEY_KP6         = 77
KEY_KPPLUS      = 78
KEY_KP1         = 79
KEY_KP2         = 80
KEY_KP3         = 81
KEY_KP0         = 82
KEY_KPDOT       = 83

KEY_ZENKAKUHANKAKU = 85
KEY_102ND       = 86
KEY_F11         = 87
KEY_F12         = 88
KEY_RO          = 89
KEY_KATAKANA    = 90
KEY_HIRAGANA    = 91
KEY_HENKAN      = 92
KEY_KATAKANAHIRAGANA = 93
KEY_MUHENKAN    = 94
KEY_KPJPCOMMA   = 95
KEY_KPENTER     = 96
KEY_RIGHTCTRL   = 97
KEY_KPSLASH     = 98
KEY_SYSRQ       = 99
KEY_RIGHTALT    = 100
KEY_LINEFEED    = 101
KEY_HOME        = 102
KEY_UP          = 103
KEY_PAGEUP      = 104
KEY_LEFT        = 105
KEY_RIGHT       = 106
KEY_END         = 107
KEY_DOWN        = 108
KEY_PAGEDOWN    = 109
KEY_INSERT      = 110
KEY_DELETE      = 111
KEY_MACRO       = 112
KEY_MUTE        = 113
KEY_VOLUMEDOWN  = 114
KEY_VOLUMEUP    = 115
KEY_POWER       = 116     # SC System Power Down
KEY_KPEQUAL     = 117
KEY_KPPLUSMINUS = 118
KEY_PAUSE       = 119
KEY_SCALE       = 120     # AL Compiz Scale (Expose)

KEY_KPCOMMA     = 121
KEY_HANGEUL     = 122
KEY_HANGUEL     = KEY_HANGEUL
KEY_HANJA       = 123
KEY_YEN         = 124
KEY_LEFTMETA    = 125
KEY_RIGHTMETA   = 126
KEY_COMPOSE     = 127

KEY_STOP        = 128   # AC Stop
KEY_AGAIN       = 129
KEY_PROPS       = 130   # AC Properties
KEY_UNDO        = 131   # AC Undo
KEY_FRONT       = 132
KEY_COPY        = 133   # AC Copy
KEY_OPEN        = 134   # AC Open
KEY_PASTE       = 135   # AC Paste
KEY_FIND        = 136   # AC Search
KEY_CUT         = 137   # AC Cut
KEY_HELP        = 138   # AL Integrated Help Center
KEY_MENU        = 139   # Menu (show menu)
KEY_CALC        = 140   # AL Calculator
KEY_SETUP       = 141
KEY_SLEEP       = 142   # SC System Sleep
KEY_WAKEUP      = 143   # System Wake Up
KEY_FILE        = 144   # AL Local Machine Browser
KEY_SENDFILE    = 145
KEY_DELETEFILE  = 146
KEY_XFER        = 147
KEY_PROG1       = 148
KEY_PROG2       = 149
KEY_WWW         = 150   # AL Internet Browser
KEY_MSDOS       = 151
KEY_COFFEE      = 152   # AL Terminal Lock/Screensaver
KEY_SCREENLOCK  = KEY_COFFEE
KEY_DIRECTION   = 153
KEY_CYCLEWINDOWS = 154
KEY_MAIL        = 155
KEY_BOOKMARKS   = 156   # AC Bookmarks
KEY_COMPUTER    = 157
KEY_BACK        = 158   # AC Back
KEY_FORWARD     = 159   # AC Forward
KEY_CLOSECD     = 160
KEY_EJECTCD     = 161
KEY_EJECTCLOSECD = 162
KEY_NEXTSONG    = 163
KEY_PLAYPAUSE   = 164
KEY_PREVIOUSSONG = 165
KEY_STOPCD      = 166
KEY_RECORD      = 167
KEY_REWIND      = 168
KEY_PHONE       = 169   # Media Select Telephone
KEY_ISO         = 170
KEY_CONFIG      = 171   # AL Consumer Control Configuration
KEY_HOMEPAGE    = 172   # AC Home
KEY_REFRESH     = 173   # AC Refresh
KEY_EXIT        = 174   # AC Exit
KEY_MOVE        = 175
KEY_EDIT        = 176
KEY_SCROLLUP    = 177
KEY_SCROLLDOWN  = 178
KEY_KPLEFTPAREN = 179
KEY_KPRIGHTPAREN = 180
KEY_NEW         = 181   # AC New
KEY_REDO        = 182   # AC Redo/Repeat

KEY_F13         = 183
KEY_F14         = 184
KEY_F15         = 185
KEY_F16         = 186
KEY_F17         = 187
KEY_F18         = 188
KEY_F19         = 189
KEY_F20         = 190
KEY_F21         = 191
KEY_F22         = 192
KEY_F23         = 193
KEY_F24         = 194

KEY_PLAYCD      = 200
KEY_PAUSECD     = 201
KEY_PROG3       = 202
KEY_PROG4       = 203
KEY_DASHBOARD   = 204   # AL Dashboard
KEY_SUSPEND     = 205
KEY_CLOSE       = 206   # AC Close
KEY_PLAY        = 207
KEY_FASTFORWARD = 208
KEY_BASSBOOST   = 209
KEY_PRINT       = 210   # AC Print
KEY_HP          = 211
KEY_CAMERA      = 212
KEY_SOUND       = 213
KEY_QUESTION    = 214
KEY_EMAIL       = 215
KEY_CHAT        = 216
KEY_SEARCH      = 217
KEY_CONNECT     = 218
KEY_FINANCE     = 219   #AL Checkbook/Finance
KEY_SPORT       = 220
KEY_SHOP        = 221
KEY_ALTERASE    = 222
KEY_CANCEL      = 223   # AC Cancel
KEY_BRIGHTNESSDOWN = 224
KEY_BRIGHTNESSUP = 225
KEY_MEDIA       = 226

KEY_SWITCHVIDEOMODE = 227   # Cycle between available video
                            # outputs (Monitor/LCD/TV-out/etc)
KEY_KBDILLUMTOGGLE  = 228
KEY_KBDILLUMDOWN    = 229
KEY_KBDILLUMUP      = 230

KEY_SEND            = 231   # AC Send
KEY_REPLY           = 232   # AC Reply
KEY_FORWARDMAIL     = 233   # AC Forward Msg
KEY_SAVE            = 234   # AC Save
KEY_DOCUMENTS       = 235

KEY_BATTERY         = 236

KEY_BLUETOOTH       = 237
KEY_WLAN            = 238
KEY_UWB             = 239

KEY_UNKNOWN         = 240

KEY_VIDEO_NEXT      = 241   # drive next video source
KEY_VIDEO_PREV      = 242   # drive previous video source
KEY_BRIGHTNESS_CYCLE = 243  # brightness up, after max is min
KEY_BRIGHTNESS_ZERO = 244   # brightness off, use ambient
KEY_DISPLAY_OFF     = 245   # display device to off state

KEY_WIMAX           = 246
KEY_RFKILL          = 247   # Key that controls all radios

# Code 255 is reserved for special needs of AT keyboard driver

BTN_MISC        = 0x100
BTN_0           = 0x100
BTN_1           = 0x101
BTN_2           = 0x102
BTN_3           = 0x103
BTN_4           = 0x104
BTN_5           = 0x105
BTN_6           = 0x106
BTN_7           = 0x107
BTN_8           = 0x108
BTN_9           = 0x109

BTN_MOUSE       = 0x110
BTN_LEFT        = 0x110
BTN_RIGHT       = 0x111
BTN_MIDDLE      = 0x112
BTN_SIDE        = 0x113
BTN_EXTRA       = 0x114
BTN_FORWARD     = 0x115
BTN_BACK        = 0x116
BTN_TASK        = 0x117

BTN_JOYSTICK    = 0x120
BTN_TRIGGER     = 0x120
BTN_THUMB       = 0x121
BTN_THUMB2      = 0x122
BTN_TOP         = 0x123
BTN_TOP2        = 0x124
BTN_PINKIE      = 0x125
BTN_BASE        = 0x126
BTN_BASE2       = 0x127
BTN_BASE3       = 0x128
BTN_BASE4       = 0x129
BTN_BASE5       = 0x12a
BTN_BASE6       = 0x12b
BTN_DEAD        = 0x12f

BTN_GAMEPAD     = 0x130
BTN_A           = 0x130
BTN_B           = 0x131
BTN_C           = 0x132
BTN_X           = 0x133
BTN_Y           = 0x134
BTN_Z           = 0x135
BTN_TL          = 0x136
BTN_TR          = 0x137
BTN_TL2         = 0x138
BTN_TR2         = 0x139
BTN_SELECT      = 0x13a
BTN_START       = 0x13b
BTN_MODE        = 0x13c
BTN_THUMBL      = 0x13d
BTN_THUMBR      = 0x13e

BTN_DIGI        = 0x140
BTN_TOOL_PEN    = 0x140
BTN_TOOL_RUBBER = 0x141
BTN_TOOL_BRUSH  = 0x142
BTN_TOOL_PENCIL = 0x143
BTN_TOOL_AIRBRUSH = 0x144
BTN_TOOL_FINGER = 0x145
BTN_TOOL_MOUSE  = 0x146
BTN_TOOL_LENS   = 0x147
BTN_TOOL_QUINTTAP = 0x148  # Five fingers on trackpad
BTN_TOUCH       = 0x14a
BTN_STYLUS      = 0x14b
BTN_STYLUS2     = 0x14c
BTN_TOOL_DOUBLETAP = 0x14d
BTN_TOOL_TRIPLETAP = 0x14e
BTN_TOOL_QUADTAP = 0x14f   # Four fingers on trackpad

BTN_WHEEL       = 0x150
BTN_GEAR_DOWN   = 0x150
BTN_GEAR_UP     = 0x151


KEY_OK          = 0x160
KEY_SELECT      = 0x161
KEY_GOTO        = 0x162
KEY_CLEAR       = 0x163
KEY_POWER2      = 0x164
KEY_OPTION      = 0x165
KEY_INFO        = 0x166   #AL OEM Features/Tips/Tutorial
KEY_TIME        = 0x167
KEY_VENDOR      = 0x168
KEY_ARCHIVE     = 0x169
KEY_PROGRAM     = 0x16a   # Media Select Program Guide
KEY_CHANNEL     = 0x16b
KEY_FAVORITES   = 0x16c
KEY_EPG         = 0x16d
KEY_PVR         = 0x16e   # Media Select Home
KEY_MHP         = 0x16f
KEY_LANGUAGE    = 0x170
KEY_TITLE       = 0x171
KEY_SUBTITLE    = 0x172
KEY_ANGLE       = 0x173
KEY_ZOOM        = 0x174
KEY_MODE        = 0x175
KEY_KEYBOARD    = 0x176
KEY_SCREEN      = 0x177
KEY_PC          = 0x178   # Media Select Computer
KEY_TV          = 0x179   # Media Select TV
KEY_TV2         = 0x17a   # Media Select Cable
KEY_VCR         = 0x17b   # Media Select VCR
KEY_VCR2        = 0x17c   # VCR Plus
KEY_SAT         = 0x17d   # Media Select Satellite
KEY_SAT2        = 0x17e
KEY_CD          = 0x17f   # Media Select CD
KEY_TAPE        = 0x180   # Select Tape
KEY_RADIO       = 0x181
KEY_TUNER       = 0x182   # Media Select Tuner
KEY_PLAYER      = 0x183
KEY_TEXT        = 0x184
KEY_DVD         = 0x185   # Media Select DVD
KEY_AUX         = 0x186
KEY_MP3         = 0x187
KEY_AUDIO       = 0x188   # AL Audio Browser
KEY_VIDEO       = 0x189   # AL Movie Browser
KEY_DIRECTORY   = 0x18a
KEY_LIST        = 0x18b
KEY_MEMO        = 0x18   # Media Select Messages
KEY_CALENDAR    = 0x18d
KEY_RED         = 0x18e
KEY_GREEN       = 0x18f
KEY_YELLOW      = 0x190
KEY_BLUE        = 0x191
KEY_CHANNELUP   = 0x192   # Channel Increment
KEY_CHANNELDOWN = 0x193   # Channel Decrement
KEY_FIRST       = 0x194
KEY_LAST        = 0x195   # Recall Last
KEY_AB          = 0x196
KEY_NEXT        = 0x197
KEY_RESTART     = 0x198
KEY_SLOW        = 0x199
KEY_SHUFFLE     = 0x19a
KEY_BREAK       = 0x19b
KEY_PREVIOUS    = 0x19c
KEY_DIGITS      = 0x19d
KEY_TEEN        = 0x19e
KEY_TWEN        = 0x19f
KEY_VIDEOPHONE  = 0x1a0   # Media Select Video Phone
KEY_GAMES       = 0x1a1   # Media Select Games
KEY_ZOOMIN      = 0x1a2   # AC Zoom In
KEY_ZOOMOUT     = 0x1a3   # AC Zoom Out
KEY_ZOOMRESET   = 0x1a4   # AC Zoom
KEY_WORDPROCESSOR = 0x1a5   # AL Word Processor
KEY_EDITOR      = 0x1a6   # AL Text Editor
KEY_SPREADSHEET = 0x1a7   # AL Spreadsheet
KEY_GRAPHICSEDITOR = 0x1a8   # AL Graphics Editor
KEY_PRESENTATION = 0x1a9   # AL Presentation App
KEY_DATABASE    = 0x1aa   # AL Database App
KEY_NEWS        = 0x1ab   # AL Newsreader
KEY_VOICEMAIL   = 0x1ac   # AL Voicemail
KEY_ADDRESSBOOK = 0x1ad   # AL Contacts/Address Book
KEY_MESSENGER   = 0x1ae   # AL Instant Messaging
KEY_DISPLAYTOGGLE = 0x1af   # Turn display (LCD) on and off
KEY_SPELLCHECK  = 0x1b0   # AL Spell Check
KEY_LOGOFF      = 0x1b1   #* AL Logoff

KEY_DOLLAR      = 0x1b2
KEY_EURO        = 0x1b3

KEY_FRAMEBACK   = 0x1b4   # Consumer - transport controls
KEY_FRAMEFORWARD = 0x1b5
KEY_CONTEXT_MENU = 0x1b6   # GenDesc - system context menu
KEY_MEDIA_REPEAT = 0x1b7   # Consumer - transport control
KEY_10CHANNELSUP = 0x1b8   # 10 channels up (10+)
KEY_10CHANNELSDOWN = 0x1b9   # 10 channels down (10-)
KEY_IMAGES       = 0x1ba   # AL Image Browser

KEY_DEL_EOL      = 0x1c0
KEY_DEL_EOS      = 0x1c1
KEY_INS_LINE     = 0x1c2
KEY_DEL_LINE     = 0x1c3

KEY_FN           = 0x1d0
KEY_FN_ESC       = 0x1d1
KEY_FN_F1        = 0x1d2
KEY_FN_F2        = 0x1d3
KEY_FN_F3        = 0x1d4
KEY_FN_F4        = 0x1d5
KEY_FN_F5        = 0x1d6
KEY_FN_F6        = 0x1d7
KEY_FN_F7        = 0x1d8
KEY_FN_F8        = 0x1d9
KEY_FN_F9        = 0x1da
KEY_FN_F10       = 0x1db
KEY_FN_F11       = 0x1dc
KEY_FN_F12       = 0x1dd
KEY_FN_1         = 0x1de
KEY_FN_2         = 0x1df
KEY_FN_D         = 0x1e0
KEY_FN_E         = 0x1e1
KEY_FN_F         = 0x1e2
KEY_FN_S         = 0x1e3
KEY_FN_B         = 0x1e4

KEY_BRL_DOT1     = 0x1f1
KEY_BRL_DOT2     = 0x1f2
KEY_BRL_DOT3     = 0x1f3
KEY_BRL_DOT4     = 0x1f4
KEY_BRL_DOT5     = 0x1f5
KEY_BRL_DOT6     = 0x1f6
KEY_BRL_DOT7     = 0x1f7
KEY_BRL_DOT8     = 0x1f8
KEY_BRL_DOT9     = 0x1f9
KEY_BRL_DOT10    = 0x1fa

KEY_NUMERIC_0    = 0x200   # used by phones, remote controls,
KEY_NUMERIC_1    = 0x201   # and other keypads
KEY_NUMERIC_2    = 0x202
KEY_NUMERIC_3    = 0x203
KEY_NUMERIC_4    = 0x204
KEY_NUMERIC_5    = 0x205
KEY_NUMERIC_6    = 0x206
KEY_NUMERIC_7    = 0x207
KEY_NUMERIC_8    = 0x208
KEY_NUMERIC_9    = 0x209
KEY_NUMERIC_STAR = 0x20a
KEY_NUMERIC_POUND = 0x20b

KEY_CAMERA_FOCUS = 0x210
KEY_WPS_BUTTON   = 0x211   # WiFi Protected Setup key

KEY_TOUCHPAD_TOGGLE = 0x212   # Request switch touchpad on or off
KEY_TOUCHPAD_ON  = 0x213
KEY_TOUCHPAD_OFF = 0x214

KEY_CAMERA_ZOOMIN = 0x215
KEY_CAMERA_ZOOMOUT = 0x216
KEY_CAMERA_UP    = 0x217
KEY_CAMERA_DOWN  = 0x218
KEY_CAMERA_LEFT  = 0x219
KEY_CAMERA_RIGHT = 0x21a

BTN_TRIGGER_HAPPY  = 0x2c0
BTN_TRIGGER_HAPPY1 = 0x2c0
BTN_TRIGGER_HAPPY2 = 0x2c1
BTN_TRIGGER_HAPPY3 = 0x2c2
BTN_TRIGGER_HAPPY4 = 0x2c3
BTN_TRIGGER_HAPPY5 = 0x2c4
BTN_TRIGGER_HAPPY6 = 0x2c5
BTN_TRIGGER_HAPPY7 = 0x2c6
BTN_TRIGGER_HAPPY8 = 0x2c7
BTN_TRIGGER_HAPPY9 = 0x2c8
BTN_TRIGGER_HAPPY10 = 0x2c9
BTN_TRIGGER_HAPPY11 = 0x2ca
BTN_TRIGGER_HAPPY12 = 0x2cb
BTN_TRIGGER_HAPPY13 = 0x2cc
BTN_TRIGGER_HAPPY14 = 0x2cd
BTN_TRIGGER_HAPPY15 = 0x2ce
BTN_TRIGGER_HAPPY16 = 0x2cf
BTN_TRIGGER_HAPPY17 = 0x2d0
BTN_TRIGGER_HAPPY18 = 0x2d1
BTN_TRIGGER_HAPPY19 = 0x2d2
BTN_TRIGGER_HAPPY20 = 0x2d3
BTN_TRIGGER_HAPPY21 = 0x2d4
BTN_TRIGGER_HAPPY22 = 0x2d5
BTN_TRIGGER_HAPPY23 = 0x2d6
BTN_TRIGGER_HAPPY24 = 0x2d7
BTN_TRIGGER_HAPPY25 = 0x2d8
BTN_TRIGGER_HAPPY26 = 0x2d9
BTN_TRIGGER_HAPPY27 = 0x2da
BTN_TRIGGER_HAPPY28 = 0x2db
BTN_TRIGGER_HAPPY29 = 0x2dc
BTN_TRIGGER_HAPPY30 = 0x2dd
BTN_TRIGGER_HAPPY31 = 0x2de
BTN_TRIGGER_HAPPY32 = 0x2df
BTN_TRIGGER_HAPPY33 = 0x2e0
BTN_TRIGGER_HAPPY34 = 0x2e1
BTN_TRIGGER_HAPPY35 = 0x2e2
BTN_TRIGGER_HAPPY36 = 0x2e3
BTN_TRIGGER_HAPPY37 = 0x2e4
BTN_TRIGGER_HAPPY38 = 0x2e5
BTN_TRIGGER_HAPPY39 = 0x2e6
BTN_TRIGGER_HAPPY40 = 0x2e7


# We avoid low common keys in module aliases so they don't get huge
#KEY_MIN_INTERESTING = KEY_MUTE
KEY_MAX             = 0x2ff
KEY_CNT             = (KEY_MAX + 1)

# Relative axes
REL_X               = 0x00
REL_Y               = 0x01
REL_Z               = 0x02
REL_RX              = 0x03
REL_RY              = 0x04
REL_RZ              = 0x05
REL_HWHEEL          = 0x06
REL_DIAL            = 0x07
REL_WHEEL           = 0x08
REL_MISC            = 0x09
REL_MAX             = 0x0f
REL_CNT             = (REL_MAX + 1)

# Absolute axes
ABS_X               = 0x00
ABS_Y               = 0x01
ABS_Z               = 0x02
ABS_RX              = 0x03
ABS_RY              = 0x04
ABS_RZ              = 0x05
ABS_THROTTLE        = 0x06
ABS_RUDDER          = 0x07
ABS_WHEEL           = 0x08
ABS_GAS             = 0x09
ABS_BRAKE           = 0x0a
ABS_HAT0X           = 0x10
ABS_HAT0Y           = 0x11
ABS_HAT1X           = 0x12
ABS_HAT1Y           = 0x13
ABS_HAT2X           = 0x14
ABS_HAT2Y           = 0x15
ABS_HAT3X           = 0x16
ABS_HAT3Y           = 0x17
ABS_PRESSURE        = 0x18
ABS_DISTANCE        = 0x19
ABS_TILT_X          = 0x1a
ABS_TILT_Y          = 0x1b
ABS_TOOL_WIDTH      = 0x1c

ABS_VOLUME          = 0x20

ABS_MISC            = 0x28

ABS_MT_SLOT         = 0x2f  # MT slot being modified
ABS_MT_TOUCH_MAJOR  = 0x30  # Major axis of touching ellipse
ABS_MT_TOUCH_MINOR  = 0x31  # Minor axis (omit if circular)
ABS_MT_WIDTH_MAJOR  = 0x32  # Major axis of approaching ellipse
ABS_MT_WIDTH_MINOR  = 0x33  # Minor axis (omit if circular)
ABS_MT_ORIENTATION  = 0x34  # Ellipse orientation
ABS_MT_POSITION_X   = 0x35  # Center X ellipse position
ABS_MT_POSITION_Y   = 0x36  # Center Y ellipse position
ABS_MT_TOOL_TYPE    = 0x37  # Type of touching device
ABS_MT_BLOB_ID      = 0x38  # Group a set of packets as a blob
ABS_MT_TRACKING_ID  = 0x39  # Unique ID of initiated contact
ABS_MT_PRESSURE     = 0x3a  # Pressure on contact area
ABS_MT_DISTANCE     = 0x3b  # Contact hover distance

ABS_MT_FIRST        = ABS_MT_TOUCH_MAJOR
ABS_MT_LAST         = ABS_MT_DISTANCE
ABS_MT_RANGE        = range(ABS_MT_FIRST, ABS_MT_LAST+1)

ABS_MAX             = 0x3f
ABS_CNT             = (ABS_MAX + 1)

# Switch events
SW_LID                  = 0x00  # set = lid shut
SW_TABLET_MODE          = 0x01  # set = tablet mode
SW_HEADPHONE_INSERT     = 0x02  # set = inserted
SW_RFKILL_ALL           = 0x03  # rfkill master switch, type "any"
                                # set = radio enabled
SW_RADIO = SW_RFKILL_ALL        # deprecated
SW_MICROPHONE_INSERT    = 0x04  # set = inserted
SW_DOCK                 = 0x05  # set = plugged into dock
SW_LINEOUT_INSERT       = 0x06  # set = inserted
SW_JACK_PHYSICAL_INSERT = 0x07  # set = mechanical switch set
SW_VIDEOOUT_INSERT      = 0x08  # set = inserted
SW_CAMERA_LENS_COVER    = 0x09  # set = lens covered
SW_KEYPAD_SLIDE         = 0x0a  # set = keypad slide out
SW_FRONT_PROXIMITY      = 0x0b  # set = front proximity sensor active
SW_ROTATE_LOCK          = 0x0c  # set = rotate locked/disabled
SW_MAX                  = 0x0f
SW_CNT                  = (SW_MAX + 1)

# Misc events
MSC_SERIAL      = 0x00
MSC_PULSELED    = 0x01
MSC_GESTURE     = 0x02
MSC_RAW         = 0x03
MSC_SCAN        = 0x04
MSC_MAX         = 0x07
MSC_CNT         = (MSC_MAX + 1)

# LEDs
LED_NUML        = 0x00
LED_CAPSL       = 0x01
LED_SCROLLL     = 0x02
LED_COMPOSE     = 0x03
LED_KANA        = 0x04
LED_SLEEP       = 0x05
LED_SUSPEND     = 0x06
LED_MUTE        = 0x07
LED_MISC        = 0x08
LED_MAIL        = 0x09
LED_CHARGING    = 0x0a
LED_MAX         = 0x0f
LED_CNT         = (LED_MAX + 1)

# Autorepeat values
REP_DELAY       = 0x00
REP_PERIOD      = 0x01
REP_MAX         = 0x01
REP_CNT         = (REP_MAX + 1)

# Sounds
SND_CLICK       = 0x00
SND_BELL        = 0x01
SND_TONE        = 0x02
SND_MAX         = 0x07
SND_CNT         = (SND_MAX + 1)

# IDs.
ID_BUS          = 0
ID_VENDOR       = 1
ID_PRODUCT      = 2
ID_VERSION      = 3

BUS_PCI         = 0x01
BUS_ISAPNP      = 0x02
BUS_USB         = 0x03
BUS_HIL         = 0x04
BUS_BLUETOOTH   = 0x05
BUS_VIRTUAL     = 0x06

BUS_ISA         = 0x10
BUS_I8042       = 0x11
BUS_XTKBD       = 0x12
BUS_RS232       = 0x13
BUS_GAMEPORT    = 0x14
BUS_PARPORT     = 0x15
BUS_AMIGA       = 0x16
BUS_ADB         = 0x17
BUS_I2C         = 0x18
BUS_HOST        = 0x19
BUS_GSC         = 0x1A
BUS_ATARI       = 0x1B
BUS_SPI         = 0x1C

# MT_TOOL types
MT_TOOL_FINGER  = 0
MT_TOOL_PEN     = 1
MT_TOOL_MAX     = 1

# Values describing the status of a force-feedback effect
FF_STATUS_STOPPED   = 0x00
FF_STATUS_PLAYING   = 0x01
FF_STATUS_MAX       = 0x01
FF_STATUS_CNT       = (FF_STATUS_MAX + 1)

# Structures used in ioctls to upload effects to a device
# They are pieces of a bigger structure (called ff_effect)

# All duration values are expressed in ms. Values above 32767 ms (0x7fff)
# should not be used and have unspecified results.

"""
 * struct ff_replay - defines scheduling of the force-feedback effect
 * @length: duration of the effect
 * @delay: delay before effect should start playing
struct ff_replay {
  __u16 length;
  __u16 delay;
};
"""

"""
 * struct ff_trigger - defines what triggers the force-feedback effect
 * @button: number of the button triggering the effect
 * @interval: controls how soon the effect can be re-triggered
struct ff_trigger {
  __u16 button;
  __u16 interval;
};
"""

"""
 * struct ff_envelope - generic force-feedback effect envelope
 * @attack_length: duration of the attack (ms)
 * @attack_level: level at the beginning of the attack
 * @fade_length: duration of fade (ms)
 * @fade_level: level at the end of fade
 *
 * The @attack_level and @fade_level are absolute values; when applying
 * envelope force-feedback core will convert to positive/negative
 * value based on polarity of the default level of the effect.
 * Valid range for the attack and fade levels is 0x0000 - 0x7fff
struct ff_envelope {
  __u16 attack_length;
  __u16 attack_level;
  __u16 fade_length;
  __u16 fade_level;
};
"""

"""
 * struct ff_constant_effect - params of a constant force-feedback effect
 * @level: strength of the effect; may be negative
 * @envelope: envelope data
struct ff_constant_effect {
  __s16 level;
  struct ff_envelope envelope;
};
"""

"""
 * struct ff_ramp_effect - params of a ramp force-feedback effect
 * @start_level: beginning strength of the effect; may be negative
 * @end_level: final strength of the effect; may be negative
 * @envelope: envelope data
struct ff_ramp_effect {
  __s16 start_level;
  __s16 end_level;
  struct ff_envelope envelope;
};
"""

"""
 * struct ff_condition_effect - spring or friction force-feedback effect
 * @right_saturation: maximum level when joystick moved all way to the right
 * @left_saturation: same for the left side
 * @right_coeff: controls how fast the force grows when the joystick moves
 *  to the right
 * @left_coeff: same for the left side
 * @deadband: size of the dead zone, where no force is produced
 * @center: position of the dead zone
struct ff_condition_effect {
  __u16 right_saturation;
  __u16 left_saturation;

  __s16 right_coeff;
  __s16 left_coeff;

  __u16 deadband;
  __s16 center;
};
"""


"""
 * struct ff_periodic_effect - params of a periodic force-feedback effect
 * @waveform: kind of the effect (wave)
 * @period: period of the wave (ms)
 * @magnitude: peak value
 * @offset: mean value of the wave (roughly)
 * @phase: 'horizontal' shift
 * @envelope: envelope data
 * @custom_len: number of samples (FF_CUSTOM only)
 * @custom_data: buffer of samples (FF_CUSTOM only)
 *
 * Known waveforms - FF_SQUARE, FF_TRIANGLE, FF_SINE, FF_SAW_UP,
 * FF_SAW_DOWN, FF_CUSTOM. The exact syntax FF_CUSTOM is undefined
 * for the time being as no driver supports it yet.
 *
 * Note: the data pointed by custom_data is copied by the driver.
 * You can therefore dispose of the memory after the upload/update.
struct ff_periodic_effect {
  __u16 waveform;
  __u16 period;
  __s16 magnitude;
  __s16 offset;
  __u16 phase;

  struct ff_envelope envelope;

  __u32 custom_len;
  __s16 __user *custom_data;
};
"""

"""
 * struct ff_rumble_effect - params of a periodic force-feedback effect
 * @strong_magnitude: magnitude of the heavy motor
 * @weak_magnitude: magnitude of the light one
 *
 * Some rumble pads have two motors of different weight. Strong_magnitude
 * represents the magnitude of the vibration generated by the heavy one.
struct ff_rumble_effect {
  __u16 strong_magnitude;
  __u16 weak_magnitude;
};
"""

"""
 * struct ff_effect - defines force feedback effect
 * @type: type of the effect (FF_CONSTANT, FF_PERIODIC, FF_RAMP, FF_SPRING,
 *  FF_FRICTION, FF_DAMPER, FF_RUMBLE, FF_INERTIA, or FF_CUSTOM)
 * @id: an unique id assigned to an effect
 * @direction: direction of the effect
 * @trigger: trigger conditions (struct ff_trigger)
 * @replay: scheduling of the effect (struct ff_replay)
 * @u: effect-specific structure (one of ff_constant_effect, ff_ramp_effect,
 *  ff_periodic_effect, ff_condition_effect, ff_rumble_effect) further
 *  defining effect parameters
 *
 * This structure is sent through ioctl from the application to the driver.
 * To create a new effect application should set its @id to -1; the kernel
 * will return assigned @id which can later be used to update or delete
 * this effect.
 *
 * Direction of the effect is encoded as follows:
 *  0 deg -> 0x0000 (down)
 *  90 deg -> 0x4000 (left)
 *  180 deg -> 0x8000 (up)
 *  270 deg -> 0xC000 (right)
struct ff_effect {
  __u16 type;
  __s16 id;
  __u16 direction;
  struct ff_trigger trigger;
  struct ff_replay replay;

  union {
    struct ff_constant_effect constant;
    struct ff_ramp_effect ramp;
    struct ff_periodic_effect periodic;
    struct ff_condition_effect condition[2]; /* One for each axis */
    struct ff_rumble_effect rumble;
  } u;
};
"""

# Force feedback effect types
FF_RUMBLE       = 0x50
FF_PERIODIC     = 0x51
FF_CONSTANT     = 0x52
FF_SPRING       = 0x53
FF_FRICTION     = 0x54
FF_DAMPER       = 0x55
FF_INERTIA      = 0x56
FF_RAMP         = 0x57

FF_EFFECT_MIN   = FF_RUMBLE
FF_EFFECT_MAX   = FF_RAMP

# Force feedback periodic effect types
FF_SQUARE       = 0x58
FF_TRIANGLE     = 0x59
FF_SINE         = 0x5a
FF_SAW_UP       = 0x5b
FF_SAW_DOWN     = 0x5c
FF_CUSTOM       = 0x5d

FF_WAVEFORM_MIN = FF_SQUARE
FF_WAVEFORM_MAX = FF_CUSTOM

# Set ff device properties
FF_GAIN         = 0x60
FF_AUTOCENTER   = 0x61

FF_MAX          = 0x7f
FF_CNT          = (FF_MAX + 1)


"""
The following constants, functions and dicts are not part of the original linux
input header file.  They are included here to make it easier to use the linux
input subsystem from python scripts.
"""

BITS_PER_WORD = 8

def NBITS(x):
    return ((x - 1) / BITS_PER_WORD) + 1

def OFFSET(x):
    return (x % BITS_PER_WORD)

def BIT(x):
    return (1 << OFFSET(x))

def INDEX(x):
    return (x / BITS_PER_WORD)

def test_bit(b, a):
    return ((a[INDEX(b)] >> OFFSET(b)) & 1)

EV_TYPES = {
    EV_SYN : 'EV_SYN',
    EV_KEY : 'EV_KEY',
    EV_REL : 'EV_REL',
    EV_ABS : 'EV_ABS',
    EV_MSC : 'EV_MSC',
    EV_SW  : 'EV_SW',
    EV_LED : 'EV_LED',
    EV_SND : 'EV_SND',
    EV_REP : 'EV_REP',
    EV_FF  : 'EV_FF',
    EV_PWR : 'EV_PWR',
    EV_FF_STATUS : 'EV_FF_STATUS'
}

EV_SIZES = {
    EV_KEY : KEY_CNT,
    EV_REL : REL_CNT,
    EV_ABS : ABS_CNT,
    EV_MSC : MSC_CNT,
    EV_SW  : SW_CNT,
    EV_LED : LED_CNT,
    EV_SND : SND_CNT,
    EV_REP : REP_CNT,
    EV_FF  : FF_CNT,
    EV_FF_STATUS : FF_STATUS_CNT
}

EV_STRINGS = {
    # Keys (only buttons, for now)
    EV_KEY: {
        BTN_0           : '0',
        BTN_1           : '1',
        BTN_2           : '0',
        BTN_3           : '3',
        BTN_4           : '4',
        BTN_5           : '5',
        BTN_6           : '6',
        BTN_7           : '7',
        BTN_8           : '8',
        BTN_9           : '9',
        BTN_LEFT        : 'LEFT',
        BTN_RIGHT       : 'RIGHT',
        BTN_MIDDLE      : 'MIDDLE',
        BTN_SIDE        : 'SIDE',
        BTN_EXTRA       : 'EXTRA',
        BTN_FORWARD     : 'FORWARD',
        BTN_BACK        : 'BACK',
        BTN_TASK        : 'TASK',
        BTN_TRIGGER     : 'TRIGGER',
        BTN_THUMB       : 'THUMB',
        BTN_THUMB2      : 'THUMB2',
        BTN_TOP         : 'TOP',
        BTN_TOP2        : 'TOP2',
        BTN_PINKIE      : 'PINKIE',
        BTN_BASE        : 'BASE',
        BTN_BASE2       : 'BASE2',
        BTN_BASE3       : 'BASE3',
        BTN_BASE4       : 'BASE4',
        BTN_BASE5       : 'BASE5',
        BTN_BASE6       : 'BASE6',
        BTN_DEAD        : 'DEAD',
        BTN_A           : 'A',
        BTN_B           : 'B',
        BTN_C           : 'C',
        BTN_X           : 'X',
        BTN_Y           : 'Y',
        BTN_Z           : 'Z',
        BTN_TL          : 'TL',
        BTN_TR          : 'TR',
        BTN_TL2         : 'TL2',
        BTN_TR2         : 'TR2',
        BTN_SELECT      : 'SELECT',
        BTN_START       : 'START',
        BTN_MODE        : 'MODE',
        BTN_THUMBL      : 'THUMBL',
        BTN_THUMBR      : 'THUMBR',
        BTN_TOOL_PEN    : 'TOOL_PEN',
        BTN_TOOL_RUBBER : 'TOOL_RUBBER',
        BTN_TOOL_BRUSH  : 'TOOL_BRUSH',
        BTN_TOOL_PENCIL : 'TOOL_PENCIL',
        BTN_TOOL_AIRBRUSH : 'TOOL_AIRBRUSH',
        BTN_TOOL_FINGER : 'TOOL_FINGER',
        BTN_TOOL_MOUSE  : 'TOOL_MOUSE',
        BTN_TOOL_LENS   : 'TOOL_LENS',
        BTN_TOOL_QUINTTAP : 'TOOL_QUINTTAP',
        BTN_TOUCH       : 'TOUCH',
        BTN_STYLUS      : 'STYLUS',
        BTN_STYLUS2     : 'STYLUS2',
        BTN_TOOL_DOUBLETAP : 'TOOL_DOUBLETAP',
        BTN_TOOL_TRIPLETAP : 'TOOL_TRIPLETAP',
        BTN_TOOL_QUADTAP : 'TOOL_QUADTAP',
        BTN_GEAR_DOWN   : 'TOOL_GEAR_DOWN',
        BTN_GEAR_UP     : 'TOOL_GEAR_UP',
    },

    # Relative axes
    EV_REL: {
        REL_X           : 'X',
        REL_Y           : 'Y',
        REL_Z           : 'Z',
        REL_RX          : 'RX',
        REL_RY          : 'RY',
        REL_RZ          : 'RZ',
        REL_HWHEEL      : 'HWHEEL',
        REL_DIAL        : 'DIAL',
        REL_WHEEL       : 'WHEEL',
        REL_MISC        : 'MISC',
    },

    # Absolute axes
    EV_ABS: {
        ABS_X               : 'X',
        ABS_Y               : 'Y',
        ABS_Z               : 'Z',
        ABS_RX              : 'RX',
        ABS_RY              : 'RY',
        ABS_RZ              : 'RZ',
        ABS_THROTTLE        : 'THROTTLE',
        ABS_RUDDER          : 'RUDDER',
        ABS_WHEEL           : 'WHEEL',
        ABS_GAS             : 'GAS',
        ABS_BRAKE           : 'BRAKE',
        ABS_HAT0X           : 'HAT0X',
        ABS_HAT0Y           : 'HAT0Y',
        ABS_HAT1X           : 'HAT1X',
        ABS_HAT1Y           : 'HAT1Y',
        ABS_HAT2X           : 'HAT2X',
        ABS_HAT2Y           : 'HAT2Y',
        ABS_HAT3X           : 'HAT3X',
        ABS_HAT3Y           : 'HAT3Y',
        ABS_PRESSURE        : 'PRESSURE',
        ABS_DISTANCE        : 'DISTANCE',
        ABS_TILT_X          : 'TILT_X',
        ABS_TILT_Y          : 'TILT_Y',
        ABS_TOOL_WIDTH      : 'TOOL_WIDTH',
        ABS_VOLUME          : 'VOLUME',
        ABS_MISC            : 'MISC',
        ABS_MT_SLOT         : 'MT_SLOT',
        ABS_MT_TOUCH_MAJOR  : 'MT_TOUCH_MAJOR',
        ABS_MT_TOUCH_MINOR  : 'MT_TOUCH_MINOR',
        ABS_MT_WIDTH_MAJOR  : 'MT_WIDTH_MAJOR',
        ABS_MT_WIDTH_MINOR  : 'MT_WIDTH_MINOR',
        ABS_MT_ORIENTATION  : 'MT_ORIENTATION',
        ABS_MT_POSITION_X   : 'MT_POSITION_X',
        ABS_MT_POSITION_Y   : 'MT_POSITION_Y',
        ABS_MT_TOOL_TYPE    : 'MT_TOOL_TYPE',
        ABS_MT_BLOB_ID      : 'MT_BLOB_ID',
        ABS_MT_TRACKING_ID  : 'MT_TRACKING_ID',
        ABS_MT_PRESSURE     : 'MT_PRESSURE',
        ABS_MT_DISTANCE     : 'MT_DISTANCE'
    },

    # Switches
    EV_SW: {
        SW_LID                  : 'LID',
        SW_TABLET_MODE          : 'TABLET_MODE',
        SW_HEADPHONE_INSERT     : 'HEADPHONE_INSERT',
        SW_RFKILL_ALL           : 'RFKILL_ALL',
        SW_MICROPHONE_INSERT    : 'MICROPHONE_INSERT',
        SW_DOCK                 : 'DOCK',
        SW_LINEOUT_INSERT       : 'LINEOUT_INSERT',
        SW_JACK_PHYSICAL_INSERT : 'JACK_PHYSICAL_INSERT',
        SW_VIDEOOUT_INSERT      : 'VIDEOOUT_INSERT',
        SW_CAMERA_LENS_COVER    : 'CAMERA_LENS_COVER',
        SW_KEYPAD_SLIDE         : 'KEYPAD_SLIDE',
        SW_FRONT_PROXIMITY      : 'FRONT_PROXIMITY',
        SW_ROTATE_LOCK          : 'ROTATE_LOCK',
    },

    # Misc events
    EV_MSC: {
        MSC_SERIAL      : 'SERIAL',
        MSC_PULSELED    : 'PULSELED',
        MSC_GESTURE     : 'GESTURE',
        MSC_RAW         : 'RAW',
        MSC_SCAN        : 'SCAN',
    },

    # LEDs
    EV_LED: {
        LED_NUML        : 'NUML',
        LED_CAPSL       : 'CAPSL',
        LED_SCROLLL     : 'SCROLLL',
        LED_COMPOSE     : 'COMPOSE',
        LED_KANA        : 'KANA',
        LED_SLEEP       : 'SLEEP',
        LED_SUSPEND     : 'SLEEP',
        LED_MUTE        : 'SLEEP',
        LED_MISC        : 'SLEEP',
        LED_MAIL        : 'SLEEP',
        LED_CHARGING    : 'SLEEP',
    },

    # Autorepeat values
    EV_REP: {
        REP_DELAY       : 'DELAY',
        REP_PERIOD      : 'PERIOD',
    },

    # Sounds
    EV_SND: {
        SND_CLICK       : 'CLICK',
        SND_BELL        : 'BELL'
    }
}
