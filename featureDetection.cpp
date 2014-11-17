#include "featureDetection.h"
#include "faceDetection.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/ml/ml.hpp>
#include <boost/filesystem.hpp>
#include <vector>
#include <iostream>
#include <stdio.h>


using namespace std;
using namespace cv;
using namespace boost::filesystem ;

void buildSiftDictionary(int i){
    CascadeClassifier face_classifier = getCascadeClassifier();
	initModule_nonfree() ;
	//to store the input file names
	string filename ;
	//to store the current input image
	Mat input;

	//To store the keypoints that will be extracted by SIFT
	vector<KeyPoint> keypoints;
	//To store the SIFT descriptor of current image
	Mat descriptor;
	//To store all the descriptors that are extracted from all the images.
	Mat featuresUnclustered;
	//The SIFT feature extractor and descriptor
	Ptr<FeatureDetector> detector = FeatureDetector::create("SIFT");
	Ptr<DescriptorExtractor> extractor = DescriptorExtractor::create("SIFT");

	//Images to extract feature descriptors and build the vocabulary
	for (directory_iterator it1("../data"); it1 != directory_iterator() ; it1++){
		path p = it1->path() ;
		cout << "Folder " << p.string() << endl ;
		for(directory_iterator it2(p); it2 != directory_iterator() ; it2 ++){
			cout << it2->path() << endl ;
			path p2 = it2->path() ;
			if(is_regular_file(it2->status())){
                // Loading file
				filename = p2.string() ;
				input = imread(filename, CV_LOAD_IMAGE_GRAYSCALE);

                // Generating mask for face on the image
                vector<Rect> faces = detectFaces(face_classifier, input);
                Mat mask = Mat::zeros(input.size[0], input.size[1], CV_8U);
                mask(faces.front()) = 1;

				//compute the descriptors for each keypoint and put it in a single Mat object
				detector->detect(input, keypoints, mask);
				extractor->compute(input, keypoints,descriptor);
				featuresUnclustered.push_back(descriptor);
			}
		}
	}


	cout << "features Unclustered " << featuresUnclustered.size() << endl ;

	//Construct BOWKMeansTrainer
	//the number of bags
	int dictionarySize=i;
	//define Term Criteria
	TermCriteria tc(CV_TERMCRIT_ITER,100,0.001);
	//retries number
	int retries=1;
	//necessary flags
	int flags=KMEANS_PP_CENTERS;
	//Create the BoW (or BoF) trainer
	BOWKMeansTrainer bowTrainer(dictionarySize,tc,retries,flags);
	//cluster the feature vectors
	Mat dictionary=bowTrainer.cluster(featuresUnclustered) ;
	cout << "Dico cree" << endl ;
	//store the vocabulary
	FileStorage fs("../dictionary.yml", FileStorage::WRITE);
	fs << "vocabulary" << dictionary;
	fs.release();

	cout << " Dictionnaire OK" << endl ;
	
}


int createSVMClassifier(void) {
	
    //prepare BOW descriptor extractor from the dictionary
    Mat dictionary; 
    FileStorage fs("../dictionary.yml", FileStorage::READ);
    fs["vocabulary"] >> dictionary;
    fs.release();    
    cout << "dictionary loaded" << endl ;

    //create a nearest neighbor matcher
	Ptr<DescriptorMatcher> matcher(new FlannBasedMatcher) ;
	//The SIFT feature extractor and descriptor
	Ptr<FeatureDetector> detector = FeatureDetector::create("SIFT") ; 
	Ptr<DescriptorExtractor> extractor = DescriptorExtractor::create("SIFT") ; 

    //create BoF (or BoW) descriptor extractor
    BOWImgDescriptorExtractor bowDE(extractor,matcher);
    //Set the dictionary with the vocabulary we created in the first step
    bowDE.setVocabulary(dictionary);

    //init
	string filename ;
    Mat input ;
    vector<KeyPoint> keypoints;
    Mat bowDescriptor;
	map<int,Mat> training_set ;
	map<int,string> names ;
	int counter ;
	int index = 0 ;
	string celebrityName ;

	for (directory_iterator it1("../data"); it1 != directory_iterator() ; it1++){
		path p = it1->path() ;
		celebrityName = p.filename().string() ;
		cout << " -- Traite : " << celebrityName << endl ;
		Mat samples(0,dictionary.rows,CV_32FC1) ;
		counter = 0 ;
		for(directory_iterator it2(p); it2 != directory_iterator() ; it2 ++){
			path p2 = it2->path() ;
			if(is_regular_file(it2->status())){
                // Load the image
				filename = p2.string();
				cout << filename << endl;
				input = imread(filename, CV_LOAD_IMAGE_GRAYSCALE);

				if(input.size[0] > 0 && input.size[1] > 0){
					counter ++ ;
					//Detect SIFT keypoints (or feature points)
					detector->detect(input,keypoints);
					//extract BoW (or BoF) descriptor from given image
					bowDE.compute(input,keypoints,bowDescriptor);
					samples.push_back(bowDescriptor) ;
				}
			}
		}
		if (counter > 0 ){
			training_set.insert(pair<int,Mat>(index,samples)) ;
			names.insert(pair<int,string>(index,celebrityName)) ;
			index ++ ;
		}
	}
        
	cout << "Images chargees et analysees" << endl ;

	CvSVMParams params;
    params.svm_type    = CvSVM::C_SVC;
	params.kernel_type = CvSVM::POLY;
	params.degree = 3 ;
	params.gamma =  5;
	params.coef0 = 1 ;
    params.term_crit   = cvTermCriteria(CV_TERMCRIT_ITER, 100, 1e-6);
	
	Mat labels,temp ;
	string fname ;

	for (int x=0;x<index;x++){
		Mat samples(0,dictionary.rows,CV_32FC1) ;
		counter = 0 ;

		for(int y=0;y<index;y++){
			if(y != x){
				samples.push_back(training_set[y]) ;
				counter += training_set[y].rows ;
			}
		}
		samples.push_back(training_set[x]) ;
		labels = Mat::zeros(counter,1,CV_32FC1) ;
		temp = Mat::ones(training_set[x].rows,1,CV_32FC1) ;
		labels.push_back(temp);

		CvSVM classifier ;
		Mat samples_32f ;
		samples.convertTo(samples_32f, CV_32F);
		if(samples.rows != 0){ 
			classifier.train(samples_32f,labels,Mat(),Mat(),params);		
		}
		else
			cout << "Le classifieur pour " <<  names[x] << " n'a pas pu etre construit" << endl ;

		fname = "../classifiers/" + names[x] + ".yml";
		cout << "Store : " << fname << endl ;
		classifier.save(fname.c_str()) ;
		cout << "Stored" << endl ;
	}
	
	
	cout << "Classifieurs crees" << endl ;

	return index ;
	
}

// Do NOT use that !
map<int,CvSVM*> loadSVMClassifier(void){
	map<int,CvSVM*> classifiers ;
	char * path = new char[15];
	for (int x=0 ; x<3 ; x++){
		sprintf(path,"../classifiers/classifier%i.yml",x);
		CvSVM my_svm ;
		my_svm.load(path) ;
		classifiers.insert(pair<int,CvSVM*>(x,&my_svm)) ;
		cout << "classifieur " << x << " bien charge" << endl ;
		waitKey() ;
	}

	return classifiers ;
}

void predict(void){
	
	/*
	int count_folders = 0 ; //pour plus tard ...
	for(directory_iterator it("../classifiers"); it != directory_iterator(); ++it){
		count_folders ++ ;
	}
	*/
	CvSVM classifiers[3] ;
	String celebrities[3] ;
	int index = 0 ;
	for (directory_iterator it("../classifiers"); it != directory_iterator() ; it++) { 
		path p = it->path() ;
		if(is_regular_file(it->status())){
			classifiers[index].load(p.string().c_str()) ;
			celebrities[index] = p.stem().string() ;
			cout << "Added " << p.string() << " = " << p.stem().string() << endl ;
			index ++ ;
		}
	}

	cout << "Classifieurs charges" << endl ;
	waitKey() ;

	//prepare BOW descriptor extractor from the dictionary
    Mat dictionary; 
    FileStorage fs("../dictionary.yml", FileStorage::READ);
    fs["vocabulary"] >> dictionary;
    fs.release();    
    cout << "dictionary loaded" << endl ;
    //create a nearest neighbor matcher
	Ptr<DescriptorMatcher> matcher(new FlannBasedMatcher) ;
	//The SIFT feature extractor and descriptor
	Ptr<FeatureDetector> detector = FeatureDetector::create("SIFT") ; 
	Ptr<DescriptorExtractor> extractor = DescriptorExtractor::create("SIFT") ;
    //create BoF (or BoW) descriptor extractor
    BOWImgDescriptorExtractor bowDE(extractor,matcher);
    //Set the dictionary with the vocabulary we created in the first step
    bowDE.setVocabulary(dictionary);
	Mat input ;
    vector<KeyPoint> keypoints;  
    Mat bowDescriptor;   
	string filename;


	for (directory_iterator it1("../data"); it1 != directory_iterator() ; it1++) { //each folder in ../data
		path p = it1->path() ;
		cout << "Folder " << p.string() << endl ;
		waitKey() ;
		for(directory_iterator it2(p); it2 != directory_iterator() ; it2 ++){ //each file in the folder    
			cout << it2->path() << endl ;
			path p2 = it2->path() ;
			if(is_regular_file(it2->status())){
				filename = p2.string() ;
				input = imread(filename, CV_LOAD_IMAGE_GRAYSCALE); //Load as grayscale     
				if(input.size[0] > 0 && input.size[1] > 0){
					detector->detect(input,keypoints);
					bowDE.compute(input,keypoints,bowDescriptor);
					float min = 2  ;
					int prediction =0 ;
					for(int x=0;x<3;x++){
						if (classifiers[x].predict(bowDescriptor,true) < min){
							prediction = x ;
							min = classifiers[x].predict(bowDescriptor,true) ;
						}
						cout << classifiers[x].predict(bowDescriptor,true) << " " ;
					}
					cout <<endl ;
					cout << "Classe retenue : " << prediction << " = " << celebrities[prediction] << endl ;
				}
				cout << endl ;
			}
		}
	}

}
