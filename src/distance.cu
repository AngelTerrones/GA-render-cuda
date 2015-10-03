#include <stdio.h>
#include <cuda.h>
#include <builtin_types.h>
#include <sys/time.h>

#define N_THREADS 256
// To check for nasty errors
#define CHECK(call) {                                                   \
        const cudaError_t error = call;                                 \
        if(error != cudaSuccess){                                       \
            printf("Error: %s:%d, ", __FILE__, __LINE__);               \
            printf("code:%d, reason: %s\n", error, cudaGetErrorString(error)); \
            exit(-10*error);                                            \
        }                                                               \
    }

// GPU pointers
unsigned char *si_cuda;
unsigned char *ri_cuda;
unsigned long long *tmp_cuda;
unsigned long long *d_cuda;

/**
 * Measure time
 */
double cpuSeconds(){
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return((double)tp.tv_sec + (double)tp.tv_usec*1e-6);
}

/**
 * GPU kernel
 */
__global__ void GPUDistance(const unsigned char *si, const unsigned char *ri, unsigned long long *d, unsigned long long *tmp_cuda, const int size){
    int index = (blockDim.x * blockIdx.x) + threadIdx.x; // Global index
    int tid = threadIdx.x; // local index
    unsigned long long tmp;
    unsigned long long *ldata = tmp_cuda + (blockDim.x * blockIdx.x); // local pointer to global memory

    // check boundary
    if(index >= size)
        return;

    // Diff images
    tmp = si[index] - ri[index];
    tmp_cuda[index] = tmp * tmp;

    // reduce: sum the diff vector
    for(int stride = blockDim.x >> 1; stride > 0; stride >>= 1){
        __syncthreads();
        if(tid < stride)
            ldata[tid] += ldata[tid + stride];
    }
    __syncthreads(); // needed?

    // copy the block result.
    if(tid == 0){
        d[blockIdx.x] = ldata[0];
    }
}

/**
 * Copy source image to GPU
 */
void CopySourceImage(const unsigned char *image, const int size){
    cudaMemcpy(si_cuda, image, size, cudaMemcpyHostToDevice);
}

/**
 * Copy test image to GPU
 */
void CopyRenderImage(const unsigned char *image, const int size){
    cudaMemcpy(ri_cuda, image, size, cudaMemcpyHostToDevice);
}

/**
 * Alloc GPU memory, and set to zero.
 * The number of bytes to alloc is a multiple of N_THREADS.
 */
void MallocGPUMemory(const int size){
    int n_blocks = (size + N_THREADS - 1)/N_THREADS;
    int size2 = n_blocks * N_THREADS;
    CHECK(cudaMalloc((void **)&si_cuda, size2));
    CHECK(cudaMalloc((void **)&ri_cuda, size2));
    CHECK(cudaMalloc((void **)&tmp_cuda, size2 * sizeof(unsigned long long)));
    CHECK(cudaMalloc((void **)&d_cuda, n_blocks * sizeof(unsigned long long)));

    // clear memory
    CHECK(cudaDeviceSynchronize());
    CHECK(cudaMemset((void *)si_cuda, 0, size2));
    CHECK(cudaMemset((void *)ri_cuda, 0, size2));
    CHECK(cudaMemset((void *)tmp_cuda, 0, size2 * sizeof(unsigned long long)));
    CHECK(cudaMemset((void *)d_cuda, 0, n_blocks * sizeof(unsigned long long)));
    printf("CUDA memory created and set to 0\n\n");
}

/**
 * Free the GPU memory
 */
void FreeGPUMemory(void){
     cudaFree(si_cuda);
     cudaFree(ri_cuda);
     cudaFree(tmp_cuda);
     cudaFree(d_cuda);
}

/**
 * Launch the GPU kernel, and performs the final reduction in the result vector.
 */
unsigned long long DistanceGPU(unsigned char *ri, const int size){
    int threadsPerBlock = N_THREADS;
    int blocksPerGrid = (size + threadsPerBlock - 1)/threadsPerBlock;
    unsigned long long distance[blocksPerGrid];
    unsigned long long tmp = 0L;

    int n_blocks = (size + N_THREADS - 1)/N_THREADS;
    int size2 = n_blocks * N_THREADS;

    CopyRenderImage(ri, size);

    GPUDistance<<<blocksPerGrid, threadsPerBlock>>>(si_cuda, ri_cuda, d_cuda, tmp_cuda, size2);
    cudaDeviceSynchronize();

    cudaMemcpy(distance, d_cuda, blocksPerGrid * sizeof(unsigned long long), cudaMemcpyDeviceToHost);
    for(int i = 0; i < blocksPerGrid; i++)
        tmp += distance[i];
    return tmp;
}
