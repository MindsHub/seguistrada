import cv2
import numpy as np
import math
import time


def getTargetHeight(targetWidth, cameraInclination, fovy, yOnScreen):
	alpha1 = math.pi/2 - cameraInclination
	alpha2 = math.pi/2 - math.atan(math.tan(fovy/2) * (1-2*yOnScreen))
	#print(alpha1, alpha2, math.pi-alpha1-alpha2, width, yOnScreen)
	return targetWidth * (math.sin(alpha2) / math.sin(math.pi-alpha1-alpha2) * yOnScreen)

def warpImage(image, corners, targetWidth, targetHeight):
	corners = np.array(corners, dtype=np.float32)
	#print(corners)
	#corners = order_points(corners)
	target = [(0,0),(targetWidth-1,0),(targetWidth-1,targetHeight-1),(0,targetHeight-1)]
	target = np.array(target, dtype=np.float32)

	mat = cv2.getPerspectiveTransform(corners, target)
	out = cv2.warpPerspective(image, mat, (targetWidth, targetHeight))
	return out

def getStreetRect(img, cameraInclination, fovy, x0, targetWidth):
	height, width, _ = np.shape(img)
	screenRatio = width / height
	#print(screenRatio)

	tanLineAngle = (math.tan(cameraInclination) / math.tan(fovy/2) + 1) / screenRatio
	#print("Tan line angle =", tanLineAngle, " -->  line angle =", math.atan(tanLineAngle), "rad =", math.degrees(math.atan(tanLineAngle)), "degrees")

	def f(x):
		return tanLineAngle * x
	def fInverse(y):
		return y / tanLineAngle

	x, y = None, None
	if f(x0) > 1/screenRatio:
		x, y = fInverse(1/screenRatio), 1/screenRatio
	else:
		x, y = x0, f(x0)

	corners = [[x*width, height - y*width], [(1-x)*width, height - y*width], [width, height], [0,height]]

	targetHeight = int(getTargetHeight(targetWidth, cameraInclination, fovy, y*screenRatio))
	return corners, warpImage(img, corners, targetWidth, targetHeight)


radiuses = [(1.6**(i/4)) for i in range(40)] + [-(1.6**(i/4)) for i in range(39,-1,-1)]

def circularShift(src, r):
	height, width, channels = np.shape(src)
	direction = -1 if r<0 else 1
	r = abs(r)

	if r <= height:
		newWidth = width + r
	else:
		newWidth = width + r - int(np.sqrt(r**2 - height**2)) - 1
	newHeight = min(height, r)

	shifted = np.full((newHeight, newWidth, channels), -1, dtype=np.int16)
	for h in range(1, newHeight+1):
		shift = int(r - np.sqrt(r**2 - h**2))
		if direction == -1:
			shift = newWidth-width-shift
		shifted[newHeight-h,shift:shift+width,:] = src[height-h,:,:]

	return shifted

def columnAverage(src):
	_, width, _ = np.shape(src)
	average = np.zeros((width,), dtype=np.uint8)

	for w in range(width):
		average[w] = np.average(src[:,w,:], weights=(src[:,w,:]>=0))

	# in lines where only <=4 pixels are valid, tha values can't be considered ok
	for w in range(2*width//3, width):
		countValid = np.sum(src[:,w,:]>=0)
		if countValid <= 10:
			average[w] = average[w-1]
	for w in range(width//3, -1, -1):
		countValid = np.sum(src[:,w,:]>=0)
		if countValid <= 10:
			average[w] = average[w+1]

	return average

def maxDifference(arr):
	diff = -1
	for i in range(8,len(arr)-9):
		diff = max(diff, arr[i]-arr[i+1] if arr[i]>arr[i+1] else arr[i+1]-arr[i])
	return diff

def circularColumnAverage(rect, radius):
	return columnAverage(circularShift(rect, radius))


def processImage(src, x0, baseWidth):
	corn, rect = getStreetRect(src, math.radians(5.0), math.radians(41.41), x0, baseWidth)

	#cv2.imshow("rect", rect)
	#cv2.imshow("frame", frame)

	#print(corn)
	#prev = corn[-1]
	#for c in corn:
	#	cv2.circle(img, (int(c[0]), int(c[1])), radius=5, color=(255,255,255,255), thickness=7)
	#	cv2.line(img, (int(prev[0]), int(prev[1])), (int(c[0]), int(c[1])), color=(255,255,255,255), thickness=2)
	#	prev = c


	bestDiff = -1
	bestRadius = None
	bestShift = None
	bestAverage = None
	for radiusScale in radiuses:
		average = circularColumnAverage(rect, int(radiusScale*baseWidth))
		diff = maxDifference(average)

		if (diff > bestDiff) or (diff == bestDiff and abs(radius) > abs(bestRadius)):
			bestDiff = diff
			bestRadius = radius
			bestShift = shifted
			bestAverage = average

	cv2.imshow("bestShift", bestShift.astype(np.uint8))
	cv2.imshow("bestAverage", arrToImg(bestAverage))
	print(bestRadius)

def maxDifferenceHighlight(arr):
	diff = -1
	for i in range(len(arr)-1):
		dif = arr[i]-arr[i+1] if arr[i]>arr[i+1] else arr[i+1]-arr[i]
		if dif > diff:
			diff = dif
			highlight = i
	return diff,highlight

def arrToImg(a, highlight=None):
	width = np.shape(a)[0]
	res = np.empty((5, width, 3), dtype=np.uint8)
	for i in range(width):
		res[:,i,:] = a[i]

	if highlight is not None:
		res[:,highlight,0] = 0
	return res

def main():
	i=0

	cap = cv2.VideoCapture("2020-04-04 20-18-40.mp4")
	while not cap.isOpened():
		cap = cv2.VideoCapture("2020-04-04 20-18-40.mp4")
		cv2.waitKey(1000)

	done = False
	while True:
		flag, frame = cap.read()
		if flag:
			img = np.copy(frame)
			if i%4 == 0:
				start = time.time()
				processImage(frame, img, 0.3205, 50)
				end = time.time()
				print(end - start, "s")
			i+=1

		if cv2.waitKey(1) == 27:
			break
		if cap.get(cv2.CAP_PROP_POS_FRAMES) == cap.get(cv2.CAP_PROP_FRAME_COUNT):
			break

if __name__ == "__main__":
	main()
