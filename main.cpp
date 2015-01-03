#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#ifdef _WIN32_
	#include "GL/glut.h"
	#include <windows.h>
#elif defined(_LINUX_)
    #include <GL/glut.h>
    #include <sys/time.h>
#else
    #error "Platform not defined!"
#endif

#include "verlet.h"


/*******************************************************************************
 *
 * VARIABLES DECLARATION
 *
 ******************************************************************************/
#define SCR_SIZE (500)
#define NET_SIZE (12)

verlet_t *verlets_ptr = NULL;


double t1 = 0.0f;
float telapsed = 0.0f;


//  The number of frames
int frameCount = 0;

//  Number of frames per second
float fps = 0;

//  currentTime - previousTime is the time elapsed
//  between every call of the Idle function
int currentTime = 0, previousTime = 0;


/*******************************************************************************
 *
 *
 *
 ******************************************************************************/
double get_time()
{
#ifdef _WIN32
    SYSTEMTIME st;
    GetSystemTime(&st);
    return (double) 60.0f * st.wMinute + st.wSecond + st.wMilliseconds / 1000.0f;
#elif defined(_LINUX_)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + tv.tv_usec / 1000000.0f;
#endif
}

/*******************************************************************************
 *
 * This function is passed to glutDisplayFunc in order to display OpenGL
 * contents on the window.
 ******************************************************************************/
void func_display(void)
{
    unsigned int i, j, index;
    double etime = get_time();
    float dt = (float)(etime-t1);
    t1 = etime;

    telapsed += dt;
    if (telapsed >= 0.05f)
    {
        verlets_integrate(verlets_ptr, telapsed);
        telapsed = 0.0f;
    }

    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW) ;
    glLoadIdentity() ;
    /* Drawing fish net wires */
    glBegin(GL_QUADS);
        glColor3f(1.0, 0.0, 0.0);   /* Red */
        for (i = 0, index = 0; i < (NET_SIZE - 1); i++)
        {
            for (j = 0; j < (NET_SIZE - 1); j++)
            {
                index = i * NET_SIZE + j;
                glVertex3f(verlets_ptr->position[index].x,
                           verlets_ptr->position[index].y,
                           verlets_ptr->position[index].z);

                glVertex3f(verlets_ptr->position[index + NET_SIZE].x,
                           verlets_ptr->position[index + NET_SIZE].y,
                           verlets_ptr->position[index + NET_SIZE].z);

                glVertex3f(verlets_ptr->position[index + NET_SIZE + 1].x,
                           verlets_ptr->position[index + NET_SIZE + 1].y,
                           verlets_ptr->position[index + NET_SIZE + 1].z);

                glVertex3f(verlets_ptr->position[index + 1].x,
                           verlets_ptr->position[index + 1].y,
                           verlets_ptr->position[index + 1].z);
            }
        }
    glEnd();

    /* Drawing fish net nodes */
    glBegin(GL_POINTS);
        glColor3f(1.0, 1.0, 1.0);   /* White */
        for (i = 0; i < verlets_ptr->nVerlets; i++)
        {
            glVertex3f(verlets_ptr->position[i].x,
                       verlets_ptr->position[i].y,
                       verlets_ptr->position[i].z);
        }
    glEnd();

    glFinish();
    glFlush();
    glutSwapBuffers();
    return;
}

/*******************************************************************************
 *
 *  Calculate and print FPS
 *
 ******************************************************************************/
void func_idle(void)
{
    //  Increase frame count
    frameCount++;
    //  Get the number of milliseconds since glutInit called
    //  (or first call to glutGet(GLUT ELAPSED TIME)).
    currentTime = glutGet(GLUT_ELAPSED_TIME);

    //  Calculate time passed
    int timeInterval = currentTime - previousTime;

    if(timeInterval > 1000)
    {
        char title[40] = {0};
        //  calculate the number of frames per second
        fps = frameCount / (timeInterval / 1000.0f);

        previousTime = currentTime;
        frameCount = 0;

        sprintf(title, "Fishnet simulator. FPS: %4.2f", fps);
        glutSetWindowTitle(title);
    }
    return;
}
/*******************************************************************************
 *
 * Catch hinge
 *
 ******************************************************************************/
void func_mouse(int button, int state, int x, int y)
{
    (void)state;
    if(GLUT_LEFT_BUTTON == button)
    {
        int index = 0;
        float min_distance = sqrtf(powf((float)SCR_SIZE, 2) + powf((float)SCR_SIZE, 2));;
        for (int i = 0; i < (int)verlets_ptr->nVerlets; i++)
        {
            float distance = sqrtf(powf(verlets_ptr->position[i].x - x, 2) + powf(verlets_ptr->position[i].y - y, 2));
            if(min_distance >= distance)
            {
                index = i;
                min_distance = distance;
            }
        }
        verlets_ptr->hinge[2].index = index;
    }
    return;
}

/*******************************************************************************
 *
 * Drag hinge
 *
 ******************************************************************************/
void func_motion(int x, int y)
{
    verlets_ptr->hinge[2].hinged = 1;
    verlets_ptr->hinge[2].verlet.x = (float)x;
    verlets_ptr->hinge[2].verlet.y = (float)y;
    verlets_ptr->hinge[2].verlet.z = 1.0f;
    return;
}

/*******************************************************************************
 *
 * Release hinge
 *
 ******************************************************************************/
void func_passive_motion(int x, int y)
{
    (void) x;
    (void) y;
    verlets_ptr->hinge[2].hinged = 0;
    return;
}

/*******************************************************************************
 *
 * This function is passed to the glutReshapeFunc and is called
 * whenever the window is resized.
 ******************************************************************************/
void func_reshape(int width,int height)
{
    (void) width;
    (void) height;
    glutReshapeWindow(SCR_SIZE, SCR_SIZE);
}


/*******************************************************************************
 *
 * The main routine
 *
 *****************************************************************************/
int main(int argc, char **argv)
{
    //Create particles
    verlets_ptr = verlets_alloc(NET_SIZE);
    if(NULL == verlets_ptr)
        return EXIT_FAILURE;

    t1 = get_time();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(SCR_SIZE, SCR_SIZE);
    glutCreateWindow("Fishnet simulator.");
    glutDisplayFunc(func_display);
    glutIdleFunc(func_idle);

    glutMouseFunc(func_mouse);
    glutPassiveMotionFunc(func_passive_motion);
    glutMotionFunc(func_motion);
    glutReshapeFunc(func_reshape);


    glClearColor(0.0, 0.0, 0.0, 1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, SCR_SIZE, SCR_SIZE, 0, -(SCR_SIZE)/2, (SCR_SIZE)/2);
    glMatrixMode(GL_MODELVIEW);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    /* Enabling smoothing */
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glPointSize( 6.0 );

    glutMainLoop();

    verlets_ptr = verlets_free(verlets_ptr);

    return EXIT_SUCCESS;
}

