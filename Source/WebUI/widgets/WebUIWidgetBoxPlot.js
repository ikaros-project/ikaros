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
            {'name':'outlier_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'orientation', 'default':"vertical", 'type':'string', 'control': 'menu', 'options': "vertical", 'class':'true'},
            {'name':'labels', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'x_axis_label', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'y_axis_label', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'show_x_labels', 'default':"yes", 'type':'bool', 'control': 'checkbox'},

            {'name': "STYLE", 'control':'header'},
            {'name':'stroke_color', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'fill_color', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'line_width', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'outlier_radius', 'default':3, 'type':'float', 'control': 'textedit'},

            {'name': "COORDINATE SYSTEM", 'control':'header'},
            {'name':'y_min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'y_max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'auto_range', 'default':"yes", 'type':'bool', 'control': 'checkbox'},
            {'name':'include_zero', 'default':"no", 'type':'bool', 'control': 'checkbox'},
            {'name':'show_y_axis', 'default':'yes', 'type':'bool', 'control': 'checkbox'},
            {'name':'show_x_axis', 'default':'yes', 'type':'bool', 'control': 'checkbox'},
            {'name':'left_scale_ticks', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'left_tick_marks', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'horizontal_grid_lines', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'decimals', 'default':2, 'type':'int', 'control': 'textedit'},
        ];
    }

    init()
    {
        super.init();
        this.data = [];
        this.outliers = [];
        this.metadata = null;
    }

    requestData(data_set)
    {
        super.requestData(data_set);
        this.addSourceMetadata(data_set, this.parameters.source);
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

    getBoxRows()
    {
        if(Array.isArray(this.data) && this.data.length >= 7)
            return {
                lowerWhisker: 1,
                q1: 2,
                median: 3,
                q3: 4,
                upperWhisker: 5
            };

        return {
            lowerWhisker: 0,
            q1: 1,
            median: 2,
            q3: 3,
            upperWhisker: 4
        };
    }

    getOutlierValues(column)
    {
        if(!Array.isArray(this.outliers))
            return [];

        return this.outliers
            .map(row => Array.isArray(row) ? parseFloat(row[column]) : null)
            .filter(value => Number.isFinite(value));
    }

    getLabels()
    {
        const explicitLabels = String(this.parameters.labels || "").split(",").map(label => label.trim());
        if(explicitLabels.some(label => label !== ""))
            return explicitLabels;

        const metadataLabels = this.metadata?.labels;
        if(Array.isArray(metadataLabels) && Array.isArray(metadataLabels[1]))
            return metadataLabels[1].map(label => String(label ?? "").trim());

        return [];
    }

    drawBox(width, height, i)
    {
        const rows = this.getBoxRows();
        const lowerWhisker = this.getBoxValue(rows.lowerWhisker, i);
        const q1 = this.getBoxValue(rows.q1, i);
        const median = this.getBoxValue(rows.median, i);
        const q3 = this.getBoxValue(rows.q3, i);
        const upperWhisker = this.getBoxValue(rows.upperWhisker, i);

        if([lowerWhisker, q1, median, q3, upperWhisker].some(v => v === null))
            return;

        const yLowerWhisker = this.getPlotYForValue(lowerWhisker, height);
        const yQ1 = this.getPlotYForValue(q1, height);
        const yMedian = this.getPlotYForValue(median, height);
        const yQ3 = this.getPlotYForValue(q3, height);
        const yUpperWhisker = this.getPlotYForValue(upperWhisker, height);

        const centerX = width / 2;
        const boxWidth = Math.max(1, width * 0.58);
        const whiskerWidth = Math.max(1, width * 0.36);
        const boxLeft = centerX - boxWidth / 2;
        const boxTop = Math.min(yQ1, yQ3);
        const boxHeight = Math.max(1, Math.abs(yQ3 - yQ1));

        this.setColor(i);
        this.canvas.lineWidth = Math.max(1, parseFloat(this.parameters.line_width) || 1);
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
        const radius = Math.max(1, parseFloat(this.parameters.outlier_radius) || 3);

        this.canvas.save();
        this.setColor(i);
        this.canvas.lineWidth = Math.max(1, parseFloat(this.parameters.line_width) || 1);

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
        if(!this.format.show_x_labels)
            return;

        const labels = this.getLabels();
        if(labels.length === 0 || labels.every(label => label.trim() === ""))
            return;

        const effectiveSpaceLeft = this.getEffectiveSpaceLeft(height);
        const boxSlotWidth = width / (n + (n - 1) * this.format.spacing);
        const boxSpacing = (1 + this.format.spacing) * boxSlotWidth;

        this.canvas.save();
        this.canvas.font = this.format.label_font;
        this.canvas.fillStyle = this.format.labelColor;
        this.canvas.textAlign = "center";
        this.canvas.textBaseline = "top";

        const y = -this.format.space_bottom + 6;
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
        const x_axis_label = String(this.parameters.x_axis_label || "").trim();
        const y_axis_label = String(this.parameters.y_axis_label || "").trim();
        if(x_axis_label === "" && y_axis_label === "")
            return;

        const contentLeft = this.format.margin_left;
        const contentTop = this.format.margin_top;
        const contentWidth = this.format.width;
        const contentHeight = this.format.height;
        const effectiveSpaceLeft = this.getEffectiveSpaceLeft(contentHeight);
        const plotWidth = contentWidth - effectiveSpaceLeft - this.format.space_right;
        const plotLeft = contentLeft + effectiveSpaceLeft;
        const plotCenterX = plotLeft + plotWidth / 2;
        const plotCenterY = contentTop + this.format.space_top + (contentHeight - this.format.space_top - this.format.space_bottom) / 2;

        this.resetCanvasTransform(-0.5, -0.5);
        this.canvas.save();
        this.canvas.font = this.format.label_font;
        this.canvas.fillStyle = this.format.labelColor;

        if(x_axis_label !== "")
        {
            this.canvas.textAlign = "center";
            this.canvas.textBaseline = "top";
            this.canvas.fillText(x_axis_label, plotCenterX, contentTop + contentHeight + 8);
        }

        if(y_axis_label !== "")
        {
            this.canvas.translate(Math.max(10, contentLeft * 0.35), plotCenterY);
            this.canvas.rotate(-Math.PI / 2);
            this.canvas.textAlign = "center";
            this.canvas.textBaseline = "middle";
            this.canvas.fillText(y_axis_label, 0, 0);
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
        this.data = this.getSource('source');
        if(!Array.isArray(this.data))
        {
            this.data = [];
            this.outliers = [];
            this.metadata = null;
            this.draw(0, 0);
            return;
        }
        this.metadata = this.getSourceMetadata('source', null);
        const outliers = this.getSource('outlier_source');
        this.outliers = Array.isArray(outliers) ? outliers : [];
        if(Array.isArray(this.outliers) && this.outliers.length > 0 && !Array.isArray(this.outliers[0]))
            this.outliers = [this.outliers];

        if(!Array.isArray(this.data[0]))
            this.data = [this.data];
        if(this.data.length < 5 || !Array.isArray(this.data[0]) || this.data[0].length === 0)
        {
            this.draw(0, 0);
            return;
        }

        if(this.parameters.auto_range)
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

                this.computedMax = this.roundUpToSignificantFigure(nextMax);
                this.computedMin = this.roundDownToSignificantFigure(nextMin);
            }
        }
        else
        {
            this.computedMin = null;
            this.computedMax = null;
        }

        this.draw(this.getBoxCount(), 1);
    }
};

webui_widgets.add('webui-widget-boxplot', WebUIWidgetBoxPlot);
