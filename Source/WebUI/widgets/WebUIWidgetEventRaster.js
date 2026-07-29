class WebUIWidgetEventRaster extends WebUIWidgetCanvas
{
    static template()
    {
        return [
            {'name':"EVENT RASTER", 'control':'header'},
            {'name':'title', 'default':"Event Raster", 'type':'string', 'control':'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'event_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'clear_source', 'default':"", 'type':'source', 'control':'textedit'},

            {'name':"DETECTION", 'control':'header'},
            {'name':'threshold', 'default':0.5, 'type':'float', 'control':'textedit'},
            {'name':'edge_triggered', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'absolute_value', 'default':"no", 'type':'bool', 'control':'checkbox'},
            {'name':'sample_interval', 'default':1, 'type':'int', 'control':'textedit'},

            {'name':"HISTORY", 'control':'header'},
            {'name':'history', 'default':100, 'type':'float', 'control':'textedit'},
            {'name':'time_unit', 'default':"ticks", 'type':'string', 'control':'menu', 'options':"ticks,seconds"},
            {'name':'scroll', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'time_min', 'default':0, 'type':'float', 'control':'textedit'},
            {'name':'time_max', 'default':100, 'type':'float', 'control':'textedit'},
            {'name':'maximum_events', 'default':10000, 'type':'int', 'control':'textedit'},

            {'name':"CHANNELS", 'control':'header'},
            {'name':'channel_min', 'default':0, 'type':'int', 'control':'textedit'},
            {'name':'channel_max', 'default':-1, 'type':'int', 'control':'textedit'},
            {'name':'flip_channels', 'default':"no", 'type':'bool', 'control':'checkbox'},
            {'name':'show_channel_labels', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'labels', 'default':"", 'type':'string', 'control':'textedit'},
            {'name':'label_width', 'default':72, 'type':'int', 'control':'textedit'},
            {'name':'label_font', 'default':"12px sans-serif", 'type':'string', 'control':'textedit'},
            {'name':'row_gap', 'default':1, 'type':'float', 'control':'textedit'},

            {'name':"EVENT STYLE", 'control':'header'},
            {'name':'event_style', 'default':"tick", 'type':'string', 'control':'menu', 'options':"tick,dot,square"},
            {'name':'event_size', 'default':3, 'type':'float', 'control':'textedit'},
            {'name':'event_color', 'default':'#202020', 'type':'string', 'control':'textedit'},
            {'name':'channel_colors', 'default':'', 'type':'string', 'control':'textedit'},
            {'name':'background_color', 'default':'#ffffff', 'type':'string', 'control':'textedit'},
            {'name':'alternate_row_color', 'default':'#f3f5f7', 'type':'string', 'control':'textedit'},
            {'name':'magnitude_mode', 'default':"none", 'type':'string', 'control':'menu', 'options':"none,height,width,opacity,color"},
            {'name':'value_min', 'default':0, 'type':'float', 'control':'textedit'},
            {'name':'value_max', 'default':1, 'type':'float', 'control':'textedit'},
            {'name':'color_map', 'default':"fire", 'type':'string', 'control':'menu', 'options':"gray,fire,spectrum,custom"},
            {'name':'color_map_colors', 'default':'', 'type':'string', 'control':'textedit'},
            {'name':'show_color_legend', 'default':"no", 'type':'bool', 'control':'checkbox'},
            {'name':'color_legend_width', 'default':14, 'type':'int', 'control':'textedit'},
            {'name':'color_legend_ticks', 'default':5, 'type':'int', 'control':'textedit'},
            {'name':'color_legend_decimals', 'default':2, 'type':'int', 'control':'textedit'},

            {'name':"DECORATIONS", 'control':'header'},
            {'name':'show_time_scale', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'time_grid_lines', 'default':5, 'type':'int', 'control':'textedit'},
            {'name':'show_grid', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'grid_color', 'default':'#d7dce0', 'type':'string', 'control':'textedit'},
            {'name':'axis_color', 'default':'#59636e', 'type':'string', 'control':'textedit'},
            {'name':'show_now_line', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'now_color', 'default':'#d1495b', 'type':'string', 'control':'textedit'},
            {'name':'decimals', 'default':1, 'type':'int', 'control':'textedit'},
        ];
    }

    init()
    {
        super.init();
        this.capacity = 0;
        this.eventTimes = new Float64Array(0);
        this.eventChannels = new Int32Array(0);
        this.eventValues = new Float64Array(0);
        this.eventHead = 0;
        this.eventCount = 0;
        this.previousActive = new Uint8Array(0);
        this.channelCount = 0;
        this.lastSeenTick = null;
        this.sampleCounter = 0;
        this.clearWasActive = false;
        this.syntheticTime = 0;
    }

    ensureCapacity()
    {
        const capacity = Math.max(1, Math.trunc(Number(this.parameters.maximum_events) || 10000));
        if(capacity === this.capacity)
            return;
        const oldTimes = this.eventTimes;
        const oldChannels = this.eventChannels;
        const oldValues = this.eventValues;
        const oldCapacity = this.capacity;
        const oldCount = this.eventCount;
        const oldHead = this.eventHead;
        this.capacity = capacity;
        this.eventTimes = new Float64Array(capacity);
        this.eventChannels = new Int32Array(capacity);
        this.eventValues = new Float64Array(capacity);
        this.eventHead = 0;
        this.eventCount = 0;
        if(oldCapacity === 0)
            return;
        const copyCount = Math.min(oldCount, capacity);
        for(let logical = oldCount - copyCount; logical < oldCount; logical++)
        {
            const index = (oldHead - oldCount + logical + oldCapacity) % oldCapacity;
            this.appendEvent(oldTimes[index], oldChannels[index], oldValues[index]);
        }
    }

    appendEvent(time, channel, value=1)
    {
        if(!Number.isFinite(time) || !Number.isFinite(channel) || !Number.isFinite(value) || this.capacity === 0)
            return;
        const index = this.eventHead;
        this.eventTimes[index] = time;
        this.eventChannels[index] = Math.trunc(channel);
        this.eventValues[index] = value;
        this.eventHead = (this.eventHead + 1) % this.capacity;
        this.eventCount = Math.min(this.capacity, this.eventCount + 1);
        this.channelCount = Math.max(this.channelCount, Math.trunc(channel) + 1);
    }

    clearEvents()
    {
        this.eventHead = 0;
        this.eventCount = 0;
        this.previousActive.fill(0);
    }

    eventIndex(logical)
    {
        return (this.eventHead - this.eventCount + logical + this.capacity) % this.capacity;
    }

    currentTime(tick)
    {
        if(this.parameters.time_unit === "seconds")
        {
            const duration = typeof controller !== "undefined" ? Number(controller.tick_duration) : 0;
            return tick * (Number.isFinite(duration) && duration > 0 ? duration : 1);
        }
        return tick;
    }

    denseValues()
    {
        const data = this.getSource('source', null);
        if(data === null || data === undefined)
            return [];
        return this.flattenSource(data).map(Number);
    }

    sparseRows()
    {
        const data = this.getSource('event_source', null);
        if(!Array.isArray(data) || data.length === 0)
            return [];
        if(Array.isArray(data[0]))
            return data;
        return [data];
    }

    sampleDense(values, time)
    {
        if(this.previousActive.length !== values.length)
            this.previousActive = new Uint8Array(values.length);
        this.channelCount = values.length;
        const threshold = Number(this.parameters.threshold) || 0;
        const absolute = this.toBool(this.parameters.absolute_value);
        const edge = this.toBool(this.parameters.edge_triggered);
        for(let channel = 0; channel < values.length; channel++)
        {
            const raw = Number(values[channel]);
            const value = absolute ? Math.abs(raw) : raw;
            const active = Number.isFinite(value) && value > threshold;
            if(active && (!edge || !this.previousActive[channel]))
                this.appendEvent(time, channel, value);
            this.previousActive[channel] = active ? 1 : 0;
        }
    }

    sampleSparse(rows, defaultTime)
    {
        for(const row of rows)
        {
            if(!Array.isArray(row) || row.length === 0)
                continue;
            const channel = Number(row[0]);
            const time = row.length > 1 ? Number(row[1]) : defaultTime;
            const value = row.length > 2 ? Number(row[2]) : 1;
            this.appendEvent(time, channel, value);
        }
    }

    updateClearState()
    {
        const value = Number(this.sourceScalar(this.getSource('clear_source', 0), 0));
        const active = Number.isFinite(value) && value !== 0;
        if(active && !this.clearWasActive)
            this.clearEvents();
        this.clearWasActive = active;
    }

    sampleInputs()
    {
        const controllerTick = typeof controller !== "undefined" ? Number(controller.tick) : Number.NaN;
        const tick = Number.isFinite(controllerTick) ? controllerTick : this.syntheticTime++;
        if(Number.isFinite(controllerTick) && controllerTick === this.lastSeenTick)
            return this.currentTime(tick);
        this.lastSeenTick = controllerTick;
        const now = this.currentTime(tick);
        const interval = Math.max(1, Math.trunc(Number(this.parameters.sample_interval) || 1));
        const shouldSample = this.sampleCounter % interval === 0;
        this.sampleCounter++;
        if(!shouldSample)
            return now;
        const sparse = this.sparseRows();
        if(sparse.length > 0)
            this.sampleSparse(sparse, now);
        else
            this.sampleDense(this.denseValues(), now);
        return now;
    }

    timeRange(now)
    {
        if(this.toBool(this.parameters.scroll))
        {
            const history = Math.max(Number.EPSILON, Number(this.parameters.history) || 100);
            return {minimum:now - history, maximum:now};
        }
        let minimum = Number(this.parameters.time_min);
        let maximum = Number(this.parameters.time_max);
        if(!Number.isFinite(minimum)) minimum = 0;
        if(!Number.isFinite(maximum) || maximum === minimum) maximum = minimum + 1;
        if(maximum < minimum) [minimum, maximum] = [maximum, minimum];
        return {minimum, maximum};
    }

    channelRange()
    {
        const minimum = Math.max(0, Math.trunc(Number(this.parameters.channel_min) || 0));
        const configuredMaximum = Math.trunc(Number(this.parameters.channel_max));
        const maximum = Number.isFinite(configuredMaximum) && configuredMaximum >= minimum ? configuredMaximum : Math.max(minimum, this.channelCount - 1);
        return {minimum, maximum, count:maximum - minimum + 1};
    }

    normalizedMagnitude(value)
    {
        const minimum = Number(this.parameters.value_min);
        const maximum = Number(this.parameters.value_max);
        if(!Number.isFinite(value) || !Number.isFinite(minimum) || !Number.isFinite(maximum) || maximum === minimum)
            return 0;
        return Math.max(0, Math.min(1, (value - minimum) / (maximum - minimum)));
    }

    channelColor(channel)
    {
        const colors = String(this.parameters.channel_colors || "").split(',').map(color => color.trim()).filter(Boolean);
        return colors.length > 0 ? colors[channel % colors.length] : String(this.parameters.event_color || "#202020");
    }

    drawEvent(x, y, rowHeight, channel, value, colorMap)
    {
        const fraction = this.normalizedMagnitude(value);
        const mode = this.parameters.magnitude_mode || "none";
        const baseSize = Math.max(1, Number(this.parameters.event_size) || 3);
        const size = mode === "width" ? Math.max(1, baseSize * (0.25 + 0.75 * fraction)) : baseSize;
        const height = mode === "height" ? rowHeight * (0.15 + 0.75 * fraction) : Math.max(2, rowHeight - 2 * Math.max(0, Number(this.parameters.row_gap) || 0));
        this.canvas.globalAlpha = mode === "opacity" ? 0.15 + 0.85 * fraction : 1;
        this.canvas.fillStyle = mode === "color" ? this.getColorMapColor(value, this.parameters.value_min, this.parameters.value_max, colorMap) : this.channelColor(channel);
        this.canvas.strokeStyle = this.canvas.fillStyle;
        this.canvas.lineWidth = size;
        if(this.parameters.event_style === "dot")
        {
            this.canvas.beginPath();
            this.canvas.arc(x, y, size, 0, 2 * Math.PI);
            this.canvas.fill();
        }
        else if(this.parameters.event_style === "square")
            this.canvas.fillRect(x - size, y - size, 2 * size, 2 * size);
        else
        {
            this.canvas.beginPath();
            this.canvas.moveTo(x, y - height / 2);
            this.canvas.lineTo(x, y + height / 2);
            this.canvas.stroke();
        }
    }

    formatTime(value)
    {
        const decimals = Math.max(0, Math.min(10, Math.trunc(Number(this.parameters.decimals) || 0)));
        return Number(value).toFixed(decimals).replace(/\.?0+$/, "");
    }

    drawRaster(now)
    {
        const width = Math.max(1, this.format.width);
        const height = Math.max(1, this.format.height);
        const labelWidth = this.toBool(this.parameters.show_channel_labels) ? Math.max(0, Number(this.parameters.label_width) || 72) : 0;
        const scaleHeight = this.toBool(this.parameters.show_time_scale) ? 26 : 0;
        const legendWidth = this.parameters.magnitude_mode === "color" ? this.getVerticalColorLegendSpace() : 0;
        const plotX = labelWidth;
        const plotY = 0;
        const plotWidth = Math.max(1, width - labelWidth - legendWidth);
        const plotHeight = Math.max(1, height - scaleHeight);
        const times = this.timeRange(now);
        const channels = this.channelRange();
        const rowHeight = plotHeight / channels.count;
        const labels = String(this.parameters.labels || "").split(',').map(label => label.trim());
        const colorMap = this.getColorMap(this.parameters.color_map, this.parameters.color_map_colors);

        this.canvas.fillStyle = this.parameters.background_color || "#ffffff";
        this.canvas.fillRect(0, 0, width, height);
        for(let row = 0; row < channels.count; row++)
        {
            if(row % 2 === 1 && this.parameters.alternate_row_color)
            {
                this.canvas.fillStyle = this.parameters.alternate_row_color;
                this.canvas.fillRect(plotX, plotY + row * rowHeight, plotWidth, rowHeight);
            }
            if(this.toBool(this.parameters.show_channel_labels))
            {
                const channel = this.toBool(this.parameters.flip_channels) ? channels.maximum - row : channels.minimum + row;
                this.canvas.fillStyle = this.format.axis_color || this.parameters.axis_color || "#59636e";
                this.canvas.font = this.parameters.label_font || "12px sans-serif";
                this.canvas.textAlign = "right";
                this.canvas.textBaseline = "middle";
                this.canvas.fillText(labels[channel] || String(channel), plotX - 7, plotY + (row + 0.5) * rowHeight);
            }
        }

        if(this.toBool(this.parameters.show_grid))
        {
            this.canvas.strokeStyle = this.parameters.grid_color || "#d7dce0";
            this.canvas.lineWidth = 1;
            const gridLines = Math.max(1, Math.trunc(Number(this.parameters.time_grid_lines) || 5));
            for(let line = 0; line <= gridLines; line++)
            {
                const x = plotX + line * plotWidth / gridLines;
                this.canvas.beginPath();
                this.canvas.moveTo(x, plotY);
                this.canvas.lineTo(x, plotY + plotHeight);
                this.canvas.stroke();
            }
            for(let row = 0; row <= channels.count; row++)
            {
                const y = plotY + row * rowHeight;
                this.canvas.beginPath();
                this.canvas.moveTo(plotX, y);
                this.canvas.lineTo(plotX + plotWidth, y);
                this.canvas.stroke();
            }
        }

        this.canvas.save();
        this.canvas.beginPath();
        this.canvas.rect(plotX, plotY, plotWidth, plotHeight);
        this.canvas.clip();
        for(let logical = 0; logical < this.eventCount; logical++)
        {
            const index = this.eventIndex(logical);
            const time = this.eventTimes[index];
            const channel = this.eventChannels[index];
            if(time < times.minimum || time > times.maximum || channel < channels.minimum || channel > channels.maximum)
                continue;
            const x = plotX + (time - times.minimum) * plotWidth / (times.maximum - times.minimum);
            const row = this.toBool(this.parameters.flip_channels) ? channels.maximum - channel : channel - channels.minimum;
            const y = plotY + (row + 0.5) * rowHeight;
            this.drawEvent(x, y, rowHeight, channel, this.eventValues[index], colorMap);
        }
        this.canvas.restore();
        this.canvas.globalAlpha = 1;

        if(this.toBool(this.parameters.show_now_line) && now >= times.minimum && now <= times.maximum)
        {
            const x = plotX + (now - times.minimum) * plotWidth / (times.maximum - times.minimum);
            this.canvas.strokeStyle = this.parameters.now_color || "#d1495b";
            this.canvas.lineWidth = 1;
            this.canvas.beginPath();
            this.canvas.moveTo(x, plotY);
            this.canvas.lineTo(x, plotY + plotHeight);
            this.canvas.stroke();
        }

        if(this.toBool(this.parameters.show_time_scale))
        {
            const ticks = Math.max(1, Math.trunc(Number(this.parameters.time_grid_lines) || 5));
            this.canvas.fillStyle = this.format.axis_color || this.parameters.axis_color || "#59636e";
            this.canvas.font = this.format.scale_font || "12px sans-serif";
            this.canvas.textBaseline = "top";
            for(let tick = 0; tick <= ticks; tick++)
            {
                const fraction = tick / ticks;
                const x = plotX + fraction * plotWidth;
                this.canvas.textAlign = tick === 0 ? "left" : tick === ticks ? "right" : "center";
                this.canvas.fillText(this.formatTime(times.minimum + fraction * (times.maximum - times.minimum)), x, plotY + plotHeight + 5);
            }
        }

        if(this.parameters.magnitude_mode === "color")
            this.drawVerticalColorLegend(plotX + plotWidth + 8, plotY + 4, Math.max(1, plotHeight - 8), Number(this.parameters.value_min), Number(this.parameters.value_max), colorMap);
    }

    update()
    {
        this.ensureCapacity();
        this.updateClearState();
        const now = this.sampleInputs();
        this.beginCanvasDraw();
        this.drawRaster(now);
    }
}


webui_widgets.add('webui-widget-event-raster', WebUIWidgetEventRaster);
