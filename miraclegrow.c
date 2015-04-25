/* ex: set tabstop=4:  -*- tab-width: 4; -*-
 *
 * CGV Open Examination 2000
 *
 * Candidate Number: xxxxx
 *
 * Copyright Peter Hollobon 2000-2015
 */
 
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "wood.h"


/* Define for frame-rate display */
//#define FPS

#ifdef FPS
/* Frame rate variables */
double FPS_av = 0.0;
int FPS_count = 0;
#endif

/*#define SIMPLECHUTE*/


/* Convenience macros for generating random numbers */
#define FLOAT_RAND (rand() / (float)RAND_MAX)
#define RANDOM_RANGE(lo, hi) ((lo) + (hi - lo) * FLOAT_RAND)


/* Object names for selection */
#define DOOR_NAME 1


/*
 * Geometry
 */

#define CHUTE_LENGTH 10.0
#define CHUTE_WIDTH 3.0
#define CHUTE_ANGLE 9.5

/*	The number of sections (each a "U" shape) the tube is split into
	along the X axis (for increased specular effect)
*/	
#define CHUTE_SECTIONS 8

/* Set in initialise() */
GLfloat trayDistance = 0.0;

#define TANK_WIDTH 10.0
#define TANK_HEIGHT 13.0
#define TANK_GLASS_THICKNESS 0.4

#define DOOR_BOTTOM 5.0
#define DOOR_RISE_HEIGHT 0.6
#define DOOR_RATE 0.05
#define DOOR_X TANK_WIDTH * HEXHORIZ + 0.01

#define TRAY_WIDTH 10.0
#define TRAY_HEIGHT 3.0

#define FLOORSIZE 20.0

/* sqrt(0.5**2 - 0.25**2) */
#define HEXHORIZ .4330127018

GLfloat hexPrism[][3] = {
	{HEXHORIZ * TANK_WIDTH, 	0.0,	0.25 * TANK_WIDTH},
	{0.0,		0.0,	0.50 * TANK_WIDTH},
	{-HEXHORIZ * TANK_WIDTH,	0.0,	0.25 * TANK_WIDTH},
	{-HEXHORIZ * TANK_WIDTH,	0.0,	-0.25 * TANK_WIDTH},
	{0.0,		0.0,	-0.5 * TANK_WIDTH},
	{HEXHORIZ * TANK_WIDTH,	0.0,	-0.25 * TANK_WIDTH},
	{HEXHORIZ * TANK_WIDTH, 	TANK_HEIGHT,	0.25 * TANK_WIDTH},
	{0.0,		TANK_HEIGHT,	0.5 * TANK_WIDTH},
	{-HEXHORIZ * TANK_WIDTH,	TANK_HEIGHT,	0.25 * TANK_WIDTH},
	{-HEXHORIZ * TANK_WIDTH,	TANK_HEIGHT,	-0.25 * TANK_WIDTH},
	{0.0,		TANK_HEIGHT,	-0.5 * TANK_WIDTH},
	{HEXHORIZ * TANK_WIDTH,	TANK_HEIGHT,	-0.25 * TANK_WIDTH}
};	

int hexagonFacets[] = {
	5,4,3,2,1,0,-1,
	0,1,7,6,-1,
	1,2,8,7,-1,
	2,3,9,8,-1,
	3,4,10,9,-1,
	4,5,11,10,-1,
	5,0,6,11,-1,
	6,7,8,9,10,11,-1
};

int hexFacets[][4] = {
	{0,1,7,6},
	{1,2,8,7},
	{2,3,9,8},
	{3,4,10,9},
	{4,5,11,10},
	{5,0,6,11}
};

GLfloat hexMidPoints[6][3];

#define FRONT_FACING 1
#define REAR_FACING 2

int hexOrientation[6];

GLfloat hexPrismNormals[8][3];


/*
 * Display Lists ****************************************************
 */

/* Base of display list */
GLuint listbase;

/* Display list identifiers */
#define LIST_LEAF 0
#define LIST_CHUTE 1
#define LIST_TRAY 2
#define LIST_MESH 3
#define LIST_FLOOR 4
#define LIST_DOOR 5


/*
 * Particle System (flowing liquid) **********************************
 */

#define GRAVITY -0.051

#define MAX_PARTICLES 10000

/* Structure containing the state of a single particle */
struct particle {
	int onEarth;			/* Particle is lying on the earth */
	GLfloat location[3];	/* 3D coordinates of the particle */
	GLfloat velocity[3];	/* Velocity of particle */
	GLfloat time;			/* Time particle has been "alive" (for gravity) */
};

/* Array of particle structures */
struct particle particles[MAX_PARTICLES];

/* Flag indicating whether particle system for liquid is active */
int particlesActive = 0;

/* Number of particles currently "alive" */
int particleCount = 0;


/*
 * Liquid within tank ************************************************
 */

int liquidFlowing = 0;

/*	Rate at which liquid is moving downwards in tank; slows when top of
	liquid is below top of aperture in tank
*/
GLfloat liquidRate = 0.15;

/* Current depth of liquid within tank */
GLfloat depth = TANK_HEIGHT - DOOR_BOTTOM - 0.2;


/*
GLfloat tanktest[][4] = {
	{1.0, 0.0, 0.0, 1.3},
	{0.0, 1.0, 0.0, 1.3},
	{0.0, 0.0, 1.0, 1.3},
	{1.0, 1.0, 0.0, 1.3},
	{0.0, 1.0, 1.0, 1.3},
	{1.0, 0.0, 1.0, 1.3}
};	
*/


/*
 * Material definitions **********************************************
 */

GLfloat tankADcol[] = {0.2, 0.2, 0.2, 0.3};
//GLfloat tankADcol[] = {0.7, 0.7, 0.7, 1.0};
//GLfloat tankSPcolour[] = {1.0, 1.0, 1.0, 1.0};
GLfloat tankSPcol[] = {0.3, 0.3, 0.3, 0.4};
GLfloat tankShine[] = {0.0};

GLfloat liquidADcol[] = {0.2, 0.2, 0.9, 0.7};
GLfloat liquidSPcol[] = {0.2, 0.2, 0.4, 0.7};
GLfloat liquidShine[] = {10.0};

GLfloat chuteADcol[] = {0.3, 0.3, 0.3, 1.0};
GLfloat chuteSPcol[] = {1.0, 1.0, 1.0, 1.0};
GLfloat chuteShine[] = {50.0};

GLfloat floorADcol[] = {0.05, 0.3, 0.05, 1.0};
//GLfloat floorSPcol[] = {0.9, 0.9, 0.9, 1.0};
GLfloat floorSPcol[] = {0.0, 0.0, 0.0, 1.0};
GLfloat floorShine[] = {0.0};

GLfloat trayADcol[] = {0.3, 0.3, 0.3, 1.0};
GLfloat traySPcol[] = {0.6, 0.6, 0.6, 1.0};
GLfloat trayShine[] = {0.0};

GLfloat doorADcol[] = {0.15, 0.15, 0.2, 1.0};
GLfloat doorSPcol[] = {0.5, 0.5, 0.55, 1.0};
GLfloat doorShine[] = {50.0};

GLfloat earthADcol[] = {0.24, 0.18, 0.07, 1.0};
GLfloat earthSPcol[] = {0.1, 0.1, 0.05, 1.0};
GLfloat earthShine[] = {3.0};

GLfloat plantADcol[] = {0.24, 0.58, 0.07, 1.0};
GLfloat plantSPcol[] = {0.2, 0.7, 0.05, 1.0};
GLfloat plantShine[] = {0.0};


/*
 * Triangular Mesh (model of earth) **********************************
 */

#define MESH_X 20
#define MESH_Y 20

/* All vertices in the mesh */
GLfloat triVertices[MESH_X][MESH_Y][3];

/* Normals for each triangle */
GLfloat triNormals[2*MESH_X][MESH_Y][3];


/* Window parameters */
GLint width=300, height=300;


int doorActive = 0;
GLfloat doorOffset = 0.0;


/*
 * Viewing parameters ************************************************
 */

/*	Theta and phi control the point on the sphere from which the scene
	is viewed.  They are controlled with keys 1-4 */
double theta = 0.37;
double phi = 1.07;

/*	Amount theta / phi are incremented / decremented when the viewpoint
	is moved */
#define ROTATE_STEP M_PI / 60

/* The distance from the viewer to the point being looked at */
double dist = 40.0;

/* Point being looked at: set to just above the middle of the chute */
#define LOOKAT_X ((trayDistance + HEXHORIZ*TANK_WIDTH) / 2.0)
#define LOOKAT_Y (DOOR_BOTTOM + 2.0)
#define LOOKAT_Z 0.0


/*
 * Plant variables ***************************************************
 */

#define PLANT_INITIAL_RULE "FF[RF][LF]"
#define TREE_F_RULE	"F[RF][LF]F[LF][RF]"


/* The maximum number of branches that may be growing at the same time */
#define MAX_CONCURRENT_GROWS 5

/* The percentage an active branch increases in length with every frame */
#define BRANCH_INCREMENT 10.0

/*	The maximum number of 'F' operations permitted in the plant 
	description.  This limits the size of the plant
*/	
#define MAX_PLANT_SIZE 30

/*	The description for the plant.  This consists of a simple language based
	model: 'F' is forward, 'R' is rotate right, 'L' is rotate left.  '[' pushes
	the current position and scales all branch lengths down by 1/3.  ']'
	pops the topmost position from the stack and makes branch lengths longer
	correspondingly
*/	
char *plantStructure;


/*	The current position within the plant (used to determine which branches
	are growing.  The drawPlant() function will draw the plant until it
	reaches the "plantPosition"th 'F', at which point it will stop.
	Therefore incrementing plantPosition over time will cause the plant to
	"grow" as more and more of the plant description is interpreted
*/
int plantPosition = 0;

/*	The maximum value for plantPosition. This is either the end of the plant
	description, or a predefined maximum size, whichever is the smaller
*/	
int maxPlantPosition = 0;

float branchPercentage = 0.0;

/* Flag indicating whether the plant is active (growing) or not */
int plantActive = 0;

/*	Azimuth range determines the amount that a right- or left-facing
	branch can be randomly rotated from true left or right
*/
#define AZIMUTH_RANGE 25.0

/*	Each left and right operation has associated with it an elevation
	angle, which is randomly set.  Forward operations also have a 
	much smaller random elevation angle.  This array stores these
	angles.  
*/	
GLfloat plantElevations[1000];

/*	The azimuth array is similar to the elevation one: each L and R
	operation has a randomly set azimuth (specified as an offset from
	true left or right).  Forward operations also have an azimuth
*/	
GLfloat plantAzimuths[1000];


/*
 * Light definitions *************************************************
 */

#define LIGHT_COUNT 3

/*	Array of OpenGL light identifiers.  For each light in this
	array, there must be corresponding entries in the light component
	and position arrays
*/
GLenum light[] = {GL_LIGHT0, GL_LIGHT1, GL_LIGHT2};

/* Ambient light component for each light */
GLfloat lightAmbient[][4] = {
	{0.3, 0.3, 0.3, 1.0},
	{0.5, 0.5, 0.5, 1.0},
	{0.3, 0.3, 0.3, 1.0} 
};


/* Diffuse light component for each light */
GLfloat lightDiffuse[][4]  = {
	{0.9, 0.9, 0.9, 1.0},
	{0.0, 0.0, 0.0, 1.0}, 
	{0.0, 0.0, 0.0, 1.0} 
};

/* Specular light component for each light */
GLfloat lightSpecular[][4] = {
	{0.1, 0.1, 0.1, 1.0},
	{1.0, 1.0, 1.0, 1.0},
	{1.0, 1.0, 1.0, 1.0}
};

/* Position for each light */
GLfloat lightPosition[][4] = {
	{-10.0,  5.0, 6.0, 1.0},
	{ 4.0,  2.0, 8.0, 1.0},
	{ 6.0,  9.0, -8.0, 1.0}
};

/*	For set-up
	Order is dist, theta, phi */
GLfloat lightSet[][3] = {
	{7.0, 0.0, 0.0},
	{7.0, 1.0, 1.0},
	{7.0, 2.0, 2.0}
};

int curLight = 0;


/*
 * Function prototypes
 */
void getTriangle(int x, int y, GLfloat vertices[][3]);
void setLightPositions(void);
void setLighting(void);
void updateLookAt(void);
void dispVertex(GLfloat vertex[]);
void calcTankFaces(void);
int getLiquidTopFacing(void);


/**
 * Normalize a vector by dividing by its length.
 * 
 * (from /usr/course/3/cgv/solns/mill.c)
 */
void normalize(GLfloat p[]) {
	float d;
	int i;
    
	d = sqrt(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);

    if (d > 0.0) 
        for(i=0; i<3; i++) p[i]/=d;
}


/** 
 * Calculate the cross product of the two vectors v1 and v2.  Result
 * is returned in the 3rd argument.
 * 
 * (from /usr/course/3/cgv/solns/mill.c)
 */
void crossProduct(GLfloat v1[3], GLfloat v2[3], GLfloat ans[3]) {
    ans[0] = v1[1]*v2[2] - v1[2]*v2[1];
    ans[1] = v1[2]*v2[0] - v1[0]*v2[2];
    ans[2] = v1[0]*v2[1] - v1[1]*v2[0];
}


/*
 * Calculate the dot product of the two vectors v and w
 */
GLfloat dotProduct(GLfloat v[], GLfloat w[]) {
	int i;
	GLfloat result = 0.0;

	for (i=0; i<3; i++)
		result += v[i] * w[i];

	return result;
}


/** 
 * Compute the normals for a surface or polyhedron described by a 
 * vertex array and a facet array indexing into the vertex array.
 * The normals are stored in a third array, which must have the
 * same number of rows as there are facets in the model.
 *
 * (from /usr/course/3/cgv/solns/mill.c)
 */
void makeNormals( 
            GLfloat vs[][3],        /* vertex array */
            int fs[],               /* facet list */
            int nrFacets,           /* number of facets */
            GLfloat ns[][3] ) {     /* array to hold normals */

	int f, i, c;
	GLfloat vec1[3], vec2[3];       /* temporary vectors */
    
	i = 0;                          /* index into the facet list */

	/* Find the normal by each facet */
	for (f = 0; f < nrFacets; f++) {
		/*	Take the first three points in the current facet and use
			them to define two vectors: one from point 2 to point 3,
			one from point 2 to point 1. 
		*/
		for (c = 0; c < 3; c++) {
			vec1[c] = vs[ fs[i+2] ][c] - vs[ fs[i+1] ][c];
			vec2[c] = vs[ fs[i]   ][c] - vs[ fs[i+1] ][c];
		}

		/*	The cross product of these vectors gives a vector in the
			direction of the normal; to get the normal itself, we need
			to normalize the resulting vector.
		*/
		crossProduct(vec1, vec2, ns[f]);
		normalize( ns[f] );

		/* Find the end of the current facet in the facet list */
		while (fs[i] >= 0) i++;

		/* Move to the first vertex of the next facet */
		i++;
    }
}


/**
 * When calculating which faces of the two hexagonal volumes (tank and
 * liquid), a location on each hexagon is required to calculate the 
 * direction of projection vector from the viewpoint.  The midpoint of
 * each facet of the hexagon is used for this purpose.
 *
 * This function finds the midpoints of the six facets and stores them
 * in an array.
 */
void genMidPoints(void) {
	int i;

	for (i=0; i<6; i++) {
		hexMidPoints[i][0] = 
			(hexPrism[hexFacets[i][0]][0] +
			hexPrism[hexFacets[i][1]][0]) / 2.0;
		hexMidPoints[i][1] = 
			(hexPrism[hexFacets[i][1]][1] +
			hexPrism[hexFacets[i][2]][1]) / 2.0;
		hexMidPoints[i][2] = 
			(hexPrism[hexFacets[i][0]][2] +
			hexPrism[hexFacets[i][1]][2]) / 2.0;

		dispVertex(hexMidPoints[i]);
	}
}


/** 
 * Draw the tank and liquid.  As the tank and liquid are translucent, 
 * blending is used, which necessitates changing the order in which
 * the polygons are drawn depending on the location of the viewer
 * (and, in the case of the liquid, the level of the liquid in the 
 * tank).
 *
 * This cannot be compiled into a display list because of its dynamic
 * nature
 */
void tankAndLiquid(void) {
	int f, i;
	int liquidTopFacing;

	/*	Determine whether the top of the liquid is front or rear facing
	 	This must be done more regularly than determining the orientation
	 	of the other translucent polygons because the top of the liquid
		moves downwards as the liquid flows out of the tank
	*/
	liquidTopFacing = getLiquidTopFacing();

	glPushMatrix();

	/* Move up slightly so we don't intersect the floor */
	glTranslatef(0.0, 0.01, 0.0);

	/*	The tank and liquid are translucent.  Blending is used to
		alter the colour of the two objects, dependent on their alpha
		value.  The depth buffer is made read only whilst drawing these
		translucent objects to ensure that all faces are rendered, 
		including those that would be obscured if polygons in front of
		them were opaque.
	*/
    glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	/* Draw the rear-facing tank polygons first */
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, tankADcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tankSPcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, tankShine);
	for (f=0; f<6; f++) {
		if (hexOrientation[f] == REAR_FACING) {
			glBegin(GL_POLYGON);
				glNormal3fv(hexPrismNormals[f+1]);

				for(i=0; i<4; i++)
					glVertex3fv(hexPrism[hexFacets[f][i]]);
			glEnd();
		}	
	}


	/*	Now the rear facing liquid polygons, which will be "in front"
		of the rear tank polygons
	*/
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, liquidADcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, liquidSPcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, liquidShine);

	/*	The liquid object is just a scaled version of the tank, with
		different material properties and a top.

		The current transformation matrix is pushed so it can be restored
		to draw the front-facing polygons of the tank after drawing the
		liquid
	*/	
	glPushMatrix();

	/*	Scale the liquid in the X and Z directions to accommodate the 
		thickness of the glass tank.  Scale in the Y direction to
		reflect the quantity of liquid within the tank
	*/
	glScalef(
		(TANK_WIDTH - TANK_GLASS_THICKNESS * 2.0) / TANK_WIDTH,
		(depth + DOOR_BOTTOM) / TANK_HEIGHT,
		(TANK_WIDTH - TANK_GLASS_THICKNESS * 2.0) / TANK_WIDTH);

	/* Draw rear facing liquid polygons */
	for (f=0; f<6; f++) {
		if (hexOrientation[f] == REAR_FACING) {
			glBegin(GL_POLYGON);
				glNormal3fv(hexPrismNormals[f+1]);

				for(i=0; i<4; i++)
					glVertex3fv(hexPrism[hexFacets[f][i]]);
			glEnd();
		}	
	}

	/*	If the top of the liquid is rear facing (viewpoint is
		underneath it), it must be drawn before the front facing
		liquid polygons...
	*/	
	if (liquidTopFacing == REAR_FACING) {
		glBegin(GL_POLYGON);	
			glNormal3fv(hexPrismNormals[7]);
			for (f=6; f<12; f++)
				glVertex3fv(hexPrism[f]);
		glEnd();
	}

	/*	Now the front facing liquid polygons */
	for (f=0; f<6; f++) {
		if (hexOrientation[f] == FRONT_FACING) {
			glBegin(GL_POLYGON);
				glNormal3fv(hexPrismNormals[f+1]);

				for(i=0; i<4; i++)
					glVertex3fv(hexPrism[hexFacets[f][i]]);
			glEnd();
		}	
	}

	/*	...but if the top of the liquid is front facing, it must
		be drawn after the front facing liquid polygons
	*/	
	if (liquidTopFacing == FRONT_FACING) {
		glBegin(GL_POLYGON);	
			glNormal3fv(hexPrismNormals[7]);
			for (f=6; f<12; f++)
				glVertex3fv(hexPrism[f]);
		glEnd();
	}

	/* Pop modelview matrix to get rid of scaling for liquid */
	glPopMatrix();
	
	/*	And finally the front facing tank polygons, which will 
		always be the nearest translucent polygons
	*/	
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, tankADcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tankSPcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, tankShine);
	for (f=0; f<6; f++) {
		if (hexOrientation[f] == FRONT_FACING) {
			glBegin(GL_POLYGON);
				glNormal3fv(hexPrismNormals[f+1]);

				for(i=0; i<4; i++)
					glVertex3fv(hexPrism[hexFacets[f][i]]);
			glEnd();
		}	
	}

	/*	Disable blending and reenable writing to the depth buffer so
		hidden surface removal will take place (which is desirable for
		opaque polygons)
	*/	
    glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	glPopMatrix();
}
    

/**
 * This function draws the chute that the liquid flows down to move from
 * the tank to the growing tray.  There are two ways in which the chute
 * can be drawn: one draws a simple chute with three polygons -- one for
 * the bottom and one for each side.  The other way draws a geometrically
 * identical chute comprised of several polygons.  This version gives a 
 * better specular highlight, which is desirable because this chute is
 * supposed to be made from a metallic material.
 */ 
void chute(void) {
#ifndef SIMPLECHUTE
	int i;
	GLfloat incr = CHUTE_LENGTH / (float)CHUTE_SECTIONS;
#endif	

	glPushMatrix();
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, chuteADcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, chuteSPcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, chuteShine);

#ifdef SIMPLECHUTE
	glBegin(GL_POLYGON);
		glNormal3f(0.0, 0.0, -1.0);

		glVertex3f(0.0, 1.0, 0.0);
		glVertex3f(CHUTE_LENGTH, 1.0, 0.0);
		glVertex3f(CHUTE_LENGTH, 0.0, 0.0);
		glVertex3f(0.0, 0.0, 0.0);
	glEnd();	

	glBegin(GL_POLYGON);
		glNormal3f(0.0, 1.0, 0.0);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(0.0, 0.0, CHUTE_WIDTH);
		glVertex3f(CHUTE_LENGTH, 0.0, CHUTE_WIDTH);
		glVertex3f(CHUTE_LENGTH, 0.0, 0.0);
	glEnd();	

	glBegin(GL_POLYGON);
		glNormal3f(0.0, 0.0, 1.0);
		glVertex3f(0.0, 0.0, CHUTE_WIDTH);
		glVertex3f(CHUTE_LENGTH, 0.0, CHUTE_WIDTH);
		glVertex3f(CHUTE_LENGTH, 1.0, CHUTE_WIDTH);
		glVertex3f(0.0, 1.0, CHUTE_WIDTH);
	glEnd();	

#else
	/*	The chute is drawn as a series of smaller "U" shaped polygons.
	 	This is because it is a metallic material, and specular effects
	 	are more effective on many smaller polygons rather than fewer
	 	larger ones.  There is obviously a performance comprimise here,
	 	but is not significant.
	 */
	for (i = 0; i < CHUTE_SECTIONS; i++) {
		/* Far side of chute */
		glBegin(GL_POLYGON);
			glNormal3f(0.0, 0.0, 1.0);

			glVertex3f(i * incr, 1.0, 0.0);
			glVertex3f(i * incr, 0.0, 0.0);
			glVertex3f((i+1) * incr, 0.0, 0.0);
			glVertex3f((i+1) * incr, 1.0, 0.0);
		glEnd();	

		/* Bottom of chute */
		glBegin(GL_POLYGON);
			glNormal3f(0.0, 1.0, 0.0);

			glVertex3f(i * incr, 0.0, 0.0);
			glVertex3f(i * incr, 0.0, CHUTE_WIDTH);
			glVertex3f((i+1) * incr, 0.0, CHUTE_WIDTH);
			glVertex3f((i+1) * incr, 0.0, 0.0);
		glEnd();	

		/* Near side of chute */
		glBegin(GL_POLYGON);
			glNormal3f(0.0, 0.0, 1.0);

			glVertex3f(i * incr, 0.0, CHUTE_WIDTH);
			glVertex3f((i+1) * incr, 0.0, CHUTE_WIDTH);
			glVertex3f((i+1) * incr, 1.0, CHUTE_WIDTH);
			glVertex3f(i * incr, 1.0, CHUTE_WIDTH);
		glEnd();	
	}
#endif

	glPopMatrix();
}


/**
 * This function draws the growing tray.  This is just a cube with the
 * top missing -- it is scaled to the correct size when the display
 * list for it is created.
 * 
 * The sides are texture mapped with a wood-like texture.
 */ 
void growingTray(void) {
	glPushMatrix();
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, trayADcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, traySPcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, trayShine);

	/* base */
	glBegin(GL_POLYGON);
		glNormal3f(0.0, 1.0, 0.0);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(1.0, 0.0, 0.0);
		glVertex3f(1.0, 0.0, 1.0);
		glVertex3f(0.0, 0.0, 1.0);
	glEnd();	

	/* Enable texture mapping -- only for the sides of the tray */
	glEnable(GL_TEXTURE_2D);

	/* front */
	glBegin(GL_POLYGON);
		glNormal3f(0.0, 0.0, 1.0);

		glTexCoord2f(0.0, 0.0);
		glVertex3f(0.0, 0.0, 0.0);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(0.0, 1.0, 0.0);
		glTexCoord2f(2.0, 1.0);
		glVertex3f(1.0, 1.0, 0.0);
		glTexCoord2f(2.0, 0.0);
		glVertex3f(1.0, 0.0, 0.0);
	glEnd();	

	/* back */
	glBegin(GL_POLYGON);
		glNormal3f(0.0, 0.0, -1.0);

		glTexCoord2f(0.0, 0.0);
		glVertex3f(0.0, 0.0, 1.0);
		glTexCoord2f(2.0, 0.0);
		glVertex3f(1.0, 0.0, 1.0);
		glTexCoord2f(2.0, 1.0);
		glVertex3f(1.0, 1.0, 1.0);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(0.0, 1.0, 1.0);
	glEnd();	

	/* left side */
	glBegin(GL_POLYGON);
		glNormal3f(-1.0, 0.0, 0.0);

		glTexCoord2f(0.0, 0.0);
		glVertex3f(0.0, 0.0, 0.0);
		glTexCoord2f(2.0, 0.0);
		glVertex3f(0.0, 0.0, 1.0);
		glTexCoord2f(2.0, 1.0);
		glVertex3f(0.0, 1.0, 1.0);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(0.0, 1.0, 0.0);
	glEnd();	

	/* right side */
	glBegin(GL_POLYGON);
		glNormal3f(1.0, 0.0, 0.0);

		glTexCoord2f(0.0, 0.0);
		glVertex3f(1.0, 0.0, 0.0);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(1.0, 1.0, 0.0);
		glTexCoord2f(2.0, 1.0);
		glVertex3f(1.0, 1.0, 1.0);
		glTexCoord2f(2.0, 0.0);
		glVertex3f(1.0, 0.0, 1.0);
	glEnd();	

	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
}


/**
 * Draw the floor.  This is just a quadrilateral that lies underneath
 * the other objects
 */
void bfloor(void) {
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, floorADcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, floorSPcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, floorShine);

	glTranslatef(10.0, 0.0, 0.0);
	glScalef(1.0, 1.0, 0.5);
	
	glBegin(GL_QUADS);
		glNormal3f(0.0, 1.0, 0.0);
		glVertex3f(-FLOORSIZE, 0.0, -FLOORSIZE);
		glVertex3f(-FLOORSIZE, 0.0, FLOORSIZE);
		glVertex3f(FLOORSIZE, 0.0, FLOORSIZE);
		glVertex3f(FLOORSIZE, 0.0, -FLOORSIZE);
	glEnd();	
}	


/** 
 * 
 */
void genLSAngles(char *data) {
	int pos = 0;
	int angle = 0;

	while (data[pos]) {
		if (data[pos] == 'R') {	
			plantElevations[angle] = RANDOM_RANGE(10.0, 43.0);
			plantAzimuths[angle] =
				RANDOM_RANGE(-AZIMUTH_RANGE, AZIMUTH_RANGE);
			angle++;
		} else if (data[pos] == 'L') {	
			plantElevations[angle] = RANDOM_RANGE(10.0, 43.0);
			plantAzimuths[angle] =
				RANDOM_RANGE(180.0-AZIMUTH_RANGE, 180.0+AZIMUTH_RANGE);
			angle++;
		} else if (data[pos] == 'F') {
			plantElevations[angle] = RANDOM_RANGE(0.0, 20.0);
			plantAzimuths[angle] = RANDOM_RANGE(0.0, 360.0);
			angle++;
			maxPlantPosition++;
		}	
	
		pos++;
	}

	if (maxPlantPosition > MAX_PLANT_SIZE)
		maxPlantPosition = MAX_PLANT_SIZE;

//	printf("Generated %d angles\n" ,angle);
}

/**
 *
 */
char *parseLSystem(char *data) {
	char fsub[] = TREE_F_RULE;
	char *out;
	int opos = 0;
	int ipos = 0;

	/*	Allocate the maximum possible amount of memory that could be used
		if every character in the input is an F and every single one gets
		rewritten
	*/	
	out = (char *)malloc(strlen(data) * strlen(fsub));

	while ((data[ipos]) && (opos < strlen(data) * strlen(fsub))) {
		/*	If the character is an F, perform the substitution with
			a 0.75 probability
		*/
		if ((data[ipos] == 'F') && ((random() % 4) != 3)) {
			/* Concatenate the substitution to the output string */
			strcat(out, fsub);
			
			/* Update output string position accordingly */
			opos += strlen(fsub);

			/* Increment input string position */
			ipos++;
		} else 
			/* Copy the character, incrementing input and output positions  */
			out[opos++] = data[ipos++];	
	}

	/* Null terminate the created string */
	out[opos] = 0;

	
	return out;
}


void drawPlant(char *data) {
	int pos = 0;		/* Position in the plant description array */
	int depth = 1;		/* Recursive rule application depth ('[', ']') */
	int angles = 0;		/* Offset into angles (elev. & azim.) arrays */
	int curpos = 0;		/* Number of the "F" we're currently at */
	float multiplier = 1.0;		/* Scale multiplier for the current branch */
	int i;

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, plantADcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, plantSPcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, plantShine);

	glPushMatrix();
	while ((data[pos]) && (curpos <= plantPosition)) {
		if ((data[pos] == 'R') || (data[pos] == 'L')) {
			glRotatef(plantAzimuths[angles], 0.0, 1.0, 0.0);
			glRotatef(plantElevations[angles], 0.0, 0.0, 1.0);
			angles++;
		}
		else if (data[pos] == '[') {
			glPushMatrix();
			depth++;
		}
		else if (data[pos] == ']') {
			if (curpos > plantPosition - MAX_CONCURRENT_GROWS) {
				glPushMatrix();
				glScalef(
					branchPercentage/100.0,
					branchPercentage/100.0,
					branchPercentage/100.0
				); 
				glCallList(listbase + LIST_LEAF);
				glPopMatrix();
			} else 	
				glCallList(listbase + LIST_LEAF);

			glPopMatrix();
			depth--;
		}	
		else if (data[pos] == 'F') {
			curpos++;
			if (curpos > plantPosition - MAX_CONCURRENT_GROWS)
				multiplier = branchPercentage / 100.0;
			else 
				multiplier = 1.0;

			glRotatef(plantAzimuths[angles], 0.0, 1.0, 0.0);
			glRotatef(plantElevations[angles], 0.0, 0.0, 1.0);
			angles++;

			glLineWidth(1.0);
			
			/*	Draw a line for the branch which is reduced by a third
				in length with every sub-branch
			*/	
			glBegin(GL_LINES);
				glVertex3f(0.0, 0.0, 0.0);
				glVertex3f(
					0.0,
					2.0 * pow((2.0/3.0), (double)depth) * multiplier,
					0.0);
			glEnd();
				
			glTranslatef(
				0.0,
				2.0 * pow((2.0/3.0), (double)depth) * multiplier,
				0.0);
		}	
		
		pos++;
	}

	/*	If we're part way through drawing the tree, it is likely that we
		will stop drawing before an equal number of '['s and ']'s have
		been interpreted.  As each '[' results in a glPushMatrix(), it's
		important to make sure the matrix stack is in an identical state
		as when the function was started when we leave.  We can use
		the "depth" variable to perform the correct number of glPopMatrix()
		calls here.
	*/	
	for (i = 1; i < depth; i++)
		glPopMatrix();

	glPopMatrix();
}


/**
 * Draw the door for the tank.  This consists of a triangle and a line
 * above it, reaching to the top of the tank.  When the door is opened,
 * this line protrudes above the tank.
 */
void door(void) {
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, doorADcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, doorSPcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, doorShine);

	glBegin(GL_TRIANGLES);
		glNormal3f(1.0, 0.0, 0.0);

		glVertex3f(0.0, 0.0, -CHUTE_WIDTH / 2.0);
		glVertex3f(0.0, 4.0, 0.0);
		glVertex3f(0.0, 0.0, CHUTE_WIDTH / 2.0);
	glEnd();

	glBegin(GL_POLYGON);
		glNormal3f(1.0, 0.0, 0.0);

		glVertex3f(0.0, 4.0, 0.1);
		glVertex3f(0.0, 4.0, -0.1);
		glVertex3f(0.0, TANK_HEIGHT - DOOR_BOTTOM, -0.1);
		glVertex3f(0.0, TANK_HEIGHT - DOOR_BOTTOM, 0.1);
	glEnd();	
}


/**
 * Draw all active particles in the system
 */
void drawParticles(void) {
	int i;
	
	/* Particles are translucent, so enabled blending */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* Use the same material properties as for liquid in the tank */
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, liquidADcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, liquidSPcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, liquidShine);

	/* Iterate through array of particles, drawing as points */
	glBegin(GL_POINTS);
		for (i = 0; i < particleCount; i++) {
			glVertex3fv(particles[i].location);
		}	
	glEnd();

	glDisable(GL_BLEND);
}


/**
 * Initialise a single particle
 */
void initParticle(int i) {
	float speed = 0.4 +
		RANDOM_RANGE(depth*depth / 70.0, depth*depth / 50.0);

	/* 	Initial particle location is anywhere within the aperture on
		the side of the tank
	*/
	particles[i].location[0] = DOOR_X;

	particles[i].location[1] =
		RANDOM_RANGE(DOOR_BOTTOM + 0.1, DOOR_BOTTOM + doorOffset);

	particles[i].location[2] =
		RANDOM_RANGE(-CHUTE_WIDTH/2 + 0.2, CHUTE_WIDTH/2 - 0.2);

	/*	Velocity is set to the angle of the chute in X and Y directions,
		and a small random component for Z, which will mean not all
		particles move in a straight line (they bounce off the side of the
		chute when they reach the edge; see updateParticles
	*/
	particles[i].velocity[0] = speed * cos(CHUTE_ANGLE * (M_PI / 180));
	particles[i].velocity[1] = -speed * sin(CHUTE_ANGLE * (M_PI / 180));
	particles[i].velocity[2] = RANDOM_RANGE(-0.2, 0.2);

	particles[i].onEarth = 0;

	particles[i].time = 1.0;
}


/**
 * Initialise a range of particles.  Automatically restricts number of particles
 * in system.
 */
void initParticles(int start, int length) {
	int i;

	/*	If more particles are being requested than are available, limit
		number generated to fit */
	if (start + length > (MAX_PARTICLES-1))
		length = MAX_PARTICLES - start - 1;

	/* Initialise each particle */
	for (i = start; i < start + length; i++)
		initParticle(i);
	
	particleCount += length;
}


/**
 * Update all active particles in the system.  Alters position according to
 * velocity, updates velocity according to gravity, friction, edges of chute
 * etc., and controls dispersion of particles once they hit the earth
 */
void updateParticles(void) {
	int i;
	float xchuteend;

	xchuteend = 
		cos(CHUTE_ANGLE * (M_PI/180)) * CHUTE_LENGTH + TANK_WIDTH/2;

	/* Add new particles, quantity corresponding to pressure at opening */
	if (liquidFlowing)
		initParticles(particleCount, (int)depth*12);

	/* Alter position and velocity of particles */
	for (i = 0; i < particleCount; i++) {

		/*	If the particle has not yet fallen onto the earth, apply
			various forces and constraints to make it move down the
			chute nicely and fall off the end. */
		if (particles[i].onEarth == 0) {

			/*	Check if particle has hit the earth.  This is a bit of an 
				approximation; it would be more work to find out exactly
				where the surface of the earth is.
			*/
			if ((particles[i].location[1] < (TRAY_HEIGHT / 2.0) * 1.5)) {
				particles[i].onEarth = 1;
				particles[i].velocity[0] = RANDOM_RANGE(-0.6, 0.6);
				particles[i].velocity[1] = -0.02;
				particles[i].velocity[2] = RANDOM_RANGE(-0.6, 0.6);

				/* Initiate plant growth when firts drop hits soil */
				if (plantActive == 0) {
					plantActive = 1;
				}
			}

			/*	Apply gravity to Y part of velocity once a particle has 
				moved off the end of chute
			*/
			else if (particles[i].location[0] > xchuteend) {

				/* Standard v = ut + (at^2)/2 equation */
				particles[i].velocity[1] = 
					particles[i].velocity[1] * particles[i].time
					+ (GRAVITY * (particles[i].time*particles[i].time)) / 2;

				/*	Liquid tends to "stick together" as it falls off
					a chute like this, so the Z velocity is altered
					according to the particle's location to make the
					particles do this.
				*/
				particles[i].velocity[2] =
					-particles[i].location[2] * (0.5/CHUTE_WIDTH);
			} else {

				/*	If a particle gets near the side of the chute, then
					bounce it off by changing the sign of the velocity
					in the Z direction and reducing the velocity somewhat
				*/
				if (particles[i].location[2] >= CHUTE_WIDTH/2 - 0.2) {
					particles[i].location[2] = CHUTE_WIDTH/2 - 0.2;
					particles[i].velocity[2] = -particles[i].velocity[2] * 0.7;
				} else if (particles[i].location[2] <= -CHUTE_WIDTH/2 + 0.2) {
					particles[i].location[2] = -CHUTE_WIDTH/2 + 0.2;
					particles[i].velocity[2] = -particles[i].velocity[2] * 0.7;
				}	
			}
		} else {
			/* Bounce particles off the front and back of the tray */
			if (particles[i].location[0] > TRAY_WIDTH + trayDistance - 0.8) {
				particles[i].location[0] = TRAY_WIDTH + trayDistance - 0.8;
				particles[i].velocity[0] = -particles[i].velocity[0] * 0.85;
			}	
			else if (particles[i].location[0] < trayDistance + 0.8) {
				particles[i].location[0] = trayDistance + 0.8;
				particles[i].velocity[0] = -particles[i].velocity[0] * 0.85;
			}

			/* Bounce particles off sides of the tray */
			if (particles[i].location[2] > (TRAY_WIDTH/2.0) - 0.8) {
				particles[i].location[2] = (TRAY_WIDTH/2.0) - 0.8;
				particles[i].velocity[2] = -particles[i].velocity[2] * 0.85;
			}
			else if (particles[i].location[2] < (-TRAY_WIDTH/2.0) + 0.8) {
				particles[i].location[2] = (-TRAY_WIDTH/2.0) + 0.8;
				particles[i].velocity[2] = -particles[i].velocity[2] * 0.85;
			}	
		}

		/* Alter position according to velocity */
		particles[i].location[0] += particles[i].velocity[0];
		particles[i].location[1] += particles[i].velocity[1];
		particles[i].location[2] += particles[i].velocity[2];

		/* Slow particles down (friction) */
/*		particles[i].velocity[0] *= 0.995;
		particles[i].velocity[1] *= 0.995;
		particles[i].velocity[2] *= 0.995;
*/
		/* Increase time (time used in gravity calculation) */
		particles[i].time += 0.001;

		/*	Check for particle death (particle has been "absorbed"
			into soil (moved until surface of soil mesh)) */
		if (particles[i].location[1] <= 1.7) {
			/*	Move last particle into the space this one occupied
				and decrease overall particle count by one */
			particles[i] = particles[--particleCount];
		}	
	}	

	/*	If there are no particles left, set a flag to tell animate() to
		stop calling this routine to update them */
	if (particleCount < 1)
		particlesActive = 0;
}


/**
 * Generate the vertices for the triangular mesh used for modelling the 
 * earth in the growing tray.
 *
 * The structure of the grid is similar to this diagram:
 * |/__\/__\/__\..
 * |\  /\  /\  /
 * |_\/__\/__\/..
 * | /\  /\  /\
 * |/__\/__\/__\..
 *
 * The triangles are equilateral (before vertices are peturbed).  There are
 * twice as many triangles along the X axis as the Y.  Triangles at the sides
 * of the grid are truncated to ensure the squareness of the grid.
 *
 * The grid produced is a unit square along the X and Y axes, with vertices
 * on the Z axis varying randomly between -0.3 and 0.3 to give the surface
 * an uneven appearance.
 *
 * The X and Y components are also randomly peturbed in an attempt to give
 * the grid a less regular appearance.  It makes the triangular structure
 * slightly less obvious.
 *
 */
void genTriMesh(void) {
	int x, y;
	float triHeight = sqrt(0.75);

	for (y = 0; y < MESH_Y; y++) {
		for (x = 0; x < MESH_X; x++) {
			/*	If at the left or right sides of the grid, keep all
				vertices aligned with the left or the right: no random
				perturbation is performed.  The triangles will not be
				equilateral as a result of this: they will be half the
				size (horizontally) as the result of the triangles
			*/
			if ((x==0) || (x==MESH_X-1))
				triVertices[x][y][0] = (((float)x) / (float)(MESH_X-1));
			else	
				/*	Offset X by 0.5 at even values of Y -- see the diagram
					above to see why this is necessary
				*/	
				triVertices[x][y][0] = (
						((float)x + (y % 2 ? 0.0 : 0.5) 
						+ RANDOM_RANGE(-0.3, 0.3)) / (float)(MESH_X-1)
					);

			/*	Don't randomly peturb Y values at the top or bottom of the
				grid, to ensure that it stays square
			*/	
			if ((y==0) || (y==MESH_Y-1))
				triVertices[x][y][1] =
					(triHeight * (float)y) /
					(triHeight * ((float)MESH_Y-1.0));
			else	
				triVertices[x][y][1] =
					(triHeight * (float)y + RANDOM_RANGE(-0.3, 0.3))
					/ (triHeight * ((float)MESH_Y-1.0));

			/*	Randomly peturb the vertices in the Z direction -- this
				gives the soil an uneven surface
			*/	
			triVertices[x][y][2] = RANDOM_RANGE(-0.4, 0.4);
		}	
	}	
}


void genTriNormals(void) {
	int x, y;
	int c;
	GLfloat verts[3][3];
	GLfloat nv1[3], nv2[3];

	for (y = 0; y < MESH_Y-1; y++) {
		for (x = 0; x < (MESH_X-1)*2; x++) {
			getTriangle(x, y, verts);

			for (c = 0; c < 3; c++) {
				nv1[c] = verts[2][c] - verts[1][c];
				nv2[c] = verts[0][c] - verts[1][c];
			}
			crossProduct(nv1, nv2, triNormals[x][y]);
			normalize(triNormals[x][y]);
		}
	}	
}


/**
 * Get the three vertices of the specified triangle in the triangle
 * mesh.  Triangles are addressed in an obvious (x,y) fashion -- there
 * are twice as many triangles in the X direction as the Y as there are
 * triangles both the right way up and upside-down along the X axis
 *
 * The vertices argument must be a 3x3 array.
 */
void getTriangle(int x, int y, GLfloat vertices[][3]) {
	int i;
	
	if (y % 2 == 0) {
		/* Even on Y axis */
		if (x % 2 == 0) {
			x /= 2;
			for (i = 0; i < 3; i++) {
				vertices[0][i] = triVertices[x][y][i];
				vertices[1][i] = triVertices[x+1][y][i];
				vertices[2][i] = triVertices[x][y+1][i];
			}
		} else {
			x = (x+1)/2;
			for (i = 0; i < 3; i++) {
				vertices[0][i] = triVertices[x][y][i];
				vertices[1][i] = triVertices[x][y+1][i];
				vertices[2][i] = triVertices[x-1][y+1][i];
			}
		}
	} else {
		/* Odd on Y axis */
		if (x % 2 == 0) {
			x /= 2;
			for (i = 0; i < 3; i++) {
				vertices[0][i] = triVertices[x][y][i];
				vertices[1][i] = triVertices[x+1][y+1][i];
				vertices[2][i] = triVertices[x][y+1][i];
			}	
		} else {
			x = (x+1)/2;
			for (i = 0; i < 3; i++) {
				vertices[0][i] = triVertices[x][y][i];
				vertices[1][i] = triVertices[x][y+1][i];
				vertices[2][i] = triVertices[x-1][y][i];
			}	
		}
	}
}


/**
 * Draw the triangular mesh.  
 */
void drawTriMesh(void) {
	int x, y;
	GLfloat verts[3][3];
	GLfloat earthADcol_r[3];
	GLfloat coloffset;

	glPushMatrix();
	
	/*	Rotate, scale and translate the mesh into the correct position
		within the growing tray
	*/	
	glTranslatef(trayDistance, 2.0, -TRAY_WIDTH / 2.0);
	glScalef(TRAY_WIDTH, TRAY_HEIGHT / 3.0, TRAY_WIDTH);
	glRotatef(270.0, 0.0, 1.0, 0.0);
	glRotatef(270.0, 1.0, 0.0, 0.0);

	/* Set material properties */
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, earthADcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, earthSPcol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, earthShine);

	glBegin(GL_TRIANGLES);
	for (y = 0; y < MESH_Y-1; y++) {
		for (x = 0; x < (MESH_X-1)*2; x++) {
			/*	Offset the brighness of each triangle drawn slightly
				by a random amount, to get a more earthy appearance
			*/
			coloffset = RANDOM_RANGE(-0.05, 0.05);
			earthADcol_r[0] = earthADcol[0] + coloffset;
			earthADcol_r[1] = earthADcol[1] + coloffset;
			earthADcol_r[2] = earthADcol[2] + coloffset;
			glMaterialfv(
				GL_FRONT_AND_BACK,
				GL_AMBIENT_AND_DIFFUSE,
				earthADcol_r
			);
			
			/* Retrieve the vertices of the triangle */
			getTriangle(x, y, verts);

			glNormal3fv(triNormals[x][y]);
			glVertex3fv(verts[0]);
			glVertex3fv(verts[1]);
			glVertex3fv(verts[2]);
		}
	}	
	glEnd();	

	glPopMatrix();
}


void dispVertex(GLfloat vertex[]) {
	printf("(%f, %f, %f)\n", vertex[0], vertex[1], vertex[2]);
}


void draw(void) {
	glInitNames();
	glPushName(0);

	/* Set up the viewing location */
	updateLookAt();
	
	/* 	Call display lists to draw the growing tray, soil mesh, chute and
		floor
	*/
	glCallList(listbase + LIST_TRAY);
	glCallList(listbase + LIST_MESH);
	glCallList(listbase + LIST_CHUTE);
	glCallList(listbase + LIST_FLOOR);

	/*	Draw the plant -- move the base to the centre of the growing tray
		and scale it up to a reasonable size
	*/	
	glPushMatrix();
	glTranslatef(trayDistance + (TRAY_WIDTH / 2.0), 2.0, 0.0);
	glScalef(1.5, 1.5, 1.5);
	drawPlant(plantStructure);
	glPopMatrix();

	/*	Update the particle system for the flowing liquid and draw the
		particles in it, if it is active */
	if (particlesActive) {
		updateParticles();
		drawParticles();
	}

	/* Draw the tank and liquid */
	tankAndLiquid();

	/* Draw the door in the correct location */
	glLoadName(DOOR_NAME);
	glPushMatrix();
	glTranslatef(DOOR_X, DOOR_BOTTOM + doorOffset, 0.0);
	glCallList(listbase + LIST_DOOR);
	glPopMatrix();
	glLoadName(0);
}


void leaf() {
// NEEDS NORMALS
	glPushMatrix();
	glScalef(0.5, 0.5, 0.5);
	glBegin(GL_POLYGON);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(0.7, 1.0, -0.5);
		glVertex3f(0.0, 2.4, -0.5);
	glEnd();	

	glBegin(GL_POLYGON);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(-0.7, 1.0, -0.5);
		glVertex3f(0.0, 2.4, -0.5);
	glEnd();	
	glPopMatrix();
}


/**
 * Create the wood effect texture used for the growing tray
 */
void makeWoodTexture(void) {
	GLubyte woodTexture[64][64][3];
	GLubyte *loc;
	int i;

	loc = (GLubyte*)woodTexture;

	/* Decode the texture data from the header file into an array */
	for (i = 0; i < woodwidth*woodheight; i++) {
		HEADER_PIXEL(header_data, loc);
		loc += 3;
	}	
  
//	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	/* */
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	/* 	Use the nearest pixel function for minification and magnification 
		filters.  The alternative GL_LINEAR looks better but is 
		considerably slower
	*/	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	/* Generate the texture */
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 64, 64, 0,
		GL_RGB, GL_UNSIGNED_BYTE, woodTexture);
}


/**
 *
 */
GLint doSelect(GLint x, GLint y) {
	GLuint selBuf[20];
	GLint viewport[4];
	GLint hits;
	
	glGetIntegerv(GL_VIEWPORT, viewport);

	glSelectBuffer(sizeof(selBuf), selBuf);
	glRenderMode(GL_SELECT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	/*	Use a pick matrix to restrict drawing to small (5x5) region
		around the cursor
	*/	
	gluPickMatrix((GLdouble) x, (GLdouble) (viewport[3] - y), 
                  5.0, 5.0, viewport);

	/* Apply the perspective transform */			  
	gluPerspective(45.0, width/height, 2.0, 80.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
	/*	Draw the scene in the usual way -- it is not rendered to the
		screen in selection mode.  All primitives that are near the 
		cursor are stored in the selection buffer
	*/	
    draw();
        
	/* Restore the projection matrix so normal rendering can resume */
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();	

	glFlush();

	/* Check whether there were any hits */
	hits = glRenderMode(GL_RENDER);
	if (hits <= 0)
		return -1;

	/* Get the name out of the hit record and return it to the caller */
	return selBuf[(hits - 1) * 4 + 3];
}


/**
 * The display callback function, called every time GL redraws the screen.
 * It simply clears the colour and depth buffers, then calls the draw 
 * function to render the current frame.  If frame rate counting is
 * enabled, this is performed here
 */
void display(void) {
#ifdef FPS
	int rstart, rend;

	// Get current time in ms
	rstart = glutGet(GLUT_ELAPSED_TIME);
#endif	
	
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw();

#ifdef FPS
	rend = glutGet(GLUT_ELAPSED_TIME);
	FPS_av += 1000.0/(rend-rstart);
	FPS_count++;
    printf("%0.1f fps  %0.1f fps av.  %d particles\n",
		1000.0/(rend-rstart), FPS_av / (double)FPS_count, particleCount
	);
#endif	

    glutSwapBuffers();
}


void updateLookAt(void) {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(
		LOOKAT_X + dist * cos(theta) * cos(phi),
		LOOKAT_Y + dist * sin(theta),
		LOOKAT_Z + dist * cos(theta) * sin(phi),
		LOOKAT_X, LOOKAT_Y, LOOKAT_Z,
		0.0, cos(theta) < 0.0 ? -1.0 : 1.0, 0.0
	);

	setLightPositions();
}


/** 
 * The window reshape callback function.  Glut will call this function when
 * the window is resized.  It resets the viewport to the new window size, 
 * and updates the projection matrix to reflect the new window size
 */
void reshape(int w, int h) {

	height = h;
	width = w;
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)w/(float)h, 2.0, 80.0);
}


/**
 * The keyboard callback function.  This is called whenever a key is
 * pressed.  It's purpose in this program is to let the user move the
 * viewpoint around the surface of a sphere, centered just above the
 * chute, using the keys '1' - '4'.
 */
void keyboard(unsigned char key, int x, int y) {
	switch (key) {
		case '1':
			phi += ROTATE_STEP;
			if (phi > 2*M_PI - ROTATE_STEP)
				phi = 0.0;
			break;
		case '2':
			phi -= ROTATE_STEP;
			if (phi < 0.0)
				phi = 2*M_PI;
			break;
		case '3':
			theta += ROTATE_STEP;
			if (theta > 2*M_PI - ROTATE_STEP)
				theta = 0.0;
			break;
		case '4':
			theta -= ROTATE_STEP;
			if (theta < 0.0)
				theta = 2*M_PI;
			break;
		case '5':
			dist -= 1.0;
			break;
		case '6':
			dist += 1.0;
			break;

		case 'a': 
			curLight = 0;
			puts("Current light: 0");
			break;
		case 'b': 
			curLight = 1;
			puts("Current light: 1");
			break;
		case 'c': 
			curLight = 2;
			puts("Current light: 2");
			break;
		case 'e':
			lightSet[curLight][2] += ROTATE_STEP;
			if (lightSet[curLight][2] > 2*M_PI - ROTATE_STEP)
				lightSet[curLight][2] = 0.0;
			break;
		case 'r':
			lightSet[curLight][2] -= ROTATE_STEP;
			if (lightSet[curLight][2] < 0.0)
				lightSet[curLight][2] = 2*M_PI;
			break;
		case 't':
			lightSet[curLight][1] += ROTATE_STEP;
			if (lightSet[curLight][1] > 2*M_PI - ROTATE_STEP)
				lightSet[curLight][1] = 0.0;
			break;
		case 'y':
			lightSet[curLight][1] -= ROTATE_STEP;
			if (lightSet[curLight][1] < 0.0)
				lightSet[curLight][1] = 2*M_PI;
			break;
		case '[':
			lightSet[curLight][0] += 1.0;
			break;
		case ']':
			lightSet[curLight][0] -= 1.0;
			break;
			
		case 'q': case 'Q':
			exit(0);
			break;
	}	

/*
	printf("theta: %f\n", theta);
	printf("phi: %f\n", phi);
*/

	calcTankFaces();
	
    glutPostRedisplay();
}


/**
 * Determine which faces of the tank (and correspondingly the liquid
 * within it) are forward and back facing.
 *
 * The algorithm used is the standard back face culling one -- find
 * the dot product of the normal of the polygon and the direction of
 * projection: if it's greater than 0 the polygon is front facing,
 * otherwise it's rear facing.  Rear facing polygons must be drawn
 * first to ensure that blending works correctly -- see drawTankAndLiquid()
 */
void calcTankFaces(void) {
	int i;
	GLfloat dop[3];

	for (i = 0; i < 6; i++) {
		dop[0] =
			hexMidPoints[i][0] - (LOOKAT_X + dist * cos(theta) * cos(phi));
		dop[1] =
			hexMidPoints[i][1] - (LOOKAT_Y + dist * sin(theta));
		dop[2] =
			hexMidPoints[i][2] - (LOOKAT_Z + dist * cos(theta) * sin(phi));
		normalize(dop);

		if (dotProduct(hexPrismNormals[i+1], dop) >= 0.0) {
			hexOrientation[i] = FRONT_FACING;
		} else {
			hexOrientation[i] = REAR_FACING;
		}	
	}
}	


/**
 * This function determines which direction the top of the liquid in the
 * tank is facing.  This needs to be done more regularly than for the 
 * sides of the tank, as the top of the liquid moves down as the liquid
 * is released from the tank.
 */
int getLiquidTopFacing(void) {
	GLfloat dop[3];
	GLfloat topNormal[] = {0.0, -1.0, 0.0};

	dop[0] = 0.0 - (LOOKAT_X + dist * cos(theta) * cos(phi));
	dop[1] = (depth + DOOR_BOTTOM) - (LOOKAT_Y + dist * sin(theta));
	dop[2] = 0.0 - (LOOKAT_Z + dist * cos(theta) * sin(phi));
	normalize(dop);

	if (dotProduct(topNormal, dop) >= 0.0) {
		return FRONT_FACING;
	} else {
		return REAR_FACING;
	}	
}


/**
 * Idle callback
 */
void animate(void) {
	int i;

	/* Move level of liquid down if it's flowing and control the rate */
	if (liquidFlowing) {
		depth -= liquidRate;

		/*	Slow down liquid once its surface is beneath the top of
			the aperture
		*/
		if (depth < DOOR_RISE_HEIGHT)
			liquidRate *= 0.97;

		/*	Stop liquid flowing once its surface is beneath the bottom of 
			the aperture
		*/
		if (depth < 0.0)
			liquidFlowing = 0;
	}		


	/* If the door is in the middle of opening, raise it another step */
	if (doorActive) {
		doorOffset += DOOR_RATE;

		/* Stop the door once it's reached it's maximum height */
		if (doorOffset >= DOOR_RISE_HEIGHT)
			doorActive = 0;
	}		


	/* If the plant is active, grow it a bit */
	if (plantActive == 1) {
		/* Increment percentage length of all active branches */
		branchPercentage += BRANCH_INCREMENT;


		if (branchPercentage >= 100.0) {
			branchPercentage = BRANCH_INCREMENT;
			plantPosition += MAX_CONCURRENT_GROWS;
		}	

		/*	Stop the plant growing if we move past the end of the
			plant description
		*/	
		if (plantPosition > maxPlantPosition)
			plantActive = -1;	
	}	
    
    glutPostRedisplay();
}


/* 
 * Mouse callback
 */
void mouse(int button, int state, int mouseX, int mouseY) {
	GLint hit;

	if (state == GLUT_DOWN) {
		puts("Mouse click");
		hit = doSelect((GLint) mouseX, (GLint) mouseY);
		if (hit != -1)
			printf("Hit: %d\n", hit);
		else
			puts("No hit detected");

		if ((hit == DOOR_NAME) && (!doorActive) && (doorOffset == 0.0)) {
			doorActive = 1;
			liquidFlowing = 1;
			particlesActive = 1;
		}	
	}
}


/**
 * Initialisation function
 *
 * Various variables and arrays are initialised here, and display lists are
 * compiled for all static objects (i.e. those which do not change
 * their appearance during the animation).  Display lists result in a slight
 * performance increase.
 *
 * Various OpenGL parameters are also set here.
 */
void initialise(void) {
	int i;
	char ldata[] = PLANT_INITIAL_RULE;
	char *temp;
	
	/*	Calculate the distance the tray should be from the origin in the
	 	X direction.  This is a relatively complex operation and is used
		regularly, so it is set here rather than being made a macro which
		is evaluated every time it is used
	*/	
 	trayDistance =
		(TANK_WIDTH/2) + cos(CHUTE_ANGLE * (M_PI/180))*CHUTE_LENGTH - 2.0;

	/* Set the colour the window is cleared to to a light blue colour */
	glClearColor(0.3, 0.4, 0.7, 1.0);
    
	/* Generate the normal vectors for each face on the tank / liquid */
	makeNormals(hexPrism, hexagonFacets, 8, hexPrismNormals);
	genMidPoints();

	/* Set up texture mapping and generate the wood effect texture */
	makeWoodTexture();

	genTriMesh();
	genTriNormals();

    /*	Enable hidden surface removal
		(the depth buffer is made read only for translucent polygons)
	*/ 
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	
	/* Light back of polygons using back material parameters */
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	/* Compute specular reflections from origin of eye coordinate system */
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

	/* Make sure there's no kind of (very time consuming) smoothing on */
	glDisable(GL_POLYGON_SMOOTH);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POINT_SMOOTH);

	/*	Ensure that normal vectors remain unit length
		when objects are scaled
	*/
	glEnable(GL_NORMALIZE);

	/* Set the point size for the particle system */
	glPointSize(2.5);
	
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	/* Create some display lists */
	listbase = glGenLists(7);

	/*	Create list for chute, including a transform to move it to the
		correct position
	*/	 
	glNewList(listbase + LIST_CHUTE, GL_COMPILE);
		glPushMatrix();
		glTranslatef(DOOR_X, DOOR_BOTTOM, -(CHUTE_WIDTH / 2.0));
		glRotatef(-CHUTE_ANGLE, 0.0, 0.0, 1.0);
		chute();
		glPopMatrix();
	glEndList();
		
	/*	Create list for chute, including a transform to move it to the
		correct position
	*/	 
	glNewList(listbase + LIST_TRAY, GL_COMPILE);
		glPushMatrix();
		glTranslatef(trayDistance, 0.1, -TRAY_WIDTH / 2);
		glScalef(TRAY_WIDTH, TRAY_HEIGHT, TRAY_WIDTH);
		growingTray();
		glPopMatrix();
	glEndList();

	/* Create list for the triangular mesh used to model the earth */	 
	glNewList(listbase + LIST_MESH, GL_COMPILE);
		glPushMatrix();
		drawTriMesh();
		glPopMatrix();
	glEndList();	

	/* Create list for the floor */
	glNewList(listbase + LIST_FLOOR, GL_COMPILE);
		glPushMatrix();
		bfloor();
		glPopMatrix();
	glEndList();	

	/* Create list for the door */
	glNewList(listbase + LIST_DOOR, GL_COMPILE);
		door();
	glEndList();	

	/* Create list for a leaf */
	glNewList(listbase + LIST_LEAF, GL_COMPILE);
		leaf();
	glEndList();	

	/* Generate the model for the plant to be drawn */

	/* Pass 1 */
	temp = parseLSystem(ldata);

	/* Pass 2 */
	plantStructure = parseLSystem(temp);

	/*	parseLSystem allocates memory with malloc() for its result -- 
		release the memory allocated for the result of the first pass
	*/	
	free(temp);

	/* Generate random angles for elevation and azimuth for plant */
	genLSAngles(plantStructure);

	/* Set up properties for lights, and enable them */
	setLighting();

	/* Find the initial orientation of each tank (and liquid) face */
	calcTankFaces();
}


/**
 * Set up the lights.  This function sets the position of each light
 * and its ambient, diffuse and specular components, using values from
 * the arrays defined at the beginning of this file
 */
void setLighting(void) {
	int l;
     
	glEnable(GL_LIGHTING);

	for (l = 0; l < LIGHT_COUNT; l++) {
		glLightfv(light[l], GL_POSITION, lightPosition[l]); 
		glLightfv(light[l], GL_AMBIENT,  lightAmbient[l]); 
		glLightfv(light[l], GL_DIFFUSE,  lightDiffuse[l]);
		glLightfv(light[l], GL_SPECULAR, lightSpecular[l]); 

		glEnable(light[l]);
	}
}


/**
 * Set the positions of the lights.  This needs to be called after the
 * viewpoint is changed with gluLookAt, to ensure the lights are in
 * their correct positions.
 */
void setLightPositions() {
	int l;
	GLfloat white[] = {1.0,1.0,1.0,1.0};

	for (l = 0; l < LIGHT_COUNT; l++) {
		lightPosition[l][0] =
			lightSet[l][0] * cos(lightSet[l][1]) * cos(lightSet[l][2]);
		lightPosition[l][1] =
			lightSet[l][0] * sin(lightSet[l][1]);
		lightPosition[l][2] =
			lightSet[l][0] * cos(lightSet[l][1]) * sin(lightSet[l][2]);
	}
     
	for (l = 0; l < LIGHT_COUNT; l++)
		glLightfv(light[l], GL_POSITION, lightPosition[l]); 

/*
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, white);

	glPointSize(6.0);
	glBegin(GL_POINTS);
	for (l = 0; l < LIGHT_COUNT; l++)
		glVertex3fv(lightPosition[l]);
	glEnd();	
*/	
}


int main(int argc, char *argv[]) {

	/*	Seed the random number generator with the current time
		in seconds since 1970
	*/	
	srand((long)time(NULL));
                
	glutInit(&argc, argv);
	/*	Set the initial display mode to a window with double buffering
	 	(to avoid seeing objects being drawn), RGBA colour, and 
		depth buffering (for hidden surface removal)
	*/	
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

	glutInitWindowSize(width, height);
	glutCreateWindow("CGV Open Exam 2000: Candidate 19327");
    
	initialise();

	/* Set up callbacks */
	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(animate);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);

	/* Enter the glut event loop */
	glutMainLoop();

	return 0;
}
