
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
    return files

def getProjectedRoadDistance(cameraInclination, cameraHeight, fovy):
    """returns the distance from the camera to the street at the very bottom of the screen"""
    return 2 * cameraHeight * tan(pi/2 - cameraInclination - fovy/2)

def getProjectedRoadWidth(screenRatio, cameraInclination, cameraHeight, fovy):
    """returns the width of the street that is visible at the very bottom of the screen"""
    return (screenRatio * tan(fovy/2) / cos(cameraInclination)
            * getProjectedRoadDistance(cameraInclination, cameraHeight, fovy))


def getTrainingSamples(p):
    projectedRoadWidth = getProjectedRoadWidth(p.width/p.height, p.cameraInclination,
                                               p.cameraHeight, p.fovy)
    projectedRoadDistance = getProjectedRoadDistance(p.cameraInclination, p.cameraHeight, p.fovy)
    projectedRoadDistancePixels = int(projectedRoadDistance * p.profileWidth / projectedRoadWidth)
    radiuses = im.getRoadRadiuses(100, p.profileWidth / projectedRoadWidth)

    print(projectedRoadWidth, projectedRoadDistance, projectedRoadDistancePixels)

    for path, realRadius in getFiles(p.datasetPath):
        img = cv2.imread(path)
        _, rect = im.getStreetRect(img, p.cameraInclination, p.fovy,
                                   p.upperRectLineHeight, p.profileWidth)

        if (realRadius > 3/2*radiuses[0]/p.profileWidth*projectedRoadWidth
                or realRadius < 3/2*radiuses[-1]/p.profileWidth*projectedRoadWidth):
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
                print(radius)
                shifted = im.circularShift(rect, radius, projectedRoadDistancePixels)
                if shifted is not None:
                    profile = im.columnAverage(shifted)
                    profile = im.pruneColumnAverage(shifted, profile, 2)
                    yield profile, 1.0 if i == bestIndex else 0.0

def collectAllTrainingData(p):
    profiles = []
    scores = []
    for profile, score in getTrainingSamples(p):
        profiles.append(profile)
        scores.append(score)
        if score == 1.0:
            cv2.imshow("profile", im.arrToImg(profile))
            cv2.waitKey(0)
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
