#pragma once
#include <cmath>
#include <numbers>

namespace MathUtils
{

    static constexpr float DEG2RAD = (std::numbers::pi_v<float> / 180.0f);

    static inline float degreesToRadians(float angle)
    {
        return angle * DEG2RAD;
    }

    static inline float decidegresToRadians(float angle)
    {
        return (angle / 10.0f) * DEG2RAD;
    }

    struct vector3D
    {
        float x;
        float y;
        float z;
    };

    struct eulerAngles
    {
        float roll;
        float pitch;
        float yaw;
    };

    struct quaternion
    {
        float w;
        float x;
        float y;
        float z;
    };

    // Convert Euler angles (in degrees) to Quaternion 
    static quaternion computeQuaternionFromEuler(const eulerAngles &euler)
    {
        quaternion quat;

        eulerAngles eulerCopy = euler;
        if (eulerCopy.roll > 180.0f)
            eulerCopy.roll -= 360.0f;
        if (eulerCopy.pitch > 180.0f)
            eulerCopy.pitch -= 360.0f;
        if (eulerCopy.yaw > 180.0f)
            eulerCopy.yaw -= 360.0f;

        const float cosRoll = cosf(degreesToRadians(eulerCopy.roll) * 0.5f);
        const float sinRoll = sinf(degreesToRadians(eulerCopy.roll) * 0.5f);

        const float cosPitch = cosf(degreesToRadians(eulerCopy.pitch) * 0.5f);
        const float sinPitch = sinf(degreesToRadians(eulerCopy.pitch) * 0.5f);

        const float cosYaw = cosf(degreesToRadians(-eulerCopy.yaw) * 0.5f);
        const float sinYaw = sinf(degreesToRadians(-eulerCopy.yaw) * 0.5f);

        quat.w = cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw;
        quat.x = sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw;
        quat.y = cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw;
        quat.z = cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw;

        return quat;
    }

    static quaternion quaternionConjugate(const quaternion &qaut)
    {
        quaternion result;
        result.w = qaut.w;
        result.x = -qaut.x;
        result.y = -qaut.y;
        result.z = -qaut.z;
        return result;
    }

    static quaternion quaternionMultiply(const quaternion &a, const quaternion &b)
    {
        quaternion result;

        result.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
        result.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
        result.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
        result.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;

        return result;
    }

    static vector3D quaternionRotateVector(const vector3D &vect, const quaternion &quat)
    {
        vector3D result;
        quaternion refConj;
        quaternion vectQuat;

        vectQuat.w = 0;
        vectQuat.x = vect.x;
        vectQuat.y = vect.y;
        vectQuat.z = vect.z;

        refConj = quaternionConjugate(quat);
        vectQuat = quaternionMultiply(refConj, vectQuat); // conj(q) * v
        vectQuat = quaternionMultiply(vectQuat, quat);    // (conj(q) * v) * q

        result.x = vectQuat.x;
        result.y = vectQuat.y;
        result.z = vectQuat.z;
        return result;
    }

    static vector3D transformVectorEarthToBody(const vector3D &vector, const quaternion &quat, bool nedToNeu = true)
    {

        vector3D modifiedV = vector;
        // HACK: This is needed to correctly transform from NED (sensor frame) to NEU (navigation)
        if (nedToNeu)
        {
            modifiedV.y = -modifiedV.y;
        }

        // From earth frame to body frame
        return quaternionRotateVector(modifiedV, quat);
    }

    static float LatDistanceM(double lat1, double lon1, double elev1, double lat2, double lon2, double elev2)
    {
        double dist;
        dist = sin(degreesToRadians(lat1)) * sin(degreesToRadians(lat2)) + cos(degreesToRadians(lat1)) * cos(degreesToRadians(lat2)) * cos(degreesToRadians(lon1 - lon2));
        dist = acos(dist);
        // dist = (6371 * pi * dist) / 180;
        // got dist in radian, no need to change back to degree and convert to rad again.
        dist = 6371000 * dist;
        double dh = elev1 - elev2;
        dist = sqrt(dist * dist + dh * dh);
        return (float)dist;
    }

    static int SmallestPowerOfTwo(int value, int minValue)
    {
        if (value < minValue)
        {
            return minValue;
        }

        if (value < 2)
        {
            return 2;
        }

        int powerOfTwo = 1;
        while (powerOfTwo < value)
        {
            powerOfTwo <<= 1;
        }

        return powerOfTwo;
    }

} // namespace MathUtils