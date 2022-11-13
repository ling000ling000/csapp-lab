/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";

void trans_32(int M, int N, int A[N][M], int B[M][N]);
void trans_64(int M, int N, int A[N][M], int B[M][N]);
void trans_61(int M, int N, int A[N][M], int B[M][N]);

void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    switch (M)
    {
    case 32:
        trans_32(M, N, A, B);
        break;
    case 64:
        trans_64(M, N, A, B);
        break;
    case 61:
        trans_61(M, N, A, B);
        break;
    }
}

void trans_32(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    for (i = 0; i < M; i += 8)
    {
        for (j = 0; j < N; j += 8)
        {
            for (int ii = i; ii < i + 8; ii ++ )
            {
                int tmp1 = A[ii][j];
                int tmp2 = A[ii][j + 1];
                int tmp3 = A[ii][j + 2];
                int tmp4 = A[ii][j + 3];
                int tmp5 = A[ii][j + 4];
                int tmp6 = A[ii][j + 5];
                int tmp7 = A[ii][j + 6];
                int tmp8 = A[ii][j + 7];

                B[j][ii] = tmp1;
                B[j + 1][ii] = tmp2;
                B[j + 2][ii] = tmp3;
                B[j + 3][ii] = tmp4;
                B[j + 4][ii] = tmp5;
                B[j + 5][ii] = tmp6;
                B[j + 6][ii] = tmp7;
                B[j + 7][ii] = tmp8;
            }
        }
    }
}

void trans_64(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, x, y;
    int x1, x2, x3, x4, x5, x6, x7, x8;
    for (i = 0; i < N; i += 8 )
    {
        for (j = 0; j < M; j += 8 )
        {
            // 1块8*8，我们分成4块来做，每一块4*4
            // 处理左上
            for (x = i; x < i + 4; x ++ ) 
            {
                x1 = A[x][j]; // A左上
                x2 = A[x][j+1]; 
                x3 = A[x][j+2]; 
                x4 = A[x][j+3]; 

                x5 = A[x][j+4]; // A右上
                x6 = A[x][j+5]; 
                x7 = A[x][j+6]; 
                x8 = A[x][j+7];  
                
                B[j][x] = x1; // 翻转，A左上存到B左上
                B[j+1][x] = x2; 
                B[j+2][x] = x3; 
                B[j+3][x] = x4;

                B[j][x+4] = x5; // 将A右上,存到B的右上
                B[j+1][x+4] = x6; 
                B[j+2][x+4] = x7; 
                B[j+3][x+4] = x8;
            }
            // 处理左下
            for (y = j; y < j + 4; y ++ )
            {
                x1 = A[i+4][y]; // A左下
                x2 = A[i+5][y]; 
                x3 = A[i+6][y]; 
                x4 = A[i+7][y];

                x5 = B[y][i+4]; // B右上，也就是之前的A右上
                x6 = B[y][i+5]; 
                x7 = B[y][i+6]; 
                x8 = B[y][i+7];
                
                B[y][i+4] = x1; // A左下翻转到B右上
                B[y][i+5] = x2; 
                B[y][i+6] = x3; 
                B[y][i+7] = x4;

                B[y+4][i] = x5; // B右上 赋值到 B左下， 这样A右上也完成了翻转
                B[y+4][i+1] = x6; 
                B[y+4][i+2] = x7; 
                B[y+4][i+3] = x8;
            }
            // 处理右下
            for (x = i + 4; x < i + 8; x ++ )
            {
                x1 = A[x][j+4]; // 第四块没有任何改动，和原来效果是一样的
                x2 = A[x][j+5]; 
                x3 = A[x][j+6]; 
                x4 = A[x][j+7];

                B[j+4][x] = x1; 
                B[j+5][x] = x2; 
                B[j+6][x] = x3; 
                B[j+7][x] = x4;
            }
        }
    }
}

void trans_61(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    int ex = 17;  // 看别人选17
    for (i = 0; i < N; i += ex) {      // 枚举每ex行
        for (j = 0; j < M; j += ex) {  // 枚举每ex列
            // 下面开始转置这个ex * ex的块
            for (int x = i; x < N && x < i + ex; ++x)
                for (int y = j; y < M && y < j + ex; ++y) B[y][x] = A[x][y];
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

