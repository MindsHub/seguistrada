from math import tan, atan, sin, pi, radians, sqrt
import time
import json
import types
import cv2
import numpy as np


def getTargetHeight(targetWidth, cameraInclination, fovy, yOnScreen):
    alpha1 = pi/2 - cameraInclination
    alpha2 = pi/2 - atan(tan(fovy/2) * (1-2*yOnScreen))
    #print(alpha1, alpha2, pi-alpha1-alpha2, width, yOnScreen)
    return targetWidth * (sin(alpha2) / sin(pi-alpha1-alpha2) * yOnScreen)

def warpImage(image, corners, targetWidth, targetHeight):
    corners = np.array(corners, dtype=np.float32)
    #print(corners)
    #corners = order_points(corners)
    target = [(0, 0), (targetWidth-1, 0), (targetWidth-1, targetHeight-1), (0, targetHeight-1)]
    target = np.array(target, dtype=np.float32)

    mat = cv2.getPerspectiveTransform(corners, target)
    out = cv2.warpPerspective(image, mat, (targetWidth, targetHeight))
    return out

def getStreetRect(img, cameraInclination, fovy, upperRectLineHeight, targetWidth):
    height, width, _ = np.shape(img)
    screenRatio = width / height
    tanLineAngle = (tan(cameraInclination) / tan(fovy/2) + 1) / screenRatio

    def f(x):
        return tanLineAngle * x
    def fInverse(y):
        return y / tanLineAngle

    if f(upperRectLineHeight) > 1/screenRatio:
        x, y = fInverse(1/screenRatio), 1/screenRatio
    else:
        x, y = upperRectLineHeight, f(upperRectLineHeight)

    corners = [[x*width, height - y*width], [(1-x)*width, height - y*width],
               [width, height], [0, height]]

    targetHeight = int(getTargetHeight(targetWidth, cameraInclination, fovy, y*screenRatio))
    return corners, warpImage(img, corners, targetWidth, targetHeight)


integerInfinity = 1<<31 - 1 # max value for 32bit signed ints (needed in numpy)

def getRoadRadiuses(count, multiplier):
    """returns 2*count radiuses sorted from bigger to smaller"""
    positives = []
    negatives = []
    for i in range(count):
        radius = int(multiplier * count / (i+1))
        if radius > integerInfinity:
            radius = integerInfinity
        elif radius < 2:
            radius = 2

        positives.append(radius)
        negatives.append(-radius)
    return positives + negatives[::-1]

def enumerateRadiuses(radiuses):
    """yields all correspodingIndex,radiuses and also None,infinity"""
    yield None, integerInfinity
    for radius in enumerate(radiuses):
        yield radius


def circularShift(src, r):
    height, width, channels = np.shape(src)
    direction = 1 if r < 0 else -1
    r = abs(r)

    if r <= width:
        if r <= height:
            newHeight = r
        else:
            newHeight = height
    else:
        if r <= height:
            newHeight = int(sqrt(2*r*width - width*width))
        else:
            newHeight = min(height, int(sqrt(2*r*width - width*width)))

    shifted = np.full((newHeight, width, channels), -1, dtype=np.int16)
    for h in range(1, newHeight+1):
        shift = int(r - sqrt(r*r - h*h))
        if direction == 1:
            shifted[newHeight-h, shift:width] = src[newHeight-h, 0:width-shift]
        else:
            shifted[newHeight-h, 0:width-shift] = src[newHeight-h, shift:width]

    return shifted

def columnAverage(src):
    _, width, _ = np.shape(src)
    average = np.zeros((width,), dtype=np.uint8)

    for w in range(width):
        average[w] = np.average(src[:, w, :], weights=(src[:, w, :] >= 0))

    return average

def pruneColumnAverage(src, average, count):
    """in lines where only <=count pixels are valid, the values can't be considered ok"""
    _, width, _ = np.shape(src)
    for w in range(2*width//3, width):
        countValid = np.sum(src[:, w, :] >= 0)
        if countValid <= count:
            average[w] = average[w-1]
    for w in range(width//3, -1, -1):
        countValid = np.sum(src[:, w, :] >= 0)
        if countValid <= count:
            average[w] = average[w+1]
    return average


def maxDifference(arr):
    diff = -1
    for i in range(8, len(arr)-9):
        diff = max(diff, arr[i]-arr[i+1] if arr[i] > arr[i+1] else arr[i+1]-arr[i])
    return diff

def circularColumnAverage(rect, radius):
    return columnAverage(circularShift(rect, radius))


def processImage(src, p):
    start = time.time()
    _, rect = getStreetRect(src, p.cameraInclination, p.fovy,
                            p.upperRectLineHeight, p.profileWidth)

    #cv2.imshow("rect", rect)
    #cv2.imshow("frame", frame)

    #print(corn)
    #prev = corn[-1]
    #for c in corn:
    #    cv2.circle(img, (int(c[0]), int(c[1])), radius=5, color=(255,255,255,255), thickness=7)
    #    cv2.line(img, (int(prev[0]), int(prev[1])), (int(c[0]), int(c[1])),
    #           color=(255,255,255,255), thickness=2)
    #    prev = c


    bestDiff = -1
    bestRadius = None
    bestShift = None
    bestAverage = None
    for _, radius in enumerateRadiuses(processImage.radiuses):
        shifted = circularShift(rect, radius)
        average = columnAverage(shifted)
        average = pruneColumnAverage(shifted, average, 10)
        diff = maxDifference(average)

        if (diff > bestDiff) or (diff == bestDiff and abs(radius) > abs(bestRadius)):
            bestDiff = diff
            bestRadius = radius
            bestShift = shifted
            bestAverage = average

    end = time.time()
    print(end - start, "s")
    cv2.imshow("bestShift", bestShift.astype(np.uint8))
    cv2.imshow("bestAverage", arrToImg(bestAverage))
    print(bestRadius)

def getParams():
    res = types.SimpleNamespace()
    data = json.load(open("./params.json"))

    res.width = data["width"]
    res.height = data["height"]
    res.cameraInclination = radians(data["cameraInclination"])
    res.cameraHeight = data["cameraHeight"]
    res.upperRectLineHeight = data["upperRectLineHeight"]
    res.profileWidth = data["profileWidth"]
    res.videoPath = data["videoPath"]
    res.datasetPath = data["datasetPath"]

    res.fovx = radians(data["fovx"])
    res.fovy = 2 * atan(tan(res.fovx/2) / res.width * res.height)
    return res

def arrToImg(a, highlight=None):
    width = np.shape(a)[0]
    res = np.empty((5, width, 3), dtype=np.uint8)
    for i in range(width):
        res[:, i, :] = a[i]

    if highlight is not None:
        res[:, highlight, 0] = 0
    return res

def main():
    p = getParams()
    processImage.radiuses = getRoadRadiuses(40, 10 * p.profileWidth)

    cap = cv2.VideoCapture(p.videoPath)
    while not cap.isOpened():
        cap = cv2.VideoCapture(p.videoPath)
        cv2.waitKey(1000)

    i = 0
    while True:
        flag, frame = cap.read()
        if flag:
            if i%4 == 0:
                processImage(frame, p)
            i += 1

        if cv2.waitKey(1) == 27:
            break
        if cap.get(cv2.CAP_PROP_POS_FRAMES) == cap.get(cv2.CAP_PROP_FRAME_COUNT):
            break

if __name__ == "__main__":
    main()
