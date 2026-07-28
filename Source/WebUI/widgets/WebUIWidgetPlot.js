class WebUIWidgetPlot extends WebUIWidgetGraph
{
    getSelectedIndices(columnCount)
    {
        if(!Number.isInteger(columnCount) || columnCount <= 0)
            return [];

        const select = String(this.parameters.select ?? "").trim();
        if(select === "")
            return Array.from({length: columnCount}, (_, index) => index);

        const indices = [];
        const seen = new Set();
        for(const token of select.split(/[,\s]+/))
        {
            if(token === "")
                continue;
            const index = Math.trunc(Number(token));
            if(!Number.isInteger(index) || index < 0 || index >= columnCount || seen.has(index))
                continue;
            seen.add(index);
            indices.push(index);
        }
        return indices;
    }

    getSelectedData(data)
    {
        if(!Array.isArray(data) || data.length === 0 || !Array.isArray(data[0]))
            return [];

        const selectedIndices = this.getSelectedIndices(data[0].length);
        return data.map((row) =>
        {
            if(!Array.isArray(row))
                return [];
            return selectedIndices.map((index) => row[index]);
        });
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

    static template()
    {
        return [
            {'name': "PLOT", 'control':'header'},
            {'name':'title', 'default':"Plot", 'type':'string', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'select', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'buffer_size', 'default':50, 'type':'int', 'control': 'textedit'},
            {'name':'orientation', 'default':"vertical", 'type':'string', 'min':0, 'max':2, 'control': 'menu', 'options': "vertical", 'class':'true'},
            {'name': "STYLE", 'control':'header'},
            {'name':'stroke_color', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'line_width', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name': "COORDINATE SYSTEM", 'control':'header'},
            {'name':'y_min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'y_max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'auto_range', 'default':"yes", 'type':'bool', 'control': 'checkbox'},
            {'name':'include_zero', 'default':"yes", 'type':'bool', 'control': 'checkbox'},
            {'name':'show_x_axis', 'default':'yes', 'type':'bool', 'control': 'checkbox'},
            {'name':'show_y_axis', 'default':'yes', 'type':'bool', 'control': 'checkbox'},
            {'name':'left_scale_ticks', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'left_tick_marks', 'default':5, 'type':'int', 'control': 'textedit'},
            {'name':'horizontal_grid_lines', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'decimals', 'default':2, 'type':'int', 'control': 'textedit'},

        ]};

    init()
    {
      

        super.init();
        this.data = [];
        this.buffer = [];
        this.ix = 0;
        this.bufferCapacity = null;

        this.onclick = function ()
        {
            if(main.edit_mode)
                return;
            alert(this.data);
        }; // last matrix
   }

    getBufferSize()
    {
        const configuredSize = Number(this.parameters.buffer_size);
        if(configuredSize === 0)
            return Math.max(1, Math.floor(Number(this.width) || 1));
        return Number.isFinite(configuredSize) ? Math.max(1, Math.floor(configuredSize)) : 50;
    }

    getOrderedBuffer()
    {
        if(this.buffer.length === 0)
            return [];
        const capacity = this.bufferCapacity || this.getBufferSize();
        if(this.buffer.length < capacity)
            return this.buffer.slice();
        return this.buffer.slice(this.ix).concat(this.buffer.slice(0, this.ix));
    }

    resizeBuffer(capacity)
    {
        if(this.bufferCapacity === capacity)
            return;

        this.buffer = this.getOrderedBuffer().slice(-capacity);
        this.ix = 0;
        this.bufferCapacity = capacity;
    }
/*
    drawBarHorizontal(width, height, i) // not used
    {
        this.canvas.beginPath();
        this.setColor(i)
        this.canvas.rect(0, 0, width, height);
        this.canvas.fill();
        this.canvas.stroke();
    }

    drawBarVertical(width, height, i)// not used
    {
        this.canvas.beginPath();
        this.setColor(i)
        this.canvas.rect(0, 0, width, height);
        this.canvas.fill();
        this.canvas.stroke();
    }
*/
    drawPlotHorizontal(width, height, y)
    {
    /*
        let n = this.data[0].length;
        let bar_height = (height)/(n + (n-1)*this.format.spacing);
        let bar_spacing = Math.round((1 + this.format.spacing) * bar_height);
        bar_height = Math.round(bar_height);
        let min = 0;
        let max = 1;

        for(let i=0; i<n; i++)
        {
            let h = (this.data[y][i]-min)/(max-min);
            this.canvas.save();
            this.drawBarVertical(h*width, bar_height, i);
            this.canvas.restore();
            this.canvas.translate(0, bar_spacing);
        }
    */
    }

    drawPlotVertical(width, height, y)
    {
        if(!Array.isArray(this.data) || this.data.length === 0 || !Array.isArray(this.data[0]))
            return;
        const history = this.getOrderedBuffer();
        if(history.length === 0)
            return;
        const selectedIndices = this.getSelectedIndices(this.data[0].length);
        if(selectedIndices.length === 0)
            return;
        let dx = width/Math.max(1, this.getBufferSize() - 1);
        this.canvas.lineWidth = Math.max(1, parseFloat(this.parameters.line_width) || 1);

        for(let xx=0; xx<selectedIndices.length; xx++)
        {
            const selectedIndex = selectedIndices[xx];
            this.canvas.beginPath();
            this.setColor(selectedIndex);
            let started = false;
            for(let i=0; i<history.length; i++)
            {
                const x = i * dx;
                const value = parseFloat(history[i]?.[y]?.[selectedIndex]);
                if(!Number.isFinite(value))
                {
                    started = false;
                    continue;
                }
                const yy = this.getPlotYForValue(value, height);
                if(!started)
                {
                    this.canvas.moveTo(x, yy);
                    started = true;
                }
                else
                    this.canvas.lineTo(x, yy);
            }
            this.canvas.stroke();
        }
    }

    update()
    {
        this.data = this.getSource('source');
        if(!Array.isArray(this.data))
        {
            this.data = [];
            this.clearCanvas();
            return;
        }
        if(!Array.isArray(this.data[0])) // FIXME: Fix for arbitrary matrix sizes
            this.data = [this.data];
        if(!this.data.length || !Array.isArray(this.data[0]) || !this.data[0].length)
        {
            this.clearCanvas();
            return;
        }

        const bufferSize = this.getBufferSize();
        this.resizeBuffer(bufferSize);
        if(this.buffer.length < bufferSize)
            this.buffer.push(this.data);
        else
        {
            this.buffer[this.ix] = this.data;
            this.ix = (this.ix + 1) % bufferSize;
        }

        if(this.parameters.auto_range)
        {
            const values = this.getFiniteValues(this.getSelectedData(this.data));
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
                    this.computedMax = this.roundUpToSignificantFigure(nextMax);
                else if(nextMax > this.computedMax)
                    this.computedMax = this.roundUpToSignificantFigure(nextMax);

                if(!Number.isFinite(this.computedMin))
                    this.computedMin = this.roundDownToSignificantFigure(nextMin);
                else if(nextMin < this.computedMin)
                    this.computedMin = this.roundDownToSignificantFigure(nextMin);
            }
        }
        else
        {
            this.computedMin = null;
            this.computedMax = null;
        }

        const selectedData = this.getSelectedData(this.data);
        if(!selectedData.length || !Array.isArray(selectedData[0]) || selectedData[0].length === 0)
        {
            this.clearCanvas();
            return;
        }

        this.draw(selectedData[0].length, selectedData.length);
    }
};


webui_widgets.add('webui-widget-plot', WebUIWidgetPlot);
