#pragma once

#include <glm/glm/glm.hpp>

#include <tuple>
#include <vector>

#include "Utility/Hash.h"

namespace util
{

// NearestPointFinder finds the nearest point to a query point.
// Points are inserted into the finder to allow it to build an
// optimized data structure for performing the search.  Currently
// implemented as a trivial exhaustive search.
template <typename Point, typename PointContainer>
class NearestPointFinder
{
public:
    NearestPointFinder(PointContainer const & pointContainer, Point minCorner, Point maxCorner)
    : pointContainer(pointContainer), minCorner(minCorner), maxCorner(maxCorner)
    {
    }

    void AddPoint(int index)
    {
        pointIndices.push_back(index);
    }

    std::tuple<int, float> FindNearestPoint(Point point)
    {
        float nearestDistance = glm::distance(minCorner, maxCorner);

        int nearestIndex = -1;

        for (int pointIndex : pointIndices)
        {
            if (float distance = glm::distance(point, pointContainer[pointIndex]); distance < nearestDistance)
            {
                nearestIndex = pointIndex;
                nearestDistance = distance;
            }
        }

        return {nearestIndex, nearestDistance};
    }

private:
    std::vector<int> pointIndices;
    PointContainer const & pointContainer;
    Point minCorner{0}, maxCorner{1};
};

class LowDiscrepancyBlueNoiseGenerator
{
public:
    LowDiscrepancyBlueNoiseGenerator(int seedParam) : nearestPointFinder(points, glm::vec2{0}, glm::vec2{1})
    {
        seed = int(FNV1a(seedParam));
    }

    // Algorithm:  First, add one random point.  For the remainder of the desired count,
    // generate 30 random points and find the one that is furthest from any point in the
    // set.  Add that one to the set and repeat.
    void GeneratePoints(size_t count)
    {
        points.emplace_back(glm::vec2{Random(seed++), Random(seed++)});
        nearestPointFinder.AddPoint(0);

        for (int i = 0; i < count - 1; ++i)
        {
            float furthestDistance = 0;
            glm::vec2 furthestCandidate;

            for (int candidateIndex = 0; candidateIndex < 30; ++candidateIndex)
            {
                glm::vec2 candidate = {Random(seed++), Random(seed++)};

                auto [nearestCandidateIndex, distance] = nearestPointFinder.FindNearestPoint(candidate);
                if (distance > furthestDistance)
                {
                    furthestCandidate = candidate;
                    furthestDistance = distance;
                }
            }

            points.push_back(furthestCandidate);

            nearestPointFinder.AddPoint(int(points.size() - 1));
        }
    }

    const std::vector<glm::vec2> & GetPoints() {return points;}

private:
    NearestPointFinder<glm::vec2, std::vector<glm::vec2>> nearestPointFinder;

    std::vector<glm::vec2> points;

    int seed = 0;

    static float Random(uint32_t seed)
    {
        return float(FNV1a(FNV1a(seed))) / float(std::numeric_limits<uint64_t>::max());
    }
};

} // namespace util.
