#include "Chromosome.hpp"

Chromosome::Chromosome(){
    length = 0;
    listPolygon = NULL;
    fitness = 0;
}

Chromosome::~Chromosome(){
    Delete();
}

void Chromosome::Create(int nPolygons){
    Delete();
    listPolygon = new Polygon[nPolygons];
    length = nPolygons;
    fitness = 0;
}

void Chromosome::Delete(){
    if(listPolygon != NULL)
        delete [] listPolygon;

    listPolygon = NULL;
    fitness = 0;
    length = 0;
}

int Chromosome::Length(){
    return length;
}

void Chromosome::Clone(Chromosome *copy){
    if(length == 0)
        return;

    copy->Create(length);
    copy->fitness = fitness;

    // copy data
    for(int i = 0; i < length; i++){
        copy->listPolygon[i] = listPolygon[i];
    }
}

void Chromosome::Mutate(){
    // Add/remove a random point to each polygon
    for(int i = 0; i < length; i++){
        int nVertex = listPolygon[i].NPoints(); // get number of points
        // Add a point if possible, and with 10% of prob.
        // new point = ( point[location-1] + pont[location] )/2
        if(nVertex < MAX_POINTS && ( (rand() % 100) < 10 )){
            int location = ( rand() % (nVertex - 1) ) + 1;
            QPoint newVertex = ( listPolygon[i].Vertex()[location - 1] + listPolygon[i].Vertex()[location ] )/2;
            listPolygon[i].Vertex().insert(location, newVertex);
            listPolygon[i].NPoints(++nVertex);
        }
        // remove point with 10% of prob.
        if(nVertex > MIN_POINTS && ( (rand() % 100) < 10 )){
            int location = rand() % nVertex;
            listPolygon[i].Vertex().removeAt(location);
            listPolygon[i].NPoints(--nVertex);
        }
    }

    // mutate a random field
    int polygon = rand() % length;
    int field = rand() % ( 4 + listPolygon[polygon].NPoints() );

    switch (field) {
        case 0:
            listPolygon[polygon].Red(listPolygon[polygon].Red() + rand()%51 - 25);
            break;
        case 1:
            listPolygon[polygon].Green(listPolygon[polygon].Green() + rand()%51 - 25);
            break;
        case 2:
            listPolygon[polygon].Blue(listPolygon[polygon].Blue() + rand()%51 - 25);
            break;
        case 3:
            listPolygon[polygon].Alpha(listPolygon[polygon].Alpha() + rand()%51 - 25);
            break;
        default:
            if(rand()%2){
                int temp = listPolygon[polygon].Vertex()[field - 4].x() + rand() % 21 - 10;
                listPolygon[polygon].Vertex()[field - 4].rx() = max(0, min (temp, listPolygon[polygon].XMax()));
            }else{
                int temp = listPolygon[polygon].Vertex()[field - 4].y() + rand() % 21 - 10;
                listPolygon[polygon].Vertex()[field - 4].ry() = max(0, min (temp, listPolygon[polygon].YMax()));
            }
            break;
    }
}

Polygon *Chromosome::DNA(){
    return listPolygon;
}

unsigned long long &Chromosome::Fitness(){
    return fitness;
}
