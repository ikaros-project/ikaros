class WebUIWidgetBoxPlot extends WebUIWidgetGraph
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
            {'name': "BOX PLOT", 'control':'header'},
            {'name':'title', 'default':"Box Plot", 'type':'string', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'outlierSource', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'direction', 'default':"vertical", 'type':'string', 'control': 'menu', 'options': "vertical", 'class':'true'},
            {'name':'labels', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'xAxisLabel', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'yAxisLabel', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'drawLabelsX', 'default':true, 'type':'bool', 'control': 'checkbox'},

            {'name': "STYLE", 'control':'header'},
            {'name':'color', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'fill', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'lineWidth', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'outlierRadius', 'default':3, 'type':'float', 'control': 'textedit'},

            {'name': "COORDINATE SYSTEM", 'control':'header'},
            {'name':'min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'auto', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'include_zero', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'yAxis', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'leftScale', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'leftTickMarks', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'horizontalGridlines', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'decimals', 'default':2, 'type':'int', 'control': 'textedit'},
        ];
    }

    init()
    {
        super.init();
        this.data = [];
        this.outliers = [];
    }

    getBoxCount()
    {
        if(!Array.isArray(this.data) || this.data.length < 5 || !Array.isArray(this.data[0]))
            return 0;
        return this.data[0].length;
    }

    getBoxValue(row, column)
    {
        const value = this.data?.[row]?.[column];
        const numeric = parseFloat(value);
        return Number.isFinite(numeric) ? numeric : null;
    }

    getOutlierValues(column)
    {
        if(!Array.isArray(this.outliers))
            return [];

        return this.outliers
            .map(row => Array.isArray(row) ? parseFloat(row[column]) : null)
            .filter(value => Number.isFinite(value));
    }

    drawBox(width, height, i)
    {
        const lowerWhisker = this.getBoxValue(0, i);
        const q1 = this.getBoxValue(1, i);
        const median = this.getBoxValue(2, i);
        const q3 = this.getBoxValue(3, i);
        const upperWhisker = this.getBoxValue(4, i);

        if([lowerWhisker, q1, median, q3, upperWhisker].some(v => v === null))
            return;

        const yLowerWhisker = this.getPlotYForValue(lowerWhisker, height);
        const yQ1 = this.getPlotYForValue(q1, height);
        const yMedian = this.getPlotYForValue(median, height);
        const yQ3 = this.getPlotYForValue(q3, height);
        const yUpperWhisker = this.getPlotYForValue(upperWhisker, height);

        const centerX = width / 2;
        const boxWidth = Math.max(8, width * 0.58);
        const whiskerWidth = Math.max(6, width * 0.36);
        const boxLeft = centerX - boxWidth / 2;
        const boxTop = Math.min(yQ1, yQ3);
        const boxHeight = Math.max(1, Math.abs(yQ3 - yQ1));

        this.setColor(i);
        this.canvas.lineWidth = parseFloat(this.parameters.lineWidth) || 1;
        this.canvas.lineCap = "butt";
        this.canvas.lineJoin = "miter";

        this.canvas.beginPath();
        this.canvas.moveTo(centerX, yUpperWhisker);
        this.canvas.lineTo(centerX, boxTop);
        this.canvas.moveTo(centerX, boxTop + boxHeight);
        this.canvas.lineTo(centerX, yLowerWhisker);
        this.canvas.moveTo(centerX - whiskerWidth / 2, yUpperWhisker);
        this.canvas.lineTo(centerX + whiskerWidth / 2, yUpperWhisker);
        this.canvas.moveTo(centerX - whiskerWidth / 2, yLowerWhisker);
        this.canvas.lineTo(centerX + whiskerWidth / 2, yLowerWhisker);
        this.canvas.stroke();

        this.canvas.beginPath();
        this.canvas.rect(boxLeft, boxTop, boxWidth, boxHeight);
        this.canvas.fill();
        this.canvas.stroke();

        this.canvas.beginPath();
        this.canvas.moveTo(boxLeft, yMedian);
        this.canvas.lineTo(boxLeft + boxWidth, yMedian);
        this.canvas.stroke();

        this.drawOutliers(width, height, i);
    }

    drawOutliers(width, height, i)
    {
        const values = this.getOutlierValues(i);
        if(values.length === 0)
            return;

        const centerX = width / 2;
        const radius = Math.max(1, parseFloat(this.parameters.outlierRadius) || 3);

        this.canvas.save();
        this.setColor(i);
        this.canvas.lineWidth = parseFloat(this.parameters.lineWidth) || 1;

        for(const value of values)
        {
            const y = this.getPlotYForValue(value, height);
            this.canvas.beginPath();
            this.canvas.arc(centerX, y, radius, 0, 2 * Math.PI);
            this.canvas.fill();
            this.canvas.stroke();
        }

        this.canvas.restore();
    }

    drawPlotVertical(width, height)
    {
        const n = this.getBoxCount();
        if(n === 0)
            return;

        const boxSlotWidth = width / (n + (n - 1) * this.format.spacing);
        const boxSpacing = (1 + this.format.spacing) * boxSlotWidth;

        for(let i=0; i<n; i++)
        {
            this.canvas.save();
            this.canvas.translate(i * boxSpacing, 0);
            this.drawBox(boxSlotWidth, height, i);
            this.canvas.restore();
        }
    }

    drawLabelsVertical(width, height, n)
    {
        if(!this.format.drawLabelsX)
            return;

        const labels = String(this.parameters.labels || "").split(",");
        if(labels.length === 0 || labels.every(label => label.trim() === ""))
            return;

        const effectiveSpaceLeft = this.getEffectiveSpaceLeft(height);
        const boxSlotWidth = width / (n + (n - 1) * this.format.spacing);
        const boxSpacing = (1 + this.format.spacing) * boxSlotWidth;

        this.canvas.save();
        this.canvas.font = this.format.labelFont;
        this.canvas.fillStyle = this.format.labelColor;
        this.canvas.textAlign = "center";
        this.canvas.textBaseline = "top";

        const y = -this.format.spaceBottom + 6;
        for(let i=0; i<n; i++)
        {
            const label = (labels[i] ?? "").trim();
            const x = effectiveSpaceLeft + i * boxSpacing + boxSlotWidth / 2;
            this.canvas.fillText(label, x, y);
        }
        this.canvas.restore();
    }

    drawAxisTitles()
    {
        const xAxisLabel = String(this.parameters.xAxisLabel || "").trim();
        const yAxisLabel = String(this.parameters.yAxisLabel || "").trim();
        if(xAxisLabel === "" && yAxisLabel === "")
            return;

        const contentLeft = this.format.marginLeft;
        const contentTop = this.format.marginTop;
        const contentWidth = this.format.width;
        const contentHeight = this.format.height;
        const effectiveSpaceLeft = this.getEffectiveSpaceLeft(contentHeight);
        const plotWidth = contentWidth - effectiveSpaceLeft - this.format.spaceRight;
        const plotLeft = contentLeft + effectiveSpaceLeft;
        const plotCenterX = plotLeft + plotWidth / 2;
        const plotCenterY = contentTop + this.format.spaceTop + (contentHeight - this.format.spaceTop - this.format.spaceBottom) / 2;

        this.resetCanvasTransform(-0.5, -0.5);
        this.canvas.save();
        this.canvas.font = this.format.labelFont;
        this.canvas.fillStyle = this.format.labelColor;

        if(xAxisLabel !== "")
        {
            this.canvas.textAlign = "center";
            this.canvas.textBaseline = "top";
            this.canvas.fillText(xAxisLabel, plotCenterX, contentTop + contentHeight + 8);
        }

        if(yAxisLabel !== "")
        {
            this.canvas.translate(Math.max(10, contentLeft * 0.35), plotCenterY);
            this.canvas.rotate(-Math.PI / 2);
            this.canvas.textAlign = "center";
            this.canvas.textBaseline = "middle";
            this.canvas.fillText(yAxisLabel, 0, 0);
        }

        this.canvas.restore();
    }

    draw(size_x, size_y)
    {
        super.draw(size_x, size_y);
        this.drawAxisTitles();
    }

    update()
    {
        if(this.data = this.getSource('source'))
        {
            const outliers = this.getSource('outlierSource');
            this.outliers = Array.isArray(outliers) ? outliers : [];
            if(Array.isArray(this.outliers) && this.outliers.length > 0 && !Array.isArray(this.outliers[0]))
                this.outliers = [this.outliers];

            if(!Array.isArray(this.data))
                return;
            if(!Array.isArray(this.data[0]))
                this.data = [this.data];
            if(this.data.length < 5 || !Array.isArray(this.data[0]) || this.data[0].length === 0)
                return;

            if(this.parameters.auto)
            {
                const values = this.getFiniteValues(this.data).concat(this.getFiniteValues(this.outliers));
                if(values.length > 0)
                {
                    let nextMax = Math.max(...values);
                    let nextMin = Math.min(...values);
                    if(this.parameters.include_zero)
                    {
                        nextMax = Math.max(0, nextMax);
                        nextMin = Math.min(0, nextMin);
                    }

                    this.computedMax = this.roundUpToSignificantFigure(nextMax || 1);
                    this.computedMin = this.roundDownToSignificantFigure(nextMin || 0);
                }
            }
            else
            {
                this.computedMin = null;
                this.computedMax = null;
            }

            this.draw(this.getBoxCount(), 1);
        }
    }
};

webui_widgets.add('webui-widget-boxplot', WebUIWidgetBoxPlot);
