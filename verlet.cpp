#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "verlet.h"

/*******************************************************************************
 *
 *
 *
 ******************************************************************************/
verlet_t *verlets_alloc(unsigned int n_side)
{
    unsigned int i, j, k;
    float restlength;

    //Allocate memory for all the verlets
    verlet_t *verlets = (verlet_t *)malloc(sizeof(verlet_t));
    if(NULL == verlets)
        return verlets_free(verlets);
    memset(verlets, 0, sizeof(verlet_t));

    //Set up indices
    verlets->nVerlets = n_side * n_side;
    verlets->nConstraints = 6 * ( n_side - 1 ) * ( n_side - 1 );
    verlets->nIterations = 10;
    verlets->nHinges = 3;

    verlets->position = (vector3d_t *)malloc(verlets->nVerlets * sizeof(vector3d_t));
    verlets->old_position = (vector3d_t *)malloc(verlets->nVerlets * sizeof(vector3d_t));
    verlets->acceleration = (vector3d_t *)malloc(verlets->nVerlets * sizeof(vector3d_t));
    verlets->constraint = (constraint_t *)malloc( verlets->nConstraints * sizeof(constraint_t));
    verlets->hinge = (hinge_t *)malloc( verlets->nHinges * sizeof(hinge_t));
    if((NULL == verlets->position)||
       (NULL == verlets->old_position)||
       (NULL == verlets->acceleration)||
       (NULL == verlets->constraint)||
       (NULL == verlets->hinge))
        return verlets_free(verlets);

    restlength = 300.0f / (float)(n_side-1);

    //Initialize verlet state
    for (i = 0, k = 0; i < n_side; i++ )
    {
        for (j = 0; j < n_side; j++ )
        {
            //All verlets starts from an height of 20 m in a 10x10 grid
            verlets->position[k].x = 125.0f + restlength*(float)i;
            verlets->position[k].y = restlength*(float)j;
            verlets->position[k].z =  0;

            //No initial velocity
            verlets->old_position[k].x = verlets->position[k].x;
            verlets->old_position[k].y = verlets->position[k].y;
            verlets->old_position[k].z = verlets->position[k].z;

            //No initial acceleration
            verlets->acceleration[k].x = 0.0f;
            verlets->acceleration[k].y = 0.0f;
            verlets->acceleration[k].z = 0.0f;

            k++;
        }
    }

    //Initialize verlet geometry and constraints
    for (i = 0, k = 0; i < (n_side-1); i++ )
    {
        for (j = 0; j < (n_side-1); j++ )
        {
            unsigned int m = i * n_side + j;

            /* Initialize verlet constraints */
            verlets->constraint[k].verlet1 = m;
            verlets->constraint[k].verlet2 = m + 1;
            verlets->constraint[k].restlength = restlength;

            verlets->constraint[k+1].verlet1 = m;
            verlets->constraint[k+1].verlet2 = m + n_side;
            verlets->constraint[k+1].restlength = restlength;

            verlets->constraint[k+2].verlet1 = m;
            verlets->constraint[k+2].verlet2 = m + n_side + 1;
            verlets->constraint[k+2].restlength = (float)sqrtf(2.0f) * restlength;

            verlets->constraint[k+3].verlet1 = m + n_side;
            verlets->constraint[k+3].verlet2 = m + 1;
            verlets->constraint[k+3].restlength = (float)sqrtf(2.0f) * restlength;

            verlets->constraint[k+4].verlet1 = m + n_side;
            verlets->constraint[k+4].verlet2 = m + n_side + 1;
            verlets->constraint[k+4].restlength = restlength;

            verlets->constraint[k+5].verlet1 = m + 1;
            verlets->constraint[k+5].verlet2 = m + n_side + 1;
            verlets->constraint[k+5].restlength = restlength;

            k += 6;
        }
    }

    //Initialize verlet hinges
    verlets->hinge[0].hinged = 1;
    verlets->hinge[0].index = 0;
    verlets->hinge[0].verlet.x =  125.0f;
    verlets->hinge[0].verlet.y = 20.0f;
    verlets->hinge[0].verlet.z =   0.0f;

    verlets->hinge[1].hinged = 1;
    verlets->hinge[1].index = verlets->nVerlets - n_side;
    verlets->hinge[1].verlet.x = 375.0f;
    verlets->hinge[1].verlet.y = 20.0f;
    verlets->hinge[1].verlet.z =   0.0f;

    verlets->hinge[2].hinged = 0;
    verlets->hinge[2].index = n_side;
    verlets->hinge[2].verlet.x =   475.0f;
    verlets->hinge[2].verlet.y = 20.0f;
    verlets->hinge[2].verlet.z =   0.0f;

    return verlets;
}

/*******************************************************************************
 *
 *
 *
 ******************************************************************************/
verlet_t *verlets_free(verlet_t *verlets)
{
    if(NULL == verlets)
        return NULL;

    if(NULL != verlets->position)
        free(verlets->position);

    if(NULL != verlets->old_position)
        free(verlets->old_position);

    if(NULL != verlets->acceleration)
        free(verlets->acceleration);

    if(NULL != verlets->constraint)
        free(verlets->constraint);

     if(NULL == verlets->hinge)
        free(verlets->hinge);

    free(verlets);
    return NULL;
}

/*******************************************************************************
 *
 * Helper function for subtraction two vectors
 *
 ******************************************************************************/
void sub (const vector3d_t * v1, const vector3d_t *v2, vector3d_t *v3)
{
    v3->x = v1->x - v2->x;
    v3->y = v1->y - v2->y;
    v3->z = v1->z - v2->z;
}

/*******************************************************************************
 *
 * Helper function for calculation scalar product
 *
 ******************************************************************************/
float scalar_product(const vector3d_t * v1, const vector3d_t * v2)
{
    return ( v1->x * v2->x + v1->y * v2->y + v1->z * v2->z);
}

/*******************************************************************************
 *
 * Manage the constraints
 *
 ******************************************************************************/
void verlets_constraint(verlet_t *verlets)
{
    unsigned int i, j;
    float deltalength, diff;
    vector3d_t delta;

    for (j = 0; j < verlets->nIterations; j++ )
    {
        for (i = 0; i < verlets->nConstraints; i++ )
        {
            sub( 	&verlets->position[verlets->constraint[i].verlet2],
                &verlets->position[verlets->constraint[i].verlet1],
                &delta );

            deltalength = sqrtf(scalar_product(&delta, &delta));
            diff = ( deltalength - verlets->constraint[i].restlength) / deltalength;

            delta.x *= diff * 0.1f;
            delta.y *= diff * 0.1f;
            delta.z *= diff * 0.1f;

            verlets->position[verlets->constraint[i].verlet1].x += delta.x;
            verlets->position[verlets->constraint[i].verlet1].y += delta.y;
            verlets->position[verlets->constraint[i].verlet1].z += delta.z;

            verlets->position[verlets->constraint[i].verlet2].x -= delta.x;
            verlets->position[verlets->constraint[i].verlet2].y -= delta.y;
            verlets->position[verlets->constraint[i].verlet2].z -= delta.z;
        }

        //Constrain required particles of the cloth
        for (i = 0; i < verlets->nHinges; i++ )
        {
            if(verlets->hinge[i].hinged)
            {
                verlets->position[verlets->hinge[i].index].x = verlets->hinge[i].verlet.x;
                verlets->position[verlets->hinge[i].index].y = verlets->hinge[i].verlet.y;
                verlets->position[verlets->hinge[i].index].z = verlets->hinge[i].verlet.z;
            }
        }
    }
}

/*
 *
 */
/*******************************************************************************
 *
 * Manage the whole verlets dynamic, including constrains
 *
 ******************************************************************************/
void verlets_integrate(verlet_t *verlets, float delta_time)
{
    unsigned int i;
    vector3d_t gravity;

    /* Accumulate gravity accelerations */
    gravity.x = 0;
    gravity.y = 98.0f;
    gravity.z = 0;
    for (i = 0; i < verlets->nVerlets; i++ )
    {
        verlets->acceleration[i].x = gravity.x;
        verlets->acceleration[i].y = gravity.y;
        verlets->acceleration[i].z = gravity.z;
    }

    //Dynamic equations of motion
    for (i = 0; i < verlets->nVerlets; i++ )
    {
        vector3d_t temp;

        temp.x = verlets->position[i].x;
        temp.y = verlets->position[i].y;
        temp.z = verlets->position[i].z;

        // http://en.wikipedia.org/wiki/Verlet_integration
        verlets->position[i].x += verlets->position[i].x - verlets->old_position[i].x + verlets->acceleration[i].x * delta_time * delta_time;
        verlets->position[i].y += verlets->position[i].y - verlets->old_position[i].y + verlets->acceleration[i].y * delta_time * delta_time;
        verlets->position[i].z += verlets->position[i].z - verlets->old_position[i].z + verlets->acceleration[i].z * delta_time * delta_time;

        verlets->old_position[i].x = temp.x;
        verlets->old_position[i].y = temp.y;
        verlets->old_position[i].z = temp.z;
    }

    //Satisfy the constraints
    verlets_constraint(verlets);
}
