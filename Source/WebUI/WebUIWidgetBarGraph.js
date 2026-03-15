class WebUIWidgetBarGraph extends WebUIWidgetGraph
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
            {'name': "BAR GRAPH", 'control':'header'},
            {'name':'title', 'default':"Bar Graph", 'type':'string', 'control': 'textedit'},
 //           {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},

            {'name': "STYLE", 'control':'header'},
            {'name':'direction', 'default':"vertical", 'type':'string', 'min':0, 'max':2, 'control': 'menu', 'options': "horizontal,vertical", 'class':'true'},
            {'name':'transpose', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'labels', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'color', 'default':'', 'type':'string', 'control': 'textedit'},   // TODO: no default = get from CSS would be a good functionality
            {'name':'fill', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'lineWidth', 'default':1, 'type':'float', 'control': 'textedit'},
 //           {'name':'lineDash', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'lineCap', 'default':"", 'type':'string', 'control': 'menu', 'options': "butt,round,quare"},
            {'name':'lineJoin', 'default':"", 'type':'string', 'control': 'menu', 'options': "miter,round,bevel"},

            {'name': "COORDINATE SYSTEM", 'control':'header'},
            {'name':'min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'auto', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'include_zero', 'default':true, 'type':'bool', 'control': 'checkbox'},
            
            {'name': "FRAME", 'control':'header'},
            {'name':'show_title', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    init()
    {
        super.init();
        this.data = [];

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

        for(let i=0; i<n; i++)
        {
            const valueX = ((this.data[y][i] - min) / (max - min)) * width;
            const left = Math.min(axisX, valueX);
            const barWidth = Math.abs(valueX - axisX);
            this.canvas.save();
            this.canvas.translate(left, 0);
            this.drawBarVertical(barWidth, bar_height, i);
            this.canvas.restore();
            this.canvas.translate(0, bar_spacing);
        }
    }

    drawPlotVertical(width, height, y)
    {
        if(!Array.isArray(this.data) || this.data.length === 0 || !Array.isArray(this.data[0]))
            return;
        let n = this.data[0].length;
        let bar_width = (width)/(n + (n-1)*this.format.spacing);
        let bar_spacing = Math.round((1 + this.format.spacing) * bar_width);
        bar_width = Math.round(bar_width);
        const {min, max} = this.getYRange();
        let axisY = height;
        if(min <= 0 && max >= 0)
            axisY = this.getPlotYForValue(0, height);
        else if(max < 0)
            axisY = 0;

        for(let i=0; i<n; i++)
        {
            const valueY = this.getPlotYForValue(this.data[y][i], height);
            const top = Math.min(axisY, valueY);
            const barHeight = Math.abs(valueY - axisY);
            this.canvas.save();
            this.canvas.translate(0, top);
            this.drawBarVertical(bar_width, barHeight, i);
            this.canvas.restore();
            this.canvas.translate(bar_spacing, 0);
        }
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


webui_widgets.add('webui-widget-bar-graph', WebUIWidgetBarGraph);
