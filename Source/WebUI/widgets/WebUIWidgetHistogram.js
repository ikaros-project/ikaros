class WebUIWidgetHistogram extends WebUIWidgetGraph
{
    roundUpToSignificantFigure(value)
    {
        if(!Number.isFinite(value) || value === 0)
            return 0;
        const scale = Math.pow(10, Math.floor(Math.log10(Math.abs(value))));
        return Math.ceil(value / scale) * scale;
    }

    roundDownToSignificantFigure(value)
    {
        if(!Number.isFinite(value) || value === 0)
            return 0;
        const scale = Math.pow(10, Math.floor(Math.log10(Math.abs(value))));
        return Math.floor(value / scale) * scale;
    }

    static template()
    {
        return [
            {'name': "HISTOGRAM", 'control':'header'},
            {'name':'title', 'default':"Histogram", 'type':'string', 'control': 'textedit'},
 //           {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'histogramMinSource', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'histogramMaxSource', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'meanSource', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'stdevSource', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'xAxisLabel', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'yAxisLabel', 'default':"", 'type':'string', 'control': 'textedit'},

            {'name': "STYLE", 'control':'header'},
            {'name':'direction', 'default':"vertical", 'type':'string', 'min':0, 'max':2, 'control': 'menu', 'options': "horizontal,vertical", 'class':'true'},
            {'name':'transpose', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'labels', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'color', 'default':'', 'type':'string', 'control': 'textedit'},   // TODO: no default = get from CSS would be a good functionality
            {'name':'fill', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'curveColor', 'default':'black', 'type':'string', 'control': 'textedit'},
            {'name':'lineWidth', 'default':1, 'type':'float', 'control': 'textedit'},
 //           {'name':'lineDash', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'lineCap', 'default':"", 'type':'string', 'control': 'menu', 'options': "butt,round,quare"},
            {'name':'lineJoin', 'default':"", 'type':'string', 'control': 'menu', 'options': "miter,round,bevel"},

            {'name': "LAYOUT", 'control':'header'},
            {'name':'marginLeft', 'default':4, 'type':'int', 'control': 'textedit'},
            {'name':'marginRight', 'default':4, 'type':'int', 'control': 'textedit'},
            {'name':'marginTop', 'default':4, 'type':'int', 'control': 'textedit'},
            {'name':'marginBottom', 'default':4, 'type':'int', 'control': 'textedit'},
            {'name':'spaceLeft', 'default':4, 'type':'int', 'control': 'textedit'},
            {'name':'spaceRight', 'default':4, 'type':'int', 'control': 'textedit'},
            {'name':'spaceTop', 'default':4, 'type':'int', 'control': 'textedit'},
            {'name':'spaceBottom', 'default':4, 'type':'int', 'control': 'textedit'},

            {'name': "COORDINATE SYSTEM", 'control':'header'},
            {'name':'min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'auto', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'include_zero', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'xAxis', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'yAxis', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'leftScale', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'leftTickMarks', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'bottomScale', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'bottomTickMarks', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'horizontalGridlines', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'decimals', 'default':2, 'type':'int', 'control': 'textedit'},
        ]};

    init()
    {
        super.init();
        this.data = [];
        this.metadata = null;

        this.onclick = function () {
            if(main.edit_mode)
                return;
            if(!this.data)
                return;
            let s = "";
            for(let r of this.data)
            {
                for(let c of r)
                    s += c+"\t";
                s += "\n"
            }
            alert(s);
        };
    }

    requestData(data_set)
    {
        super.requestData(data_set);
        this.addSourceMetadata(data_set, this.parameters.source);
    }

    drawBarHorizontal(width, height, i)
    {
        this.canvas.beginPath();
        this.setColor(i)
        this.canvas.rect(0, 0, width, height);
        this.canvas.fill();
        this.canvas.stroke();
    }

    drawBarVertical(width, height, i)
    {
        this.canvas.beginPath();
        this.setColor(i)
        this.canvas.rect(0, 0, width, height);
        this.canvas.fill();
        this.canvas.stroke();
    }

    setHistogramColor(i)
    {
        this.setColor(i);
    }

    drawPlotHorizontal(width, height, y)
    {
        if(!Array.isArray(this.data) || this.data.length === 0 || !Array.isArray(this.data[0]))
            return;
        let n = this.data[0].length;
        let bar_height = (height)/(n + (n-1)*this.format.spacing);
        let bar_spacing = Math.round((1 + this.format.spacing) * bar_height);
        bar_height = Math.round(bar_height);
        const {min, max} = this.getYRange();
        let axisX = 0;
        if(min <= 0 && max >= 0)
            axisX = (-min / (max - min)) * width;
        else if(max < 0)
            axisX = width;

        this.canvas.save();
            for(let i=0; i<n; i++)
            {
                const valueX = ((this.data[y][i] - min) / (max - min)) * width;
                const left = Math.min(axisX, valueX);
                const barWidth = Math.abs(valueX - axisX);
                this.canvas.save();
                this.canvas.translate(left, 0);
                this.drawBarVertical(barWidth, bar_height, y);
                this.canvas.restore();
                this.canvas.translate(0, bar_spacing);
            }
        this.canvas.restore();
    }

    drawPlotVertical(width, height, y)
    {
        if(!Array.isArray(this.data) || this.data.length === 0 || !Array.isArray(this.data[0]))
            return;
        let n = this.data[0].length;
        const rawRange = this.getHistogramRawXRange();
        const scaleRange = this.getHistogramXRange();
        const useScaledBins = rawRange && scaleRange && scaleRange.max > scaleRange.min;
        let bar_width = (width)/(n + (n-1)*this.format.spacing);
        let bar_spacing = Math.round((1 + this.format.spacing) * bar_width);
        bar_width = Math.round(bar_width);
        let scaledBinWidth = bar_width;
        let scaledBinStep = bar_spacing;
        if(useScaledBins)
        {
            const rawBinWidth = (rawRange.max - rawRange.min) / n;
            scaledBinWidth = Math.max(1, rawBinWidth * width / (scaleRange.max - scaleRange.min));
        }
        const {min, max} = this.getYRange();
        let axisY = height;
        if(min <= 0 && max >= 0)
            axisY = this.getPlotYForValue(0, height);
        else if(max < 0)
            axisY = 0;

        this.canvas.save();
            for(let i=0; i<n; i++)
            {
                const valueY = this.getPlotYForValue(this.data[y][i], height);
                const top = Math.min(axisY, valueY);
                const barHeight = Math.abs(valueY - axisY);
                const barX = useScaledBins ? (rawRange.min + i * ((rawRange.max - rawRange.min) / n) - scaleRange.min) * width / (scaleRange.max - scaleRange.min) : i * scaledBinStep;
                this.canvas.save();
                this.canvas.translate(barX, top);
                this.drawBarVertical(scaledBinWidth, barHeight, y);
                this.canvas.restore();
            }
        this.canvas.restore();

        this.drawGaussianCurve(width, height, y);
    }

    drawGaussianCurve(width, height, y)
    {
        const rawRange = this.getHistogramRawXRange();
        const scaleRange = this.getHistogramXRange();
        if(!rawRange || !scaleRange || !this.parameters.meanSource || !this.parameters.stdevSource)
            return;

        const mean = this.getSourceFloatValueAt('meanSource', y);
        let stdev = this.getSourceFloatValueAt('stdevSource', y);
        if(!Number.isFinite(mean))
            return;

        const row = this.data?.[y];
        if(!Array.isArray(row) || row.length === 0)
            return;

        const sampleCount = row.reduce((sum, value) => {
            const numeric = parseFloat(value);
            return sum + (Number.isFinite(numeric) ? numeric : 0);
        }, 0);
        if(sampleCount <= 0)
            return;

        const rowMax = row.reduce((max, value) => {
            const numeric = parseFloat(value);
            return Number.isFinite(numeric) ? Math.max(max, numeric) : max;
        }, 0);
        if(rowMax <= 0)
            return;

        const rawRangeWidth = rawRange.max - rawRange.min;
        const scaleRangeWidth = scaleRange.max - scaleRange.min;
        if(rawRangeWidth <= 0 || scaleRangeWidth <= 0)
            return;
        const binWidth = rawRangeWidth / row.length;
        if(!Number.isFinite(stdev) || stdev <= 0)
            stdev = rawRangeWidth / Math.max(6, row.length * 2);
        stdev = Math.max(stdev, binWidth / 3);

        const sigma = stdev;
        const normalizer = 1 / (sigma * Math.sqrt(2 * Math.PI));
        const peak = normalizer;
        const steps = Math.max(200, row.length * 24, Math.round(width));

        this.canvas.save();
        const curveColors = String(this.parameters.curveColor || "").split(",").map(c => c.trim()).filter(c => c !== "");
        this.canvas.strokeStyle = curveColors.length ? curveColors[y % curveColors.length] : "black";
        this.canvas.lineWidth = Math.max(2, (parseFloat(this.parameters.lineWidth) || 1) + 1);
        this.canvas.lineCap = "round";
        this.canvas.lineJoin = "round";
        this.canvas.fillStyle = "transparent";
        this.canvas.beginPath();

        let started = false;
        for(let i=0; i<=steps; i++)
        {
            const xValue = scaleRange.min + (i / steps) * scaleRangeWidth;
            const x = (xValue - scaleRange.min) * width / scaleRangeWidth;
            const z = (xValue - mean) / sigma;
            const expectedCount = rowMax * (normalizer * Math.exp(-0.5 * z * z)) / peak;
            const yy = this.getPlotYForValue(expectedCount, height);

            if(!Number.isFinite(yy))
                continue;

            if(!started)
            {
                this.canvas.moveTo(x, yy);
                started = true;
            }
            else
                this.canvas.lineTo(x, yy);
        }

        if(started)
            this.canvas.stroke();
        this.canvas.restore();
    }

    getHistogramLabelWidth()
    {
        const labels = this.getHistogramLabels();
        if(labels.length === 0 || labels.every(label => label.trim() === "") || !this.canvas)
            return 0;

        this.canvas.save();
        this.canvas.font = this.format.labelFont;
        let width = 0;
        for(const label of labels)
            width = Math.max(width, this.canvas.measureText(label.trim()).width);
        this.canvas.restore();

        return Math.ceil(width + 16);
    }

    getYAxisTitleWidth()
    {
        return String(this.parameters.yAxisLabel || "").trim() === "" ? 0 : 18;
    }

    getEffectiveSpaceLeft(height)
    {
        const rowLabelWidth = this.getHistogramLabelWidth();
        const yAxisTitleWidth = this.getYAxisTitleWidth();

        if(!this.isYAxisEnabled())
            return rowLabelWidth + yAxisTitleWidth;

        const yDecorations =
            this.format.yAxis ||
            this.format.leftScale > 0 ||
            this.format.leftTickMarks > 0 ||
            this.format.horizontalGridlines > 0 ||
            this.format.horizontalGridlinesOver > 0;

        const base = yDecorations ? super.getEffectiveSpaceLeft(height) : 0;
        return base + rowLabelWidth + yAxisTitleWidth;
    }

    getEffectiveSpaceBottom()
    {
        if(!this.isXAxisEnabled())
            return 0;

        const range = this.getHistogramXRange();
        const xAxisLabel = String(this.parameters.xAxisLabel || "").trim();
        const xDecorations =
            this.format.xAxis ||
            (range && (this.format.bottomScale > 0 || this.format.bottomTickMarks > 0)) ||
            xAxisLabel !== "";
        let space = xDecorations ? (this.format.spaceBottom || 0) : 0;

        if(range && this.format.bottomScale > 0)
            space = Math.max(space, this.format.scaleOffset + 14);
        if(xAxisLabel !== "")
            space += 20;

        return space;
    }

    drawLabelsHorizontal(width, height, n)
    {
        const labels = this.getHistogramLabels();
        if(labels.length === 0 || labels.every(label => label.trim() === ""))
            return;

        const paneHeight = height / Math.max(1, n);
        const yAxisTitleWidth = this.getYAxisTitleWidth();
        const labelColumnWidth = this.getHistogramLabelWidth();
        if(labelColumnWidth <= 0)
            return;

        this.canvas.save();
        this.canvas.font = this.format.labelFont;
        this.canvas.fillStyle = this.format.labelColor;
        this.canvas.textAlign = "right";
        this.canvas.textBaseline = "middle";

        for(let i=0; i<n; i++)
        {
            const label = (labels[i] ?? "").trim();
            if(label !== "")
                this.canvas.fillText(label, yAxisTitleWidth + labelColumnWidth - 8, i * paneHeight + paneHeight / 2);
        }

        this.canvas.restore();
    }

    getHistogramLabels()
    {
        const explicitLabels = String(this.parameters.labels || "").split(",").map(label => label.trim());
        if(explicitLabels.some(label => label !== ""))
            return explicitLabels;

        const metadataLabels = this.metadata?.labels;
        const dimension = this.toBool(this.parameters.transpose) ? 1 : 0;
        if(Array.isArray(metadataLabels) && Array.isArray(metadataLabels[dimension]))
            return metadataLabels[dimension].map(label => String(label ?? "").trim());

        return [];
    }

    getHistogramXRange()
    {
        const range = this.getHistogramRawXRange();
        if(!range)
            return null;

        let {min, max} = range;

        if(min === max)
            max = min + 1;

        const ticks = Math.max(2, this.format.bottomScale || 5);
        const nice = this.getNiceAxisRange(min, max, ticks);
        return nice || {min, max};
    }

    getNiceAxisRange(min, max, ticks)
    {
        if(!Number.isFinite(min) || !Number.isFinite(max))
            return null;
        if(min === max)
            return {min, max: min + 1, step: 1};

        const span = max - min;
        const step = this.getNiceStep(span / Math.max(1, ticks - 1));
        if(!Number.isFinite(step) || step <= 0)
            return {min, max};

        const niceMin = Math.floor(min / step) * step;
        const niceMax = Math.ceil(max / step) * step;
        return {
            min: this.cleanAxisValue(niceMin),
            max: this.cleanAxisValue(niceMax),
            step
        };
    }

    getNiceStep(value)
    {
        if(!Number.isFinite(value) || value <= 0)
            return 1;

        const exponent = Math.floor(Math.log10(value));
        const magnitude = Math.pow(10, exponent);
        const fraction = value / magnitude;

        let niceFraction = 1;
        if(fraction <= 1)
            niceFraction = 1;
        else if(fraction <= 2)
            niceFraction = 2;
        else if(fraction <= 2.5)
            niceFraction = 2.5;
        else if(fraction <= 5)
            niceFraction = 5;
        else
            niceFraction = 10;

        return niceFraction * magnitude;
    }

    cleanAxisValue(value)
    {
        if(!Number.isFinite(value))
            return value;
        return parseFloat(value.toPrecision(12));
    }

    getHistogramRawXRange()
    {
        if(!this.parameters.histogramMinSource || !this.parameters.histogramMaxSource)
            return null;

        let min = this.getSourceFloatValue('histogramMinSource');
        let max = this.getSourceFloatValue('histogramMaxSource');
        if(!Number.isFinite(min) || !Number.isFinite(max))
            return null;

        if(min > max)
            [min, max] = [max, min];

        if(min === max)
            max = min + 1;

        return {min, max};
    }

    firstFiniteValue(value)
    {
        if(Array.isArray(value))
        {
            for(const item of value)
            {
                const numeric = this.firstFiniteValue(item);
                if(Number.isFinite(numeric))
                    return numeric;
            }
            return NaN;
        }

        const numeric = parseFloat(value);
        return Number.isFinite(numeric) ? numeric : NaN;
    }

    getSourceFloatValue(parameterName)
    {
        const source = this.parameters[parameterName];
        if(!source)
            return NaN;

        return this.firstFiniteValue(this.getSource(parameterName, NaN));
    }

    getSourceFloatValueAt(parameterName, index)
    {
        const source = this.parameters[parameterName];
        if(!source)
            return NaN;

        const data = this.getSource(parameterName, NaN);
        const flattened = [];
        const visit = (value) =>
        {
            if(Array.isArray(value))
            {
                for(const item of value)
                    visit(item);
                return;
            }

            const numeric = parseFloat(value);
            if(Number.isFinite(numeric))
                flattened.push(numeric);
        };
        visit(data);

        if(index < 0 || index >= flattened.length)
            return NaN;
        return flattened[index];
    }

    drawBottomScale(width, height)
    {
        // Drawn after the plot as an overlay in drawHistogramBottomScale().
    }

    drawBottomTickMarks(width, height)
    {
        // Drawn after the plot as an overlay in drawHistogramBottomScale().
    }

    drawHistogramBottomScale()
    {
        if(!this.isXAxisEnabled())
            return;

        const range = this.getHistogramXRange();
        const xAxisLabel = String(this.parameters.xAxisLabel || "").trim();
        const yAxisLabel = String(this.parameters.yAxisLabel || "").trim();
        if(!range && xAxisLabel === "" && yAxisLabel === "")
            return;

        const nScale = this.format.bottomScale;
        const nTicks = this.format.bottomTickMarks;
        if(!range && xAxisLabel === "")
            return;

        const contentLeft = this.format.marginLeft;
        const contentTop = this.format.marginTop;
        const contentHeight = this.format.height;
        const effectiveSpaceBottom = this.getEffectiveSpaceBottom();
        const effectiveSpaceLeft = this.getEffectiveSpaceLeft(contentHeight);
        const plotLeft = contentLeft + effectiveSpaceLeft;
        const plotWidth = this.format.width - effectiveSpaceLeft - this.format.spaceRight;
        const hasXAxisLabel = xAxisLabel !== "";
        const labelGap = hasXAxisLabel ? Math.max(14, parseInt(this.format.labelFont) || 14) : 0;
        const y = contentTop + contentHeight - effectiveSpaceBottom + this.format.spaceBottom;

        this.resetCanvasTransform(-0.5, -0.5);
        this.canvas.save();
        this.canvas.strokeStyle = this.format.axisColor;
        this.canvas.fillStyle = this.format.axisColor;

        if(range && nTicks > 0)
        {
            this.canvas.beginPath();
            this.canvas.lineWidth = 1;
            const tickCount = nTicks === 1 ? 1 : nTicks;
            for(let i=0; i<tickCount; i++)
            {
                const x = plotLeft + (tickCount === 1 ? 0 : i * plotWidth / (tickCount - 1));
                this.canvas.moveTo(Math.round(x), y);
                this.canvas.lineTo(Math.round(x), y + 7);
            }
            this.canvas.stroke();
        }

        if(range && nScale > 0)
        {
            this.canvas.font = this.format.scaleFont;
            this.canvas.textAlign = "center";
            this.canvas.textBaseline = "top";
            const labelCount = nScale === 1 ? 1 : nScale;
            for(let i=0; i<labelCount; i++)
            {
                const x = plotLeft + (labelCount === 1 ? 0 : i * plotWidth / (labelCount - 1));
                const value = range.min + (labelCount === 1 ? 0 : i * (range.max - range.min) / (labelCount - 1));
                this.canvas.fillText(this.formatScaleValue(value), x, y + this.format.scaleOffset);
            }
        }

        if(hasXAxisLabel)
        {
            this.canvas.font = this.format.labelFont;
            this.canvas.textAlign = "center";
            this.canvas.textBaseline = "top";
            this.canvas.fillText(xAxisLabel, plotLeft + plotWidth / 2, y + this.format.scaleOffset + 14);
        }

        if(yAxisLabel !== "")
        {
            this.canvas.font = this.format.labelFont;
            this.canvas.textAlign = "center";
            this.canvas.textBaseline = "middle";
            this.canvas.save();
            this.canvas.translate(contentLeft + this.getYAxisTitleWidth() / 2, contentTop + contentHeight / 2);
            this.canvas.rotate(-Math.PI / 2);
            this.canvas.fillText(yAxisLabel, 0, 0);
            this.canvas.restore();
        }

        this.canvas.restore();
    }

    draw(size_x, size_y)
    {
        const originalSpaceBottom = this.format.spaceBottom;
        const originalXAxis = this.format.xAxis;
        const originalYAxis = this.format.yAxis;
        const originalLeftScale = this.format.leftScale;
        const originalLeftTickMarks = this.format.leftTickMarks;
        const originalHorizontalGridlines = this.format.horizontalGridlines;
        const originalHorizontalGridlinesOver = this.format.horizontalGridlinesOver;
        this.format.spaceBottom = this.getEffectiveSpaceBottom();
        this.format.xAxis = this.isXAxisEnabled();
        const yAxisEnabled = this.isYAxisEnabled();
        this.format.yAxis = yAxisEnabled;
        if(!yAxisEnabled)
        {
            this.format.leftScale = 0;
            this.format.leftTickMarks = 0;
            this.format.horizontalGridlines = 0;
            this.format.horizontalGridlinesOver = 0;
        }
        super.draw(size_x, size_y);
        this.format.spaceBottom = originalSpaceBottom;
        this.format.xAxis = originalXAxis;
        this.format.yAxis = originalYAxis;
        this.format.leftScale = originalLeftScale;
        this.format.leftTickMarks = originalLeftTickMarks;
        this.format.horizontalGridlines = originalHorizontalGridlines;
        this.format.horizontalGridlinesOver = originalHorizontalGridlinesOver;
        this.drawHistogramBottomScale();
    }

    isXAxisEnabled()
    {
        return this.toBool(this.parameters.xAxis);
    }

    isYAxisEnabled()
    {
        return this.toBool(this.parameters.yAxis);
    }

    // update() get the data for the graph and calls WebUIWidgetGraph::draw() which in turn calls
    // drawPlotHorizontal() or drawPlotVertical()
    
    transpose(d)
    {
        if(!Array.isArray(d) || d.length === 0 || !Array.isArray(d[0]))
            return [];
        var e = d[0].map(function(col, i){
            return d.map(function(row){
                return row[i];
            });
        });
        return e;
    }

    update()
    {
        if(this.data = this.getSource('source'))
        {
            this.metadata = this.getSourceMetadata('source', null);
            if(!Array.isArray(this.data))
                return;
            if(typeof this.data[0] != "object") // FIXME: Fix for arbitrary matrix sizes
                this.data = [this.data];
            if(!this.data.length || !Array.isArray(this.data[0]) || !this.data[0].length)
                return;

            if(this.parameters.auto)
            {
                const values = this.getFiniteValues(this.data);
                if(values.length > 0)
                {
                    let nextMax = Math.max(...values);
                    let nextMin = Math.min(...values);
                    if(this.parameters.include_zero)
                    {
                        nextMax = Math.max(0, nextMax);
                        nextMin = Math.min(0, nextMin);
                    }
                    if(!Number.isFinite(this.computedMax))
                        this.computedMax = this.roundUpToSignificantFigure(nextMax || 1);
                    else if(nextMax > this.computedMax)
                        this.computedMax = this.roundUpToSignificantFigure(nextMax || 1);

                    if(!Number.isFinite(this.computedMin))
                        this.computedMin = this.roundDownToSignificantFigure(nextMin || 0);
                    else if(nextMin < this.computedMin)
                        this.computedMin = this.roundDownToSignificantFigure(nextMin || 0);
                }
            }
            else
            {
                this.computedMin = null;
                this.computedMax = null;
            }

            if(this.parameters.transpose)
                this.data = this.transpose(this.data); // TODO: should be changed in drawing instead
            if(!this.data.length || !Array.isArray(this.data[0]) || !this.data[0].length)
                return;

            this.draw(this.data[0].length, this.data.length);
        }
    }
};


webui_widgets.add('webui-widget-histogram', WebUIWidgetHistogram);
