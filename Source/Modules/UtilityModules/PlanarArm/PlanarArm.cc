#include "ikaros.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace ikaros;

namespace
{
    constexpr float epsilon = 1.0e-5f;

    struct Point
    {
        float x;
        float y;
    };

    struct Segment
    {
        Point a;
        Point b;
        float radius;
    };


    Point operator+(Point a, Point b) { return {a.x + b.x, a.y + b.y}; }
    Point operator-(Point a, Point b) { return {a.x - b.x, a.y - b.y}; }
    Point operator*(Point p, float scale) { return {p.x * scale, p.y * scale}; }
    float dot(Point a, Point b) { return a.x * b.x + a.y * b.y; }
    float length(Point p) { return std::sqrt(dot(p, p)); }


    Point closest_point(Point p, const Segment & segment)
    {
        const Point direction = segment.b - segment.a;
        const float denominator = dot(direction, direction);
        const float t = denominator <= epsilon ? 0.0f :
            std::clamp(dot(p - segment.a, direction) / denominator, 0.0f, 1.0f);
        return segment.a + direction * t;
    }


    float point_segment_distance(Point p, const Segment & segment)
    {
        return length(p - closest_point(p, segment));
    }


    bool segments_intersect(Point a, Point b, Point c, Point d)
    {
        const auto cross = [](Point u, Point v) { return u.x * v.y - u.y * v.x; };
        const Point r = b - a;
        const Point s = d - c;
        const float denominator = cross(r, s);
        if(std::abs(denominator) <= epsilon)
            return false;
        const float t = cross(c - a, s) / denominator;
        const float u = cross(c - a, r) / denominator;
        return t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f;
    }


    float segment_distance(const Segment & first, const Segment & second)
    {
        if(segments_intersect(first.a, first.b, second.a, second.b))
            return 0.0f;
        return std::min({point_segment_distance(first.a, second),
                         point_segment_distance(first.b, second),
                         point_segment_distance(second.a, first),
                         point_segment_distance(second.b, first)});
    }
}


class PlanarArm: public Module
{
    parameter basePosition_;
    parameter linkLengths_;
    parameter linkWidth_;
    parameter initialJointAngles_;
    parameter jointMin_;
    parameter jointMax_;
    parameter maxJointStep_;
    parameter initialGripper_;
    parameter palmLength_;
    parameter fingerLength_;
    parameter gripperWidth_;
    parameter fingerOpenAngle_;
    parameter fingerClosedAngle_;
    parameter movableObjectParameter_;
    parameter circleObstacleCount_;
    parameter circleObstacles_;
    parameter lineObstacleCount_;
    parameter lineObstacles_;
    parameter worldWidth_;
    parameter worldHeight_;
    parameter solverIterations_;

    matrix jointCommand_;
    matrix gripperCommand_;
    matrix reset_;
    matrix jointAngles_;
    matrix jointPositions_;
    matrix endEffector_;
    matrix movableObject_;
    matrix linkContacts_;
    matrix gripperContacts_;
    matrix objectContacts_;
    matrix blocked_;
    matrix enclosed_;
    matrix armLines_;
    matrix objects_;
    matrix walls_;

    std::vector<float> angles_;
    std::vector<Segment> segments_;
    Point objectPosition_{};
    float objectRadius_ = 0.0f;
    float gripper_ = 1.0f;


    void validateVector(const matrix & values, int size, const std::string & name)
    {
        if(values.size() != size)
            throw exception("PlanarArm: " + name + " must contain one value per arm link.", path_);
    }


    float obstacleValue(const matrix & values, int row, int column) const
    {
        return values.rank() == 1 ? values(column) : values(row, column);
    }


    std::vector<Segment> geometry(const std::vector<float> & angles, float gripper) const
    {
        const matrix base = basePosition_.as_matrix();
        const matrix lengths = linkLengths_.as_matrix();
        std::vector<Segment> result;
        result.reserve(angles.size() + 3);
        Point cursor{base(0), base(1)};
        float orientation = 0.0f;
        for(int i = 0; i < int(angles.size()); ++i)
        {
            orientation += angles[i];
            const Point next{cursor.x + lengths(i) * std::cos(orientation),
                             cursor.y + lengths(i) * std::sin(orientation)};
            result.push_back({cursor, next, 0.5f * linkWidth_.as_float()});
            cursor = next;
        }

        const Point forward{std::cos(orientation), std::sin(orientation)};
        const Point perpendicular{-forward.y, forward.x};
        const float halfPalm = 0.5f * palmLength_.as_float();
        const Point left = cursor + perpendicular * halfPalm;
        const Point right = cursor - perpendicular * halfPalm;
        const float radius = 0.5f * gripperWidth_.as_float();
        result.push_back({left, right, radius});

        const float fingerAngle = -fingerClosedAngle_.as_float() +
            std::clamp(gripper, 0.0f, 1.0f) *
            (fingerClosedAngle_.as_float() + fingerOpenAngle_.as_float());
        const Point leftDirection{std::cos(orientation + fingerAngle),
                                  std::sin(orientation + fingerAngle)};
        const Point rightDirection{std::cos(orientation - fingerAngle),
                                   std::sin(orientation - fingerAngle)};
        result.push_back({left, left + leftDirection * fingerLength_.as_float(), radius});
        result.push_back({right, right + rightDirection * fingerLength_.as_float(), radius});
        return result;
    }


    bool armHitsFixed(const std::vector<Segment> & geometry) const
    {
        const matrix circles = circleObstacles_.as_matrix();
        const matrix lines = lineObstacles_.as_matrix();
        for(const Segment & segment: geometry)
        {
            if(std::min({segment.a.x, segment.b.x}) < segment.radius ||
               std::max({segment.a.x, segment.b.x}) > worldWidth_.as_float() - segment.radius ||
               std::min({segment.a.y, segment.b.y}) < segment.radius ||
               std::max({segment.a.y, segment.b.y}) > worldHeight_.as_float() - segment.radius)
                return true;
            for(int row = 0; row < circleObstacleCount_.as_int(); ++row)
                if(point_segment_distance({obstacleValue(circles, row, 0),
                                           obstacleValue(circles, row, 1)}, segment) <
                   obstacleValue(circles, row, 2) + segment.radius - epsilon)
                    return true;
            for(int row = 0; row < lineObstacleCount_.as_int(); ++row)
            {
                const Segment obstacle{{obstacleValue(lines, row, 0), obstacleValue(lines, row, 1)},
                                       {obstacleValue(lines, row, 2), obstacleValue(lines, row, 3)},
                                       0.5f * obstacleValue(lines, row, 4)};
                if(segment_distance(segment, obstacle) < segment.radius + obstacle.radius - epsilon)
                    return true;
            }
        }
        return false;
    }


    bool projectFromSegment(Point & point, float radius, const Segment & segment,
                            Point fallback) const
    {
        const Point closest = closest_point(point, segment);
        Point normal = point - closest;
        float distance = length(normal);
        const float minimum = radius + segment.radius;
        if(distance >= minimum - epsilon)
            return false;
        if(distance <= epsilon)
        {
            normal = fallback;
            distance = length(normal);
            if(distance <= epsilon)
            {
                const Point direction = segment.b - segment.a;
                normal = {-direction.y, direction.x};
                distance = length(normal);
            }
        }
        point = point + normal * ((minimum - distance) / distance);
        return true;
    }


    bool touchesSegment(Point point, float radius, const Segment & segment) const
    {
        constexpr float contactTolerance = 1.0e-3f;
        return point_segment_distance(point, segment) <=
               radius + segment.radius + contactTolerance;
    }


    bool solveObject(const std::vector<Segment> & oldGeometry,
                     const std::vector<Segment> & newGeometry, Point & point)
    {
        objectContacts_.reset();
        linkContacts_.reset();
        gripperContacts_.reset();
        const matrix circles = circleObstacles_.as_matrix();
        const matrix lines = lineObstacles_.as_matrix();
        const int iterations = std::max(1, solverIterations_.as_int());
        for(int iteration = 0; iteration < iterations; ++iteration)
        {
            bool changed = false;
            for(int index = 0; index < int(newGeometry.size()); ++index)
            {
                const Point oldMiddle = (oldGeometry[index].a + oldGeometry[index].b) * 0.5f;
                const Point newMiddle = (newGeometry[index].a + newGeometry[index].b) * 0.5f;
                if(projectFromSegment(point, objectRadius_, newGeometry[index], newMiddle - oldMiddle))
                {
                    changed = true;
                    objectContacts_(0) = 1.0f;
                    if(index < int(angles_.size()))
                        linkContacts_(index) = 1.0f;
                    else
                        gripperContacts_(index - int(angles_.size())) = 1.0f;
                }
            }
            for(int row = 0; row < circleObstacleCount_.as_int(); ++row)
            {
                const Point center{obstacleValue(circles, row, 0),
                                   obstacleValue(circles, row, 1)};
                Point normal = point - center;
                float distance = length(normal);
                const float minimum = objectRadius_ + obstacleValue(circles, row, 2);
                if(distance < minimum - epsilon)
                {
                    if(distance <= epsilon)
                    {
                        normal = {1.0f, 0.0f};
                        distance = 1.0f;
                    }
                    point = point + normal * ((minimum - distance) / distance);
                    objectContacts_(1) = 1.0f;
                    changed = true;
                }
            }
            for(int row = 0; row < lineObstacleCount_.as_int(); ++row)
            {
                const Segment obstacle{{obstacleValue(lines, row, 0), obstacleValue(lines, row, 1)},
                                       {obstacleValue(lines, row, 2), obstacleValue(lines, row, 3)},
                                       0.5f * obstacleValue(lines, row, 4)};
                if(projectFromSegment(point, objectRadius_, obstacle, {1.0f, 0.0f}))
                {
                    objectContacts_(2) = 1.0f;
                    changed = true;
                }
            }
            const Point before = point;
            point.x = std::clamp(point.x, objectRadius_, worldWidth_.as_float() - objectRadius_);
            point.y = std::clamp(point.y, objectRadius_, worldHeight_.as_float() - objectRadius_);
            if(length(point - before) > epsilon)
            {
                objectContacts_(3) = 1.0f;
                changed = true;
            }
            if(!changed)
                break;
        }

        const float armValidationTolerance = std::max(1.0e-3f, 0.025f * objectRadius_);
        for(int index = 0; index < int(newGeometry.size()); ++index)
            if(point_segment_distance(point, newGeometry[index]) <
               objectRadius_ + newGeometry[index].radius - armValidationTolerance)
                return false;
        constexpr float fixedValidationTolerance = 1.0e-3f;
        for(int row = 0; row < circleObstacleCount_.as_int(); ++row)
            if(length(point - Point{obstacleValue(circles, row, 0),
                                    obstacleValue(circles, row, 1)}) <
               objectRadius_ + obstacleValue(circles, row, 2) - fixedValidationTolerance)
                return false;
        for(int row = 0; row < lineObstacleCount_.as_int(); ++row)
        {
            const Segment obstacle{{obstacleValue(lines, row, 0), obstacleValue(lines, row, 1)},
                                   {obstacleValue(lines, row, 2), obstacleValue(lines, row, 3)},
                                   0.5f * obstacleValue(lines, row, 4)};
            if(point_segment_distance(point, obstacle) <
               objectRadius_ + obstacle.radius - fixedValidationTolerance)
                return false;
        }
        return point.x >= objectRadius_ - epsilon &&
               point.x <= worldWidth_.as_float() - objectRadius_ + epsilon &&
               point.y >= objectRadius_ - epsilon &&
               point.y <= worldHeight_.as_float() - objectRadius_ + epsilon;
    }


    void refreshContactSensors()
    {
        linkContacts_.reset();
        gripperContacts_.reset();
        objectContacts_.reset();
        constexpr float contactTolerance = 1.0e-3f;
        for(int index = 0; index < int(segments_.size()); ++index)
        {
            if(!touchesSegment(objectPosition_, objectRadius_, segments_[index]))
                continue;
            objectContacts_(0) = 1.0f;
            if(index < int(angles_.size()))
                linkContacts_(index) = 1.0f;
            else
                gripperContacts_(index - int(angles_.size())) = 1.0f;
        }

        const matrix circles = circleObstacles_.as_matrix();
        for(int row = 0; row < circleObstacleCount_.as_int(); ++row)
            if(length(objectPosition_ - Point{obstacleValue(circles, row, 0),
                                              obstacleValue(circles, row, 1)}) <=
               objectRadius_ + obstacleValue(circles, row, 2) + contactTolerance)
                objectContacts_(1) = 1.0f;

        const matrix lines = lineObstacles_.as_matrix();
        for(int row = 0; row < lineObstacleCount_.as_int(); ++row)
        {
            const Segment obstacle{{obstacleValue(lines, row, 0), obstacleValue(lines, row, 1)},
                                   {obstacleValue(lines, row, 2), obstacleValue(lines, row, 3)},
                                   0.5f * obstacleValue(lines, row, 4)};
            if(point_segment_distance(objectPosition_, obstacle) <=
               objectRadius_ + obstacle.radius + contactTolerance)
                objectContacts_(2) = 1.0f;
        }
        if(objectPosition_.x <= objectRadius_ + contactTolerance ||
           objectPosition_.x >= worldWidth_.as_float() - objectRadius_ - contactTolerance ||
           objectPosition_.y <= objectRadius_ + contactTolerance ||
           objectPosition_.y >= worldHeight_.as_float() - objectRadius_ - contactTolerance)
            objectContacts_(3) = 1.0f;
    }


    void resetState()
    {
        const matrix initialAngles = initialJointAngles_.as_matrix();
        for(int i = 0; i < int(angles_.size()); ++i)
            angles_[i] = std::clamp(initialAngles(i), jointMin_.as_matrix()(i), jointMax_.as_matrix()(i));
        gripper_ = std::clamp(initialGripper_.as_float(), 0.0f, 1.0f);
        const matrix object = movableObjectParameter_.as_matrix();
        objectPosition_ = {object(0), object(1)};
        objectRadius_ = object(2);
        segments_ = geometry(angles_, gripper_);
        blocked_(0) = armHitsFixed(segments_) ? 1.0f : 0.0f;
        if(blocked_(0) < 0.5f)
        {
            Point resolvedObject = objectPosition_;
            if(solveObject(segments_, segments_, resolvedObject))
                objectPosition_ = resolvedObject;
            else
                blocked_(0) = 1.0f;
        }
    }


    void emitLine(matrix & destination, int row, int id, const Segment & segment,
                  float red, float green, float blue)
    {
        destination(row, 0) = float(id);
        destination(row, 1) = 1.0f;
        destination(row, 2) = segment.a.x;
        destination(row, 3) = segment.a.y;
        destination(row, 4) = segment.b.x;
        destination(row, 5) = segment.b.y;
        destination(row, 6) = red;
        destination(row, 7) = green;
        destination(row, 8) = blue;
        destination(row, 9) = 1.0f;
        destination(row, 10) = 2.0f * segment.radius;
    }


    void emitOutputs()
    {
        const int linkCount = int(angles_.size());
        jointAngles_.reset();
        jointPositions_.reset();
        armLines_.reset();
        objects_.reset();
        walls_.reset();
        for(int i = 0; i < linkCount; ++i)
        {
            jointAngles_(i) = angles_[i];
            jointPositions_(i, 0) = segments_[i].a.x;
            jointPositions_(i, 1) = segments_[i].a.y;
        }
        jointPositions_(linkCount, 0) = segments_[linkCount - 1].b.x;
        jointPositions_(linkCount, 1) = segments_[linkCount - 1].b.y;
        float orientation = 0.0f;
        for(float angle: angles_)
            orientation += angle;
        endEffector_(0) = segments_[linkCount - 1].b.x;
        endEffector_(1) = segments_[linkCount - 1].b.y;
        endEffector_(2) = orientation;

        const int gripperContactCount = int(gripperContacts_(0) > 0.5f) +
                                        int(gripperContacts_(1) > 0.5f) +
                                        int(gripperContacts_(2) > 0.5f);
        const bool enclosed = gripperContactCount >= 2;
        enclosed_(0) = enclosed ? 1.0f : 0.0f;
        movableObject_(0) = objectPosition_.x;
        movableObject_(1) = objectPosition_.y;
        movableObject_(2) = objectRadius_;
        movableObject_(3) = enclosed_(0);

        for(int row = 0; row < int(segments_.size()); ++row)
        {
            const bool gripperSegment = row >= linkCount;
            emitLine(armLines_, row, row + 1, segments_[row],
                     gripperSegment ? 0.15f : 0.12f,
                     gripperSegment ? 0.55f : 0.35f,
                     gripperSegment ? 0.95f : 0.75f);
            emitLine(walls_, row, row + 1, segments_[row],
                     gripperSegment ? 0.15f : 0.12f,
                     gripperSegment ? 0.55f : 0.35f,
                     gripperSegment ? 0.95f : 0.75f);
        }

        objects_(0, 0) = 1.0f;
        objects_(0, 1) = 1.0f;
        objects_(0, 2) = objectPosition_.x;
        objects_(0, 3) = objectPosition_.y;
        objects_(0, 4) = objectRadius_;
        objects_(0, 5) = 0.0f;
        objects_(0, 6) = enclosed ? 0.20f : 0.95f;
        objects_(0, 7) = enclosed ? 0.85f : 0.55f;
        objects_(0, 8) = 0.18f;
        objects_(0, 9) = 1.0f;

        const matrix circles = circleObstacles_.as_matrix();
        for(int row = 0; row < circleObstacleCount_.as_int(); ++row)
        {
            objects_(row + 1, 0) = float(row + 2);
            objects_(row + 1, 1) = 3.0f;
            objects_(row + 1, 2) = obstacleValue(circles, row, 0);
            objects_(row + 1, 3) = obstacleValue(circles, row, 1);
            objects_(row + 1, 4) = obstacleValue(circles, row, 2);
            objects_(row + 1, 6) = 0.42f;
            objects_(row + 1, 7) = 0.42f;
            objects_(row + 1, 8) = 0.46f;
            objects_(row + 1, 9) = 1.0f;
        }

        const matrix lines = lineObstacles_.as_matrix();
        for(int row = 0; row < lineObstacleCount_.as_int(); ++row)
        {
            const Segment segment{{obstacleValue(lines, row, 0), obstacleValue(lines, row, 1)},
                                  {obstacleValue(lines, row, 2), obstacleValue(lines, row, 3)},
                                  0.5f * obstacleValue(lines, row, 4)};
            emitLine(walls_, int(segments_.size()) + row,
                     int(segments_.size()) + row + 1, segment, 0.25f, 0.25f, 0.27f);
        }
    }


    void Init() override
    {
        Bind(basePosition_, "base_position");
        Bind(linkLengths_, "link_lengths");
        Bind(linkWidth_, "link_width");
        Bind(initialJointAngles_, "initial_joint_angles");
        Bind(jointMin_, "joint_min");
        Bind(jointMax_, "joint_max");
        Bind(maxJointStep_, "max_joint_step");
        Bind(initialGripper_, "initial_gripper");
        Bind(palmLength_, "palm_length");
        Bind(fingerLength_, "finger_length");
        Bind(gripperWidth_, "gripper_width");
        Bind(fingerOpenAngle_, "finger_open_angle");
        Bind(fingerClosedAngle_, "finger_closed_angle");
        Bind(movableObjectParameter_, "movable_object");
        Bind(circleObstacleCount_, "circle_obstacle_count");
        Bind(circleObstacles_, "circle_obstacles");
        Bind(lineObstacleCount_, "line_obstacle_count");
        Bind(lineObstacles_, "line_obstacles");
        Bind(worldWidth_, "world_width");
        Bind(worldHeight_, "world_height");
        Bind(solverIterations_, "solver_iterations");
        Bind(jointCommand_, "JOINT_COMMAND");
        Bind(gripperCommand_, "GRIPPER");
        Bind(reset_, "RESET");
        Bind(jointAngles_, "JOINT_ANGLES");
        Bind(jointPositions_, "JOINT_POSITIONS");
        Bind(endEffector_, "END_EFFECTOR");
        Bind(movableObject_, "MOVABLE_OBJECT");
        Bind(linkContacts_, "LINK_CONTACTS");
        Bind(gripperContacts_, "GRIPPER_CONTACTS");
        Bind(objectContacts_, "OBJECT_CONTACTS");
        Bind(blocked_, "BLOCKED");
        Bind(enclosed_, "ENCLOSED");
        Bind(armLines_, "ARM_LINES");
        Bind(objects_, "OBJECTS");
        Bind(walls_, "WALLS");

        const matrix lengths = linkLengths_.as_matrix();
        if(lengths.size() < 1)
            throw exception("PlanarArm: link_lengths must contain at least one link.", path_);
        validateVector(initialJointAngles_.as_matrix(), lengths.size(), "initial_joint_angles");
        validateVector(jointMin_.as_matrix(), lengths.size(), "joint_min");
        validateVector(jointMax_.as_matrix(), lengths.size(), "joint_max");
        if(basePosition_.as_matrix().size() != 2)
            throw exception("PlanarArm: base_position must contain x and y.", path_);
        if(movableObjectParameter_.as_matrix().size() != 3 || movableObjectParameter_.as_matrix()(2) <= 0.0f)
            throw exception("PlanarArm: movable_object must contain x, y, and a positive radius.", path_);
        const matrix circles = circleObstacles_.as_matrix();
        if(circleObstacleCount_.as_int() < 0 ||
           (circleObstacleCount_.as_int() > 0 && circles.size() != 3 * circleObstacleCount_.as_int()))
            throw exception("PlanarArm: circle_obstacles must contain circle_obstacle_count x,y,radius rows.", path_);
        const matrix lines = lineObstacles_.as_matrix();
        if(lineObstacleCount_.as_int() < 0 ||
           (lineObstacleCount_.as_int() > 0 && lines.size() != 5 * lineObstacleCount_.as_int()))
            throw exception("PlanarArm: line_obstacles must contain line_obstacle_count x1,y1,x2,y2,width rows.", path_);
        for(int i = 0; i < lengths.size(); ++i)
            if(lengths(i) <= 0.0f || jointMin_.as_matrix()(i) > jointMax_.as_matrix()(i))
                throw exception("PlanarArm: link lengths must be positive and joint limits ordered.", path_);
        if(maxJointStep_.as_float() <= 0.0f || palmLength_.as_float() <= 0.0f ||
           fingerLength_.as_float() <= 0.0f || worldWidth_.as_float() <= 0.0f ||
           worldHeight_.as_float() <= 0.0f)
            throw exception("PlanarArm: dimensions and max_joint_step must be positive.", path_);

        const int linkCount = lengths.size();
        if(jointAngles_.size() != linkCount ||
           jointPositions_.rank() != 2 || jointPositions_.rows() != linkCount + 1 || jointPositions_.cols() != 2 ||
           armLines_.rank() != 2 || armLines_.rows() != linkCount + 3 || armLines_.cols() != 11 ||
           objects_.rank() != 2 || objects_.rows() != circleObstacleCount_.as_int() + 1 || objects_.cols() != 18 ||
           walls_.rank() != 2 || walls_.rows() != linkCount + 3 + lineObstacleCount_.as_int() || walls_.cols() != 11)
            throw exception("PlanarArm: output shapes were not resolved from the configured geometry.", path_);

        angles_.resize(lengths.size());
        resetState();
        refreshContactSensors();
        emitOutputs();
    }


    void Tick() override
    {
        if(reset_.connected() && reset_(0) > 0.5f)
        {
            resetState();
            refreshContactSensors();
            emitOutputs();
            return;
        }

        std::vector<float> target = angles_;
        const matrix minimum = jointMin_.as_matrix();
        const matrix maximum = jointMax_.as_matrix();
        if(jointCommand_.connected())
            for(int i = 0; i < int(target.size()); ++i)
                target[i] = std::clamp(jointCommand_(i), minimum(i), maximum(i));
        const float targetGripper = gripperCommand_.connected() ?
            std::clamp(gripperCommand_(0), 0.0f, 1.0f) : gripper_;

        float largestChange = std::abs(targetGripper - gripper_);
        for(int i = 0; i < int(target.size()); ++i)
            largestChange = std::max(largestChange, std::abs(target[i] - angles_[i]));
        const int steps = std::max(1, int(std::ceil(largestChange / maxJointStep_.as_float())));
        const std::vector<float> startAngles = angles_;
        const float startGripper = gripper_;
        blocked_(0) = 0.0f;
        linkContacts_.reset();
        gripperContacts_.reset();
        objectContacts_.reset();

        for(int step = 1; step <= steps; ++step)
        {
            const float fraction = float(step) / float(steps);
            std::vector<float> candidateAngles(target.size());
            for(int i = 0; i < int(target.size()); ++i)
                candidateAngles[i] = startAngles[i] + fraction * (target[i] - startAngles[i]);
            const float candidateGripper = startGripper + fraction * (targetGripper - startGripper);
            const std::vector<Segment> candidateGeometry = geometry(candidateAngles, candidateGripper);
            if(armHitsFixed(candidateGeometry))
            {
                blocked_(0) = 1.0f;
                break;
            }

            Point candidateObject = objectPosition_;
            if(!solveObject(segments_, candidateGeometry, candidateObject))
            {
                blocked_(0) = 1.0f;
                break;
            }
            angles_ = candidateAngles;
            gripper_ = candidateGripper;
            segments_ = candidateGeometry;
            objectPosition_ = candidateObject;
        }
        refreshContactSensors();
        emitOutputs();
    }
};

INSTALL_CLASS(PlanarArm)
