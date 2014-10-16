/*
  CSCI 480
  Assignment 2
 */

#include <stdio.h>
#include <stdlib.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include "pic.h"
#include "Matrix.h"
#include <iostream>
#include <vector>
using namespace std;

/* represents one control point along the spline */
struct point {
   double x;
   double y;
   double z;
};

float s = 0.5;

double basisMatrix[4][4] = {{-s,2-s,s-2,s},
    {2*s,s-3,3-2*s,-s},
    {-s,0,s,0},
    {0,1,0,0}};

//This loads all of the textures into an array
GLuint skyTexture[5];
char* skyPic1 = (char*)"/Users/jerrywebb/Documents/cs420/exercises/assign1/SClouds.jpg";
char* skyPic2 = (char*)"/Users/jerrywebb/Documents/cs420/exercises/assign1/SCloudsFlipped.jpg";
char* skyPic3 = (char*)"/Users/jerrywebb/Documents/cs420/exercises/assign1/LightClouds.jpg";
char* skyPic4 = (char*)"/Users/jerrywebb/Documents/cs420/exercises/assign1/LightCloudsFlipped.jpg";
char* skyPic5 = (char*)"/Users/jerrywebb/Documents/cs420/exercises/assign1/IMG_0321.jpg";

void texLoad(int i,char *filename){
    Pic* img;
    img = jpeg_read(filename, NULL);
    glBindTexture(GL_TEXTURE_2D, skyTexture[i]);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB, img->nx, img->ny, 0, GL_RGBA,GL_UNSIGNED_BYTE, &img->pix[0]);
    pic_free(img);
}
//
//void textureInit(){
//    glGenTextures(5, skyTexture);
//    texLoad(0, skyPic1);
//    texLoad(1, skyPic2);
//    texLoad(2, skyPic3);
//    texLoad(3, skyPic4);
//    texLoad(4, skyPic5);
//}



//This creates the basis matrix under the matrix class
Mat mBasis(4,4,*basisMatrix);

/* see <your pic directory>/pic.h for type Pic */
Pic * g_pHeightData;

/*Creates an array of size 2048 to allow up to 2048 animations to be created. This also allows the frames to be named in a sequential order as they're being created*/
char myFilenm[2048];

/*imported mouse and menu settings from assignment one*/
int g_iMenuId;

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;
CONTROLSTATE g_ControlState = ROTATE;

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};

/* spline struct which contains how many control points, and an array of control points */
struct spline {
   int numControlPoints;
   struct point *points;
};

/* the spline array */
struct spline *g_Splines;

/* total number of splines */
int g_iNumOfSplines;

Mat mFinal(4,3);

int loadSplines(char *argv) {
    char *cName = (char *)malloc(128 * sizeof(char));
    FILE *fileList;
    FILE *fileSpline;
    int iType, i = 0, j, iLength;
    
    
    /* load the track file */
    fileList = fopen(argv, "r");
    if (fileList == NULL) {
        printf ("can't open file\n");
        exit(1);
    }
    
    /* stores the number of splines in a global variable */
    fscanf(fileList, "%d", &g_iNumOfSplines);
    
    g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));
    
    /* reads through the spline files */
    for (j = 0; j < g_iNumOfSplines; j++) {
        i = 0;
        fscanf(fileList, "%s", cName);
        fileSpline = fopen(cName, "r");
        
        if (fileSpline == NULL) {
            printf ("can't open file\n");
            exit(1);
        }
        
        /* gets length for spline file */
        fscanf(fileSpline, "%d %d", &iLength, &iType);
        
        /* allocate memory for all the points */
        g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
        g_Splines[j].numControlPoints = iLength;
        
        /* saves the data to the struct */
        while (fscanf(fileSpline, "%lf %lf %lf",
                      &g_Splines[j].points[i].x,
                      &g_Splines[j].points[i].y, 
                      &g_Splines[j].points[i].z) != EOF) {
            i++;
        }
    }
    
    free(cName);
    
    return 0;
}

int mainCounter =0;

void createControlArray(int PointIteration,int SplineNumber, double *CPoints){
    int counter = 0;
    for(int i = 0; i<PointIteration-2;i++){
        CPoints[counter] = s*g_Splines[SplineNumber].points[i].x;
        CPoints[counter +1] = s*g_Splines[SplineNumber].points[i].y;
        CPoints[counter +2] = s*g_Splines[SplineNumber].points[i].z;
        counter +=3;
    }

}

double U[10];

GLfloat vertices[8][3] =
{{-5.0, -5.0, -2.5}, {5.0, -5.0, -2.5},
	 	 {5.0, 5.0, -2.5}, {-5.0, 5.0, -2.5}, {-5.0, -5.0, 2.5},
	 	 {5.0, -5.0, 2.5}, {5.0, 5.0, 2.5}, {-5.0, 5.0, 2.5}};

GLfloat colors[8][3] =
{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0},
	 	 {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0},
	 	 {1.0, 0.0, 1.0}, {1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}};


void face(int a, int b, int c, int d){
//    glBindTexture(GL_TEXTURE_2D, skyTexture[0]);
//    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBegin(GL_POLYGON);
//			 glColor3fv(colors[a]);
        glTexCoord2f(0.5, 0.0);
        glVertex3fv(vertices[a]);
//			 glColor3fv(colors[b]);
        glTexCoord2f(0.0, 0.0);
        glVertex3fv(vertices[b]);
//			 glColor3fv(colors[c]);
    glTexCoord2f(0.0, 0.5);
        glVertex3fv(vertices[c]);
//			 glColor3fv(colors[d]);
    glTexCoord2f(0.5, 0.5);
        glVertex3fv(vertices[d]);
    glEnd();
//    glDisable(GL_TEXTURE_2D);
}

void colorcube(void){
//	 	 face(0,3,2,1);
//	 	 face(2,3,7,6);
//	 	 face(0,4,7,3);
//	 	 face(1,2,6,5);
//	 	 face(4,5,6,7);
//	 	 face(0,1,5,4);
}



//void createUValues(double key){
//    double numberOfValues = (1/key);
//    for(int i = 0; i<numberOfValues;i++){
//        if(i == 0){
//            U[i] = 0;
//        }
//        else
//            U[i] = key + U[i-1];
//
//    }
//
//    return;
//}

//Mat ComputePointLocation(int uSelection,int splineNum){
//    double arrayinput[1][4];
//    double ControlPointArray[g_Splines[splineNum].numControlPoints * 3];
////    double ArraySize = sizeof(ControlPointArray)/sizeof(*ControlPointArray);
//    double tempArray[g_Splines[splineNum].numControlPoints * 3];
//    /*This creates an array of constantly increasing U values (key) and stores them into a 1x4 matrix*/
//    createUValues(0.1);
//    arrayinput[0][3] = 1;
//    arrayinput[0][2] = U[uSelection];
//    arrayinput[0][1] = U[uSelection]*U[uSelection];
//    arrayinput[0][0] = U[uSelection]*U[uSelection]*U[uSelection];
//    
//    Mat mKey(1,4,*arrayinput);
//
//    createControlArray(g_Splines[splineNum].numControlPoints,splineNum, ControlPointArray);
//
//    int k = 0;
//    int outputCounter = 0;
//    for(int j =0;j<7;j++){
//        //if(k != (g_Splines[0].numControlPoints-1)){
//        double tempA[12];
//        for(int i =0; i<12;i++){
//            tempA[i] = ControlPointArray[k];
//            k++;
////               cout<<ControlPointArray[k]<< "     " << k << endl;
//        }
//        Mat mControlMatrix(4,3,tempA);
//        mControlMatrix.show();
//        Mat mtempMatrix = mBasis.mul(mControlMatrix);
//        mFinal = mKey.mul(mtempMatrix);
//        mFinal.show();
//        tempArray[outputCounter] = mFinal(0,0);
//        tempArray[outputCounter+1] = mFinal(0,1);
//        tempArray[outputCounter+2] = mFinal(0,2);
//        //tempArray[outputCounter+3] = mFinal(0,2);
//        outputCounter +=3;
//    }
//    Mat mOut((g_Splines[splineNum].numControlPoints/3),3,tempArray);
//    return mOut;
//}

void drawPlain(){
    glBegin(GL_POLYGON);
        glColor3f(0.0, 1.0, 0.0);
        glVertex3f(-2, -2, -2.5);
        glVertex3f(2, -2, -2.5);
        glVertex3f(-2, 2, -2.5);
        glVertex3f(2, 2, -2.5);
    glEnd();
}

double catmullRom(float u, float v0, float v1, float v2, float v3){
    double h1,h2,h3,h4;
    h1 = (u*u*u)*(v0*basisMatrix[0][0] + v1*basisMatrix[0][1] + v2*basisMatrix[0][2] + v3*basisMatrix[0][3]);
    h2 = (u*u)*(v0*basisMatrix[1][0] + v1*basisMatrix[1][1] + v2*basisMatrix[1][2] + v3*basisMatrix[1][3]);
    h3 = u*(v0*basisMatrix[2][0] + v1*basisMatrix[2][1] + v2*basisMatrix[2][2] + v3*basisMatrix[2][3]);
    h4 = v0*basisMatrix[3][0] + v1*basisMatrix[3][1] + v2*basisMatrix[3][2] + v3*basisMatrix[3][3];
    
//    double x = (((h4*u + h3)*u +h2)*u + h1);
    double point = h4 +h3 + h2 +h1;
//    cout << point << endl;
    return point;
}

void calculateCatmullRomPoints(double u, int PointIteration){
    //double  u = 0.1;
    float x = catmullRom(u, g_Splines[0].points[PointIteration].x,  g_Splines[0].points[PointIteration+1].x,  g_Splines[0].points[PointIteration +2].x,  g_Splines[0].points[PointIteration + 3].x);
    float y = catmullRom(u, g_Splines[0].points[PointIteration].y,  g_Splines[0].points[PointIteration +1].y,  g_Splines[0].points[PointIteration+2].y,  g_Splines[0].points[PointIteration+3].y);
    float z = catmullRom(u, g_Splines[0].points[PointIteration].z,  g_Splines[0].points[PointIteration+1].z,  g_Splines[0].points[PointIteration+2].z,  g_Splines[0].points[PointIteration+3].z);
    
    cout << x << "  " << y << "  " << z << endl;
}

void drawPointsForBackground(){
    
    int height, width;
    height = g_pHeightData->ny; //the x pixel position
    width = g_pHeightData->nx; //the y pixel position
    GLfloat vertices[g_pHeightData->nx][g_pHeightData->ny];
    
    glBegin(GL_POINTS);
    for(int i = 0; i<height-1;i++){
        for(int j = 0; j<width-1; j++){
            unsigned char heightVal = PIC_PIXEL(g_pHeightData, j, i, 0);
            
            
            glColor3f( 0.0,heightVal/255,0.0);
            glVertex3f(j-width/2, i-height/2, heightVal/10 - 120);
        }
    }
    
    glEnd();
}

//renders the image using triangles
void drawTrianglesForBackground(){
    //Gathers the height and width of the imported picture ex: 256x256 ect.
    int height, width;
    height = g_pHeightData->ny;
    width = g_pHeightData->nx;
    
    glBegin(GL_TRIANGLES);
    for(int i = 0; i<height-1;i++){
        for(int j = 0; j<width-1; j++){
            
            /*These are used to calculate the z height of each pixel by calculating the darkness of each pixel within the greyscale*/
            GLfloat heightVal = PIC_PIXEL(g_pHeightData, j, i, 0);
            GLfloat heightVal1 = PIC_PIXEL(g_pHeightData, j, i+1, 0);
            GLfloat heightVal2 = PIC_PIXEL(g_pHeightData, j+1, i, 0);
            GLfloat heightVal3 = PIC_PIXEL(g_pHeightData, j+1, i+1, 0);
            //unsigned char heightValGreen = PIC_PIXEL(g_pHeightData, j, i, 1);
            //unsigned char heightValBlue = PIC_PIXEL(g_pHeightData, j, i, 2);
            //renders
            glColor3f(0.0, heightVal/255,0.0);
            glVertex3f(j-width/2, i-height/2, heightVal/10 - 120);
            //glColor3f(heightValRed/255, 0.0, 0.0);
            glVertex3f(j-width/2, (i+1)-height/2, heightVal1/10 - 120);
            //glColor3f(0.0, 0.0, heightValRed/255);
            glVertex3f((j+1)-width/2, i-height/2, heightVal2/10 - 120);
            
            glColor3f( 0.0, heightVal/255,0.0);
            glVertex3f(j-width/2, (i+1)-height/2, heightVal1/10 - 120);
            //            glColor3f(0.0, heightValRed/255, 0.0);
            glVertex3f((j+1)-width/2, (i+1)-height/2, heightVal3/10 - 120);
            //            glColor3f(0.0, 0.0, heightValRed/255);
            glVertex3f((j+1)-width/2, i-height/2, heightVal2/10 - 120);
        }
    }
    glEnd();
}

GLfloat ctrlpoints[4][3] = {
    { -1.0, -1.0, 1.0}, { -0.5, 1.0, 0.0},
    {0.5, -1.0, 1.0}, {1.0, 1.0, 0.0}};

void addTextures(){
    glBindTexture(GL_TEXTURE_2D, skyTexture[0]);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    
    
    glEnable(GL_TEXTURE_2D);
    face(0,3,2,1);
    
    
    
    glBindTexture(GL_TEXTURE_2D, skyTexture[1]);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    
    
    face(2,3,7,6);
    
    glBindTexture(GL_TEXTURE_2D, skyTexture[2]);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    
    
    face(0,4,7,3);
    
    glBindTexture(GL_TEXTURE_2D, skyTexture[3]);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    
    face(1,2,6,5);
    
    glBindTexture(GL_TEXTURE_2D, skyTexture[2]);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    
    face(4,5,6,7);
    //
    glBindTexture(GL_TEXTURE_2D, skyTexture[4]);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    
    face(0,1,5,4);
    glDisable(GL_TEXTURE_2D);
}

//void drawpoints(){
//    
//    glBegin(GL_POINTS);
//    glColor3f(0.0, 0.0, 0.0);
//    glLineWidth(1.5);
//    
//    for(int k =0; k<g_iNumOfSplines;k++){
//        Mat mFirstPoint(ComputePointLocation(0,k));
//        glVertex3d(mFirstPoint(0,0), mFirstPoint(0,1), mFirstPoint(0,2)-1);
//        glVertex3d(mFirstPoint(1,0), mFirstPoint(1,1), mFirstPoint(1,2)-1);
//        for(int l =1; l<9;l++){
//            Mat pointLocation(ComputePointLocation(l,k));
//            for(int i = 1;i<g_Splines[k].numControlPoints-2;i++){
//                glVertex3d(pointLocation(i-1,0), pointLocation(i-1,1), pointLocation(i-1,2)-1);
//                glVertex3d(pointLocation(i,0), pointLocation(i,1), pointLocation(i,2)-1);
//            }
//        }
//        Mat mLastPoint(ComputePointLocation(g_Splines[k].numControlPoints-1,k));
//        glVertex3d(mLastPoint(g_Splines[k].numControlPoints-2,0), mLastPoint(g_Splines[k].numControlPoints-2,1), mLastPoint(g_Splines[k].numControlPoints-2,2)-1);
//        glVertex3d(mLastPoint(g_Splines[k].numControlPoints-1,0), mLastPoint(g_Splines[k].numControlPoints-1,1), mLastPoint(g_Splines[k].numControlPoints-1,2)-1);
//    }
//    glEnd();
//}
//
vector<double> pointArray;

void drawSpline(){
   // double splinePoints[];
    double u = 0.0;
    //glBegin(GL_POINTS);
      glColor3b(0.0, 0.0, 0.0);
    for(int k =0; k< g_iNumOfSplines;k++){
        u = 0;
  
        for(int i = 0;i<1000;i++){
            for(int PointIteration = 0;PointIteration<g_Splines[k].numControlPoints;PointIteration+=4){
//        calculateCatmullRomPoints(0.1,i);
                float x = catmullRom(u, g_Splines[k].points[PointIteration].x,  g_Splines[k].points[PointIteration+1].x,  g_Splines[k].points[PointIteration +2].x,  g_Splines[k].points[PointIteration + 3].x);
                float y = catmullRom(u, g_Splines[k].points[PointIteration].y,  g_Splines[k].points[PointIteration +1].y,  g_Splines[k].points[PointIteration+2].y,  g_Splines[k].points[PointIteration+3].y);
                float z = catmullRom(u, g_Splines[k].points[PointIteration].z,  g_Splines[k].points[PointIteration+1].z,  g_Splines[k].points[PointIteration+2].z,  g_Splines[k].points[PointIteration+3].z);
                //glVertex3f(x, y, z-1);
                pointArray.push_back(x);
                pointArray.push_back(y);
                pointArray.push_back(z);
            }
            u +=0.001;
        }
    }
    //glEnd();
    glBegin(GL_POINTS);
    glColor3d(0.0, 0.0, 0.0);
    for(int i =0;i<pointArray.size();i+=3){
        if(i<3){
            glVertex3f(pointArray.at(i), pointArray.at(i+1), pointArray.at(i+2)-1);
        }
        else{
            glVertex3f(pointArray.at(i), pointArray.at(i+1), pointArray.at(i+2)-1);
            glVertex3f(pointArray.at(i-3), pointArray.at(i-2), pointArray.at(i-1)-1);
        }
    }
    glEnd();

    
}


void display(void){
	// clear buffers
//    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    
    glLoadIdentity();
    
    glRotatef(g_vLandRotate[0]/10 , 1, 0, 0);
    glRotatef(g_vLandRotate[1]/10 , 0, 1, 0);
    glRotatef(g_vLandRotate[2]/10 , 0, 0, 1);
    glTranslated(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]);
    glScaled(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);
    
    addTextures();
    glLoadIdentity();
//        drawSpline();
    
    drawPointsForBackground();
  //  drawTrianglesForBackground();

    glutSwapBuffers();
}

void reshape(int w, int h){
    
    glViewport(0,0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(120,(GLfloat) w/h, .01, 1000.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Init()
{
    /* setup gl view here */
    glClearColor(1.0, 1.0, 1.0, 1.0);
//    glClearColor(0.0,0.0,0.0,0.0);
//    glShadeModel(GL_SMOOTH);
//    glEnable(GL_LINE_SMOOTH);
//    glEnable(GL_BLEND);
    glLineWidth(1.5);
    glGenTextures(5, skyTexture);
    texLoad(0, skyPic1);
    texLoad(1, skyPic2);
    texLoad(2, skyPic3);
    texLoad(3, skyPic4);
    texLoad(4, skyPic5);

}

void Idle(){
	/* make the screen update */
    glutPostRedisplay();
}

/* converts mouse drags into information about
 rotation/translation/scaling */
void mousedrag(int x, int y)
{
    int vMouseDelta[2] = {x-g_vMousePos[0], y-g_vMousePos[1]};
    
    switch (g_ControlState)
    {
        case TRANSLATE:
            if (g_iLeftMouseButton)
            {
                g_vLandTranslate[0] += vMouseDelta[0]*0.01;
                g_vLandTranslate[1] -= vMouseDelta[1]*0.01;
            }
            if (g_iMiddleMouseButton)
            {
                g_vLandTranslate[2] += vMouseDelta[1]*0.01;
            }
            break;
        case ROTATE:
            if (g_iLeftMouseButton)
            {
                g_vLandRotate[0] += vMouseDelta[1];
                g_vLandRotate[1] += vMouseDelta[0];
            }
            if (g_iMiddleMouseButton)
            {
                g_vLandRotate[2] += vMouseDelta[1];
            }
            break;
        case SCALE:
            if (g_iLeftMouseButton)
            {
                g_vLandScale[0] *= 1.0+vMouseDelta[0]*0.01;
                g_vLandScale[1] *= 1.0-vMouseDelta[1]*0.01;
            }
            if (g_iMiddleMouseButton)
            {
                g_vLandScale[2] *= 1.0-vMouseDelta[1]*0.01;
            }
            break;
    }
    g_vMousePos[0] = x;
    g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
    g_vMousePos[0] = x;
    g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y)
{
    
    switch (button)
    {
        case GLUT_LEFT_BUTTON:
            g_iLeftMouseButton = (state==GLUT_DOWN);
            break;
        case GLUT_MIDDLE_BUTTON:
            g_iMiddleMouseButton = (state==GLUT_DOWN);
            break;
        case GLUT_RIGHT_BUTTON:
            g_iRightMouseButton = (state==GLUT_DOWN);
            break;
    }
    
    switch(glutGetModifiers())
    {
        case GLUT_ACTIVE_CTRL:
            g_ControlState = TRANSLATE;
            break;
        case GLUT_ACTIVE_SHIFT:
            g_ControlState = SCALE;
            break;
        default:
            g_ControlState = ROTATE;
            break;
    }
    
    g_vMousePos[0] = x;
    g_vMousePos[1] = y;
}

void menufunc(int value)
{
    switch (value)
    {
        case 0:
            exit(0);
            break;
    }
}

void keyboard(unsigned char key, int x, int y)
{
    if (key=='q' || key == 'Q')
        exit(0);
}

int main (int argc, char ** argv)
{
  if (argc<2)
  {  
  printf ("usage: %s <trackfile>\n", argv[0]);
  exit(0);
  }
    
    if (argc<2)
    {
        printf ("usage: %s heightfield.jpg\n", argv[1]);
        exit(1);
    }
    
    g_pHeightData = jpeg_read(argv[2], NULL);
    if (!g_pHeightData)
    {
        printf ("error reading %s.\n", argv[2]);
        exit(1);
    }

    
    loadSplines(argv[1]);
    glutInit(&argc,argv);
    
    // request double buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
    
    // set window size
    glutInitWindowSize(1024, 800);

    // set window position
    glutInitWindowPosition(0, 0);
    
    // creates a window
    glutCreateWindow("Assignment 2");

    cout << g_iNumOfSplines << endl;
    
    glutDisplayFunc(display);

    
    g_iMenuId = glutCreateMenu(menufunc);
    glutSetMenu(g_iMenuId);
    glutAddMenuEntry("Quit",0);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    glutIdleFunc(Idle);
    
    /* callback for mouse drags */
    glutMotionFunc(mousedrag);
    /* callback for idle mouse movement */
    glutPassiveMotionFunc(mouseidle);
    /* callback for mouse button changes */
    glutMouseFunc(mousebutton);
    
    glutKeyboardFunc(keyboard);
    /*generates the U value array*/

    Init();
//    textureInit();
    glutReshapeFunc(reshape);

//    glEnable(GL_LINE_SMOOTH);
    glutMainLoop();

  return 0;
}


