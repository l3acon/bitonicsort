#ifndef STL_CL_cNORM
#define STL_CL_cNORM

//test in GLGraphicWidget.cpp
//glMultMatrixd(m_viewpoint.transformMatrix().data())


// System includes
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cmath>

#include "cli.h"


extern void stlclComputeNormal(
    unsigned int nVerticies,
    float *verticies, 
    float *normalBuffer, 
    CLI * cli,
    std::vector<cl_int> &errors);

#endif