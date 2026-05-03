class WebUIWidgetScatterPlot extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "SCATTER PLOT", 'control':'header'},
            {'name':'title', 'default':"Scatter Plot", 'type':'string', 'control': 'textedit'},
            {'name':'sourceX', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'sourceY', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'regressionSource', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'labels', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'xAxisLabel', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'yAxisLabel', 'default':"", 'type':'string', 'control': 'textedit'},

            {'name': "STYLE", 'control':'header'},
            {'name':'color', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'fill', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'pointRadius', 'default':3, 'type':'float', 'control': 'textedit'},
            {'name':'lineWidth', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'drawLines', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'drawLegend', 'default':true, 'type':'bool', 'control': 'checkbox'},

            {'name': "LAYOUT", 'control':'header'},
            {'name':'marginLeft', 'default':4, 'type':'int', 'control': 'textedit'},
            {'name':'marginRight', 'default':4, 'type':'int', 'control': 'textedit'},
            {'name':'marginTop', 'default':4, 'type':'int', 'control': 'textedit'},
            {'name':'marginBottom', 'default':4, 'type':'int', 'control': 'textedit'},
            {'name':'spaceLeft', 'default':42, 'type':'int', 'control': 'textedit'},
            {'name':'spaceRight', 'default':8, 'type':'int', 'control': 'textedit'},
            {'name':'spaceTop', 'default':8, 'type':'int', 'control': 'textedit'},
            {'name':'spaceBottom', 'default':34, 'type':'int', 'control': 'textedit'},

            {'name': "COORDINATE SYSTEM", 'control':'header'},
            {'name':'minX', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'maxX', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'minY', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'maxY', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'auto', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'include_zero', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'xAxis', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'yAxis', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'bottomScale', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'bottomTickMarks', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'leftScale', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'leftTickMarks', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'horizontalGridlines', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'verticalGridlines', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'decimals', 'default':2, 'type':'int', 'control': 'textedit'},
        ];
    }

    init()
    {
        super.init();
        this.xData = [];
        this.yData = [];
        this.regression = [];
        this.metadata = null;
        this.xRange = {min: 0, max: 1};
        this.yRange = {min: 0, max: 1};
        this.channelsAreColumns = true;
    }

    requestData(data_set)
    {
        super.requestData(data_set);
        this.addSourceMetadata(data_set, this.parameters.sourceY);
    }

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

    normalizeMatrix(data)
    {
        if(!Array.isArray(data))
            return [];
        if(data.length === 0)
            return [];
        if(!Array.isArray(data[0]))
            return [data];
        return data;
    }

    getChannelCount()
    {
        if(!Array.isArray(this.yData) || this.yData.length === 0)
            return 0;
        if(this.channelsAreColumns)
            return this.yData[0]?.length || 0;
        return this.yData.length;
    }

    inferOrientation()
    {
        const shape = this.metadata?.shape;
        const labels = this.metadata?.labels;
        const rowLabels = Array.isArray(labels) && Array.isArray(labels[0]) ? labels[0] : [];
        const columnLabels = Array.isArray(labels) && Array.isArray(labels[1]) ? labels[1] : [];
        if(Array.isArray(shape) && shape.length >= 2 && (rowLabels.length > 0 || columnLabels.length > 0))
        {
            const rowsAreSamples = rowLabels.length === shape[0] && rowLabels.every(label => String(label).startsWith("sample "));
            const columnsAreSamples = columnLabels.length === shape[1] && columnLabels.every(label => String(label).startsWith("sample "));
            if(rowsAreSamples)
            {
                this.channelsAreColumns = true;
                return;
            }
            if(columnsAreSamples)
            {
                this.channelsAreColumns = false;
                return;
            }
        }

        const rows = this.yData.length;
        const cols = this.yData[0]?.length || 0;
        this.channelsAreColumns = rows >= cols;
    }

    getLabels()
    {
        const explicitLabels = String(this.parameters.labels || "").split(",").map(label => label.trim());
        if(explicitLabels.some(label => label !== ""))
            return explicitLabels;

        const metadataLabels = this.metadata?.labels;
        if(Array.isArray(metadataLabels) && Array.isArray(metadataLabels[1]) && !metadataLabels[1].every(label => String(label).startsWith("sample ")))
            return metadataLabels[1].map(label => String(label ?? "").trim());
        if(Array.isArray(metadataLabels) && Array.isArray(metadataLabels[0]) && !metadataLabels[0].every(label => String(label).startsWith("sample ")))
            return metadataLabels[0].map(label => String(label ?? "").trim());

        return [];
    }

    finitePairs(channel)
    {
        const pairs = [];
        if(!this.channelsAreColumns)
        {
            const samples = this.yData[channel]?.length || 0;
            for(let sample=0; sample<samples; sample++)
            {
                const x = this.getXValue(sample, channel);
                const y = parseFloat(this.yData[channel]?.[sample]);
                if(Number.isFinite(x) && Number.isFinite(y))
                    pairs.push({x, y});
            }
            return pairs;
        }

        const samples = this.yData.length;
        for(let sample=0; sample<samples; sample++)
        {
            const x = this.getXValue(sample, channel);
            const y = parseFloat(this.yData[sample]?.[channel]);
            if(Number.isFinite(x) && Number.isFinite(y))
                pairs.push({x, y});
        }
        return pairs;
    }

    firstFinite(...values)
    {
        for(const value of values)
        {
            const numeric = parseFloat(value);
            if(Number.isFinite(numeric))
                return numeric;
        }
        return Number.NaN;
    }

    getXValue(sample, channel)
    {
        if(this.channelsAreColumns)
            return this.firstFinite(
                this.xData?.[sample]?.[channel],
                this.xData?.[sample]?.[0],
                this.xData?.[0]?.[sample],
                this.xData?.[0]?.[0]
            );

        return this.firstFinite(
            this.xData?.[channel]?.[sample],
            this.xData?.[0]?.[sample],
            this.xData?.[sample]?.[0],
            this.xData?.[0]?.[0]
        );
    }

    getRegressionValue(row, channel)
    {
        const value = parseFloat(this.regression?.[row]?.[channel]);
        return Number.isFinite(value) ? value : null;
    }

    computeRanges()
    {
        if(!this.parameters.auto)
        {
            this.xRange = this.validRange(parseFloat(this.parameters.minX), parseFloat(this.parameters.maxX), 0, 1);
            this.yRange = this.validRange(parseFloat(this.parameters.minY), parseFloat(this.parameters.maxY), 0, 1);
            return;
        }

        const xValues = [];
        const yValues = [];
        const channelCount = this.getChannelCount();
        for(let channel=0; channel<channelCount; channel++)
        {
            for(const pair of this.finitePairs(channel))
            {
                xValues.push(pair.x);
                yValues.push(pair.y);
            }
        }

        if(this.isTrue(this.parameters.drawLines))
        {
            const xMin = xValues.length ? Math.min(...xValues) : 0;
            const xMax = xValues.length ? Math.max(...xValues) : 1;
            for(let channel=0; channel<channelCount; channel++)
            {
                const slope = this.getRegressionValue(0, channel);
                const intercept = this.getRegressionValue(1, channel);
                if(slope === null || intercept === null)
                    continue;
                yValues.push(intercept + slope * xMin);
                yValues.push(intercept + slope * xMax);
            }
        }

        if(this.parameters.include_zero)
        {
            xValues.push(0);
            yValues.push(0);
        }

        const rawXMin = xValues.length ? Math.min(...xValues) : 0;
        const rawXMax = xValues.length ? Math.max(...xValues) : 1;
        const rawYMin = yValues.length ? Math.min(...yValues) : 0;
        const rawYMax = yValues.length ? Math.max(...yValues) : 1;

        this.xRange = this.validRange(this.roundDownToSignificantFigure(rawXMin), this.roundUpToSignificantFigure(rawXMax || 1), 0, 1);
        this.yRange = this.validRange(this.roundDownToSignificantFigure(rawYMin), this.roundUpToSignificantFigure(rawYMax || 1), 0, 1);
    }

    validRange(min, max, fallbackMin, fallbackMax)
    {
        if(!Number.isFinite(min))
            min = fallbackMin;
        if(!Number.isFinite(max))
            max = fallbackMax;
        if(min === max)
            max = min + 1;
        return {min, max};
    }

    isTrue(value)
    {
        return value === true || value === "true" || value === 1 || value === "1";
    }

    xToPlot(value, width)
    {
        return (value - this.xRange.min) * width / (this.xRange.max - this.xRange.min);
    }

    yToPlot(value, height)
    {
        return (this.yRange.max - value) * height / (this.yRange.max - this.yRange.min);
    }

    formatValue(value)
    {
        const decimals = parseInt(this.parameters.decimals) || 0;
        if(!Number.isFinite(value))
            return "";
        return value.toFixed(decimals).replace(/\.?0+$/, "");
    }

    getPlotRect()
    {
        const left = parseFloat(this.parameters.marginLeft || 0) + parseFloat(this.parameters.spaceLeft || 0);
        const top = parseFloat(this.parameters.marginTop || 0) + parseFloat(this.parameters.spaceTop || 0);
        const right = parseFloat(this.parameters.marginRight || 0) + parseFloat(this.parameters.spaceRight || 0);
        const bottom = parseFloat(this.parameters.marginBottom || 0) + parseFloat(this.parameters.spaceBottom || 0);
        const width = Math.max(1, this.format.width - left - right);
        const height = Math.max(1, this.format.height - top - bottom);
        return {left, top, width, height};
    }

    setSeriesColor(channel)
    {
        this.setColor(channel);
    }

    drawGrid(rect)
    {
        this.canvas.save();
        this.canvas.translate(rect.left, rect.top);
        this.canvas.strokeStyle = this.format.gridColor;
        this.canvas.lineWidth = 1;

        const horizontal = parseInt(this.parameters.horizontalGridlines) || 0;
        for(let i=0; i<horizontal; i++)
        {
            const y = horizontal <= 1 ? 0 : i * rect.height / (horizontal - 1);
            this.canvas.beginPath();
            this.canvas.moveTo(0, y);
            this.canvas.lineTo(rect.width, y);
            this.canvas.stroke();
        }

        const vertical = parseInt(this.parameters.verticalGridlines) || 0;
        for(let i=0; i<vertical; i++)
        {
            const x = vertical <= 1 ? 0 : i * rect.width / (vertical - 1);
            this.canvas.beginPath();
            this.canvas.moveTo(x, 0);
            this.canvas.lineTo(x, rect.height);
            this.canvas.stroke();
        }

        this.canvas.restore();
    }

    drawAxes(rect)
    {
        this.canvas.save();
        this.canvas.translate(rect.left, rect.top);
        this.canvas.strokeStyle = this.format.axisColor;
        this.canvas.fillStyle = this.format.axisColor;
        this.canvas.lineWidth = 1;
        this.canvas.font = this.format.scaleFont;

        if(this.isTrue(this.parameters.yAxis))
        {
            this.canvas.beginPath();
            this.canvas.moveTo(0, 0);
            this.canvas.lineTo(0, rect.height);
            this.canvas.stroke();
        }

        if(this.isTrue(this.parameters.xAxis))
        {
            this.canvas.beginPath();
            this.canvas.moveTo(0, rect.height);
            this.canvas.lineTo(rect.width, rect.height);
            this.canvas.stroke();
        }

        const leftScale = parseInt(this.parameters.leftScale) || 0;
        if(this.isTrue(this.parameters.yAxis) && leftScale > 0)
        {
            this.canvas.textAlign = "right";
            this.canvas.textBaseline = "middle";
            for(let i=0; i<leftScale; i++)
            {
                const value = this.yRange.min + (leftScale - 1 - i) * (this.yRange.max - this.yRange.min) / Math.max(1, leftScale - 1);
                const y = i * rect.height / Math.max(1, leftScale - 1);
                this.canvas.fillText(this.formatValue(value), -8, y);
            }
        }

        const bottomScale = parseInt(this.parameters.bottomScale) || 0;
        if(this.isTrue(this.parameters.xAxis) && bottomScale > 0)
        {
            this.canvas.textAlign = "center";
            this.canvas.textBaseline = "top";
            for(let i=0; i<bottomScale; i++)
            {
                const value = this.xRange.min + i * (this.xRange.max - this.xRange.min) / Math.max(1, bottomScale - 1);
                const x = i * rect.width / Math.max(1, bottomScale - 1);
                this.canvas.fillText(this.formatValue(value), x, rect.height + 8);
            }
        }

        const leftTicks = parseInt(this.parameters.leftTickMarks) || 0;
        if(this.isTrue(this.parameters.yAxis) && leftTicks > 0)
        {
            for(let i=0; i<leftTicks; i++)
            {
                const y = i * rect.height / Math.max(1, leftTicks - 1);
                this.canvas.beginPath();
                this.canvas.moveTo(0, y);
                this.canvas.lineTo(-5, y);
                this.canvas.stroke();
            }
        }

        const bottomTicks = parseInt(this.parameters.bottomTickMarks) || 0;
        if(this.isTrue(this.parameters.xAxis) && bottomTicks > 0)
        {
            for(let i=0; i<bottomTicks; i++)
            {
                const x = i * rect.width / Math.max(1, bottomTicks - 1);
                this.canvas.beginPath();
                this.canvas.moveTo(x, rect.height);
                this.canvas.lineTo(x, rect.height + 5);
                this.canvas.stroke();
            }
        }

        this.canvas.restore();
    }

    drawAxisLabels(rect)
    {
        const xAxisLabel = String(this.parameters.xAxisLabel || "").trim();
        const yAxisLabel = String(this.parameters.yAxisLabel || "").trim();

        this.canvas.save();
        this.canvas.font = this.format.labelFont;
        this.canvas.fillStyle = this.format.labelColor;

        if(xAxisLabel !== "")
        {
            this.canvas.textAlign = "center";
            this.canvas.textBaseline = "bottom";
            this.canvas.fillText(xAxisLabel, rect.left + rect.width / 2, this.format.height - 2);
        }

        if(yAxisLabel !== "")
        {
            this.canvas.translate(12, rect.top + rect.height / 2);
            this.canvas.rotate(-Math.PI / 2);
            this.canvas.textAlign = "center";
            this.canvas.textBaseline = "middle";
            this.canvas.fillText(yAxisLabel, 0, 0);
        }

        this.canvas.restore();
    }

    drawPoints(rect)
    {
        const radius = Math.max(1, parseFloat(this.parameters.pointRadius) || 3);
        const channelCount = this.getChannelCount();

        this.canvas.save();
        this.canvas.translate(rect.left, rect.top);
        for(let channel=0; channel<channelCount; channel++)
        {
            this.setSeriesColor(channel);
            for(const pair of this.finitePairs(channel))
            {
                const x = this.xToPlot(pair.x, rect.width);
                const y = this.yToPlot(pair.y, rect.height);
                if(x < -radius || x > rect.width + radius || y < -radius || y > rect.height + radius)
                    continue;
                this.canvas.beginPath();
                this.canvas.arc(x, y, radius, 0, 2 * Math.PI);
                this.canvas.fill();
                this.canvas.stroke();
            }
        }
        this.canvas.restore();
    }

    drawRegressionLines(rect)
    {
        if(!this.isTrue(this.parameters.drawLines))
            return;

        const channelCount = this.getChannelCount();
        this.canvas.save();
        this.canvas.translate(rect.left, rect.top);
        this.canvas.lineWidth = Math.max(1, parseFloat(this.parameters.lineWidth) || 1);
        for(let channel=0; channel<channelCount; channel++)
        {
            const slope = this.getRegressionValue(0, channel);
            const intercept = this.getRegressionValue(1, channel);
            if(slope === null || intercept === null)
                continue;

            const y0 = intercept + slope * this.xRange.min;
            const y1 = intercept + slope * this.xRange.max;
            this.setSeriesColor(channel);
            this.canvas.beginPath();
            this.canvas.moveTo(0, this.yToPlot(y0, rect.height));
            this.canvas.lineTo(rect.width, this.yToPlot(y1, rect.height));
            this.canvas.stroke();
        }
        this.canvas.restore();
    }

    drawLegend(rect)
    {
        if(!this.isTrue(this.parameters.drawLegend))
            return;

        const labels = this.getLabels();
        const channelCount = this.getChannelCount();
        if(channelCount === 0 || labels.length === 0 || labels.every(label => label === ""))
            return;

        this.canvas.save();
        this.canvas.font = this.format.labelFont;
        this.canvas.textAlign = "left";
        this.canvas.textBaseline = "middle";

        let x = rect.left + rect.width - 90;
        let y = rect.top + 12;
        for(let channel=0; channel<channelCount; channel++)
        {
            const label = labels[channel] || `channel ${channel}`;
            this.setSeriesColor(channel);
            this.canvas.beginPath();
            this.canvas.arc(x, y, 4, 0, 2 * Math.PI);
            this.canvas.fill();
            this.canvas.stroke();
            this.canvas.fillStyle = this.format.labelColor;
            this.canvas.fillText(label, x + 10, y);
            y += 16;
        }

        this.canvas.restore();
    }

    drawFrame(rect)
    {
        if(this.format.frame == "none")
            return;

        this.canvas.save();
        this.canvas.strokeStyle = this.format.frame;
        this.canvas.beginPath();
        this.canvas.rect(rect.left, rect.top, rect.width, rect.height);
        this.canvas.stroke();
        this.canvas.restore();
    }

    update()
    {
        this.xData = this.normalizeMatrix(this.getSource('sourceX'));
        this.yData = this.normalizeMatrix(this.getSource('sourceY'));
        this.regression = this.normalizeMatrix(this.getSource('regressionSource'));
        this.metadata = this.getSourceMetadata('sourceY', null);
        this.inferOrientation();

        if(this.getChannelCount() === 0)
            return;

        this.computeRanges();

        this.resetCanvasTransform(-0.5, -0.5);
        this.canvas.clearRect(0, 0, this.width, this.height);
        const rect = this.getPlotRect();

        this.drawGrid(rect);
        this.drawRegressionLines(rect);
        this.drawPoints(rect);
        this.drawAxes(rect);
        this.drawFrame(rect);
        this.drawAxisLabels(rect);
        this.drawLegend(rect);
    }
}

webui_widgets.add('webui-widget-scatter-plot', WebUIWidgetScatterPlot);
