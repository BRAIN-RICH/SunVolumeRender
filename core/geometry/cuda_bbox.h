//
// Created by 孙万捷 on 16/5/19.
//

#ifndef SUNVOLUMERENDER_CUDA_BBOX_H
#define SUNVOLUMERENDER_CUDA_BBOX_H

#define GLM_FORCE_INLINE
#include <glm/glm.hpp>

#include <cuda_runtime.h>

#include "../cuda_ray.h"

class cudaBBox
{
public:
    __host__ __device__ cudaBBox() {}

    __host__ cudaBBox(const glm::vec3& vmin, const glm::vec3& vmax)
    {
        Set(vmin, vmax);
    }

    __host__ void Set(const glm::vec3& vmin, const glm::vec3& vmax)
    {
        this->vmin = vmin;
        this->vmax = vmax;

        invSize = 1.f / (vmax - vmin);
    }

    __device__ bool Intersect(const cudaRay& ray, float* tNear, float* tFar) const
    {
        auto invDir = 1.f / ray.dir;
        auto tbot = invDir * (vmin - ray.orig);
        auto ttop = invDir * (vmax - ray.orig);

        auto tmin = glm::min(tbot, ttop);
        auto tmax = glm::max(tbot, ttop);

        float largest_tmin = fmaxf(tmin.x, fmaxf(tmin.y, tmin.z));
        float smallest_tmax = fminf(tmax.x, fminf(tmax.y, tmax.z));

        *tNear = largest_tmin;
        *tFar = smallest_tmax;

        return smallest_tmax > largest_tmin;
    }

    __device__ bool IsInside(const glm::vec3& ptInWorld) const
    {
        if(ptInWorld.x > vmax.x || ptInWorld.y > vmax.y || ptInWorld.z > vmax.z)
            return false;
        else if(ptInWorld.x < vmin.x || ptInWorld.y < vmin.y || ptInWorld.z < vmin.z)
            return false;
        else
            return true;
    }

public:
    glm::vec3 vmin;
    glm::vec3 vmax;
    glm::vec3 invSize;
};

#endif //SUNVOLUMERENDER_CUDA_BBOX_H
