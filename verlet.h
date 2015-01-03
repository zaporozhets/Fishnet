#ifndef _VERLET_H_
#define _VERLET_H_

typedef union
{
    struct {
        float x;
        float y;
        float z;
    };
} vector3d_t;

typedef struct _constraint_t
{
    int verlet1, verlet2;
    float restlength;
} constraint_t;

typedef struct
{
    int         index;
    vector3d_t  verlet;
    int         hinged;
} hinge_t;

typedef struct
{
    vector3d_t      *position;
    vector3d_t      *old_position;
    vector3d_t      *acceleration;
    constraint_t    *constraint;
    hinge_t         *hinge;
    unsigned int nVerlets, nConstraints, nIterations, nHinges;

} verlet_t;

//Function prototypes

verlet_t *verlets_alloc(unsigned int n_side);
verlet_t *verlets_free(verlet_t *verlets);
void verlets_integrate(verlet_t *verlets, float delta_time);

#endif // _VERLET_H_
