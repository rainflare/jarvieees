#include <iostream>
#include <deque>
//#define WINVER 0x0500
#include <Windows.h>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/objdetect/objdetect.hpp"

//the following is the declaration of some variables
int minH = 0, maxH = 20, minS = 30, maxS = 150, minV = 60, maxV = 255;
cv::Mat frame, eye_tpl; /*NICO NICO NIIIIIIIII*/ //senpai
cv::Rect eye_bb;
int count = 0;
int a[1005][1005][3];

cv::Mat init;

cv::CascadeClassifier face_cascade;
cv::CascadeClassifier eye_cascade;


int detectEye(cv::Mat& im, cv::Mat& tpl, cv::Rect& rect)
{
	std::vector<cv::Rect> faces, eyes;
	face_cascade.detectMultiScale(im, faces, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, cv::Size(30, 30));

	for (int i = 0; i < faces.size(); i++)
	{
		cv::Mat face = im(faces[i]);
		eye_cascade.detectMultiScale(face, eyes, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, cv::Size(20, 20));

		if (eyes.size())
		{
			rect = eyes[0] + cv::Point(faces[i].x, faces[i].y);
			tpl = im(rect);
		}
	}

	return eyes.size(); //returns the size of eyes
}

void trackEye(cv::Mat& im, cv::Mat& tpl, cv::Rect& rect)
{
	cv::Size size(rect.width * 2, rect.height * 2);
	cv::Rect window(rect + size - cv::Point(size.width / 2, size.height / 2));

	window &= cv::Rect(0, 0, im.cols, im.rows);

	cv::Mat dst(window.width - tpl.rows + 1, window.height - tpl.cols + 1, CV_32FC1);
	cv::matchTemplate(im(window), tpl, dst, CV_TM_SQDIFF_NORMED);

	double minval, maxval;
	cv::Point minloc, maxloc;
	cv::minMaxLoc(dst, &minval, &maxval, &minloc, &maxloc);

	if (minval <= 0.2)
	{
		rect.x = window.x + minloc.x;
		rect.y = window.y + minloc.y;
	}
	else
		rect.x = rect.y = rect.width = rect.height = 0;
}

//this is a function
float innerAngle(float px1, float py1, float px2, float py2, float cx1, float cy1)
{

 float dist1 = std::sqrt(  (px1-cx1)*(px1-cx1) + (py1-cy1)*(py1-cy1) );
 float dist2 = std::sqrt(  (px2-cx1)*(px2-cx1) + (py2-cy1)*(py2-cy1) );

 float Ax, Ay;
 float Bx, By;
 float Cx, Cy;

 //find closest point to C  
 //printf("dist = %lf %lf\n", dist1, dist2);  

 Cx = cx1;
 Cy = cy1;
 if(dist1 < dist2)
 {
  Bx = px1;
  By = py1;
  Ax = px2;
  Ay = py2;


 }else{
  Bx = px2;
  By = py2;
  Ax = px1;
  Ay = py1;
 }


 float Q1 = Cx - Ax;
 float Q2 = Cy - Ay;
 float P1 = Bx - Ax;
 float P2 = By - Ay;


 float A = std::acos( (P1*Q1 + P2*Q2) / ( std::sqrt(P1*P1+P2*P2) * std::sqrt(Q1*Q1+Q2*Q2) ) );

 A = A*180/CV_PI;

 return A;
}

void CallbackFunc(int event, int x, int y, int flags, void* userdata)
{
  cv::Mat RGB = frame(cv::Rect(x, y, 1, 1));
  cv::Mat HSV;
  cv::cvtColor(RGB, HSV, CV_BGR2HSV);
  cv::Vec3b pixel = HSV.at<cv::Vec3b>(0, 0);
  if (event == cv::EVENT_LBUTTONDBLCLK) // on double left clcik
  {
      std::cout << "Click" << std::endl;
      int h = pixel.val[0];
      int s = pixel.val[1];
      int v = pixel.val[2];
      if (count == 0)
      {
          minH = h;
          maxH = h;
          minS = s;
          maxS = s;
          minV = v;
          maxV = v;
      }
      else
      {
          if (h < minH)
          {
              minH = h;
          }
          else if (h > maxH)
          {
              maxH = h;
          }
          if (s < minS)
          {
              minS = s;
          }
          else if (s > maxS)
          {
              maxS = s;
          }
          if (v < minV)
          {
              minV = v;
          }
          else if (v > maxV)
          {
              maxV = v;
          }
          
      }
      count++;
  }
  std::cout << pixel << std::endl;
}

int frames = 0;

std::deque<std::pair<int, cv::Point> > middle;
std::deque<std::pair<int, cv::Point> > vertical;
std::deque<std::pair<int, int> > fist;
std::deque<std::pair<int, int> > fistopen;
std::deque<std::pair<int, int> > eye_x;
std::deque<std::pair<int, int> > eye_y;

void press_key(DWORD KEY) {
	// This structure will be used to create the keyboard
	// input event.
	INPUT ip;

	// Set up a generic keyboard event.
	ip.type = INPUT_KEYBOARD;
	ip.ki.wScan = 0; // hardware scan code for key
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = 0;

	// Press the "A" key
	ip.ki.wVk = KEY; // virtual-key code for the "a" key
	ip.ki.dwFlags = 0; // 0 for key press
	SendInput(1, &ip, sizeof(INPUT));
}

void release_key() {
	INPUT ip;
	// Set up a generic keyboard event.
	ip.type = INPUT_KEYBOARD;
	ip.ki.wScan = 0; // hardware scan code for key
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = 0;
	// Release the "A" key
	ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &ip, sizeof(INPUT));
}

int main()
{

	// Load the cascade classifiers
	// Make sure you point the XML files to the right path, or 
	// just copy the files from [OPENCV_DIR]/data/haarcascades directory
	face_cascade.load("haarcascade_frontalface_alt2.xml");
	eye_cascade.load("haarcascade_eye.xml");

  cv::VideoCapture cap(0);
  const char* windowName = "iNTUition 2017 Hackathon";
  cv::namedWindow(windowName);
  cv::setMouseCallback(windowName, CallbackFunc, NULL);
  int inAngleMin = 200, inAngleMax = 300, angleMin = 180, angleMax = 359, lengthMin = 10, lengthMax = 80;
  /*
  cv::createTrackbar("Inner angle min", windowName, &inAngleMin, 360);
  cv::createTrackbar("Inner angle max", windowName, &inAngleMax, 360);
  cv::createTrackbar("Angle min", windowName, &angleMin, 360);
  cv::createTrackbar("Angle max", windowName, &angleMax, 360);
  cv::createTrackbar("Length min", windowName, &lengthMin, 100);
  cv::createTrackbar("Length max", windowName, &lengthMax, 100);
  */
  int framecount = 0;

  while (1)
  {
      cap >> frame;

	  if (frame.empty() || face_cascade.empty() || eye_cascade.empty()) {
		  continue;
	  }
/*
      std::cout << "Frame: " << framecount << "\n";
      if (framecount < 50) {

      }
      else if (framecount == 50) {
        for(int i=0; i<frame.rows; i++)
          for(int j=0; j<frame.cols; j++) {
            for (int k=0; k<3; k++) {
              a[j][i][k] = frame.at<cv::Vec3b>(cv::Point(j,i))[k];
            }
          }
      }
      else {
        for(int i=0; i<frame.rows; i++)
          for(int j=0; j<frame.cols; j++) {

              cv::Vec3b colorNow = frame.at<cv::Vec3b>(cv::Point(j,i));

              bool ok = true;
              for (int k = 0; k < 3; k++) {
                int cnow = colorNow[k];
                int cinit = a[j][i][k];
                //std::cout << cnow << " " << cinit << "\n";
                if (abs(cnow - cinit) > 100) {
                  ok =false;
                } 
              }

              if (!ok) {
                for (int k = 0; k < 3; k++) {
                  colorNow[k] = 0;
                }  
              }
                    
              frame.at<cv::Vec3b>(cv::Point(j,i)) = colorNow;


          }
      }

      framecount++;
*/

	  if (false) {
		  cv::Mat hsv;
		  cv::cvtColor(frame, hsv, CV_BGR2HSV);
		  cv::inRange(hsv, cv::Scalar(minH, minS, minV), cv::Scalar(maxH, maxS, maxV), hsv);
		  // Pre processing
		  int blurSize = 5;
		  int elementSize = 5;
		  cv::medianBlur(hsv, hsv, blurSize);
		  cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2 * elementSize + 1, 2 * elementSize + 1), cv::Point(elementSize, elementSize));
		  cv::dilate(hsv, hsv, element);
		  // Contour detection
		  std::vector<std::vector<cv::Point> > contours;
		  std::vector<cv::Vec4i> hierarchy;
		  cv::findContours(hsv, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
		  size_t largestContour = 0;
		  for (size_t i = 1; i < contours.size(); i++)
		  {
			  if (cv::contourArea(contours[i]) > cv::contourArea(contours[largestContour]))
				  largestContour = i;
		  }
		  cv::drawContours(frame, contours, largestContour, cv::Scalar(0, 0, 255), 1);
		  // Convex hull
		  if (!contours.empty())
		  {
			  std::vector<std::vector<cv::Point> > hull(1);
			  cv::convexHull(cv::Mat(contours[largestContour]), hull[0], false);
			  cv::drawContours(frame, hull, 0, cv::Scalar(0, 255, 0), 3);
			  if (hull[0].size() > 2)
			  {
				  std::vector<int> hullIndexes;
				  cv::convexHull(cv::Mat(contours[largestContour]), hullIndexes, true);
				  std::vector<cv::Vec4i> convexityDefects;
				  cv::convexityDefects(cv::Mat(contours[largestContour]), hullIndexes, convexityDefects);
				  cv::Rect boundingBox = cv::boundingRect(hull[0]);
				  cv::rectangle(frame, boundingBox, cv::Scalar(255, 0, 0));
				  cv::Point center = cv::Point(boundingBox.x + boundingBox.width / 2, boundingBox.y + boundingBox.height / 2);
				  std::vector<cv::Point> validPoints;
				  for (size_t i = 0; i < convexityDefects.size(); i++)
				  {
					  cv::Point p1 = contours[largestContour][convexityDefects[i][0]];
					  cv::Point p2 = contours[largestContour][convexityDefects[i][1]];
					  cv::Point p3 = contours[largestContour][convexityDefects[i][2]];
					  double angle = std::atan2(center.y - p1.y, center.x - p1.x) * 180 / CV_PI;
					  double inAngle = innerAngle(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
					  double length = std::sqrt(std::pow(p1.x - p3.x, 2) + std::pow(p1.y - p3.y, 2));
					  if (angle > angleMin - 180 && angle < angleMax - 180 && inAngle > inAngleMin - 180 && inAngle < inAngleMax - 180 && length > lengthMin / 100.0 * boundingBox.height && length < lengthMax / 100.0 * boundingBox.height)
					  {
						  validPoints.push_back(p1);
					  }
				  }
				  /*
				  std::cout << validPoints.size() << std::endl;
				  for (int i = 0; i < validPoints.size(); i++) {
					std::cout << validPoints[i].x << " " << validPoints[i].y << "\n";
				  }*/

				  while (!fist.empty() && fist.front().first < frames - 20) {
					  fist.pop_front();
				  }

				  if (!fist.empty() && validPoints.size() == 0) {
					  if (fist[0].second >= 4) {
						  std::cout << "Fist Close" << std::endl;
						  press_key(VK_CONTROL);
						  press_key(0x57);
						  release_key();
						  //system("taskkill /im chrome.exe /f");
						//system("notify-send \"Close browser!\"");
						  fist.clear();
					  }
				  }

				  fist.push_back(std::make_pair(frames, validPoints.size()));

				  for (int i = 0; i < validPoints.size(); i++) {
					  for (int j = i + 1; j < validPoints.size(); j++) {
						  if (validPoints[i].x > validPoints[j].x) {
							  auto temp = validPoints[i];
							  validPoints[i] = validPoints[j];
							  validPoints[j] = temp;
						  }
					  }
				  }

				  while (!fistopen.empty() && fistopen.front().first < frames - 20) {
					  fistopen.pop_front();
				  }

				  if (!fistopen.empty() && validPoints.size() >= 4) {
					  if (fistopen[0].second == 0) {
						  std::cout << "Fist Open" << std::endl;
						  system("start https://www.youtube.com/watch?v=dQw4w9WgXcQ");
						  //system("notify-send \"Close browser!\"");
						  fistopen.clear();
					  }
				  }

				  fistopen.push_back(std::make_pair(frames, validPoints.size()));

				  for (int i = 0; i < validPoints.size(); i++) {
					  for (int j = i + 1; j < validPoints.size(); j++) {
						  if (validPoints[i].x > validPoints[j].x) {
							  auto temp = validPoints[i];
							  validPoints[i] = validPoints[j];
							  validPoints[j] = temp;
						  }
					  }
				  }
				  /*
								for (int i = 0; i < middle.size(); i++) {
								  std::cout << middle[i].second.x << " ";
								}
								std::cout << "\n";
				  */
				  while (!middle.empty() && middle.front().first < frames - 30) {
					  middle.pop_front();
				  }

				  if (validPoints.size() >= 3) {
					  middle.push_back(std::make_pair(frames, validPoints[2]));
				  }
				  else {
					  //middle.clear();
				  }

				  if (middle.size() >= 15) {
					  if (middle[0].second.x - middle[middle.size() - 1].second.x > 200) {
						  //std::cout << "**********MOVED!************\n";
						  std::cout << "Hand Right" << std::endl;
						  //system("notify-send \"You moved your hand!\"");
						  //system("firefox");
						  //system("firefox --new-tab https://www.youtube.com/watch?v=qcOK_YATp6U");
						  press_key(VK_RIGHT);
						  release_key();
						  middle.clear();
					  }
					  else if (middle[middle.size() - 1].second.x - middle[0].second.x > 200) {
						  std::cout << "Hand Left" << std::endl;
						  press_key(VK_LEFT);
						  release_key();
						  middle.clear();
					  }
					  if (!middle.empty()) middle.pop_front();
				  }


				  for (int i = 0; i < validPoints.size(); i++) {
					  for (int j = i + 1; j < validPoints.size(); j++) {
						  if (validPoints[i].y > validPoints[j].y) {
							  auto temp = validPoints[i];
							  validPoints[i] = validPoints[j];
							  validPoints[j] = temp;
						  }
					  }
				  }

				  while (!vertical.empty() && vertical.front().first < frames - 30) {
					  vertical.pop_front();
				  }

				  if (validPoints.size() >= 3) {
					  vertical.push_back(std::make_pair(frames, validPoints[2]));
				  }
				  else {
					  //middle.clear();
				  }

				  if (vertical.size() >= 15) {
					  if (vertical[0].second.y - vertical[vertical.size() - 1].second.y > 200) {
						  //std::cout << "**********MOVED!************\n";

						  //system("notify-send \"You moved your hand!\"");
						  //system("firefox");
						  //system("firefox --new-tab https://www.youtube.com/watch?v=qcOK_YATp6U");
						  std::cout << "Hand Up" << std::endl;
						  system("start http://intuition.ieeentu.com/");
						  vertical.clear();
					  }
					  else if (vertical[vertical.size() - 1].second.y - vertical[0].second.y > 200) {
						  std::cout << "Hand Down" << std::endl;
						  system("start https://sg.finance.yahoo.com/");
						  vertical.clear();
					  }
					  if (!vertical.empty()) vertical.pop_front();
				  }

				  for (size_t i = 0; i < validPoints.size(); i++)
				  {
					  cv::circle(frame, validPoints[i], 9, cv::Scalar(0, 255, 0), 2);
				  }

			  }
		  }
	  }
	  else {
		  cv::Mat gray;
		  cv::cvtColor(frame, gray, CV_BGR2GRAY);

		  if (eye_bb.width == 0 && eye_bb.height == 0)
		  {
			  // Detection stage
			  // Try to detect the face and the eye of the user
			  detectEye(gray, eye_tpl, eye_bb);
		  }
		  else
		  {
			  // Tracking stage with template matching
			  trackEye(gray, eye_tpl, eye_bb);

			  // Draw bounding rectangle for the eye
			  cv::rectangle(frame, eye_bb, CV_RGB(0, 255, 255));
			  
			  while (!eye_x.empty() && eye_x.front().second < frames - 30) {
				  eye_x.pop_front();
			  }

			  while (!eye_y.empty() && eye_y.front().second < frames - 30) {
				  eye_y.pop_front();
			  }
			  
			  eye_x.push_back(std::make_pair(eye_bb.x, frames));
			  eye_y.push_back(std::make_pair(eye_bb.y, frames));
			  
			  if (eye_x.size() >= 20) {
				  if (eye_x.back().first - eye_x.front().first > 100) {
					  std::cout << "EYE LEFT" << std::endl;
					  system("start https://www.blackrock.com/sg");
					  eye_x.clear();
				  }
				  else if (eye_x.front().first - eye_x.back().first > 100) {
					  std::cout << "EYE RIGHT" << std::endl;
					  system("start https://www.bloomberg.com/markets/stocks");
					  eye_x.clear();
				  }
			  }
			  
			  if (eye_y.size() >= 20) {
				  if (eye_y.back().first - eye_y.front().first > 75) {
					  std::cout << "EYE DOWN" << std::endl;
					  system("start https://www.youtube.com/watch?v=zOzsjEmjjHs");
					  eye_y.clear();
				  }
				  else if (eye_y.front().first - eye_y.back().first > 75) {
					  std::cout << "EYE UP" << std::endl;
					  system("taskkill /im chrome.exe /f");
					  eye_y.clear();
				  }
			  }
			  
			  while (eye_x.size() > 30) {
				  eye_x.pop_front();
			  }
			  while (eye_y.size() > 30) {
				  eye_y.pop_front();
			  }
			  
		  }
	  }

	  frames++;

      cv::imshow(windowName, frame);
      //if (cv::waitKey(30) >= 0) break;
	  cv::waitKey(30);
  }
  return 0;
}
