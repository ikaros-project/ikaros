#include "ikaros.h"

#include <algorithm>
#include <cmath>

using namespace ikaros;

namespace
{
    constexpr float row_type_food = 2.0f;
    constexpr float row_type_aversive = 3.0f;
    constexpr int creature_cols = 9;
    constexpr int object_cols = 18;
    constexpr int object_parameter_cols = 17;
    constexpr int wall_cols = 11;
    constexpr int wall_parameter_cols = 11;

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
    parameter odour_decay_;
    parameter edit_tool_;
    parameter edit_type_;
    parameter edit_value_;
    parameter edit_opaque_;
    parameter edit_smell_;
    parameter edit_color_;
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
    matrix odour_;
    matrix reinforcement_;
    matrix reward_;
    matrix punishment_;
    matrix edit_object_enable_;
    matrix edit_wall_enable_;
    matrix edit_color_enable_;

    float x_ = 0.0f;
    float y_ = 0.0f;
    float heading_ = 0.0f;
    int selected_object_id_ = -1;
    float selected_object_type_ = 0.0f;
    int selected_color_kind_ = 0;
    int selected_color_id_ = -1;
    float selected_color_type_ = 0.0f;
    int last_edit_tool_ = -1;

    float width() const { return std::max(1.0f, world_width_.as_float()); }
    float height() const { return std::max(1.0f, world_height_.as_float()); }
    float radius() const { return std::max(0.01f, creature_radius_.as_float()); }
    float whisker_length() const { return std::max(0.0f, whisker_length_.as_float()); }
    float whisker_angle() const { return std::max(0.0f, whisker_angle_.as_float()); }
    float odour_decay() const { return std::max(0.0f, odour_decay_.as_float()); }
    float clamp_x(float x) const { return std::clamp(x, 0.0f, width()); }
    float clamp_y(float y) const { return std::clamp(y, 0.0f, height()); }
    matrix default_wall_color() const
    {
        matrix color(3);
        color(0) = 0.18f;
        color(1) = 0.18f;
        color(2) = 0.18f;
        return color;
    }
    bool has_object_geometry(const matrix & rows) const { return rows.cols() >= 17; }
    bool has_wall_geometry(const matrix & rows) const { return rows.cols() >= 11; }
    int find_object_row_by_id(const matrix & rows, int id) const
    {
        for(int row = 0; row < rows.rows(); ++row)
            if(int(rows(row, 0)) == id)
                return row;

        return -1;
    }
    int find_wall_row_by_id(const matrix & rows, int id) const
    {
        for(int row = 0; row < rows.rows(); ++row)
            if(int(rows(row, 0)) == id)
                return row;

        return -1;
    }
    int max_id_in_rows(const matrix & rows)
    {
        int max_id = 0;
        if(rows.empty() || rows.rank() < 2)
            return max_id;

        for(int row = 0; row < rows.rows(); ++row)
            max_id = std::max(max_id, int(rows(row, 0)));

        return max_id;
    }
    int next_scene_id()
    {
        return std::max({max_id_in_rows(food_), max_id_in_rows(aversive_), max_id_in_rows(walls_)}) + 1;
    }
    int current_edit_tool()
    {
        matrix tool = edit_tool_;
        return tool.empty() ? 0 : int(std::round(tool(0)));
    }
    bool object_solid(const matrix & rows, int row) const
    {
        return rows(row, 8) > 0.5f;
    }
    float object_value(const matrix & rows, int row) const
    {
        return rows(row, 4);
    }
    float object_smell(const matrix & rows, int row, int smell) const
    {
        return rows(row, 9 + smell);
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
        Bind(odour_decay_, "odour_decay");
        Bind(edit_tool_, "edit_tool");
        Bind(edit_type_, "edit_type");
        Bind(edit_value_, "edit_value");
        Bind(edit_opaque_, "edit_opaque");
        Bind(edit_smell_, "edit_smell");
        Bind(edit_color_, "edit_color");
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
        Bind(odour_, "ODOUR");
        Bind(reinforcement_, "REINFORCEMENT");
        Bind(reward_, "REWARD");
        Bind(punishment_, "PUNISHMENT");
        Bind(edit_object_enable_, "EDIT_OBJECT_ENABLE");
        Bind(edit_wall_enable_, "EDIT_WALL_ENABLE");
        Bind(edit_color_enable_, "EDIT_COLOR_ENABLE");

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
            "id",
            "type",
            "x",
            "y",
            "radius",
            "value",
            "red",
            "green",
            "blue",
            "solid",
            "smell_1",
            "smell_2",
            "smell_3",
            "smell_4",
            "smell_5",
            "smell_6",
            "smell_7",
            "smell_8"
        );
        walls_out_.set_labels(1,
            "id",
            "opaque",
            "x1",
            "y1",
            "x2",
            "y2",
            "red",
            "green",
            "blue",
            "solid",
            "line_width"
        );
        pose_.set_labels(0, "x", "y", "heading");
        contact_.set_labels(0, "food", "aversive");
        whiskers_.set_labels(0, "left", "right");
        odour_.set_labels(0, "left", "right");
        odour_.set_labels(1,
            "smell_1",
            "smell_2",
            "smell_3",
            "smell_4",
            "smell_5",
            "smell_6",
            "smell_7",
            "smell_8"
        );
        reinforcement_.set_labels(0, "value");
        reward_.set_labels(0, "value");
        punishment_.set_labels(0, "value");
        edit_object_enable_.set_labels(0, "value");
        edit_wall_enable_.set_labels(0, "value");
        edit_color_enable_.set_labels(0, "value");

        x_ = creature_x0_.as_float();
        y_ = creature_y0_.as_float();
        heading_ = creature_heading0_.as_float();
        clamp_to_world();
        emit_outputs();
    }

    void Tick() override
    {
        update_edit_tool_state();
        apply_edit_values_to_selected_item();
        step_creature();
        emit_outputs();
    }

    void update_edit_tool_state()
    {
        const int tool = current_edit_tool();
        if(tool == last_edit_tool_)
            return;

        last_edit_tool_ = tool;
        if(tool != 2)
            return;

        selected_object_id_ = -1;
        selected_object_type_ = 0.0f;
        selected_color_kind_ = 0;
        selected_color_id_ = -1;
        selected_color_type_ = 0.0f;
        SetParameter("edit_color", default_wall_color(), "");
    }

    void Command(std::string command_name, dictionary & parameters) override
    {
        if(command_name == "move_object")
        {
            move_object(parameters);
            return;
        }

        if(command_name == "move_wall")
        {
            move_wall(parameters);
            return;
        }

        if(command_name == "move_creature")
        {
            move_creature(parameters);
            return;
        }

        if(command_name == "add_object")
        {
            add_object(parameters);
            return;
        }

        if(command_name == "add_wall")
        {
            add_wall(parameters);
            return;
        }

        if(command_name == "delete_object")
        {
            delete_object(parameters);
            return;
        }

        if(command_name == "delete_wall")
        {
            delete_wall(parameters);
            return;
        }

        if(command_name == "select_object")
        {
            select_object(parameters);
            emit_outputs();
            return;
        }

        if(command_name == "select_wall")
        {
            select_wall(parameters);
            emit_outputs();
            return;
        }

        if(command_name == "clear_selection")
        {
            selected_object_id_ = -1;
            selected_object_type_ = 0.0f;
            selected_color_kind_ = 0;
            selected_color_id_ = -1;
            selected_color_type_ = 0.0f;
            emit_outputs();
            return;
        }

        Module::Command(command_name, parameters);
    }

    void select_object(dictionary & parameters)
    {
        const float type = parameters["type"];
        const int id = int(parameters["id"]);
        if(id < 1)
            return;

        if(type == row_type_food)
        {
            matrix & rows = food_;
            const int row = find_object_row_by_id(rows, id);
            select_object_row(rows, row, type, id);
        }
        else if(type == row_type_aversive)
        {
            matrix & rows = aversive_;
            const int row = find_object_row_by_id(rows, id);
            select_object_row(rows, row, type, id);
        }
    }

    void select_object_row(const matrix & rows, int row, float type, int id)
    {
        if(rows.empty() || rows.rank() < 2 || row < 0 || row >= rows.rows() || !has_object_geometry(rows))
            return;

        matrix smell(8);
        matrix color(3);
        matrix value(1);
        matrix type_index(1);
        type_index(0) = type == row_type_aversive ? 1.0f : 0.0f;
        value(0) = object_value(rows, row);
        for(int s = 0; s < 8; ++s)
            smell(s) = object_smell(rows, row, s);
        for(int c = 0; c < 3; ++c)
            color(c) = rows(row, 5 + c);

        selected_object_id_ = id;
        selected_object_type_ = type;
        selected_color_kind_ = 1;
        selected_color_id_ = id;
        selected_color_type_ = type;
        SetParameter("edit_type", type_index, "");
        SetParameter("edit_value", value, "");
        SetParameter("edit_smell", smell, "");
        SetParameter("edit_color", color, "");
    }

    void select_wall(dictionary & parameters)
    {
        const int id = int(parameters["id"]);
        matrix & rows = walls_;
        const int row = find_wall_row_by_id(rows, id);
        if(id < 1 || rows.empty() || rows.rank() < 2 || row < 0 || row >= rows.rows() || !has_wall_geometry(rows))
            return;

        matrix color(3);
        matrix opaque(1);
        for(int c = 0; c < 3; ++c)
            color(c) = rows(row, 5 + c);
        opaque(0) = rows(row, 10) > 0.5f ? 1.0f : 0.0f;

        selected_object_id_ = -1;
        selected_object_type_ = 0.0f;
        selected_color_kind_ = 2;
        selected_color_id_ = id;
        selected_color_type_ = 0.0f;
        SetParameter("edit_color", color, "");
        SetParameter("edit_opaque", opaque, "");
    }

    void apply_edit_smell_to_selected_object()
    {
        if(selected_object_id_ < 1)
            return;

        if(selected_object_type_ == row_type_food)
            apply_edit_smell_to_object_row("food", food_, find_object_row_by_id(food_, selected_object_id_));
        else if(selected_object_type_ == row_type_aversive)
            apply_edit_smell_to_object_row("aversive", aversive_, find_object_row_by_id(aversive_, selected_object_id_));
    }

    void apply_edit_smell_to_object_row(const std::string & parameter_name, parameter & rows_parameter, int row)
    {
        matrix rows = rows_parameter;
        matrix smell = edit_smell_;
        if(rows.empty() || rows.rank() < 2 || row < 0 || row >= rows.rows() || !has_object_geometry(rows) || smell.empty())
            return;

        const int smell_count = std::min(8, smell.size());
        for(int s = 0; s < smell_count; ++s)
            rows(row, 9 + s) = std::clamp(smell(s), 0.0f, 1.0f);

        SetParameter(parameter_name, rows, "");
    }

    void apply_edit_color_to_selected_item()
    {
        if(selected_color_id_ < 1)
            return;

        if(selected_color_kind_ == 1)
        {
            if(selected_color_type_ == row_type_food)
                apply_edit_color_to_object_row("food", food_, find_object_row_by_id(food_, selected_color_id_));
            else if(selected_color_type_ == row_type_aversive)
                apply_edit_color_to_object_row("aversive", aversive_, find_object_row_by_id(aversive_, selected_color_id_));
        }
        else if(selected_color_kind_ == 2)
        {
            apply_edit_color_to_wall_row(find_wall_row_by_id(walls_, selected_color_id_));
        }
    }

    void apply_edit_color_to_object_row(const std::string & parameter_name, parameter & rows_parameter, int row)
    {
        matrix rows = rows_parameter;
        matrix color = edit_color_;
        if(rows.empty() || rows.rank() < 2 || row < 0 || row >= rows.rows() || !has_object_geometry(rows) || color.empty())
            return;

        const int color_count = std::min(3, color.size());
        for(int c = 0; c < color_count; ++c)
            rows(row, 5 + c) = std::clamp(color(c), 0.0f, 1.0f);

        SetParameter(parameter_name, rows, "");
    }

    void apply_edit_color_to_wall_row(int row)
    {
        matrix rows = walls_;
        matrix color = edit_color_;
        if(rows.empty() || rows.rank() < 2 || row < 0 || row >= rows.rows() || !has_wall_geometry(rows) || color.empty())
            return;

        const int color_count = std::min(3, color.size());
        for(int c = 0; c < color_count; ++c)
            rows(row, 5 + c) = std::clamp(color(c), 0.0f, 1.0f);

        SetParameter("walls", rows, "");
    }

    void apply_edit_values_to_selected_item()
    {
        if(selected_object_id_ >= 1)
        {
            apply_edit_type_to_selected_object();
            if(selected_object_type_ == row_type_food)
                apply_edit_values_to_object_row("food", food_, find_object_row_by_id(food_, selected_object_id_));
            else if(selected_object_type_ == row_type_aversive)
                apply_edit_values_to_object_row("aversive", aversive_, find_object_row_by_id(aversive_, selected_object_id_));
            return;
        }

        if(selected_color_kind_ == 2 && selected_color_id_ >= 1)
            apply_edit_values_to_wall_row(find_wall_row_by_id(walls_, selected_color_id_));
    }

    float edit_object_type()
    {
        matrix type = edit_type_;
        if(type.empty())
            return row_type_food;

        return int(std::round(type(0))) == 1 ? row_type_aversive : row_type_food;
    }

    void apply_edit_type_to_selected_object()
    {
        if(selected_object_id_ < 1)
            return;

        const float desired_type = edit_object_type();
        if(desired_type == selected_object_type_)
            return;

        if(selected_object_type_ == row_type_food && desired_type == row_type_aversive)
            move_selected_object_between_type_parameters("food", food_, "aversive", aversive_, desired_type);
        else if(selected_object_type_ == row_type_aversive && desired_type == row_type_food)
            move_selected_object_between_type_parameters("aversive", aversive_, "food", food_, desired_type);
    }

    void move_selected_object_between_type_parameters(const std::string & source_name, parameter & source_parameter, const std::string & target_name, parameter & target_parameter, float desired_type)
    {
        matrix source = source_parameter;
        matrix target = target_parameter;
        const int row = find_object_row_by_id(source, selected_object_id_);
        if(source.empty() || source.rank() < 2 || row < 0 || row >= source.rows() || !has_object_geometry(source))
            return;

        matrix copied_row(source.cols());
        for(int col = 0; col < source.cols(); ++col)
            copied_row(col) = source(row, col);

        target.append(copied_row);
        SetParameter(source_name, rows_without_row(source, row), "");
        SetParameter(target_name, target, "");
        selected_object_type_ = desired_type;
        if(selected_color_kind_ == 1 && selected_color_id_ == selected_object_id_)
            selected_color_type_ = desired_type;
    }

    void apply_edit_values_to_wall_row(int row)
    {
        matrix rows = walls_;
        matrix color = edit_color_;
        matrix opaque = edit_opaque_;
        if(rows.empty() || rows.rank() < 2 || row < 0 || row >= rows.rows() || !has_wall_geometry(rows))
            return;

        if(!color.empty())
        {
            const int color_count = std::min(3, color.size());
            for(int c = 0; c < color_count; ++c)
                rows(row, 5 + c) = std::clamp(color(c), 0.0f, 1.0f);
        }

        if(!opaque.empty())
            rows(row, 10) = opaque(0) > 0.5f ? 1.0f : 0.0f;

        SetParameter("walls", rows, "");
    }

    void apply_edit_values_to_object_row(const std::string & parameter_name, parameter & rows_parameter, int row)
    {
        matrix rows = rows_parameter;
        matrix value = edit_value_;
        matrix smell = edit_smell_;
        matrix color = edit_color_;
        if(rows.empty() || rows.rank() < 2 || row < 0 || row >= rows.rows() || !has_object_geometry(rows))
            return;

        if(!value.empty())
            rows(row, 4) = value(0);

        if(!smell.empty())
        {
            const int smell_count = std::min(8, smell.size());
            for(int s = 0; s < smell_count; ++s)
                rows(row, 9 + s) = std::clamp(smell(s), 0.0f, 1.0f);
        }

        if(!color.empty() && selected_color_kind_ == 1 && selected_color_id_ == selected_object_id_)
        {
            const int color_count = std::min(3, color.size());
            for(int c = 0; c < color_count; ++c)
                rows(row, 5 + c) = std::clamp(color(c), 0.0f, 1.0f);
        }

        SetParameter(parameter_name, rows, "");
    }

    matrix rows_without_row(const matrix & rows, int row_to_remove) const
    {
        matrix filtered;
        if(rows.empty() || rows.rank() < 2 || row_to_remove < 0 || row_to_remove >= rows.rows())
            return filtered;

        filtered.reserve(std::max(1, rows.rows() - 1), rows.cols());
        filtered.clear();
        for(int row = 0; row < rows.rows(); ++row)
        {
            if(row == row_to_remove)
                continue;

            matrix copied_row(rows.cols());
            for(int col = 0; col < rows.cols(); ++col)
                copied_row(col) = rows(row, col);
            filtered.append(copied_row);
        }

        return filtered;
    }

    void clear_deleted_selection(int id, float type, int color_kind)
    {
        if(selected_object_id_ == id && selected_object_type_ == type)
        {
            selected_object_id_ = -1;
            selected_object_type_ = 0.0f;
        }

        if(selected_color_id_ == id && selected_color_kind_ == color_kind && (color_kind != 1 || selected_color_type_ == type))
        {
            selected_color_kind_ = 0;
            selected_color_id_ = -1;
            selected_color_type_ = 0.0f;
        }
    }

    matrix edit_color_values()
    {
        matrix color = edit_color_;
        if(color.empty())
        {
            color = matrix(3);
            color(0) = 0.48f;
            color(1) = 0.48f;
            color(2) = 0.50f;
        }

        return color;
    }

    matrix edit_smell_values()
    {
        matrix smell = edit_smell_;
        if(smell.empty())
            smell = matrix(8);

        return smell;
    }

    float edit_value_value()
    {
        matrix value = edit_value_;
        return value.empty() ? 0.0f : value(0);
    }

    void add_object(dictionary & parameters)
    {
        matrix rows = food_;
        matrix row(object_parameter_cols);
        matrix color = edit_color_values();
        matrix smell = edit_smell_values();
        const float value = edit_value_value();
        row.set(0.0f);
        const int id = next_scene_id();
        row(0) = float(id);
        row(1) = clamp_x(parameters["x"]);
        row(2) = clamp_y(parameters["y"]);
        row(3) = 8.0f;
        row(4) = value;
        row(5) = color.size() > 0 ? std::clamp(color(0), 0.0f, 1.0f) : 0.48f;
        row(6) = color.size() > 1 ? std::clamp(color(1), 0.0f, 1.0f) : 0.48f;
        row(7) = color.size() > 2 ? std::clamp(color(2), 0.0f, 1.0f) : 0.50f;
        row(8) = 0.0f;
        for(int s = 0; s < 8 && s < smell.size(); ++s)
            row(9 + s) = std::clamp(smell(s), 0.0f, 1.0f);

        rows.append(row);
        SetParameter("food", rows, "");
        selected_object_id_ = id;
        selected_object_type_ = row_type_food;
        selected_color_kind_ = 1;
        selected_color_id_ = id;
        selected_color_type_ = row_type_food;
        emit_outputs();
    }

    void add_wall(dictionary & parameters)
    {
        matrix rows = walls_;
        matrix row(wall_parameter_cols);
        matrix color = edit_color_values();
        row.set(0.0f);
        row(0) = float(next_scene_id());
        row(1) = clamp_x(parameters["x1"]);
        row(2) = clamp_y(parameters["y1"]);
        row(3) = clamp_x(parameters["x2"]);
        row(4) = clamp_y(parameters["y2"]);
        row(5) = color.size() > 0 ? std::clamp(color(0), 0.0f, 1.0f) : 0.18f;
        row(6) = color.size() > 1 ? std::clamp(color(1), 0.0f, 1.0f) : 0.18f;
        row(7) = color.size() > 2 ? std::clamp(color(2), 0.0f, 1.0f) : 0.18f;
        row(8) = 1.0f;
        row(9) = 2.0f;
        row(10) = 1.0f;

        rows.append(row);
        SetParameter("walls", rows, "");
        emit_outputs();
    }

    void delete_object(dictionary & parameters)
    {
        const float type = parameters["type"];
        const int id = int(parameters["id"]);
        if(id < 1)
            return;

        if(type == row_type_food)
            delete_object_row("food", food_, find_object_row_by_id(food_, id), id, type);
        else if(type == row_type_aversive)
            delete_object_row("aversive", aversive_, find_object_row_by_id(aversive_, id), id, type);
    }

    void delete_object_row(const std::string & parameter_name, parameter & rows_parameter, int row, int id, float type)
    {
        matrix rows = rows_parameter;
        if(rows.empty() || rows.rank() < 2 || row < 0 || row >= rows.rows())
            return;

        SetParameter(parameter_name, rows_without_row(rows, row), "");
        clear_deleted_selection(id, type, 1);
        emit_outputs();
    }

    void delete_wall(dictionary & parameters)
    {
        const int id = int(parameters["id"]);
        matrix rows = walls_;
        const int row = find_wall_row_by_id(rows, id);
        if(id < 1 || rows.empty() || rows.rank() < 2 || row < 0 || row >= rows.rows())
            return;

        SetParameter("walls", rows_without_row(rows, row), "");
        clear_deleted_selection(id, 0.0f, 2);
        emit_outputs();
    }

    void move_creature(dictionary & parameters)
    {
        x_ = parameters["x"];
        y_ = parameters["y"];
        clamp_to_world();
        SetParameter("creature_x", std::to_string(x_));
        SetParameter("creature_y", std::to_string(y_));
        emit_outputs();
    }

    void move_object(dictionary & parameters)
    {
        const float type = parameters["type"];
        const int id = int(parameters["id"]);
        const float x = clamp_x(parameters["x"]);
        const float y = clamp_y(parameters["y"]);

        if(id < 1)
            return;

        if(type == row_type_food)
        {
            matrix & rows = food_;
            const int row = find_object_row_by_id(rows, id);
            move_object_row("food", food_, row, x, y);
        }
        else if(type == row_type_aversive)
        {
            matrix & rows = aversive_;
            const int row = find_object_row_by_id(rows, id);
            move_object_row("aversive", aversive_, row, x, y);
        }
    }

    void move_object_row(const std::string & parameter_name, parameter & rows_parameter, int row, float x, float y)
    {
        matrix rows = rows_parameter;
        if(rows.empty() || rows.rank() < 2 || row < 0 || row >= rows.rows())
            return;

        if(!has_object_geometry(rows))
            return;

        rows(row, 1) = x;
        rows(row, 2) = y;
        SetParameter(parameter_name, rows, "");
        emit_outputs();
    }

    void move_wall(dictionary & parameters)
    {
        const int id = int(parameters["id"]);
        matrix walls = walls_;
        const int row = find_wall_row_by_id(walls, id);
        if(id < 1 || walls.empty() || walls.rank() < 2 || row < 0 || row >= walls.rows())
            return;

        if(!has_wall_geometry(walls))
            return;

        walls(row, 1) = clamp_x(parameters["x1"]);
        walls(row, 2) = clamp_y(parameters["y1"]);
        walls(row, 3) = clamp_x(parameters["x2"]);
        walls(row, 4) = clamp_y(parameters["y2"]);
        SetParameter("walls", walls, "");
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
            if(!has_object_geometry(rows))
                continue;

            const float object_x = rows(i, 1);
            const float object_y = rows(i, 2);
            const float object_radius = std::max(0.0f, rows(i, 3));
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
            if(!has_object_geometry(rows))
                continue;

            const bool solid = object_solid(rows, i);
            if(solid_only && !solid)
                continue;

            const float dx = x_ - rows(i, 1);
            const float dy = y_ - rows(i, 2);
            const float object_radius = std::max(0.0f, rows(i, 3));
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
            if(!has_object_geometry(objects) || !object_solid(objects, i))
                continue;

            const float rr = r + std::max(0.0f, objects(i, 3));
            if(distance_to_segment(objects(i, 1), objects(i, 2), old_x, old_y, new_x, new_y) < rr - eps)
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
            if(!has_wall_geometry(walls))
                continue;

            const bool solid = walls(i, 8) > 0.5f;
            if(!solid)
                continue;

            const float line_width = std::max(0.0f, walls(i, 9));
            const float rr = r + 0.5f * line_width;
            if(segment_distance(old_x, old_y, new_x, new_y, walls(i, 1), walls(i, 2), walls(i, 3), walls(i, 4)) < rr - eps)
                return true;
        }

        return false;
    }

    bool swept_solid_contact(float old_x, float old_y, float new_x, float new_y) const
    {
        return swept_solid_object_contact(old_x, old_y, new_x, new_y) ||
               swept_solid_wall_contact(old_x, old_y, new_x, new_y);
    }

    bool opaque_wall_blocks(float sensor_x, float sensor_y, float object_x, float object_y) const
    {
        const matrix & walls = walls_;
        if(walls.empty() || walls.rank() < 2)
            return false;

        for(int i = 0; i < walls.rows(); ++i)
        {
            if(!has_wall_geometry(walls))
                continue;

            const bool opaque = walls(i, 10) > 0.5f;
            if(opaque && segments_intersect(sensor_x, sensor_y, object_x, object_y, walls(i, 1), walls(i, 2), walls(i, 3), walls(i, 4)))
                return true;
        }

        return false;
    }

    bool collides_with_wall()
    {
        matrix & walls = walls_;
        if(walls.empty() || walls.rank() < 2)
            return false;

        const float r = radius();
        for(int i = 0; i < walls.rows(); ++i)
        {
            if(!has_wall_geometry(walls))
                continue;

            const bool solid = walls(i, 8) > 0.5f;
            if(!solid)
                continue;

            const float line_width = std::max(0.0f, walls(i, 9));
            if(distance_to_segment(x_, y_, walls(i, 1), walls(i, 2), walls(i, 3), walls(i, 4)) <= r + 0.5f * line_width)
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
            if(!has_wall_geometry(walls))
                continue;

            const bool solid = walls(i, 8) > 0.5f;
            if(!solid)
                continue;

            const float line_width = std::max(0.0f, walls(i, 9));
            const float distance = distance_to_segment(x_, y_, walls(i, 1), walls(i, 2), walls(i, 3), walls(i, 4));
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

    float reinforcement_from_rows(const matrix & rows) const
    {
        if(rows.empty() || rows.rank() < 2)
            return 0.0f;

        float value = 0.0f;
        const float r = radius();
        for(int i = 0; i < rows.rows(); ++i)
        {
            if(!has_object_geometry(rows))
                continue;

            const float dx = x_ - rows(i, 1);
            const float dy = y_ - rows(i, 2);
            const float object_radius = std::max(0.0f, rows(i, 3));
            const float rr = r + object_radius;
            if(dx * dx + dy * dy <= rr * rr)
                value += object_value(rows, i);
        }

        return value;
    }

    float reinforcement_value() const
    {
        const matrix & food = food_;
        const matrix & aversive = aversive_;
        return reinforcement_from_rows(food) + reinforcement_from_rows(aversive);
    }

    void scan_solid_circle(float start_x, float start_y, float dx, float dy, float length, float & first_contact) const
    {
        const matrix & objects = aversive_;
        if(objects.empty() || objects.rank() < 2)
            return;

        for(int i = 0; i < objects.rows(); ++i)
        {
            if(!has_object_geometry(objects))
                continue;

            const bool solid = object_solid(objects, i);
            if(!solid)
                continue;

            const float fx = start_x - objects(i, 1);
            const float fy = start_y - objects(i, 2);
            const float object_radius = std::max(0.0f, objects(i, 3));
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
            if(!has_wall_geometry(walls))
                continue;

            const bool solid = walls(i, 8) > 0.5f;
            if(!solid)
                continue;

            const float half_width = 0.5f * std::max(0.0f, walls(i, 9));
            for(int s = 0; s <= samples; ++s)
            {
                const float fraction = float(s) / float(samples);
                if(fraction >= first_contact)
                    break;

                const float px = start_x + dx * length * fraction;
                const float py = start_y + dy * length * fraction;
                if(distance_to_segment(px, py, walls(i, 1), walls(i, 2), walls(i, 3), walls(i, 4)) <= half_width)
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

    void whisker_tip(float relative_angle, float & tip_x, float & tip_y) const
    {
        const float angle = heading_ + relative_angle;
        const float length = whisker_length();
        tip_x = x_ + std::cos(angle) * length;
        tip_y = y_ + std::sin(angle) * length;
    }

    void add_odours_from_rows(const matrix & rows, float sensor_x, float sensor_y, int sensor)
    {
        if(rows.empty() || rows.rank() < 2)
            return;

        const float c = odour_decay();
        for(int i = 0; i < rows.rows(); ++i)
        {
            if(!has_object_geometry(rows))
                continue;

            const float object_x = rows(i, 1);
            const float object_y = rows(i, 2);
            if(opaque_wall_blocks(sensor_x, sensor_y, object_x, object_y))
                continue;

            const float dx = sensor_x - object_x;
            const float dy = sensor_y - object_y;
            const float distance_squared = dx * dx + dy * dy;
            const float intensity = 1.0f / (1.0f + c * distance_squared);
            for(int s = 0; s < 8; ++s)
                odour_(sensor, s) = std::clamp(odour_(sensor, s) + intensity * object_smell(rows, i, s), 0.0f, 1.0f);
        }
    }

    void update_odours()
    {
        odour_.set(0.0f);

        const float angle = whisker_angle();
        float left_x = 0.0f;
        float left_y = 0.0f;
        float right_x = 0.0f;
        float right_y = 0.0f;
        whisker_tip(-angle, left_x, left_y);
        whisker_tip(angle, right_x, right_y);

        matrix & food = food_;
        matrix & aversive = aversive_;
        add_odours_from_rows(food, left_x, left_y, 0);
        add_odours_from_rows(aversive, left_x, left_y, 0);
        add_odours_from_rows(food, right_x, right_y, 1);
        add_odours_from_rows(aversive, right_x, right_y, 1);
    }

    void append_object_rows(const matrix & objects, float type)
    {
        if(objects.empty() || objects.rank() < 2)
            return;

        for(int i = 0; i < objects.rows(); ++i)
        {
            if(!has_object_geometry(objects))
                continue;

            matrix row(object_cols);
            row.set(0.0f);
            row(0) = objects(i, 0);
            row(1) = type;
            row(2) = objects(i, 1);
            row(3) = objects(i, 2);
            row(4) = objects(i, 3);
            row(5) = objects(i, 4);
            row(6) = objects(i, 5);
            row(7) = objects(i, 6);
            row(8) = objects(i, 7);
            row(9) = object_solid(objects, i) ? 1.0f : 0.0f;
            for(int s = 0; s < 8; ++s)
                row(10 + s) = object_smell(objects, i, s);
            objects_.append(row);
        }
    }

    void append_wall_rows(const matrix & walls)
    {
        if(walls.empty() || walls.rank() < 2)
            return;

        for(int i = 0; i < walls.rows(); ++i)
        {
            if(!has_wall_geometry(walls))
                continue;

            matrix row(wall_cols);
            row.set(0.0f);
            row(0) = walls(i, 0);
            row(1) = walls(i, 10);
            row(2) = walls(i, 1);
            row(3) = walls(i, 2);
            row(4) = walls(i, 3);
            row(5) = walls(i, 4);
            row(6) = walls(i, 5);
            row(7) = walls(i, 6);
            row(8) = walls(i, 7);
            row(9) = walls(i, 8);
            row(10) = walls(i, 9);
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
        update_odours();

        const float reinforcement = reinforcement_value();
        reinforcement_(0) = reinforcement;
        reward_(0) = std::max(0.0f, reinforcement);
        punishment_(0) = std::max(0.0f, -reinforcement);

        edit_object_enable_(0) = selected_object_id_ >= 1 ? 1.0f : 0.0f;
        edit_wall_enable_(0) = selected_color_kind_ == 2 && selected_color_id_ >= 1 ? 1.0f : 0.0f;
        edit_color_enable_(0) = (selected_color_id_ >= 1 || current_edit_tool() == 2) ? 1.0f : 0.0f;
    }
};

INSTALL_CLASS(World2D)
