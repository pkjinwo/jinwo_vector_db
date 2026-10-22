/* 验证 2026-10-22 补齐的 24 个向量函数 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "jw_vecdb.h"
#include "jw_vector.h"
#include "jw_arena.h"

static int pass=0, fail=0;
#define CK(c,...) do{ if(c) pass++; else { fail++; printf("  FAIL: "); printf(__VA_ARGS__); printf("\n"); } }while(0)

int main(void){
    printf("=== 新补齐的向量函数测试 ===\n");
    jw_float32_t v[6] = {3.0f, -1.0f, 4.0f, -1.5f, 5.0f, -9.0f};

    CK(jw_vec_sum(v,6)==0.5f, "sum=%f 期望 0.5", jw_vec_sum(v,6));
    CK(fabsf(jw_vec_mean(v,6)-0.0833333f)<1e-4f, "mean=%f", jw_vec_mean(v,6));
    CK(jw_vec_max(v,6)==5.0f, "max=%f", jw_vec_max(v,6));
    CK(jw_vec_min(v,6)==-9.0f, "min=%f", jw_vec_min(v,6));
    CK(jw_vec_argmax(v,6)==4, "argmax=%u 期望 4", jw_vec_argmax(v,6));
    CK(jw_vec_argmin(v,6)==5, "argmin=%u 期望 5", jw_vec_argmin(v,6));
    CK(fabsf(jw_vec_norm_l1(v,6)-23.5f)<1e-4f, "l1=%f 期望 23.5", jw_vec_norm_l1(v,6));

    /* variance/stddev: 均值 0.0833, 方差 = E[x^2]-mean^2 */
    float e=0; for(int i=0;i<6;i++) e+=v[i]*v[i]; e/=6;
    float var=e-0.0833333f*0.0833333f;
    CK(fabsf(jw_vec_variance(v,6)-var)<1e-3f, "variance=%f 期望 %f", jw_vec_variance(v,6),var);
    CK(fabsf(jw_vec_stddev(v,6)-sqrtf(var))<1e-3f, "stddev=%f", jw_vec_stddev(v,6));

    /* is_valid: NaN 与 Inf 应判无效 */
    CK(jw_vec_is_valid(v,6)==JW_TRUE, "正常向量应 valid");
    float bad[3]={1.0f, NAN, 2.0f};
    CK(jw_vec_is_valid(bad,3)==JW_FALSE, "含 NaN 应 invalid");
    float inf[3]={1.0f, INFINITY, 2.0f};
    CK(jw_vec_is_valid(inf,3)==JW_FALSE, "含 Inf 应 invalid");

    /* is_normalized */
    float nv[4]={0.5f,0.5f,0.5f,0.5f};   /* 范数=1 */
    CK(jw_vec_is_normalized(nv,4,1e-4f)==JW_TRUE, "范数1 应判已归一化");
    CK(jw_vec_is_normalized(v,6,1e-4f)==JW_FALSE, "非单位向量应判未归一化");

    /* normalize_l2 / l1 */
    float a[4]={3.0f,4.0f,0.0f,0.0f};
    CK(jw_vec_normalize_l2(a,4)==JW_SUCCESS, "normalize_l2 应成功");
    CK(fabsf(jw_vec_norm_l2(a,4)-1.0f)<1e-5f, "归一化后范数=%f 期望 1", jw_vec_norm_l2(a,4));
    CK(fabsf(a[0]-0.6f)<1e-5f && fabsf(a[1]-0.8f)<1e-5f, "归一化值 %f,%f 期望 0.6,0.8",a[0],a[1]);
    float b[4]={-3.0f,4.0f,0.0f,0.0f};
    CK(jw_vec_normalize_l1(b,4)==JW_SUCCESS, "normalize_l1 应成功");
    CK(fabsf(jw_vec_norm_l1(b,4)-1.0f)<1e-5f, "l1归一化后=%f 期望 1", jw_vec_norm_l1(b,4));

    /* 零向量归一化应失败而非产生 NaN */
    float z[4]={0,0,0,0};
    CK(jw_vec_normalize_l2(z,4)==JW_INVALID_PARAM, "零向量 normalize_l2 应返回 INVALID_PARAM");
    CK(jw_vec_normalize_l1(z,4)==JW_INVALID_PARAM, "零向量 normalize_l1 应返回 INVALID_PARAM");

    /* div: 除零保护 */
    float d1[3]={2.0f,4.0f,6.0f}, dr[3];
    jw_vec_div(d1,3,2.0f,dr);
    CK(dr[0]==1.0f&&dr[1]==2.0f&&dr[2]==3.0f, "div 结果 %f,%f,%f",dr[0],dr[1],dr[2]);
    jw_vec_div(d1,3,0.0f,dr);
    CK(dr[0]==0.0f&&dr[1]==0.0f&&dr[2]==0.0f, "除零应写 0 而非 NaN: %f",dr[0]);

    /* cross: 3D 叉积 */
    float x[3]={1,0,0}, y[3]={0,1,0}, r[3];
    jw_vec_cross(x,y,r);
    CK(r[0]==0&&r[1]==0&&r[2]==1, "cross=%f,%f,%f 期望 0,0,1",r[0],r[1],r[2]);

    /* distance_ip / l1 */
    CK(fabsf(jw_vec_distance_ip(x,y,3)-0.0f)<1e-6f, "ip 距离=%f", jw_vec_distance_ip(x,y,3));
    CK(fabsf(jw_vec_distance_l1(x,y,3)-2.0f)<1e-6f, "l1 距离=%f 期望 2", jw_vec_distance_l1(x,y,3));

    /* copy_data */
    float cd[6]; jw_vec_copy_data(cd,v,6);
    int same=1; for(int i=0;i<6;i++) if(cd[i]!=v[i]) same=0;
    CK(same, "copy_data 应完全一致");

    /* batch */
    jw_arena_t* a2=NULL; jw_arena_create(1UL<<20,&a2);
    jw_vec_batch_t b2; jw_memset(&b2,0,sizeof(b2));
    CK(jw_vec_batch_create(a2,3,4,&b2)==JW_SUCCESS, "batch_create 应成功");
    for(jw_size_t i=0;i<b2.count;i++) for(jw_dim_t j=0;j<3;j++) b2.vectors[i][j]=(float)(i+j);
    float bo[4];
    jw_vec_batch_distance_l2(x,&b2,bo);
    CK(isfinite(bo[0])&&isfinite(bo[3]), "batch_distance_l2 应全部有限");
    jw_vec_batch_normalize(&b2);
    CK(fabsf(jw_vec_norm_l2(b2.vectors[0],3)-1.0f)<1e-4f, "batch_normalize 后范数=%f", jw_vec_norm_l2(b2.vectors[0],3));

    /* simd */
    printf("  SIMD 级别: %s (启用=%d)\n", jw_simd_level_name(jw_simd_detect()), jw_simd_is_enabled());
    CK(jw_simd_is_enabled()==JW_TRUE, "simd_is_enabled 应为 true");
    jw_simd_set_enabled(JW_FALSE);   /* 不应崩 */
    CK(jw_simd_detect()>=JW_SIMD_NONE, "simd_detect 应返回有效等级");

    jw_arena_destroy(a2);
    printf("=== %d 通过, %d 失败 ===\n", pass, fail);
    return fail;
}
