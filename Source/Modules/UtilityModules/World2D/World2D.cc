#include "ikaros.h"

#include <algorithm>
#include <cmath>

using namespace ikaros;

namespace
{
    constexpr float row_type_food = 2.0f;
    constexpr float row_type_aversive = 3.0f;
    constexpr int creature_cols = 9;
    constexpr int object_cols = 10;
    constexpr int wall_cols = 11;

    float wrap_angle(float angle)
    {
        while(angle <= -float(M_PI))
            angle += 2.0f * float(M_PI);
        while(angle > float(M_PI))
            angle -= 2.0f * float(M_PI);
        return angle;
    }
}


class World2D: public Module
{
    parameter world_width_;
    parameter world_height_;
    parameter creature_x0_;
    parameter creature_y0_;
    parameter creature_heading0_;
    parameter creature_radius_;
    parameter wheel_radius_;
    parameter wheel_base_;
    parameter max_motor_rpt_;
    parameter whisker_length_;
    parameter whisker_angle_;
    parameter food_;
    parameter aversive_;
    parameter walls_;

    matrix motor_;
    matrix creature_;
    matrix objects_;
    matrix walls_out_;
    matrix pose_;
    matrix contact_;
    matrix whiskers_;
    matrix reinforcement_;
    matrix reward_;
    matrix punishment_;

    float x_ = 0.0f;
    float y_ = 0.0f;
    float heading_ = 0.0f;

    float width() const { return std::max(1.0f, world_width_.as_float()); }
    float height() const { return std::max(1.0f, world_height_.as_float()); }
    float radius() const { return std::max(0.01f, creature_radius_.as_float()); }
    float whisker_length() const { return std::max(0.0f, whisker_length_.as_float()); }
    float whisker_angle() const { return std::max(0.0f, whisker_angle_.as_float()); }
    bool object_solid(const matrix & rows, int row) const
    {
        if(rows.cols() > 7)
            return rows(row, 7) > 0.5f;
        if(rows.cols() > 6)
            return rows(row, 6) > 0.5f;
        return true;
    }
    float object_value(const matrix & rows, int row, float default_value) const
    {
        return rows.cols() > 7 ? rows(row, 3) : default_value;
    }

    void Init() override
    {
        Bind(world_width_, "world_width");
        Bind(world_height_, "world_height");
        Bind(creature_x0_, "creature_x");
        Bind(creature_y0_, "creature_y");
        Bind(creature_heading0_, "creature_heading");
        Bind(creature_radius_, "creature_radius");
        Bind(wheel_radius_, "wheel_radius");
        Bind(wheel_base_, "wheel_base");
        Bind(max_motor_rpt_, "max_motor_rpt");
        Bind(whisker_length_, "whisker_length");
        Bind(whisker_angle_, "whisker_angle");
        Bind(food_, "food");
        Bind(aversive_, "aversive");
        Bind(walls_, "walls");

        Bind(motor_, "MOTOR");
        Bind(creature_, "CREATURE");
        Bind(objects_, "OBJECTS");
        Bind(walls_out_, "WALLS");
        Bind(pose_, "POSE");
        Bind(contact_, "CONTACT");
        Bind(whiskers_, "WHISKERS");
        Bind(reinforcement_, "REINFORCEMENT");
        Bind(reward_, "REWARD");
        Bind(punishment_, "PUNISHMENT");

        creature_.set_labels(0,
            "x",
            "y",
            "radius",
            "heading",
            "red",
            "green",
            "blue",
            "solid",
            "opaque"
        );
        objects_.set_labels(1,
            "type",
            "id",
            "x",
            "y",
            "radius",
            "value",
            "red",
            "green",
            "blue",
            "solid"
        );
        walls_out_.set_labels(1,
            "id",
            "x1",
            "y1",
            "x2",
            "y2",
            "red",
            "green",
            "blue",
            "solid",
            "line_width",
            "opaque"
        );
        pose_.set_labels(0, "x", "y", "heading");
        contact_.set_labels(0, "food", "aversive");
        whiskers_.set_labels(0, "left", "right");
        reinforcement_.set_labels(0, "value");
        reward_.set_labels(0, "value");
        punishment_.set_labels(0, "value");

        x_ = creature_x0_.as_float();
        y_ = creature_y0_.as_float();
        heading_ = creature_heading0_.as_float();
        clamp_to_world();
        emit_outputs();
    }

    void Tick() override
    {
        step_creature();
        emit_outputs();
    }

    void step_creature()
    {
        float left = 0.0f;
        float right = 0.0f;
        if(!motor_.empty())
        {
            left = motor_.size() > 0 ? motor_(0) : 0.0f;
            right = motor_.size() > 1 ? motor_(1) : left;
        }

        const float max_rpt = std::max(0.0f, max_motor_rpt_.as_float());
        left = std::clamp(left, -max_rpt, max_rpt);
        right = std::clamp(right, -max_rpt, max_rpt);

        const float wheel_radius = std::max(0.001f, wheel_radius_.as_float());
        const float wheel_base = std::max(0.001f, wheel_base_.as_float());
        const float distance_left = 2.0f * float(M_PI) * wheel_radius * left;
        const float distance_right = 2.0f * float(M_PI) * wheel_radius * right;
        const float distance = 0.5f * (distance_left + distance_right);
        const float turn = (distance_right - distance_left) / wheel_base;

        const float old_x = x_;
        const float old_y = y_;
        const float old_penetration = solid_contact_penetration();

        if(std::abs(turn) < 0.000001f)
        {
            x_ += distance * std::cos(heading_);
            y_ += distance * std::sin(heading_);
        }
        else
        {
            const float half_heading = heading_ + 0.5f * turn;
            x_ += distance * std::cos(half_heading);
            y_ += distance * std::sin(half_heading);
            heading_ = wrap_angle(heading_ + turn);
        }

        const float new_penetration = solid_contact_penetration();
        const bool blocked_by_sweep = old_penetration <= 0.00001f && swept_solid_contact(old_x, old_y, x_, y_);
        if(blocked_by_sweep || (new_penetration > 0.0f && new_penetration >= old_penetration))
        {
            x_ = old_x;
            y_ = old_y;
        }

        clamp_to_world();
    }

    void clamp_to_world()
    {
        const float r = radius();
        x_ = std::clamp(x_, r, width() - r);
        y_ = std::clamp(y_, r, height() - r);
        heading_ = wrap_angle(heading_);
    }

    bool collides_with_rows(const matrix & rows, bool solid_only) const
    {
        if(rows.empty() || rows.rank() < 2)
            return false;

        const float r = radius();
        for(int i = 0; i < rows.rows(); ++i)
        {
            if(rows.cols() < 3)
                continue;

            const float object_x = rows(i, 0);
            const float object_y = rows(i, 1);
            const float object_radius = rows.cols() > 2 ? std::max(0.0f, rows(i, 2)) : 0.0f;
            const bool solid = object_solid(rows, i);
            if(solid_only && !solid)
                continue;

            const float dx = x_ - object_x;
            const float dy = y_ - object_y;
            const float rr = r + object_radius;
            if(dx * dx + dy * dy <= rr * rr)
                return true;
        }

        return false;
    }

    bool collides_with_solid_object()
    {
        matrix & objects = aversive_;
        return collides_with_rows(objects, true);
    }

    float row_penetration(const matrix & rows, bool solid_only) const
    {
        if(rows.empty() || rows.rank() < 2)
            return 0.0f;

        float penetration = 0.0f;
        const float r = radius();
        for(int i = 0; i < rows.rows(); ++i)
        {
            if(rows.cols() < 3)
                continue;

            const bool solid = object_solid(rows, i);
            if(solid_only && !solid)
                continue;

            const float dx = x_ - rows(i, 0);
            const float dy = y_ - rows(i, 1);
            const float object_radius = rows.cols() > 2 ? std::max(0.0f, rows(i, 2)) : 0.0f;
            const float overlap = r + object_radius - std::sqrt(dx * dx + dy * dy);
            penetration = std::max(penetration, overlap);
        }

        return std::max(0.0f, penetration);
    }

    float distance_to_segment(float px, float py, float x0, float y0, float x1, float y1) const
    {
        const float vx = x1 - x0;
        const float vy = y1 - y0;
        const float wx = px - x0;
        const float wy = py - y0;
        const float length_squared = vx * vx + vy * vy;
        float t = length_squared > 0.0f ? (wx * vx + wy * vy) / length_squared : 0.0f;
        t = std::clamp(t, 0.0f, 1.0f);
        const float cx = x0 + t * vx;
        const float cy = y0 + t * vy;
        const float dx = px - cx;
        const float dy = py - cy;
        return std::sqrt(dx * dx + dy * dy);
    }

    float cross(float ax, float ay, float bx, float by) const
    {
        return ax * by - ay * bx;
    }

    bool point_on_segment(float px, float py, float x0, float y0, float x1, float y1) const
    {
        constexpr float eps = 0.000001f;
        if(std::abs(cross(x1 - x0, y1 - y0, px - x0, py - y0)) > eps)
            return false;

        return px >= std::min(x0, x1) - eps && px <= std::max(x0, x1) + eps &&
               py >= std::min(y0, y1) - eps && py <= std::max(y0, y1) + eps;
    }

    bool segments_intersect(float ax0, float ay0, float ax1, float ay1, float bx0, float by0, float bx1, float by1) const
    {
        const float a_dx = ax1 - ax0;
        const float a_dy = ay1 - ay0;
        const float b_dx = bx1 - bx0;
        const float b_dy = by1 - by0;
        const float c0 = cross(a_dx, a_dy, bx0 - ax0, by0 - ay0);
        const float c1 = cross(a_dx, a_dy, bx1 - ax0, by1 - ay0);
        const float c2 = cross(b_dx, b_dy, ax0 - bx0, ay0 - by0);
        const float c3 = cross(b_dx, b_dy, ax1 - bx0, ay1 - by0);

        if((c0 > 0.0f && c1 < 0.0f || c0 < 0.0f && c1 > 0.0f) &&
           (c2 > 0.0f && c3 < 0.0f || c2 < 0.0f && c3 > 0.0f))
            return true;

        return point_on_segment(bx0, by0, ax0, ay0, ax1, ay1) ||
               point_on_segment(bx1, by1, ax0, ay0, ax1, ay1) ||
               point_on_segment(ax0, ay0, bx0, by0, bx1, by1) ||
               point_on_segment(ax1, ay1, bx0, by0, bx1, by1);
    }

    float segment_distance(float ax0, float ay0, float ax1, float ay1, float bx0, float by0, float bx1, float by1) const
    {
        if(segments_intersect(ax0, ay0, ax1, ay1, bx0, by0, bx1, by1))
            return 0.0f;

        return std::min({
            distance_to_segment(ax0, ay0, bx0, by0, bx1, by1),
            distance_to_segment(ax1, ay1, bx0, by0, bx1, by1),
            distance_to_segment(bx0, by0, ax0, ay0, ax1, ay1),
            distance_to_segment(bx1, by1, ax0, ay0, ax1, ay1)
        });
    }

    bool swept_solid_object_contact(float old_x, float old_y, float new_x, float new_y) const
    {
        const matrix & objects = aversive_;
        if(objects.empty() || objects.rank() < 2)
            return false;

        constexpr float eps = 0.000001f;
        const float r = radius();
        for(int i = 0; i < objects.rows(); ++i)
        {
            if(objects.cols() < 3 || !object_solid(objects, i))
                continue;

            const float rr = r + std::max(0.0f, objects(i, 2));
            if(distance_to_segment(objects(i, 0), objects(i, 1), old_x, old_y, new_x, new_y) < rr - eps)
                return true;
        }

        return false;
    }

    bool swept_solid_wall_contact(float old_x, float old_y, float new_x, float new_y) const
    {
        const matrix & walls = walls_;
        if(walls.empty() || walls.rank() < 2)
            return false;

        constexpr float eps = 0.000001f;
        const float r = radius();
        for(int i = 0; i < walls.rows(); ++i)
        {
            if(walls.cols() < 4)
                continue;

            const bool solid = walls.cols() <= 7 || walls(i, 7) > 0.5f;
            if(!solid)
                continue;

            const float line_width = walls.cols() > 8 ? std::max(0.0f, walls(i, 8)) : 1.0f;
            const float rr = r + 0.5f * line_width;
            if(segment_distance(old_x, old_y, new_x, new_y, walls(i, 0), walls(i, 1), walls(i, 2), walls(i, 3)) < rr - eps)
                return true;
        }

        return false;
    }

    bool swept_solid_contact(float old_x, float old_y, float new_x, float new_y) const
    {
        return swept_solid_object_contact(old_x, old_y, new_x, new_y) ||
               swept_solid_wall_contact(old_x, old_y, new_x, new_y);
    }

    bool collides_with_wall()
    {
        matrix & walls = walls_;
        if(walls.empty() || walls.rank() < 2)
            return false;

        const float r = radius();
        for(int i = 0; i < walls.rows(); ++i)
        {
            if(walls.cols() < 4)
                continue;

            const bool solid = walls.cols() <= 7 || walls(i, 7) > 0.5f;
            if(!solid)
                continue;

            const float line_width = walls.cols() > 8 ? std::max(0.0f, walls(i, 8)) : 1.0f;
            if(distance_to_segment(x_, y_, walls(i, 0), walls(i, 1), walls(i, 2), walls(i, 3)) <= r + 0.5f * line_width)
                return true;
        }

        return false;
    }

    float wall_penetration() const
    {
        const matrix & walls = walls_;
        if(walls.empty() || walls.rank() < 2)
            return 0.0f;

        float penetration = 0.0f;
        const float r = radius();
        for(int i = 0; i < walls.rows(); ++i)
        {
            if(walls.cols() < 4)
                continue;

            const bool solid = walls.cols() <= 7 || walls(i, 7) > 0.5f;
            if(!solid)
                continue;

            const float line_width = walls.cols() > 8 ? std::max(0.0f, walls(i, 8)) : 1.0f;
            const float distance = distance_to_segment(x_, y_, walls(i, 0), walls(i, 1), walls(i, 2), walls(i, 3));
            penetration = std::max(penetration, r + 0.5f * line_width - distance);
        }

        return std::max(0.0f, penetration);
    }

    float solid_contact_penetration() const
    {
        const matrix & objects = aversive_;
        return std::max(row_penetration(objects, true), wall_penetration());
    }

    bool contacts_food()
    {
        matrix & objects = food_;
        return collides_with_rows(objects, false);
    }

    bool contacts_aversive()
    {
        matrix & objects = aversive_;
        return collides_with_rows(objects, false);
    }

    float reinforcement_from_rows(const matrix & rows, float default_value) const
    {
        if(rows.empty() || rows.rank() < 2)
            return 0.0f;

        float value = 0.0f;
        const float r = radius();
        for(int i = 0; i < rows.rows(); ++i)
        {
            if(rows.cols() < 3)
                continue;

            const float dx = x_ - rows(i, 0);
            const float dy = y_ - rows(i, 1);
            const float object_radius = std::max(0.0f, rows(i, 2));
            const float rr = r + object_radius;
            if(dx * dx + dy * dy <= rr * rr)
                value += object_value(rows, i, default_value);
        }

        return value;
    }

    float reinforcement_value() const
    {
        const matrix & food = food_;
        const matrix & aversive = aversive_;
        return reinforcement_from_rows(food, 1.0f) + reinforcement_from_rows(aversive, -1.0f);
    }

    void scan_solid_circle(float start_x, float start_y, float dx, float dy, float length, float & first_contact) const
    {
        const matrix & objects = aversive_;
        if(objects.empty() || objects.rank() < 2)
            return;

        for(int i = 0; i < objects.rows(); ++i)
        {
            if(objects.cols() < 3)
                continue;

            const bool solid = object_solid(objects, i);
            if(!solid)
                continue;

            const float fx = start_x - objects(i, 0);
            const float fy = start_y - objects(i, 1);
            const float object_radius = std::max(0.0f, objects(i, 2));
            const float c = fx * fx + fy * fy - object_radius * object_radius;
            if(c <= 0.0f)
            {
                first_contact = 0.0f;
                return;
            }

            const float b = 2.0f * (fx * dx + fy * dy);
            const float discriminant = b * b - 4.0f * c;
            if(discriminant < 0.0f)
                continue;

            const float root = std::sqrt(discriminant);
            const float t0 = (-b - root) * 0.5f;
            const float t1 = (-b + root) * 0.5f;
            if(t0 >= 0.0f && t0 <= length)
                first_contact = std::min(first_contact, t0 / length);
            else if(t1 >= 0.0f && t1 <= length)
                first_contact = std::min(first_contact, t1 / length);
        }
    }

    void scan_solid_wall(float start_x, float start_y, float dx, float dy, float length, float & first_contact) const
    {
        const matrix & walls = walls_;
        if(walls.empty() || walls.rank() < 2)
            return;

        constexpr int samples = 80;
        for(int i = 0; i < walls.rows(); ++i)
        {
            if(walls.cols() < 4)
                continue;

            const bool solid = walls.cols() <= 7 || walls(i, 7) > 0.5f;
            if(!solid)
                continue;

            const float half_width = 0.5f * (walls.cols() > 8 ? std::max(0.0f, walls(i, 8)) : 1.0f);
            for(int s = 0; s <= samples; ++s)
            {
                const float fraction = float(s) / float(samples);
                if(fraction >= first_contact)
                    break;

                const float px = start_x + dx * length * fraction;
                const float py = start_y + dy * length * fraction;
                if(distance_to_segment(px, py, walls(i, 0), walls(i, 1), walls(i, 2), walls(i, 3)) <= half_width)
                {
                    first_contact = fraction;
                    break;
                }
            }
        }
    }

    float whisker_signal(float relative_angle) const
    {
        const float length = whisker_length();
        if(length <= 0.0f)
            return 0.0f;

        const float angle = heading_ + relative_angle;
        const float dx = std::cos(angle);
        const float dy = std::sin(angle);
        float first_contact = 1.0f;

        scan_solid_circle(x_, y_, dx, dy, length, first_contact);
        scan_solid_wall(x_, y_, dx, dy, length, first_contact);

        return std::clamp(1.0f - first_contact, 0.0f, 1.0f);
    }

    void append_object_rows(const matrix & objects, float type)
    {
        if(objects.empty() || objects.rank() < 2)
            return;

        for(int i = 0; i < objects.rows(); ++i)
        {
            if(objects.cols() < 3)
                continue;

            const bool has_value = objects.cols() > 7;
            const int color_offset = has_value ? 4 : 3;
            matrix row(object_cols);
            row.set(0.0f);
            row(0) = type;
            row(1) = float(i + 1);
            row(2) = objects(i, 0);
            row(3) = objects(i, 1);
            row(4) = objects(i, 2);
            row(5) = has_value ? objects(i, 3) : (type == row_type_food ? 1.0f : -1.0f);
            row(6) = objects.cols() > color_offset ? objects(i, color_offset) : (type == row_type_food ? 0.22f : 0.85f);
            row(7) = objects.cols() > color_offset + 1 ? objects(i, color_offset + 1) : (type == row_type_food ? 0.68f : 0.20f);
            row(8) = objects.cols() > color_offset + 2 ? objects(i, color_offset + 2) : (type == row_type_food ? 0.26f : 0.18f);
            row(9) = object_solid(objects, i) ? 1.0f : 0.0f;
            objects_.append(row);
        }
    }

    void append_wall_rows(const matrix & walls)
    {
        if(walls.empty() || walls.rank() < 2)
            return;

        for(int i = 0; i < walls.rows(); ++i)
        {
            if(walls.cols() < 4)
                continue;

            matrix row(wall_cols);
            row.set(0.0f);
            row(0) = float(i + 1);
            row(1) = walls(i, 0);
            row(2) = walls(i, 1);
            row(3) = walls(i, 2);
            row(4) = walls(i, 3);
            row(5) = walls.cols() > 4 ? walls(i, 4) : 0.18f;
            row(6) = walls.cols() > 5 ? walls(i, 5) : 0.18f;
            row(7) = walls.cols() > 6 ? walls(i, 6) : 0.18f;
            row(8) = walls.cols() > 7 ? walls(i, 7) : 1.0f;
            row(9) = walls.cols() > 8 ? walls(i, 8) : 2.0f;
            row(10) = walls.cols() > 9 ? walls(i, 9) : 1.0f;
            walls_out_.append(row);
        }
    }

    void emit_outputs()
    {
        objects_.clear();
        walls_out_.clear();

        creature_(0) = x_;
        creature_(1) = y_;
        creature_(2) = radius();
        creature_(3) = heading_;
        creature_(4) = 0.13f;
        creature_(5) = 0.37f;
        creature_(6) = 0.78f;
        creature_(7) = 1.0f;
        creature_(8) = 1.0f;

        matrix & food = food_;
        matrix & aversive = aversive_;
        matrix & walls = walls_;
        append_object_rows(food, row_type_food);
        append_object_rows(aversive, row_type_aversive);
        append_wall_rows(walls);

        pose_(0) = x_;
        pose_(1) = y_;
        pose_(2) = heading_;

        contact_(0) = contacts_food() ? 1.0f : 0.0f;
        contact_(1) = contacts_aversive() ? 1.0f : 0.0f;

        const float angle = whisker_angle();
        whiskers_(0) = whisker_signal(-angle);
        whiskers_(1) = whisker_signal(angle);

        const float reinforcement = reinforcement_value();
        reinforcement_(0) = reinforcement;
        reward_(0) = std::max(0.0f, reinforcement);
        punishment_(0) = std::max(0.0f, -reinforcement);
    }
};

INSTALL_CLASS(World2D)
