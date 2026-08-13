class WebUIWidgetPolarPlot extends WebUIWidgetCanvas
{
    static template()
    {
        return [
            {'name': "POLAR PLOT", 'control':'header'},
            {'name':'title', 'default':"Polar Plot", 'type':'string', 'control':'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'label_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'labels', 'default':"", 'type':'string', 'control':'textedit'},

            {'name': "RADIAL SCALE", 'control':'header'},
            {'name':'value_min', 'default':0, 'type':'float', 'control':'textedit'},
            {'name':'value_max', 'default':1, 'type':'float', 'control':'textedit'},
            {'name':'auto_range', 'default':"no", 'type':'bool', 'control':'checkbox'},
            {'name':'include_zero', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'clamp_values', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'radial_ticks', 'default':5, 'type':'int', 'control':'textedit'},
            {'name':'show_scale', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'scale_decimals', 'default':2, 'type':'int', 'control':'textedit'},

            {'name': "ANGULAR AXES", 'control':'header'},
            {'name':'angle_offset', 'default':-90, 'type':'float', 'control':'textedit'},
            {'name':'direction', 'default':"clockwise", 'type':'string', 'control':'menu', 'options':"clockwise,counterclockwise"},
            {'name':'grid_shape', 'default':"polygon", 'type':'string', 'control':'menu', 'options':"polygon,circle"},
            {'name':'show_spokes', 'default':"yes", 'type':'bool', 'control':'checkbox'},

            {'name': "LABELS", 'control':'header'},
            {'name':'show_labels', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'label_offset', 'default':10, 'type':'float', 'control':'textedit'},
            {'name':'label_max_width', 'default':120, 'type':'float', 'control':'textedit'},
            {'name':'label_color', 'default':'', 'type':'string', 'control':'textedit'},
            {'name':'label_font', 'default':'', 'type':'string', 'control':'textedit'},

            {'name': "SERIES", 'control':'header'},
            {'name':'stroke_color', 'default':'', 'type':'string', 'control':'textedit'},
            {'name':'fill_color', 'default':'', 'type':'string', 'control':'textedit'},
            {'name':'fill_opacity', 'default':0.2, 'type':'float', 'control':'slider', 'min':0, 'max':1},
            {'name':'line_width', 'default':2, 'type':'float', 'control':'textedit'},
            {'name':'line_join', 'default':"round", 'type':'string', 'control':'menu', 'options':"miter,round,bevel"},
            {'name':'show_points', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'point_size', 'default':3, 'type':'float', 'control':'textedit'},
            {'name':'grid_color', 'default':'', 'type':'string', 'control':'textedit'},
            {'name':'grid_line_width', 'default':1, 'type':'float', 'control':'textedit'},
        ];
    }

    init()
    {
        super.init();
        this.series = [];
        this.metadata = null;
        this.minimum = 0;
        this.maximum = 1;
    }

    getSeries(data)
    {
        if(!Array.isArray(data) || data.length === 0)
            return [];
        const rows = Array.isArray(data[0]) ? data : [data];
        const axisCount = Array.isArray(rows[0]) ? rows[0].length : 0;
        if(axisCount === 0)
            return [];
        return rows.filter((row) => Array.isArray(row) && row.length === axisCount &&
            row.every((value) => Number.isFinite(Number(value)))).map((row) => row.map(Number));
    }

    flattenLabels(value)
    {
        if(!Array.isArray(value))
            return value === undefined || value === null ? [] : String(value).split(',');
        if(value.length === 1 && Array.isArray(value[0]))
            return value[0];
        return value.flat ? value.flat(Infinity) : value;
    }

    getMetadataLabels()
    {
        const labels = this.metadata?.labels;
        if(!Array.isArray(labels))
            return [];
        if(Array.isArray(labels[1]) && labels[1].length > 0)
            return labels[1];
        if(Array.isArray(labels[0]))
            return labels[0];
        return labels;
    }

    getLabels(axisCount)
    {
        const sourceLabels = this.flattenLabels(this.getSource('label_source'));
        const metadataLabels = this.getMetadataLabels();
        const explicitLabels = String(this.parameters.labels ?? "").split(',');
        let selected = sourceLabels.length > 0 ? sourceLabels : metadataLabels.length > 0 ? metadataLabels :
            explicitLabels.some((label) => label.trim() !== "") ? explicitLabels : [];
        selected = selected.map((label) => String(label ?? "").trim());
        return Array.from({length:axisCount}, (_, index) => selected[index] || String(index));
    }

    getValueRange(series)
    {
        let minimum = Number(this.parameters.value_min);
        let maximum = Number(this.parameters.value_max);
        if(this.parameters.auto_range)
        {
            minimum = Infinity;
            maximum = -Infinity;
            for(const row of series)
                for(const value of row)
                {
                    minimum = Math.min(minimum, value);
                    maximum = Math.max(maximum, value);
                }
            if(this.parameters.include_zero)
            {
                minimum = Math.min(minimum, 0);
                maximum = Math.max(maximum, 0);
            }
        }
        if(!Number.isFinite(minimum))
            minimum = 0;
        if(!Number.isFinite(maximum))
            maximum = 1;
        if(maximum <= minimum)
            maximum = minimum + 1;
        return {min:minimum, max:maximum};
    }

    getAngles(axisCount)
    {
        const offset = Number(this.parameters.angle_offset) * Math.PI / 180;
        const direction = this.parameters.direction === "counterclockwise" ? -1 : 1;
        return Array.from({length:axisCount}, (_, index) => offset + direction * 2 * Math.PI * index / axisCount);
    }

    truncateLabel(label, maximumWidth)
    {
        if(this.canvas.measureText(label).width <= maximumWidth)
            return label;
        let truncated = label;
        while(truncated.length > 1 && this.canvas.measureText(truncated + "…").width > maximumWidth)
            truncated = truncated.slice(0, -1);
        return truncated + "…";
    }

    getGeometry(width, height, labels, angles)
    {
        const labelOffset = Math.max(0, Number(this.parameters.label_offset) || 0);
        const maxLabelWidth = Math.max(0, Number(this.parameters.label_max_width) || 0);
        this.canvas.font = this.parameters.label_font || this.format.label_font || "12px sans-serif";
        const halfWidth = width / 2;
        const halfHeight = height / 2;
        let radius = Math.min(halfWidth, halfHeight) - 4;
        if(this.parameters.show_labels)
            labels.forEach((label, index) => {
                const truncated = this.truncateLabel(label, maxLabelWidth);
                const metrics = this.canvas.measureText(truncated);
                const labelWidth = Math.min(maxLabelWidth, metrics.width);
                const labelHeight = Math.max(12, (metrics.actualBoundingBoxAscent || 0) + (metrics.actualBoundingBoxDescent || 0));
                const cosine = Math.cos(angles[index]);
                const sine = Math.sin(angles[index]);
                const horizontalExtent = Math.abs(cosine) > 0.2 ? labelWidth : labelWidth / 2;
                if(Math.abs(cosine) > 0.001)
                    radius = Math.min(radius, (halfWidth - horizontalExtent - 4) / Math.abs(cosine) - labelOffset);
                if(Math.abs(sine) > 0.001)
                    radius = Math.min(radius, (halfHeight - labelHeight / 2 - 4) / Math.abs(sine) - labelOffset);
            });
        radius = Math.max(1, radius);
        return {centerX:width / 2, centerY:height / 2, radius, labelOffset, maxLabelWidth};
    }

    pointAt(geometry, angle, radius)
    {
        return {
            x:geometry.centerX + radius * Math.cos(angle),
            y:geometry.centerY + radius * Math.sin(angle)
        };
    }

    drawGrid(geometry, angles)
    {
        const ticks = Math.max(1, Math.trunc(Number(this.parameters.radial_ticks) || 1));
        this.canvas.strokeStyle = this.parameters.grid_color || this.format.grid_color || "#aaa";
        this.canvas.lineWidth = Math.max(0, Number(this.parameters.grid_line_width) || 0);
        for(let tick = 1; tick <= ticks; tick++)
        {
            const radius = geometry.radius * tick / ticks;
            this.canvas.beginPath();
            if(this.parameters.grid_shape === "circle")
                this.canvas.arc(geometry.centerX, geometry.centerY, radius, 0, 2 * Math.PI);
            else
            {
                angles.forEach((angle, index) => {
                    const point = this.pointAt(geometry, angle, radius);
                    if(index === 0)
                        this.canvas.moveTo(point.x, point.y);
                    else
                        this.canvas.lineTo(point.x, point.y);
                });
                this.canvas.closePath();
            }
            this.canvas.stroke();
        }
        if(this.parameters.show_spokes)
            for(const angle of angles)
            {
                const point = this.pointAt(geometry, angle, geometry.radius);
                this.canvas.beginPath();
                this.canvas.moveTo(geometry.centerX, geometry.centerY);
                this.canvas.lineTo(point.x, point.y);
                this.canvas.stroke();
            }
    }

    formatScaleValue(value)
    {
        const configured = Number(this.parameters.scale_decimals);
        const decimals = Number.isFinite(configured) ? Math.max(0, Math.min(20, Math.trunc(configured))) : 2;
        return value.toFixed(decimals).replace(/\.?0+$/, "");
    }

    drawScale(geometry, range)
    {
        if(!this.parameters.show_scale)
            return;
        const ticks = Math.max(1, Math.trunc(Number(this.parameters.radial_ticks) || 1));
        this.canvas.font = this.format.scale_font || "12px sans-serif";
        this.canvas.fillStyle = this.format.axis_color || "black";
        this.canvas.textAlign = "left";
        this.canvas.textBaseline = "bottom";
        for(let tick = 1; tick <= ticks; tick++)
        {
            const fraction = tick / ticks;
            const value = range.min + fraction * (range.max - range.min);
            this.canvas.fillText(this.formatScaleValue(value), geometry.centerX + 4, geometry.centerY - geometry.radius * fraction - 2);
        }
    }

    getListValue(value, index, fallback)
    {
        const list = String(value || fallback).split(',').map((item) => item.trim()).filter((item) => item !== "");
        return list.length > 0 ? list[index % list.length] : fallback;
    }

    valueRadius(value, range, radius)
    {
        let fraction = (value - range.min) / (range.max - range.min);
        if(this.parameters.clamp_values)
            fraction = Math.max(0, Math.min(1, fraction));
        return fraction * radius;
    }

    drawSeries(series, geometry, angles, range)
    {
        const lineWidth = Math.max(0, Number(this.parameters.line_width) || 0);
        const pointSize = Math.max(0, Number(this.parameters.point_size) || 0);
        const opacityValue = Number(this.parameters.fill_opacity);
        const fillOpacity = Number.isFinite(opacityValue) ? Math.max(0, Math.min(1, opacityValue)) : 0.2;
        this.canvas.lineJoin = this.parameters.line_join;
        series.forEach((row, seriesIndex) => {
            const points = row.map((value, index) => this.pointAt(geometry, angles[index], this.valueRadius(value, range, geometry.radius)));
            const stroke = this.getListValue(this.parameters.stroke_color || this.format.stroke_color, seriesIndex, "black");
            const fill = this.getListValue(this.parameters.fill_color || this.format.fill_color, seriesIndex, stroke);
            this.canvas.beginPath();
            points.forEach((point, index) => index === 0 ? this.canvas.moveTo(point.x, point.y) : this.canvas.lineTo(point.x, point.y));
            if(points.length >= 3)
                this.canvas.closePath();
            if(points.length >= 3 && fillOpacity > 0)
            {
                this.canvas.save();
                this.canvas.globalAlpha = fillOpacity;
                this.canvas.fillStyle = fill;
                this.canvas.fill();
                this.canvas.restore();
            }
            this.canvas.strokeStyle = stroke;
            this.canvas.lineWidth = lineWidth;
            this.canvas.stroke();
            if(this.parameters.show_points && pointSize > 0)
            {
                this.canvas.fillStyle = stroke;
                for(const point of points)
                {
                    this.canvas.beginPath();
                    this.canvas.arc(point.x, point.y, pointSize, 0, 2 * Math.PI);
                    this.canvas.fill();
                }
            }
        });
    }

    drawLabels(labels, geometry, angles)
    {
        if(!this.parameters.show_labels)
            return;
        this.canvas.font = this.parameters.label_font || this.format.label_font || "12px sans-serif";
        this.canvas.fillStyle = this.parameters.label_color || this.format.labelColor || "black";
        this.canvas.textBaseline = "middle";
        labels.forEach((label, index) => {
            const point = this.pointAt(geometry, angles[index], geometry.radius + geometry.labelOffset);
            const cosine = Math.cos(angles[index]);
            this.canvas.textAlign = cosine > 0.2 ? "left" : cosine < -0.2 ? "right" : "center";
            this.canvas.fillText(this.truncateLabel(label, geometry.maxLabelWidth), point.x, point.y, geometry.maxLabelWidth);
        });
    }

    drawPolarPlot(width, height)
    {
        if(this.series.length === 0)
            return;
        const axisCount = this.series[0].length;
        const labels = this.getLabels(axisCount);
        const angles = this.getAngles(axisCount);
        const geometry = this.getGeometry(width, height, labels, angles);
        const range = this.getValueRange(this.series);
        this.minimum = range.min;
        this.maximum = range.max;
        this.drawGrid(geometry, angles);
        this.drawScale(geometry, range);
        this.drawSeries(this.series, geometry, angles, range);
        this.drawLabels(labels, geometry, angles);
    }

    requestData(dataSet)
    {
        super.requestData(dataSet);
        this.addSourceMetadata(dataSet, this.parameters.source);
    }

    update()
    {
        this.series = this.getSeries(this.getSource('source'));
        this.metadata = this.getSourceMetadata('source', null);
        this.beginCanvasDraw();
        this.drawPolarPlot(this.format.width, this.format.height);
    }
}


webui_widgets.add('webui-widget-polar-plot', WebUIWidgetPolarPlot);
