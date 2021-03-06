/*
 CSCI 480
 Assignment 2
 */

#include <stdio.h>
#include <iostream.h>
#include <stdlib.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <math.h>
#include <sstream>
#include "pic.h"
#include "RgbImage.h"

using namespace std;

#define kWORLDSCALE 100
#define pathToScreenShots "/Users/rcleary/Desktop/assign2_copied/Screenshots/"

bool screenShotMode = false;
int screenShotCounter = 0;
string fileNameArray [1000];

/* Write a screenshot to the specified filename */
void saveScreenshot (char *filename)
{
    int i, j;
    Pic *in = NULL;
    
    if (filename == NULL)
        return;
    
    /* Allocate a picture buffer */
    in = pic_alloc(640, 480, 3, NULL);
    
    printf("File to save to: %s\n", filename);
    
    for (i=479; i>=0; i--) {
        glReadPixels(0, 479-i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
                     &in->pix[i*in->nx*in->bpp]);
    }
    
    if (jpeg_write(filename, in))
        printf("File saved Successfully\n");
    else
        printf("Error in Saving\n");
    
    pic_free(in);
}


/* represents one control point along the spline */
struct point {
    double x;
    double y;
    double z;
};

class vector {
public:
    double x;
    double y;
    double z;
    
    friend vector operator+(const vector &a, const vector &b);
    friend vector operator-(const vector &a, const vector &b);

    
    friend vector operator+(const vector &a, const vector &b)
    {
        vector c;
        c.x = a.x + b.x;
        c.y = a.y + b.y;
        c.z = a.z + b.z;
        
        return c;
    }
    
    friend vector operator-(const vector &a, const vector &b)
    {
        vector c;
        c.x = a.x - b.x;
        c.y = a.y - b.y;
        c.z = a.z - b.z;
        
        return c;
    }

};


vector B0,T0,N0;
vector B1,T1,N1;
vector P;

GLuint texName[5];
//array of textures
Pic * textureArray[5];


double c = 0.75; // constant used for distance between the tracks



// Animation Vectors and other Variables
//vector AP,AT0,AT1,AB0,AB1,AN0,AN1;
int check_to_animate_count = 0;
int anim_counter = 0;
int anim_counter_max;
#define kAnimCheckCountMax 500
double anim_u = 0.0;

// Animation Vector Arrays
vector *AP,*AB,*AT,*AN;

// Counters for these arrays
int ac = 0;


vector crossProduct(vector a, vector b);
vector unitVector(vector a);
vector vectorAdd(vector a, vector b);
vector vectorSub(vector a, vector b);
vector vectorScalarMult(double s, vector a);
void letThereBeLight();

double catmullRomSplineFormula(double p0, double p1, double p2, double p3, double t);
double derivativeOfCatmullRomSplineFormula(double p0, double p1, double p2, double p3, double t);
void drawVector(vector a);
void drawScene(void);

/* spline struct which contains how many control points, and an array of control points */
struct spline {
    int numControlPoints;
    struct point *points;
};

/* the spline array */
struct spline *g_Splines;

/* total number of splines */
int g_iNumOfSplines;

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;
//bool screenShotMode = false;
//int screenShotCounter = 0;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;
typedef enum { FREE, RIDING } VIEWMODE;
int SPEED = 1;

CONTROLSTATE g_ControlState = ROTATE;
VIEWMODE viewMode = RIDING;

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};


bool animationPaused = false;

void keyPressed (unsigned char key, int x, int y) {  
    if (key == 'p' || key == 'P')
        animationPaused = !animationPaused;
    else if (key == 'f' || key == 'F')
        viewMode = FREE;
    else if (key == 'r' || key == 'R')
        viewMode = RIDING;
    else if (key == '1')
        SPEED = 1;
    else if (key == '2')
        SPEED = 2;
    else if (key == '3')
        SPEED = 3;
    else if (key == '4')
        SPEED = 4;
    else if (key == '5')
        SPEED = 5;
    else if (key == '6')
        SPEED = 6;
    else if (key == '7')
        SPEED = 7;
    else if (key == '8')
        SPEED = 8;
    else if (key == '9')
        SPEED = 9;
    else if (key == 's' || key == 'S')
    {
        screenShotMode = !screenShotMode;  // Toggle screen shot mode on or off
    }
}  


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
    else {
        //printf("Successfully opened the track file named %s.\n",argv);
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
            printf ("can't open file named %s \n",cName);
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

void display() 
{
    glLoadIdentity();
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    

    
    letThereBeLight(); // add in the light
    
    ///           p        p+t       binormal
    //gluLookAt(0, 0, 20, 0, 0, 0, 0, 1, 0);
    
    //           p        p+t       binormal

    if (viewMode == RIDING)
    {
        if (anim_counter == anim_counter_max) 
            anim_counter = 0; // go back to the beginning of the track
        
        int i = anim_counter; // readability
        double px1 = AP[i].x;
        double py1 = AP[i].y;
        double pz1 = AP[i].z;
        
        // center the view point between the 2 tracks
        px1+=(c*AN[i].x/2);
        py1+=(c*AN[i].y/2);
        pz1+=(c*AN[i].z/2);
        
        // move the viewer's head a bit above the track
        px1+=(AB[i].x);
        py1+=(AB[i].y);
        pz1+=(AB[i].z);
        
        
        // the NEXT point (where you should look)
        double px2 = AP[i+1].x;
        double py2 = AP[i+1].y;
        double pz2 = AP[i+1].z;
        
        // center the view point between the 2 tracks
        px2+=(c*AN[i+1].x/2);
        py2+=(c*AN[i+1].y/2);
        pz2+=(c*AN[i+1].z/2);
        
        // move the viewer's head a bit above the track
        px2+=(AB[i+1].x);
        py2+=(AB[i+1].y);
        pz2+=(AB[i+1].z);


//        gluLookAt(px1, py1, pz1,    
//                  AP[i].x + AT[i].x, AP[i].y + AT[i].y, AP[i].z + AT[i].z,   
//                  AB[i].x, AB[i].y, AB[i].z);
        
       gluLookAt(px1, py1, pz1,    
                px2, py2, pz2,   
                AB[i].x, AB[i].y, AB[i].z);

        
    }
    else if (viewMode == FREE)
    {
        gluLookAt(0, 0, 20, 0, 0, 0, 0, 1, 0);
        
        glTranslatef(g_vLandTranslate[0]-.5, g_vLandTranslate[1]-.5, g_vLandTranslate[2]);  // the x,y, and z translation
        
        glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);
        
        glRotatef(g_vLandRotate[0], 1, 0, 0); // g_vLandRotate returns a theta (angle) value
        glRotatef(g_vLandRotate[1], 0, 1, 0);
        glRotatef(g_vLandRotate[2], 0, 0, 1);
    }

    
    
    drawScene(); // draw the scene and textures

//    //           p        p+t       binormal
//    gluLookAt(P.x, P.y, P.z,    P.x + T1.x, P.y + T1.y, P.z + T1.z,    B1.x, B1.y, B1.z);

  
   


    glBegin(GL_POINTS);
    // Draw the catmull-rom spline curve of the roller coaster:
    for (int t = 1; t < g_Splines[0].numControlPoints-2; t++)
    {
        for (double u = 0.0; u < 1.0; u+=0.01)
        {
            double p0x = g_Splines[0].points[t-1].x;
            double p1x = g_Splines[0].points[t].x;
            double p2x = g_Splines[0].points[t+1].x;
            double p3x = g_Splines[0].points[t+2].x;
            P.x = catmullRomSplineFormula(p0x,p1x,p2x,p3x,u);
            
            double p0y = g_Splines[0].points[t-1].y;
            double p1y = g_Splines[0].points[t].y;
            double p2y = g_Splines[0].points[t+1].y;
            double p3y = g_Splines[0].points[t+2].y;
            P.y = catmullRomSplineFormula(p0y,p1y,p2y,p3y,u);

            double p0z = g_Splines[0].points[t-1].z;
            double p1z = g_Splines[0].points[t].z;
            double p2z = g_Splines[0].points[t+1].z;
            double p3z = g_Splines[0].points[t+2].z;
            P.z = catmullRomSplineFormula(p0z,p1z,p2z,p3z,u);

            
            glColor3f(0.1, 0.1, 0.5); // a random color for now
            glLineWidth(10.0);
            glVertex3d(P.x, P.y, P.z); // px + nx, py + ny, pz+ nz fpr seoncd track
                
            
            if (t == 1)
            {
            vector V0; // Arbitrary Vector used to calculate the Binormal B
            V0.x = 0;
            V0.y = 1;
            V0.z = 0;
            
            
            // Tangent Vector
            T0.x = derivativeOfCatmullRomSplineFormula(p0x, p1x, p2x, p3x, u);
            T0.y = derivativeOfCatmullRomSplineFormula(p0y, p1y, p2y, p3y, u);
            T0.z = derivativeOfCatmullRomSplineFormula(p0z, p1z, p2z, p3z, u);
            
            
            // Normal Vector 
            N0 = unitVector(crossProduct(T0, V0));  // N0 = unit(T0xV0)
            
            
            // Binormal Vector
            B0 = unitVector(crossProduct(T0, N0)); // B0 = unit(T0xN0)
            
            
            // Draw the second track
            glColor3f(0.6, 0.2, 0.34); // a diff color to distinguish the 2nd track
            glVertex3d(P.x+(c*N0.x), P.y+(c*N0.y), P.z+(c*N0.z)); // px + nx, py + ny, pz+ nz for second track
            
            }
            else if (t > 1)  // calculating further NTB sets (after the initial one)
            {
                // Tangent Vector
                T1.x = derivativeOfCatmullRomSplineFormula(p0x, p1x, p2x, p3x, u);
                T1.y = derivativeOfCatmullRomSplineFormula(p0y, p1y, p2y, p3y, u);
                T1.z = derivativeOfCatmullRomSplineFormula(p0z, p1z, p2z, p3z, u);
                
                N1 = unitVector(crossProduct(B0,T1)); // N1 = unit(B0xT0) (note: T0 is essentially T1)
                
                B1 = unitVector(crossProduct(T1, N1)); // B1 = unit(T1xN1)
        
                
                // Draw the second track
                glColor3f(0.6, 0.2, 0.34); // a different color to distinguish the 2nd track
                glVertex3d(P.x+(c*N1.x), P.y+(c*N1.y), P.z+(c*N1.z)); // px + nx, py + ny, pz + nz for second track
                
                
                // for the next iteration:
                B0 = B1; // the current B is the soon-to-be old B
                N0 = N1; //     ''      N          ''           N
                T0 = T1; //     ''      T          ''           T
             
            }
        }
        
    }
    glEnd();
  
    
    
    
    
    
    vector v0,v1,v2,v3,v4,v5,v6,v7;
    
    
    double s = 0.05;
    int counter = 0;
    int drawCounter = 0;
    bool draw = false;

    // FLAT TRACK SIDES AND CROSSBARS:
    for (int t = 1; t < g_Splines[0].numControlPoints-2; t++)
    {
        draw = false;
#define kNumLoopSkipsTrackCrossbars 20
        for (double u = 0.0; u < 1.0; u+=0.01)
        {
            counter++;
            if (counter == kNumLoopSkipsTrackCrossbars)
            {
                draw = true;
                counter = 0;
            }
            if (draw == true)
            {
                drawCounter++;
                if (drawCounter == 5)
                {
                    draw = false;
                    drawCounter = 0;
                }
            }
            
            double p0x = g_Splines[0].points[t-1].x;
            double p1x = g_Splines[0].points[t].x;
            double p2x = g_Splines[0].points[t+1].x;
            double p3x = g_Splines[0].points[t+2].x;
            P.x = catmullRomSplineFormula(p0x,p1x,p2x,p3x,u);
            
            double p0y = g_Splines[0].points[t-1].y;
            double p1y = g_Splines[0].points[t].y;
            double p2y = g_Splines[0].points[t+1].y;
            double p3y = g_Splines[0].points[t+2].y;
            P.y = catmullRomSplineFormula(p0y,p1y,p2y,p3y,u);
            
            double p0z = g_Splines[0].points[t-1].z;
            double p1z = g_Splines[0].points[t].z;
            double p2z = g_Splines[0].points[t+1].z;
            double p3z = g_Splines[0].points[t+2].z;
            P.z = catmullRomSplineFormula(p0z,p1z,p2z,p3z,u);
                        
            v0 = P;
            v1 = P+T0;
            v2 = P+T0+B0;
            v3 = P+B0;
            
            if (draw)
            {
                // left face of the cross-bar
                glColor3f(0.5, 0.5, 0.5);
                glBegin(GL_QUADS);
                drawVector(v2);
                drawVector(v3);
                drawVector(v0);
                drawVector(v1);
                glEnd();
            }

            if (t == 1)
            {
                vector V0; // Arbitrary Vector used to calculate the Binormal B
                V0.x = 0;
                V0.y = 1;
                V0.z = 0;
                
                
                // Tangent Vector
                T0.x = derivativeOfCatmullRomSplineFormula(p0x, p1x, p2x, p3x, u);
                T0.y = derivativeOfCatmullRomSplineFormula(p0y, p1y, p2y, p3y, u);
                T0.z = derivativeOfCatmullRomSplineFormula(p0z, p1z, p2z, p3z, u);
                
                T0.x *= s;
                T0.y *= s;
                T0.z *= s;
                
                
                // Normal Vector 
                N0 = unitVector(crossProduct(T0, V0));  // N0 = unit(T0xV0)
                
                
                // Binormal Vector
                B0 = unitVector(crossProduct(T0, N0)); // B0 = unit(T0xN0)
                
                B0.x *= s;
                B0.y *= s;
                B0.z *= s;
                
                
                v4 = P+vectorScalarMult(c,N0);
                v5 = P+T0+vectorScalarMult(c,N0);
                v6 = P+T0+B0+vectorScalarMult(c,N0);
                v7 = P+B0+vectorScalarMult(c,N0);
                
                if (draw)
                {
                    // right face of the cross-bar
                    glBegin(GL_QUADS);
                    drawVector(v7);
                    drawVector(v6);
                    drawVector(v5);
                    drawVector(v4);
                    glEnd();
                }

                
                
//                glBegin(GL_QUADS);
//                glVertex3d(P.x+B0.x, P.y+B0.y, P.z+B0.z); // v3
//                glVertex3d(P.x+T0.x+B0.x, P.y+T0.y+B0.y, P.z+T0.z+B0.z); // v2
//                glVertex3d(P.x+T0.x, P.y+T0.y, P.z+T0.z);  // v1
//                glVertex3d(P.x, P.y, P.z);  // v0
//                glEnd();
//                
//                glBegin(GL_QUADS);
//                glVertex3d(P.x+N0.x, P.y+N0.y, P.z+N0.z); // v4
//                glVertex3d(P.x+T0.x+N0.x, P.y+T0.y+N0.y, P.z+T0.z+N0.z); // v5
//                glVertex3d(P.x+T0.x+B0.x+N0.x, P.y+T0.y+B0.y+N0.y, P.z+T0.z+B0.z+N0.z); // v6
//                glVertex3d(P.x+B0.x+N0.x, P.y+B0.y+N0.y, P.z+B0.z+N0.z); // v7
//                glEnd();
                
            }
            else if (t > 1)  // calculating further NTB sets (after the initial one)
            {
                // Tangent Vector
                T1.x = derivativeOfCatmullRomSplineFormula(p0x, p1x, p2x, p3x, u);
                T1.y = derivativeOfCatmullRomSplineFormula(p0y, p1y, p2y, p3y, u);
                T1.z = derivativeOfCatmullRomSplineFormula(p0z, p1z, p2z, p3z, u);
                
                T1.x *= s;
                T1.y *= s;
                T1.z *= s;

                
                N1 = unitVector(crossProduct(B0,T1)); // N1 = unit(B0xT0) (note: T0 is essentially T1)
                
                B1 = unitVector(crossProduct(T1, N1)); // B1 = unit(T1xN1)
                
                B1.x *= s;
                B1.y *= s;
                B1.z *= s;
                
                v4 = P+vectorScalarMult(c,N1);
                v5 = P+T0+vectorScalarMult(c,N1);
                v6 = P+T0+B0+vectorScalarMult(c,N1);
                v7 = P+B0+vectorScalarMult(c,N1);
                
                if (draw)
                {
                    // right face of the cross-bar
                    glBegin(GL_QUADS);
                    drawVector(v7);
                    drawVector(v6);
                    drawVector(v5);
                    drawVector(v4);
                    glEnd();
                }

                
                // Draw the second track
//                glColor3f(0.5, 0.5, 0.5);
//                glBegin(GL_QUADS);
//                glVertex3d(P.x+N1.x, P.y+N1.y, P.z+N1.z); // v4
//                glVertex3d(P.x+T0.x+N1.x, P.y+T0.y+N1.y, P.z+T0.z+N1.z); // v5
//                glVertex3d(P.x+T0.x+B0.x+N1.x, P.y+T0.y+B0.y+N1.y, P.z+T0.z+B0.z+N1.z); // v6
//                glVertex3d(P.x+B0.x+N1.x, P.y+B0.y+N1.y, P.z+B0.z+N1.z); // v7
//                glEnd();
                
                // for the next iteration:
                B0 = B1; // the current B is the soon-to-be old B
                N0 = N1; //     ''      N          ''           N
                T0 = T1; //     ''      T          ''           T
                
            }
        
            if (draw)
            {
                // CROSS BARS
                
                glEnable(GL_TEXTURE_2D);
                glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                glBindTexture(GL_TEXTURE_2D, texName[0]);

                glBegin(GL_QUADS);
                
                glTexCoord2f(0.0, 1.0); 
                glVertex3f(v2.x, v2.y, v2.z);
                
                glTexCoord2f(1.0, 1.0); 
                glVertex3f(v6.x, v6.y, v6.z);
                
                glTexCoord2f(1.0, 0.0); 
                glVertex3f(v7.x,v7.y,v7.z);
                
                glTexCoord2f(0.0, 0.0); 
                glVertex3f(v3.x,v3.y,v3.z);
                
            
                // front face of cross-bar
                drawVector(v3);
                drawVector(v7);
                drawVector(v4);
                drawVector(v0);
                
                // back face of cross-bar
                drawVector(v6);
                drawVector(v2);
                drawVector(v1);
                drawVector(v5);
                
                // bottom face of cross-bar
                drawVector(v4);
                drawVector(v5);
                drawVector(v1);
                drawVector(v0);

                 glEnd();
                
                glFlush();
                glDisable(GL_TEXTURE_2D);

            }
        }
    }
    glEnd();

    
    glutSwapBuffers();
    
    
    // Save screen shots ( after the screen has been updated by swapping the buffers ):
    if (screenShotMode == true)
    {
        
        if (screenShotCounter < 1000)
        {
            
            string fileNameStr = fileNameArray[screenShotCounter];
            char *charStarOfFileNameStr = (char*)fileNameStr.c_str();
            saveScreenshot(charStarOfFileNameStr);
            screenShotCounter++;
        }
        else 
        {
            printf("Error: cannot print more than 1000 frames!");
        }
        
        
    }

}



void reshapeFunction(int w, int h) {
    // Set up the perspective:
    glMatrixMode(GL_PROJECTION);
  //  gluLookAt(0, 0, 0, (w/2), (h/2), 0, 0, 0, 0);
    gluPerspective(90, (640/480), 0.1, 1000);
    
    glMatrixMode(GL_MODELVIEW); // not sure if I need this
    glLoadIdentity();

}

/* This function simply implements the formula for a Catmull-Rom spline. 
It takes either the x,y, or z values of 4 points of a curve and the value of t, 
and it returns the double value Q(t). */
double catmullRomSplineFormula(double p0, double p1, double p2, double p3, double t) {
    return 0.5 *((2 * p1) 
                     + (-p0 + p2) * t 
                        + (2*p0 - 5*p1 + 4*p2 - p3) * pow(t,2) 
                            + (-p0 + 3*p1 - 3*p2 + p3) * pow(t,3));
    
}

/* This function implements the derivative of a Catmull-Rom spline formula. 
 It takes either the x,y, or z values of 4 points of a curve and the value of t, 
 and it returns the double value Q'(t). */
double derivativeOfCatmullRomSplineFormula(double p0, double p1, double p2, double p3, double t) {
    return 0.5 *((-p0 + p2) 
                 + 2*t*(2*p0 - 5*p1 + 4*p2 - p3) 
                 + 3*pow(t,2)*(-p0 + 3*p1 - 3*p2 + p3));
}

/* This function takes a vector and draws it using glVertex3d(...) */
void drawVector(vector a) {
    glVertex3d(a.x, a.y, a.z);
}

/* Returns the cross product of the 2 vectors, a x b. */
vector crossProduct(vector a, vector b) {
    vector c;
    
    // c = a x b:
    c.x = a.y*b.z - a.z*b.y;
    c.y = -a.x*b.z + a.z*b.x;
    c.z = a.x*b.y - a.y*b.x;
     
    
    return c;
}


/* Performs vector addition. Returns vector c = vector a + vector b. */
vector vectorAdd(vector a, vector b)
{
    vector c;
    c.x = a.x + b.x;
    c.y = a.y + b.y;
    c.z = a.z + b.z;
    
    return c;
}

/* Performs vector subtraction. Returns vector c = vector a - vector b. */
vector vectorSub(vector a, vector b)
{
    vector c;
    c.x = a.x - b.x;
    c.y = a.y - b.y;
    c.z = a.z - b.z;
    
    return c;
}

/* Returns the unit vector of vector a. */
vector unitVector(vector a) {
    double magnitude = sqrt(a.x*a.x + a.y*a.y + a.z*a.z); 
    
    a.x /= magnitude;
    a.y /= magnitude;
    a.z /= magnitude;
    
    return a;
}

/* Performs scalar vector multiplication. */
vector vectorScalarMult(double s, vector a)
{
    a.x *= s;
    a.y *= s;
    a.z *= s;
    
    return a;
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

void animate()
{
    if (animationPaused)
        return;
    
    if(check_to_animate_count++ >= kAnimCheckCountMax)
    {
        check_to_animate_count = 0;

        if (anim_counter + SPEED <= anim_counter_max)
            anim_counter+=SPEED; // THIS IS DETERMINED BY THE USER BY PRESSING 1,2,3,4, OR 5
        else
            anim_counter = anim_counter_max;
            
        /* make the screen update */
        glutPostRedisplay();
    }
   
    
//    glLoadIdentity();
//    gluLookAt(AP[i].x, AP[i].y, AP[i].z,    
//              AP[i].x + AT[i].x, AP[i].y + AT[i].y, AP[i].z + AT[i].z,   
//              AB[i].x, AB[i].y, AB[i].z);
//
    
    
    /* update the animation variables */
    /*
    if(anim_t++ >= g_Splines[0].numControlPoints-2)
        anim_t = 0;
    
    anim_u += 0.001;
    if (anim_u > 1.0)
        anim_u = 0.0;
    
    double p0x = g_Splines[0].points[anim_t-1].x;
    double p1x = g_Splines[0].points[anim_t].x;
    double p2x = g_Splines[0].points[anim_t+1].x;
    double p3x = g_Splines[0].points[anim_t+2].x;
    AP.x = catmullRomSplineFormula(p0x,p1x,p2x,p3x,anim_u);
    
    double p0y = g_Splines[0].points[anim_t-1].y;
    double p1y = g_Splines[0].points[anim_t].y;
    double p2y = g_Splines[0].points[anim_t+1].y;
    double p3y = g_Splines[0].points[anim_t+2].y;
    AP.y = catmullRomSplineFormula(p0y,p1y,p2y,p3y,anim_u);
    
    double p0z = g_Splines[0].points[anim_t-1].z;
    double p1z = g_Splines[0].points[anim_t].z;
    double p2z = g_Splines[0].points[anim_t+1].z;
    double p3z = g_Splines[0].points[anim_t+2].z;
    AP.z = catmullRomSplineFormula(p0z,p1z,p2z,p3z,anim_u);

    
    if (anim_t == 1)
    {
        vector V0; // Arbitrary Vector used to calculate the Binormal AB0
        V0.x = 0;
        V0.y = 1;
        V0.z = 0;
        
        
        // Tangent Vector
        AT0.x = derivativeOfCatmullRomSplineFormula(p0x, p1x, p2x, p3x, anim_u);
        AT0.y = derivativeOfCatmullRomSplineFormula(p0y, p1y, p2y, p3y, anim_u);
        AT0.z = derivativeOfCatmullRomSplineFormula(p0z, p1z, p2z, p3z, anim_u);
        
        
        // Normal Vector 
        AN0 = unitVector(crossProduct(AT0, V0));  // AN0 = unit(AT0xV0)
        
        
        // Binormal Vector
        AB0 = unitVector(crossProduct(AT0, AN0)); // AB0 = unit(AT0xN0)
    }
    else if (anim_t > 1)  // calculating further NTB sets (after the initial one)
    {
        // Tangent Vector
        AT1.x = derivativeOfCatmullRomSplineFormula(p0x, p1x, p2x, p3x, anim_u);
        AT1.y = derivativeOfCatmullRomSplineFormula(p0y, p1y, p2y, p3y, anim_u);
        AT1.z = derivativeOfCatmullRomSplineFormula(p0z, p1z, p2z, p3z, anim_u);
        
        AN1 = unitVector(crossProduct(AB0,AT1)); // AN1 = unit(AB0xAT0) (note: AT0 is essentially AT1)
        
        AB1 = unitVector(crossProduct(AT1, AN1)); // AB1 = unit(AT1xAN1)
        
        // for the next iteration:
        AB0 = AB1; // the current B is the soon-to-be old B
        AN0 = AN1; //     ''      N          ''           N
        AT0 = AT1; //     ''      T          ''           T
        
    }
*/
    
    // Try just looping through an array of vectors
    
}



/*
 * Read a texture map from a BMP bitmap file.
 */
void loadTextureFromFile(char *filename,int n)
{    
    cout<<endl<<"Loading texture from file "<<filename<<"."<<endl;
	glClearColor (0.0, 0.0, 0.0, 0.0);
	glShadeModel(GL_FLAT);
	glEnable(GL_DEPTH_TEST);
    
    textureArray[texName[n]] = jpeg_read(filename, NULL);
    if (!textureArray[texName[n]])
    {
        printf ("error reading %s.\n", filename);
        return;
    }    

    glBindTexture(GL_TEXTURE_2D, texName[n]);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    

    // regular way of image data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, textureArray[texName[n]]->pix);

    
//    // Mipmaps:
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    
//	gluBuild2DMipmaps(GL_TEXTURE_2D, 3,theTexMap.GetNumCols(), theTexMap.GetNumRows(),
//                      GL_RGB, GL_UNSIGNED_BYTE, theTexMap.ImageData() );

    
}

/*
 * Sets up the lighting for the OpenGL scene.
 */
void letThereBeLight() {
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);
    
    // set up the Sun light
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    
    GLfloat light_ambient[] = {0.7,0.7,0.7,1.0};
    GLfloat light_diffuse[] = {1.0,1.0,1.0,1.0};
    GLfloat light_specular[] = {1.0,1.0,1.0,1.0};
    //GLfloat light_position[] = {0.0,(0.5 * kWORLDSCALE),0.0,0.0};
    GLfloat light_position[] = {-1.0,1.0,-1.0,0.0};

    // LIGHT 0:
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    
    
    // LIGHT 1:
    glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
    light_position[0] = 1.0;
    light_position[0] = 1.0;
    light_position[0] = -1.0;
    light_position[0] = 0.0;

    
    glLightfv(GL_LIGHT1, GL_POSITION, light_position);

    
    

}

/*
 * Do the texture and sky and ground, etc.
 */
void drawScene(void)
{
    
    
    
    double ws = kWORLDSCALE; // world scale
    vector p0,p1,p2,p3,p4,p5,p6,p7;
    p0.x = -ws;
    p0.y = -ws;
    p0.z = ws;
    
    p1.x = -ws;
    p1.y = -ws;
    p1.z = -ws;
    
    p2.x = ws;
    p2.y = -ws;
    p2.z = -ws;
    
    p3.x = ws;
    p3.y = -ws;
    p3.z = ws;
    
    p4.x = -ws;
    p4.y = ws;
    p4.z = ws;
    
    p5.x = -ws;
    p5.y = ws;
    p5.z = -ws;
    
    p6.x = ws;
    p6.y = ws;
    p6.z = -ws;
    
    p7.x = ws;
    p7.y = ws;
    p7.z = ws;
    
    
    
    // CLOUDS TEXTURE
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, texName[1]);
    glBegin(GL_QUADS);
    
    // first face (fixed)
    glTexCoord2f(0.0, 1.0); 
    drawVector(p5);
    
    glTexCoord2f(0.0, 0.0); 
    drawVector(p1);
    
    glTexCoord2f(1.0, 0.0); 
    drawVector(p2);
    
    glTexCoord2f(1.0, 1.0); 
    drawVector(p6);
    
       
    // next face (fixed)
    glTexCoord2f(0.0, 0.0); 
    drawVector(p3);
    
    glTexCoord2f(0.0, 1.0); 
    drawVector(p7);
    
    glTexCoord2f(1.0, 1.0); 
    drawVector(p6);
    
    glTexCoord2f(1.0, 0.0); 
    drawVector(p2);

    
    // next face
    glTexCoord2f(1.0, 1.0); 
    drawVector(p4);
    
    glTexCoord2f(1.0, 0.0); 
    drawVector(p0);
    
    glTexCoord2f(0.0, 0.0); 
    drawVector(p1);
    
    glTexCoord2f(0.0, 1.0); 
    drawVector(p5);

        
    // next face (fixed)
    glTexCoord2f(1.0, 1.0); 
    drawVector(p4);

    glTexCoord2f(1.0, 0.0); 
    drawVector(p0);
    
    glTexCoord2f(0.0, 0.0); 
    drawVector(p3);
    
    glTexCoord2f(0.0, 1.0); 
    drawVector(p7);

    glEnd();

    
    glFlush();
    glDisable(GL_TEXTURE_2D);
    

    
    
    // GROUND TEXTURE
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, texName[2]); // make GROUND texture active texture
    glBegin(GL_QUADS);
    
    glTexCoord2f(0.0, 1.0); 
    drawVector(p7);
    
    glTexCoord2f(1.0, 1.0); 
    drawVector(p4);
    
    glTexCoord2f(1.0, 0.0); 
    drawVector(p5);
    
    glTexCoord2f(0.0, 0.0); 
    drawVector(p6);
    
    
    glEnd();
    
    glFlush();
    glDisable(GL_TEXTURE_2D);
    
    
    
    
    
    
    
    // SUN TEXTURE
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, texName[3]); // make GROUND texture active texture
    glBegin(GL_QUADS);

    // next face (TOP OF SKY)
    glTexCoord2f(0.0, 1.0); 
    drawVector(p0);
    
    glTexCoord2f(0.0, 0.0); 
    drawVector(p3);
    
    glTexCoord2f(1.0, 0.0); 
    drawVector(p2);
    
    glTexCoord2f(1.0, 1.0); 
    drawVector(p1);
    
    
    glEnd();
    
    glFlush();
    glDisable(GL_TEXTURE_2D);


    
//    // GROUND sky bottom face
//    glColor3f(0.8f, 0.2f, 0.2f);  // ground color
//    glBegin(GL_QUADS);
////    glEnd();
//
//    // sky top face
////    glBegin(GL_QUADS);
//    glColor3f(0.2f, 1.0f, 1.0f); // blue sky
////    glEnd();
//    
//    // sky right face
////    glBegin(GL_QUADS);
//    glColor3f(0.4f, 0.5f, 0.0f); // blue sky
////    glEnd();
//
//    // sky left face
////    glBegin(GL_QUADS);
//    glColor3f(0.4f, 0.5f, 0.7f); // blue sky
//    
//    //sky front face 
//    glColor3f(0.7, 0.4, 0.2);
//
//    glEnd();
    
 

    glTranslatef(g_vLandTranslate[0]-.5, g_vLandTranslate[1]-.5, g_vLandTranslate[2]);  // the x,y, and z translation
    
    glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);
    
    glRotatef(g_vLandRotate[0], 1, 0, 0); // g_vLandRotate returns a theta (angle) value
    glRotatef(g_vLandRotate[1], 0, 1, 0);
    glRotatef(g_vLandRotate[2], 0, 0, 1);


    
    
    
    /*
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    
    glBegin(GL_QUADS);
    
    glTexCoord2f(0.0, 0.0); 
    glVertex3f(-1.0, -1.0, 0.0);
    
    glTexCoord2f(0.0, 1.0); 
    glVertex3f(-1.0, 1.0, 0.0);
    
    glTexCoord2f(1.0, 1.0); 
    glVertex3f(1.0, 1.0, 0.0);
    
    glTexCoord2f(1.0, 0.0); 
    glVertex3f(1.0, -1.0, 0.0);
    
    glEnd();
    
    glFlush();
    glDisable(GL_TEXTURE_2D);
  */
}


int main (int argc, char ** argv)
{
    if (argc<2)
    {  
        printf ("usage: %s <trackfile>\n", argv[0]);
        exit(0);
    }
    
    
    
    
    // PRE-LOAD THE SCREEN SHOT FILE NAME ARRAY
    for(int tmpCounter = 0; tmpCounter < 1000; tmpCounter++)
    {
        
        string fileNameStr, onesPlaceStr, tensPlaceStr, hundredsPlaceStr;
        int onesPlace = tmpCounter / 100;
        int tensPlace = tmpCounter / 10;
        int hundredsPlace = tmpCounter;
        
        
        
        
        if (hundredsPlace > 9 && hundredsPlace < 100)
            hundredsPlace %= 100;
        else if (hundredsPlace > 99)
            hundredsPlace %= 100;
        
        
        if (tensPlace > 9 && tensPlace < 100)
            tensPlace %= 10;
        else if (tensPlace > 99)
            tensPlace %= 100;
        
        if (hundredsPlace > 9 && hundredsPlace < 100)
            hundredsPlace %= 10;
        else if (hundredsPlace > 99)
            hundredsPlace %= 100;
        
        
        
        
        stringstream ss1, ss2, ss3;
        ss1 << onesPlace;
        ss1 >> onesPlaceStr;
        
        ss2 << tensPlace;
        ss2 >> tensPlaceStr;
        
        ss3 << hundredsPlace;
        ss3 >> hundredsPlaceStr;
        
        fileNameStr.append(pathToScreenShots);
        fileNameStr.append(onesPlaceStr);
        fileNameStr.append(tensPlaceStr);
        fileNameStr.append(hundredsPlaceStr);
        fileNameStr.append(".jpg");
        
        printf("%s\n",fileNameStr.c_str());
        
        
        fileNameArray[tmpCounter] = fileNameStr;     
    }
   
    
    loadSplines(argv[1]);
    
    glutInit(&argc,argv);
    
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(640, 480);
    glutCreateWindow("Assigment 2");
    printf("NUMBER OF SPLINES = %i",g_iNumOfSplines);
       
    
    // initialize some variables:
    P.x = 0;
    P.y = 0;
    P.z = 0;
    
    T1.x = 0;
    T1.y = 0;
    T1.z = 0;
    
    B1.x = 0;
    B1.y = 0;
    B1.z = 0;
    
    // Animation Vector Arrays dynamic allocation (we add a factor of 10 just to be safe)
    AP = new vector[g_Splines[0].numControlPoints * 1000];
    AB = new vector[g_Splines[0].numControlPoints * 1000];
    AT = new vector[g_Splines[0].numControlPoints * 1000];
    AN = new vector[g_Splines[0].numControlPoints * 1000];

    
    // Bring in the data for the animation array
    for (int t = 1; t < g_Splines[0].numControlPoints-2; t++)
    {
        for (double u = 0.0; u < 1.0; u+=0.01)
        {
            anim_counter_max++;
            
            double p0x = g_Splines[0].points[t-1].x;
            double p1x = g_Splines[0].points[t].x;
            double p2x = g_Splines[0].points[t+1].x;
            double p3x = g_Splines[0].points[t+2].x;
            P.x = catmullRomSplineFormula(p0x,p1x,p2x,p3x,u);
            
            double p0y = g_Splines[0].points[t-1].y;
            double p1y = g_Splines[0].points[t].y;
            double p2y = g_Splines[0].points[t+1].y;
            double p3y = g_Splines[0].points[t+2].y;
            P.y = catmullRomSplineFormula(p0y,p1y,p2y,p3y,u);
            
            double p0z = g_Splines[0].points[t-1].z;
            double p1z = g_Splines[0].points[t].z;
            double p2z = g_Splines[0].points[t+1].z;
            double p3z = g_Splines[0].points[t+2].z;
            P.z = catmullRomSplineFormula(p0z,p1z,p2z,p3z,u);
            
            AP[ac] = P; // load into the animation array

            if (t == 1)
            {
                vector V0; // Arbitrary Vector used to calculate the Binormal B
                V0.x = 0;
                V0.y = 1;
                V0.z = 0;
                
                
                // Tangent Vector
                T0.x = derivativeOfCatmullRomSplineFormula(p0x, p1x, p2x, p3x, u);
                T0.y = derivativeOfCatmullRomSplineFormula(p0y, p1y, p2y, p3y, u);
                T0.z = derivativeOfCatmullRomSplineFormula(p0z, p1z, p2z, p3z, u);
                
                
                // Normal Vector 
                N0 = unitVector(crossProduct(T0, V0));  // N0 = unit(T0xV0)
                
                
                // Binormal Vector
                B0 = unitVector(crossProduct(T0, N0)); // B0 = unit(T0xN0)
                
                // Load the animation arrays
                AT[ac] = T0;
                AB[ac] = B0;  
                AN[ac] = N0;
                
            }
            else if (t > 1)  // calculating further NTB sets (after the initial one)
            {
                // Tangent Vector
                T1.x = derivativeOfCatmullRomSplineFormula(p0x, p1x, p2x, p3x, u);
                T1.y = derivativeOfCatmullRomSplineFormula(p0y, p1y, p2y, p3y, u);
                T1.z = derivativeOfCatmullRomSplineFormula(p0z, p1z, p2z, p3z, u);
               
                N1 = unitVector(crossProduct(B0,T1)); // N1 = unit(B0xT1) 
                
                B1 = unitVector(crossProduct(T1, N1)); // B1 = unit(T1xN1)
                
                
                // Load the animation arrays
                AT[ac] = T1;
                AB[ac] = B1;
                AN[ac] = N1;
                
                // for the next iteration:
                B0 = B1; // the current B is the soon-to-be old B
                
                // These aren't necessary...
//                N0 = N1; //     ''      N          ''           N
//                T0 = T1; //     ''      T          ''           T

                
            }
        
            ac++; // increment the animation array counter
        }
    }
    
    
    
    // LOAD ALL OF THE TEXTURES INTO MEMORY:    
    glGenTextures(4, texName);

    
    // taken from http://fc02.deviantart.net/fs71/f/2011/191/e/9/wood_plank_2_textures_by_cgsiino-d3ln7wa.jpg
    char *fileStr = "/Users/rcleary/Desktop/assign2_copied/assign1/woodplanks2.jpeg";
    loadTextureFromFile(fileStr, 0);
    
    // taken from http://fc08.deviantart.net/fs70/f/2010/131/b/9/Sky_53_by_Sed_rah_Stock.jpg
    fileStr = "/Users/rcleary/Desktop/assign2_copied/assign1/sky1.jpeg";
    loadTextureFromFile(fileStr, 1);
    
    // taken from http://www.houstongrasssouth.net/wp-content/uploads/2009/03/palisades2.jpg
    fileStr = "/Users/rcleary/Desktop/assign2_copied/assign1/grass1.jpeg";
    loadTextureFromFile(fileStr, 2);

    // taken from http://www.tysto.com/articles05/pics/sky/clouds-sun-bird1.jpg
    fileStr = "/Users/rcleary/Desktop/assign2_copied/assign1/sun2.jpeg";
    loadTextureFromFile(fileStr, 3);

    
   // glBindTexture(GL_TEXTURE_2D, texName[0]);
    
    
    glutKeyboardFunc(keyPressed);
    
    glutReshapeFunc(reshapeFunction);
    
    
    glClearColor(0.0, 0.0, 0.0, 0.0);
    
    glutDisplayFunc(display);
    
    glutIdleFunc(animate);
    
    /* callback for mouse drags */
    glutMotionFunc(mousedrag);
    /* callback for idle mouse movement */
    glutPassiveMotionFunc(mouseidle);
    /* callback for mouse button changes */
    glutMouseFunc(mousebutton);

    
    glutMainLoop();
    
    return 0;
}















