
import os
import random
from math import tan, cos, pi, inf
from tensorflow import keras
import numpy as np
import cv2
import image_manipulator as im

def getFiles(path):
	files = []
	for f in os.listdir(path):
		filePath = os.path.join(path, f)
		if len(f) > 4 and f[-4:] == ".png" and os.path.isfile(filePath):
			files.append((filePath, int(f[:-4])/1000))
	random.shuffle(files)
	#return [("/media/stypox/ab0f9863-df4d-44b6-87d2-d3b003e57195/stypox/seguistrada-data/000024876.png", 24.876)]
	return files

def getProjectedRoadWidth(screenRatio, cameraInclination, cameraHeight, fovy):
	return (2 * cameraHeight * screenRatio
		* tan(pi/2 - cameraInclination - fovy/2) * tan(fovy/2)
		/ cos(cameraInclination))


def getTrainingSamples(p):
	images = getFiles(p.datasetPath)
	projectedRoadWidth = getProjectedRoadWidth(p.width/p.height, p.cameraInclination, p.cameraHeight, p.fovy)
	radiuses = im.getRoadRadiuses(100, p.profileWidth / projectedRoadWidth)

	for path, realRadius in images:
		img = cv2.imread(path)
		_, rect = im.getStreetRect(img, p.cameraInclination,
			p.fovy, p.upperRectLineHeight, p.profileWidth)

		if realRadius > 3/2*radiuses[0]/p.profileWidth*projectedRoadWidth or realRadius < 3/2*radiuses[-1]/p.profileWidth*projectedRoadWidth:
			bestIndex = None # infinity
		else:
			bestDelta = inf
			for i, radius in enumerate(radiuses):
				delta = abs(radius/p.profileWidth*projectedRoadWidth - realRadius)
				if delta < bestDelta:
					bestDelta = delta
					bestIndex = i

		otherIndex = random.randint(-1, 200)
		otherIndex = None if otherIndex == -1 else otherIndex
		for i, radius in im.enumerateRadiuses(radiuses):
			if i in [bestIndex, otherIndex]:
				profile = im.circularColumnAverage(rect, radius)
				yield profile, 1.0 if i == bestIndex else 0.0

def collectAllTrainingData(p):
	profiles = []
	scores = []
	for profile, score in getTrainingSamples(p):
		profiles.append(profile)
		scores.append(score)
	return np.array(profiles), np.array(scores)


def main():
	p = im.getParams()

	model = keras.Sequential([
		keras.layers.Dense(50, input_shape=(p.profileWidth,)),
		keras.layers.Dense(1, activation="sigmoid"),
	])

	model.compile(optimizer='adam',
		loss='binary_crossentropy',
		metrics=['accuracy'])

	profiles, scores = collectAllTrainingData(p)
	model.fit(profiles, scores, epochs=10)

if __name__ == "__main__":
	main()
