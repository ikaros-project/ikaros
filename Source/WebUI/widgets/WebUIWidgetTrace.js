class WebUIWidgetTrace extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "TRACE", 'control':'header'},
            {'name':'title', 'default':"Trace", 'type':'string', 'control':'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'x_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'y_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'clear_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'order', 'default':"row", 'type':'string', 'control':'menu', 'options':"row,col"},
            {'name':'select_x', 'default':0, 'type':'int', 'control':'textedit'},
            {'name':'select_y', 'default':1, 'type':'int', 'control':'textedit'},

            {'name': "HISTORY", 'control':'header'},
            {'name':'history_length', 'default':100, 'type':'int', 'control':'textedit'},
            {'name':'sample_interval', 'default':1, 'type':'int', 'control':'textedit'},
            {'name':'hold_missing', 'default':"no", 'type':'bool', 'control':'checkbox'},
            {'name':'reset_on_shape_change', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'break_distance', 'default':0, 'type':'float', 'control':'textedit'},

            {'name': "TRAIL STYLE", 'control':'header'},
            {'name':'trail_style', 'default':"fade-taper", 'type':'string', 'control':'menu', 'options':"solid,fade,taper,fade-taper"},
            {'name':'stroke_color', 'default':'#2385c7,#e05a47,#4c9f70,#8a63b8', 'type':'string', 'control':'textedit'},
            {'name':'fill_color', 'default':'#2385c7,#e05a47,#4c9f70,#8a63b8', 'type':'string', 'control':'textedit'},
            {'name':'line_width', 'default':3, 'type':'float', 'control':'textedit'},
            {'name':'line_cap', 'default':"round", 'type':'string', 'control':'menu', 'options':"butt,round,square"},
            {'name':'line_join', 'default':"round", 'type':'string', 'control':'menu', 'options':"miter,round,bevel"},
            {'name':'history_opacity', 'default':0.9, 'min':0, 'max':1, 'type':'float', 'control':'slider'},
            {'name':'fade_power', 'default':1.5, 'type':'float', 'control':'textedit'},
            {'name':'minimum_width_fraction', 'default':0.2, 'min':0, 'max':1, 'type':'float', 'control':'slider'},

            {'name': "MARKERS", 'control':'header'},
            {'name':'show_current', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'current_marker', 'default':"circle", 'type':'string', 'control':'menu', 'options':"none,circle,dot,cross,square"},
            {'name':'current_marker_size', 'default':7, 'type':'float', 'control':'textedit'},
            {'name':'show_start', 'default':"no", 'type':'bool', 'control':'checkbox'},
            {'name':'start_marker_size', 'default':4, 'type':'float', 'control':'textedit'},
            {'name':'show_direction', 'default':"no", 'type':'bool', 'control':'checkbox'},

            {'name': "LABELS", 'control':'header'},
            {'name':'show_labels', 'default':"no", 'type':'bool', 'control':'checkbox'},
            {'name':'labels', 'default':"", 'type':'string', 'control':'textedit'},
            {'name':'label_font', 'default':"13px sans-serif", 'type':'string', 'control':'textedit'},
            {'name':'label_offset_x', 'default':8, 'type':'float', 'control':'textedit'},
            {'name':'label_offset_y', 'default':-8, 'type':'float', 'control':'textedit'},

            {'name': "COORDINATE SYSTEM", 'control':'header'},
            {'name':'scale_visibility', 'default':"no", 'type':'string', 'control':'menu', 'options':"yes,no,invisible", 'class':'true'},
            {'name':'auto_range', 'default':"no", 'type':'bool', 'control':'checkbox'},
            {'name':'include_zero', 'default':"no", 'type':'bool', 'control':'checkbox'},
            {'name':'x_min', 'default':-1, 'type':'float', 'control':'textedit'},
            {'name':'x_max', 'default':1, 'type':'float', 'control':'textedit'},
            {'name':'y_min', 'default':-1, 'type':'float', 'control':'textedit'},
            {'name':'y_max', 'default':1, 'type':'float', 'control':'textedit'},
            {'name':'show_x_axis', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'show_y_axis', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'flip_x_axis', 'default':"no", 'type':'string', 'control':'menu', 'options':"yes,no"},
            {'name':'flip_y_axis', 'default':"no", 'type':'string', 'control':'menu', 'options':"yes,no"},
            {'name':'flip_x_canvas', 'default':"no", 'type':'string', 'control':'menu', 'options':"yes,no"},
            {'name':'flip_y_canvas', 'default':"no", 'type':'string', 'control':'menu', 'options':"yes,no"},
            {'name':'left_scale_ticks', 'default':5, 'type':'int', 'control':'textedit'},
            {'name':'bottom_scale_ticks', 'default':5, 'type':'int', 'control':'textedit'},
            {'name':'horizontal_grid_lines', 'default':5, 'type':'int', 'control':'textedit'},
            {'name':'vertical_grid_lines', 'default':5, 'type':'int', 'control':'textedit'},
            {'name':'decimals', 'default':2, 'type':'int', 'control':'textedit'},
        ];
    }

    init()
    {
        super.init();
        this.histories = [];
        this.historyCapacity = 0;
        this.sampleCounter = 0;
        this.lastSeenTick = null;
        this.clearWasActive = false;
        this.drawRanges = null;
    }

    createHistory(capacity)
    {
        return {
            x:new Float64Array(capacity),
            y:new Float64Array(capacity),
            valid:new Uint8Array(capacity),
            head:0,
            count:0,
        };
    }

    appendRaw(history, x, y, valid)
    {
        const index = history.head;
        history.x[index] = x;
        history.y[index] = y;
        history.valid[index] = valid ? 1 : 0;
        history.head = (history.head + 1) % this.historyCapacity;
        history.count = Math.min(this.historyCapacity, history.count + 1);
    }

    historyIndex(history, logicalIndex)
    {
        return (history.head - history.count + logicalIndex + this.historyCapacity) % this.historyCapacity;
    }

    copyHistory(source, target)
    {
        const count = Math.min(source.count, this.historyCapacity);
        const first = Math.max(0, source.count - count);
        for(let logicalIndex = first; logicalIndex < source.count; logicalIndex++)
        {
            const index = (source.head - source.count + logicalIndex + source.x.length) % source.x.length;
            this.appendRaw(target, source.x[index], source.y[index], source.valid[index]);
        }
    }

    ensureHistories(seriesCount)
    {
        const capacity = Math.max(2, Math.trunc(Number(this.parameters.history_length) || 100));
        if(capacity === this.historyCapacity && seriesCount === this.histories.length)
            return;
        const previous = this.histories;
        const previousCapacity = this.historyCapacity;
        this.historyCapacity = capacity;
        this.histories = Array.from({length:seriesCount}, () => this.createHistory(capacity));
        if(!this.toBool(this.parameters.reset_on_shape_change) && previousCapacity > 0)
            for(let index = 0; index < Math.min(previous.length, this.histories.length); index++)
                this.copyHistory(previous[index], this.histories[index]);
    }

    clearHistories()
    {
        for(const history of this.histories)
        {
            history.head = 0;
            history.count = 0;
            history.valid.fill(0);
        }
    }

    vectorSource(name)
    {
        const data = this.getSource(name, null);
        if(data === null || data === undefined)
            return null;
        return this.flattenSource(data).map(Number);
    }

    packedPositions()
    {
        const data = this.getSource('source', null);
        const selectX = Math.max(0, Math.trunc(Number(this.parameters.select_x) || 0));
        const selectY = Math.max(0, Math.trunc(Number(this.parameters.select_y) || 0));
        if(this.getMatrixRank(data) === 1)
            return data.length > Math.max(selectX, selectY) ? [[Number(data[selectX]), Number(data[selectY])]] : [];
        if(this.getMatrixRank(data) !== 2 || !Array.isArray(data) || data.length === 0)
            return [];

        if(this.parameters.order === "col")
        {
            if(data.length <= Math.max(selectX, selectY))
                return [];
            const count = Math.min(data[selectX].length, data[selectY].length);
            return Array.from({length:count}, (_, index) => [Number(data[selectX][index]), Number(data[selectY][index])]);
        }
        const positions = [];
        for(const row of data)
            if(Array.isArray(row) && row.length > Math.max(selectX, selectY))
                positions.push([Number(row[selectX]), Number(row[selectY])]);
        return positions;
    }

    currentPositions()
    {
        const x = this.vectorSource('x_source');
        const y = this.vectorSource('y_source');
        if(x && y)
        {
            const count = Math.min(x.length, y.length);
            return Array.from({length:count}, (_, index) => [x[index], y[index]]);
        }
        return this.packedPositions();
    }

    lastValidPoint(history, skip=0)
    {
        let skipped = 0;
        for(let logicalIndex = history.count - 1; logicalIndex >= 0; logicalIndex--)
        {
            const index = this.historyIndex(history, logicalIndex);
            if(!history.valid[index])
                return null;
            if(skipped++ < skip)
                continue;
            return {x:history.x[index], y:history.y[index], logicalIndex};
        }
        return null;
    }

    appendPosition(history, x, y)
    {
        const valid = Number.isFinite(x) && Number.isFinite(y);
        if(!valid && this.toBool(this.parameters.hold_missing))
            return;
        if(valid)
        {
            const previous = this.lastValidPoint(history);
            const breakDistance = Math.max(0, Number(this.parameters.break_distance) || 0);
            if(previous && breakDistance > 0 && Math.hypot(x - previous.x, y - previous.y) > breakDistance)
                this.appendRaw(history, 0, 0, false);
        }
        this.appendRaw(history, valid ? x : 0, valid ? y : 0, valid);
    }

    updateClearState()
    {
        const value = Number(this.sourceScalar(this.getSource('clear_source', 0), 0));
        const active = Number.isFinite(value) && value !== 0;
        if(active && !this.clearWasActive)
            this.clearHistories();
        this.clearWasActive = active;
    }

    samplePositions(positions)
    {
        if(positions.length === 0 && this.toBool(this.parameters.hold_missing))
            return;
        const controllerTick = typeof controller !== "undefined" ? Number(controller.tick) : Number.NaN;
        if(Number.isFinite(controllerTick))
        {
            if(controllerTick === this.lastSeenTick)
                return;
            this.lastSeenTick = controllerTick;
        }
        this.ensureHistories(positions.length);
        const interval = Math.max(1, Math.trunc(Number(this.parameters.sample_interval) || 1));
        const shouldSample = this.sampleCounter % interval === 0;
        this.sampleCounter++;
        if(!shouldSample)
            return;
        for(let index = 0; index < positions.length; index++)
            this.appendPosition(this.histories[index], Number(positions[index][0]), Number(positions[index][1]));
    }

    calculateRanges()
    {
        let xMin = Number(this.parameters.x_min);
        let xMax = Number(this.parameters.x_max);
        let yMin = Number(this.parameters.y_min);
        let yMax = Number(this.parameters.y_max);
        if(this.toBool(this.parameters.auto_range))
        {
            xMin = Infinity;
            xMax = -Infinity;
            yMin = Infinity;
            yMax = -Infinity;
            for(const history of this.histories)
                for(let logicalIndex = 0; logicalIndex < history.count; logicalIndex++)
                {
                    const index = this.historyIndex(history, logicalIndex);
                    if(!history.valid[index])
                        continue;
                    xMin = Math.min(xMin, history.x[index]);
                    xMax = Math.max(xMax, history.x[index]);
                    yMin = Math.min(yMin, history.y[index]);
                    yMax = Math.max(yMax, history.y[index]);
                }
            if(this.toBool(this.parameters.include_zero))
            {
                xMin = Math.min(xMin, 0);
                xMax = Math.max(xMax, 0);
                yMin = Math.min(yMin, 0);
                yMax = Math.max(yMax, 0);
            }
        }
        if(!Number.isFinite(xMin) || !Number.isFinite(xMax))
        {
            xMin = -1;
            xMax = 1;
        }
        if(!Number.isFinite(yMin) || !Number.isFinite(yMax))
        {
            yMin = -1;
            yMax = 1;
        }
        if(xMin === xMax)
        {
            xMin -= 0.5;
            xMax += 0.5;
        }
        if(yMin === yMax)
        {
            yMin -= 0.5;
            yMax += 0.5;
        }
        if(this.toBool(this.parameters.auto_range))
        {
            const xPadding = 0.05 * (xMax - xMin);
            const yPadding = 0.05 * (yMax - yMin);
            xMin -= xPadding;
            xMax += xPadding;
            yMin -= yPadding;
            yMax += yPadding;
        }
        return {xMin, xMax, yMin, yMax};
    }

    getYRange()
    {
        if(this.drawRanges)
            return {min:this.drawRanges.yMin, max:this.drawRanges.yMax};
        return super.getYRange();
    }

    traceColor(index)
    {
        this.setColor(index);
        return {stroke:this.canvas.strokeStyle, fill:this.canvas.fillStyle};
    }

    plotPoint(x, y, width, height, transform)
    {
        const ranges = this.drawRanges;
        const px = (x - ranges.xMin) * width / (ranges.xMax - ranges.xMin);
        const py = (y - ranges.yMin) * height / (ranges.yMax - ranges.yMin);
        const transformed = transform(px, py);
        return {x:transformed[0], y:transformed[1]};
    }

    drawMarker(point, type, size, color)
    {
        if(!point || type === "none" || size <= 0)
            return;
        this.canvas.save();
        this.canvas.globalAlpha = 1;
        this.canvas.strokeStyle = color.stroke;
        this.canvas.fillStyle = color.fill;
        this.canvas.lineWidth = Math.max(1, Number(this.parameters.line_width) || 1);
        this.canvas.beginPath();
        if(type === "circle" || type === "dot")
            this.canvas.arc(point.x, point.y, size, 0, 2 * Math.PI);
        else if(type === "cross")
        {
            this.canvas.moveTo(point.x - size, point.y);
            this.canvas.lineTo(point.x + size, point.y);
            this.canvas.moveTo(point.x, point.y - size);
            this.canvas.lineTo(point.x, point.y + size);
        }
        else if(type === "square")
            this.canvas.rect(point.x - size, point.y - size, 2 * size, 2 * size);
        if(type === "dot")
            this.canvas.fill();
        else
            this.canvas.stroke();
        this.canvas.restore();
    }

    drawHistory(history, traceIndex, width, height, transform)
    {
        if(history.count === 0)
            return;
        const color = this.traceColor(traceIndex);
        const style = this.parameters.trail_style || "fade-taper";
        const fade = style === "fade" || style === "fade-taper";
        const taper = style === "taper" || style === "fade-taper";
        const opacity = Math.max(0, Math.min(1, Number(this.parameters.history_opacity)));
        const fadePower = Math.max(0.01, Number(this.parameters.fade_power) || 1);
        const baseWidth = Math.max(0.1, Number(this.parameters.line_width) || 1);
        const minimumWidth = Math.max(0, Math.min(1, Number(this.parameters.minimum_width_fraction)));
        this.canvas.save();
        this.canvas.strokeStyle = color.stroke;
        this.canvas.lineCap = this.parameters.line_cap || "round";
        this.canvas.lineJoin = this.parameters.line_join || "round";
        let previous = null;
        let first = null;
        for(let logicalIndex = 0; logicalIndex < history.count; logicalIndex++)
        {
            const index = this.historyIndex(history, logicalIndex);
            if(!history.valid[index])
            {
                previous = null;
                continue;
            }
            const point = this.plotPoint(history.x[index], history.y[index], width, height, transform);
            if(!first)
                first = point;
            if(previous)
            {
                const fraction = history.count > 1 ? logicalIndex / (history.count - 1) : 1;
                this.canvas.globalAlpha = fade ? opacity * Math.pow(fraction, fadePower) : opacity;
                this.canvas.lineWidth = taper ? baseWidth * (minimumWidth + (1 - minimumWidth) * fraction) : baseWidth;
                this.canvas.beginPath();
                this.canvas.moveTo(previous.x, previous.y);
                this.canvas.lineTo(point.x, point.y);
                this.canvas.stroke();
            }
            previous = point;
        }
        this.canvas.restore();

        const currentData = this.lastValidPoint(history);
        const previousData = this.lastValidPoint(history, 1);
        const current = currentData ? this.plotPoint(currentData.x, currentData.y, width, height, transform) : null;
        if(this.toBool(this.parameters.show_start))
            this.drawMarker(first, "circle", Math.max(0, Number(this.parameters.start_marker_size) || 0), color);
        if(this.toBool(this.parameters.show_current))
            this.drawMarker(current, this.parameters.current_marker || "circle", Math.max(0, Number(this.parameters.current_marker_size) || 0), color);
        if(this.toBool(this.parameters.show_direction) && currentData && previousData)
        {
            const previousPoint = this.plotPoint(previousData.x, previousData.y, width, height, transform);
            this.canvas.save();
            this.canvas.strokeStyle = color.stroke;
            this.canvas.fillStyle = color.stroke;
            this.canvas.lineWidth = baseWidth;
            this.drawArrowHead(previousPoint.x, previousPoint.y, current.x, current.y);
            this.canvas.restore();
        }
        if(this.toBool(this.parameters.show_labels) && current)
        {
            const labels = String(this.parameters.labels || "").split(',').map((label) => label.trim());
            const label = labels[traceIndex] || String(traceIndex + 1);
            this.canvas.save();
            this.canvas.globalAlpha = 1;
            this.canvas.fillStyle = color.stroke;
            this.canvas.font = this.parameters.label_font || "13px sans-serif";
            this.canvas.textAlign = "left";
            this.canvas.textBaseline = "bottom";
            this.canvas.fillText(
                label,
                current.x + (Number(this.parameters.label_offset_x) || 0),
                current.y + (Number(this.parameters.label_offset_y) || 0)
            );
            this.canvas.restore();
        }
    }

    drawPlotHorizontal(width, height, index, transform)
    {
        for(let traceIndex = 0; traceIndex < this.histories.length; traceIndex++)
            this.drawHistory(this.histories[traceIndex], traceIndex, width, height, transform);
    }

    update()
    {
        const positions = this.currentPositions();
        this.updateClearState();
        this.samplePositions(positions);
        this.drawRanges = this.calculateRanges();
        const originalXMin = this.parameters.x_min;
        const originalXMax = this.parameters.x_max;
        this.parameters.x_min = this.drawRanges.xMin;
        this.parameters.x_max = this.drawRanges.xMax;
        this.beginCanvasDraw();
        this.drawHorizontal(1, 1);
        this.parameters.x_min = originalXMin;
        this.parameters.x_max = originalXMax;
    }
}


webui_widgets.add('webui-widget-trace', WebUIWidgetTrace);
