# CC

## Overview
This is a remote desktop/remote takeover program developed in C/C++ and Python. After learning how to make C/C++ programs run silently in on Windows, I wanted to try making a remote desktop program which could be modified to run without the user's awareness. However, for multiple reasons, those stealth features are not included in this program.

The C/C++ portion of the code is meant to run on the remote computer. I used C/C++ here because of my familiarity with the language and the support it has from the Windows libraries. The Windows libraries provide me with easy, natively-supported methods for controlling the remote computer.

The Python portion of the code is meant to run on the local computer. I used Python here because it's much faster to develop and I have some familiarity with creating GUIs in Python.

## Planned Features
* file system traversal
* upload files
* download files
* run commands
* video feed of remote computer screen
* control of remote computer's mouse
* control of remote computer's keyboard
