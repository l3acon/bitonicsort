
// STL fie I/O
// very quick implimenation that isn't 
// all correct

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>

#include "stl.h"

int stlRead(
    const char* stlFile, 
    std::vector<float> &verticies, 
    std::vector<float> &normals)
{
    float facet[12];

    FILE *ifile = fopen(stlFile, "rb");

    if (!ifile)
    {
        std::cout<<"ERROR reading file"<<std::endl;
        return 1;
    }

    char title[80];
    unsigned int nFaces;

    fread(title,80,1,ifile);
    fread( (void *) &nFaces, 4,1,ifile);
    for (size_t i = 0; i < nFaces; i+=12)
    {
        fread( (void*) facet, sizeof(float), 12,ifile);
        for(size_t j = 0; j < 3; j++)
            normals.push_back(facet[j]);

        for(size_t j = 3; j < 12; ++j)
            verticies.push_back(facet[j]);
        fread(facet, sizeof(unsigned short), 1, ifile);
    }
    return 0;
}

int stlWrite(
    const char* stlFile, 
    std::vector<float> &verticies, 
    std::vector<float> &normals);

float stlVerifyTransform(
    const float* xMat, 
    float* v, 
    float* xformedv, 
    unsigned int nFaces)
{
    float verto[9];

    for( size_t i = 0; i < nFaces; i += 12)
    {
        // x1-3 = 0 1 2
        // y1-3 = 3 4 5
        // z1-3 = 6 7 8

        //x coordinates
        verto[i+0] = xMat[0]*v[i+0] + xMat[1]*v[i+3] + xMat[2]*v[i+6] + xMat[3];
        verto[i+1] = xMat[0]*v[i+1] + xMat[1]*v[i+4] + xMat[2]*v[i+7] + xMat[3];
        verto[i+2] = xMat[0]*v[i+2] + xMat[1]*v[i+5] + xMat[2]*v[i+8] + xMat[3];
                        
        //y coordinates 
        verto[i+3] = xMat[4]*v[i+0] + xMat[5]*v[i+3] + xMat[6]*v[i+6] + xMat[7];
        verto[i+4] = xMat[4]*v[i+1] + xMat[5]*v[i+4] + xMat[6]*v[i+7] + xMat[7];
        verto[i+5] = xMat[4]*v[i+2] + xMat[5]*v[i+5] + xMat[6]*v[i+8] + xMat[7];
                       
        //z coordinates
        verto[i+6] = xMat[8]*v[i+0] + xMat[9]*v[i+3] + xMat[10]*v[i+6] + xMat[11];
        verto[i+7] = xMat[8]*v[i+1] + xMat[9]*v[i+4] + xMat[10]*v[i+7] + xMat[11];
        verto[i+8] = xMat[8]*v[i+2] + xMat[9]*v[i+5] + xMat[10]*v[i+8] + xMat[11];

        for(int j = 0; j < 9; ++j)
            if(fabs(verto[j+i]) - fabs(xformedv[j+i]) > 1e-4 )
                return j+i;
    }
    return 0.0;
}