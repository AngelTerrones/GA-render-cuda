#ifndef CROMOSOMA_H
#define CROMOSOMA_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <QDebug>
#include <QList>
#include <QPoint>
#include <algorithm>

#define MAX_POINTS   50
#define MIN_POINTS   3
#define MAX_POLYGON  50

using namespace std;

class Polygon
{
    public:
        // constructor
        Polygon(){
            r = rand() % 255;
            g = rand() % 255;
            b = rand() % 255;
            a = rand() % 255;
            nPoints = MIN_POINTS;
            xMax = yMax = 0;

            for(int i = 0; i < nPoints; i++){
                vertex << QPoint(0, 0);
            }
        }

        // destructor
        virtual ~Polygon(){};

        // initialize
        void Init(int xMax, int yMax){
            this->xMax = xMax;
            this->yMax = yMax;

            for(int i = 0; i < nPoints; i++){
                vertex[i].rx() += rand() % xMax;
                vertex[i].ry() += rand() % yMax;
            }
        }

        // Getters
        unsigned char Red     (){return r;}
        unsigned char Green   (){return g;}
        unsigned char Blue    (){return b;}
        unsigned char Alpha   (){return a;}
        int NPoints           (){return nPoints;}
        QList<QPoint> &Vertex (){return vertex;}
        int XMax              (){return xMax;}
        int YMax              (){return yMax;}

        // Setter
        void Red     (unsigned char color){ r    = color;}
        void Green   (unsigned char color){ g    = color;}
        void Blue    (unsigned char color){ b    = color;}
        void Alpha   (unsigned char color){ a    = color;}
        void NPoints (int nPoints){this->nPoints = nPoints;}

        // Overload
        Polygon &operator =(const Polygon A){
            if(this != &A){
                r = A.r;
                g = A.g;
                b = A.b;
                a = A.a;

                xMax = A.xMax;
                yMax = A.yMax;

                nPoints = A.nPoints;
                vertex = A.vertex;
            }

            return *this;
        }
    private:
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
        int           xMax;
        int           yMax;
        int           nPoints;
        QList<QPoint> vertex;
};

class Chromosome
{
    public:
        Chromosome();
        virtual ~Chromosome();
        void        Create(int nPolygons);
        void        Delete();
        int         Length();
        // Chromosome *Clone();
        void        Clone(Chromosome *copy);
        void        Mutate();
        Polygon    *DNA();
        unsigned long long &Fitness();
    private:
        Polygon            *listPolygon;
        unsigned long long  fitness;
        int                 length;
};

#endif /* CROMOSOMA_H */
