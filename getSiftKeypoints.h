#include "opencv2/objdetect/objdetect.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/ml/ml.hpp>
#include <iostream>
#include <fstream>
#include <stdio.h>

using namespace std;
using namespace cv ;

vector<KeyPoint> getSiftOnMouth(Mat input, Rect searchZone, CascadeClassifier mouth_classifier,Ptr<FeatureDetector> detector,float alpha,bool verbose) ;
vector<KeyPoint> getSiftOnNose(Mat input, Rect searchZone, CascadeClassifier nose_classifier,Ptr<FeatureDetector> detector,float alpha,bool verbose) ;
vector<KeyPoint> getSiftOnEyes1(Mat input,CascadeClassifier eyes_classifier,Ptr<FeatureDetector> detector,bool verbose) ;
vector<KeyPoint> getSiftOnEyes2(Mat input,Rect searchZone,CascadeClassifier eyes_classifier,Ptr<FeatureDetector> detector, float& alpha,bool verbose) ;
