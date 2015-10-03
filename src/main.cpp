#include <cuda.h>
#include <cstdio>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <QDir>
#include <QtGui>
#include <QDebug>
#include <QSvgGenerator>
#include "Chromosome.hpp"

// ------------------------------------------------------------------------------
// Defines
// ------------------------------------------------------------------------------
#define N_POLYGONS   50         // Use 50 polygons for render
#define N_POPULATION 10         // Size of population
#define HAREM        0.4        // Use 40% of population for harem.

// ------------------------------------------------------------------------------
// global variables... Oh HELL
// ------------------------------------------------------------------------------
int max_generation = -1;
QImage sourceImage;
int imgW = -1;
int imgH = -1;
Chromosome *populationA[N_POPULATION];
Chromosome *populationB[N_POPULATION];
int sizePopulationA = -1;
int sizePopulationB = -1;

// ------------------------------------------------------------------------------
// CUDA functions.
// ------------------------------------------------------------------------------
double cpuSeconds();
unsigned long long DistanceGPU(unsigned char *ri, const int size);
void CopySourceImage(const unsigned char *image, const int size);
void CopyRenderImage(const unsigned char *image, const int size);
void MallocGPUMemory(const int size);
void FreeGPUMemory(void);

// ------------------------------------------------------------------------------
// functions
// ------------------------------------------------------------------------------
/**
 * Decode the chromosome and generates a rastered image.
 */
QImage DrawImage(Chromosome *dna){
    QImage image(imgW, imgH, QImage::Format_RGB32);
    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    //painter.setRenderHint(QPainter::Antialiasing);

    // black background
    painter.setBrush(QBrush(QColor(0, 0, 0, 255)));
    QPolygon polyclean;
    int pointsclean[] = {0, 0, 0, imgH, imgW, imgH, imgW, 0};
    polyclean.setPoints(4, pointsclean);
    painter.drawConvexPolygon(polyclean);

    // Draw all the polygons
    Polygon *poly = dna->DNA();
    for (int n = 0; n < N_POLYGONS; n++) {
        QColor color(poly[n].Red(),
                     poly[n].Green(),
                     poly[n].Blue(),
                     poly[n].Alpha());
        painter.setBrush(QBrush(color));

        int size = poly[n].NPoints();
        int *listPoints = new int[2*size];
        for(int i = 0; i < size; i++){
            listPoints[2*i] = poly[n].Vertex()[i].x();
            listPoints[2*i + 1] = poly[n].Vertex()[i].y();
        }

        QPolygon qPoly;
        qPoly.setPoints(size, listPoints);
        painter.drawConvexPolygon(qPoly);
        delete [] listPoints;
    }
    painter.end();

    return image;
}

/**
 * Decode the chromosome and generates an SVG image.
 */
void DrawSVG(Chromosome *dna, const char *svgName){
    QSvgGenerator svgGen;
    svgGen.setFileName(QString(svgName));
    svgGen.setSize(QSize(imgW, imgH));
    svgGen.setViewBox(QRect(0, 0, imgW, imgH));

    QPainter painter(&svgGen);
    painter.setPen(Qt::NoPen);
    painter.setRenderHint(QPainter::Antialiasing);

    // Black background
    painter.setBrush(QBrush(QColor(0, 0, 0, 255)));
    QPolygon polyclean;
    int pointsclean[] = {0, 0, 0, imgH, imgW, imgH, imgW, 0};
    polyclean.setPoints(4, pointsclean);
    painter.drawConvexPolygon(polyclean);

    // Draw all the polygons
    Polygon *poly = dna->DNA();
    for (int n = 0; n < N_POLYGONS; n++) {
        QColor color(poly[n].Red(),
                     poly[n].Green(),
                     poly[n].Blue(),
                     poly[n].Alpha());
        painter.setBrush(QBrush(color));

        int size = poly[n].NPoints();
        int *listPoints = new int[2*size];

        for(int i = 0; i < size; i++){
            listPoints[2*i] = poly[n].Vertex()[i].x();
            listPoints[2*i + 1] = poly[n].Vertex()[i].y();
        }

        QPolygon qPoly;
        qPoly.setPoints(size, listPoints);
        painter.drawConvexPolygon(qPoly);
        delete [] listPoints;
    }
    painter.end();
}

/**
 * Evaluates a chromosome.
 * Fitness = sum (source_image - render_image), in RGBA space (A = alpha)
 * This function use CUDA.
 */
unsigned long long Distance(Chromosome *dna){
    int nBytesImage = sourceImage.byteCount();
    QImage imageRender = DrawImage(dna);

    unsigned long long distance2 = DistanceGPU(imageRender.bits(), nBytesImage);
    return distance2;
}

/**
 * Creates 2 childs, given 2 parentes.
 */
void Crossover(Chromosome *a, Chromosome *b){
    int length = a->Length();

    if(length <= 0)
        return;

    for(int i = 0; i < length; i++){
        // switch current polygon. 50% prob.
        if(rand() % 2){
            Polygon tmp = a->DNA()[i];
            a->DNA()[i] = b->DNA()[i];
            b->DNA()[i] = tmp;
        }
    }
}

/**
 * Returns the best individual in the population.
 */
Chromosome *SelectBest(){
    if(sizePopulationA == 0)
        return NULL;

    unsigned long long min = ~0x00;
    int index = 0;

    for(int i = 0; i < sizePopulationA; i++){
        if(populationA[i]->Fitness() < min){
            min = populationA[i]->Fitness();
            index = i;
        }
    }
    return populationA[index];
}

/**
 * Selects 2 random members, and returns the best.
 */
Chromosome *SelectTournament(){
    int i = rand() % sizePopulationA;
    int j = rand() % sizePopulationA;

    if(populationA[i]->Fitness() < populationA[j]->Fitness())
        return populationA[i];
    else
        return populationA[j];
}

/**
 * Insert a member/chromosome into a temporal population
 */
void InsertPobB(Chromosome *dna){
    if(sizePopulationA <= sizePopulationB)
        return;

    if(populationB[sizePopulationB] == NULL){
        populationB[sizePopulationB] = new Chromosome();
    }

    dna->Clone(populationB[sizePopulationB]);

    sizePopulationB++;
}

/**
 * Copy population from B (temporal) to A.
 */
void UpdatePopulation(){
    for(int i = 0; i < sizePopulationB; i++){
        populationA[i]->Delete();
        delete populationA[i];
        populationA[i] = populationB[i];
        populationB[i] = NULL;
    }

    sizePopulationA = sizePopulationB;
    sizePopulationB = 0;
}

/**
 * A step.
 * Elitism & Harem.
 */
void GAStep(){
    Chromosome *elite = SelectBest();
    Chromosome clonA;
    Chromosome clonB;
    Chromosome *ap = NULL;

    // Elitism
    elite->Clone(&clonA);
    InsertPobB(&clonA);

    // harem
    int fraction = int(HAREM * sizePopulationA);
    for(int i = 0; i < fraction; i += 2){
        elite->Clone(&clonA);
        SelectTournament()->Clone(&clonB);
        Crossover(&clonA, &clonB);
        clonA.Mutate();
        clonB.Mutate();

        clonA.Fitness() = Distance(&clonA);
        clonB.Fitness() = Distance(&clonB);

        InsertPobB(&clonA);
        InsertPobB(&clonB);
    }

    // Fill
    while((sizePopulationA - sizePopulationB) > 0){
        ap = SelectTournament();
        ap->Clone(&clonA);
        ap = SelectTournament();
        ap->Clone(&clonB);

        Crossover(&clonA, &clonB);
        clonA.Mutate();
        clonB.Mutate();
        clonA.Fitness() = Distance(&clonA);

        clonB.Fitness() = Distance(&clonB);

        if(clonA.Fitness() < clonB.Fitness())
            InsertPobB(&clonA);
        else
            InsertPobB(&clonB);
    }

    UpdatePopulation();
}

/**
 * Perform the GAStep for N generations.
 */
void RunGA(){
    int generations = 0;
    while(max_generation >= 0 && generations <= max_generation){
        GAStep();
        if(generations % 100 == 0){
            std::cout << "Generacion: " << generations << "\n";
        }
        if(generations % 1000 == 0){
            Chromosome *best = SelectBest();
            QImage image = DrawImage(best);
            image.save(QString("out/out_") + QString::number(generations) + ".png");
            qDebug() << "Fitness: " << best->Fitness();
        }
        generations++;
    }
    DrawSVG(SelectBest(), "Final.svg");
}

/**
 * Initialize populations.
 */
void InitGA(){
    sizePopulationA = sizePopulationB = 0;

    for(int i = 0; i < N_POPULATION; i++){
        // population A
        populationA[i] = new Chromosome();
        populationA[i]->Create(N_POLYGONS);
        for(int j = 0; j < N_POLYGONS; j++){
            populationA[i]->DNA()[j].Init(imgW, imgH);
        }
        populationA[i]->Fitness() = Distance(populationA[i]);
        sizePopulationA++;

        // population B
        populationB[i] = NULL;
    }
}

/**
 * FREE ALL THE MEMORY!!!!
 */
void CleanUp(){
    for(int i = 0; i < sizePopulationA; i++){
        if(populationA[i] != NULL){
            populationA[i]->Delete();
            delete populationA[i];
        }
        if(populationB[i] != NULL){
            populationB[i]->Delete();
            delete populationB[i];
        }
    }
}

/**
 * Here we go...
 */
int main(int argc, char *argv[]){
    int option;

    // New seed
    srand(time(NULL));

    // Get parameters.
    while((option = getopt(argc, argv, "g:")) != -1){
        switch (option) {
            // get the generation limit
            case 'g': {
                max_generation = atoi(optarg);
                if(max_generation == 0){
                    std::cerr << "ERROR: Especificar limite de generarion mayor a cero: -g 100\n";
                    exit(EXIT_FAILURE);
                }
                else if(max_generation < 0){
                    std::cerr << "ERROR: El numero de generaciones debe ser positivo\n";
                    exit(EXIT_FAILURE);
                }
                break;
            }
            default:
                break;
        }
    }

    // get source file
    if(optind >= argc){
        std::cerr << "Especificar imagen a replicar\n";
        exit(EXIT_FAILURE);
    }

    // check if the file exists.
    FILE *fp = fopen(argv[optind], "r");
    if(fp == NULL){
        std::cerr << "ERROR: Imagen no existe: " << argv[optind] << "\n";
        exit(EXIT_FAILURE);
    }
    fclose(fp);

    // -------------------------------------------------------------------------
    // CUDA
    // :D
    int deviceCount = 0;
    int cudaDevice = 0;
    char cudaDeviceName[100];
    cuInit(0);
    cuDeviceGetCount(&deviceCount);
    cuDeviceGet(&cudaDevice, 0);
    cuDeviceGetName(cudaDeviceName, 100, cudaDevice);

    qDebug() << "Cuda info:";
    qDebug() << "Number of devices: " << deviceCount;
    qDebug() << "Device name: " << cudaDeviceName << "\n";
    // End CUDA
    // -------------------------------------------------------------------------

    // Load image
    sourceImage = QImage(argv[optind]);
    imgW = sourceImage.width();
    imgH = sourceImage.height();

    // -------------------------------------------------------------------------
    // copy reference image to GPU
    MallocGPUMemory(sourceImage.byteCount());
    CopySourceImage(sourceImage.bits(), sourceImage.byteCount());
    // -------------------------------------------------------------------------

    std::cout << "Maximo de generaciones: " << max_generation << "\n";
    QDir::current().mkpath("out");

    InitGA();
    RunGA();
    CleanUp();

    // Free GPU memory
    FreeGPUMemory();

    return 0;
}
