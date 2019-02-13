class WebUIWidgetBarGraph extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "BAR GRAPH", 'control':'header'},
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'},
 //           {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},

            {'name': "STYLE", 'control':'header'},
            {'name':'direction', 'default':"vertical", 'type':'string', 'min':0, 'max':2, 'control': 'menu', 'values': "horizontal,vertical", 'class':'true'},
            {'name':'labels', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'color', 'default':'', 'type':'string', 'control': 'textedit'},   // TODO: no default = get from CSS would be a good functionality
            {'name':'fill', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'lineWidth', 'default':1, 'type':'float', 'control': 'textedit'},
 //           {'name':'lineDash', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'lineCap', 'default':"", 'type':'string', 'control': 'menu', 'values': "butt,round,quare"},
            {'name':'lineJoin', 'default':"", 'type':'string', 'control': 'menu', 'values': "miter,round,bevel"},

            {'name': "COORDINATE SYSTEM", 'control':'header'},
            {'name':'min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'max', 'default':1, 'type':'float', 'control': 'textedit'},
            
            {'name': "FRAME", 'control':'header'},
            {'name':'show_title', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    init()
    {
        super.init();
        this.data = [];

        this.onclick = function () {
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
        let n = this.data[0].length;
        let bar_height = (height)/(n + (n-1)*this.format.spacing);
        let bar_spacing = Math.round((1 + this.format.spacing) * bar_height);
        bar_height = Math.round(bar_height);
        let min = parseFloat(this.parameters.min);
        let max = parseFloat(this.parameters.max);

        for(let i=0; i<n; i++)
        {
            let h = (this.data[y][i]-min)/(max-min);
            this.canvas.save();
            this.drawBarVertical(h*width, bar_height, i);
            this.canvas.restore();
            this.canvas.translate(0, bar_spacing);
        }
    }

    drawPlotVertical(width, height, y)
    {
        let n = this.data[0].length;
        let bar_width = (width)/(n + (n-1)*this.format.spacing);
        let bar_spacing = Math.round((1 + this.format.spacing) * bar_width);
        bar_width = Math.round(bar_width);
        let min = parseFloat(this.parameters.min);
        let max = parseFloat(this.parameters.max);

        for(let i=0; i<n; i++)
        {
            let h = (this.data[y][i]-min)/(max-min);
            this.canvas.save();
            this.canvas.translate(0, (1-h)*height);
            this.drawBarVertical(bar_width, h*height, i);
            this.canvas.restore();
            this.canvas.translate(bar_spacing, 0);
        }
    }

    // update() gest the data for the graph and calls WebUIWidgetGraph::draw() which in turn calls
    // drawPlotHorizontal() or drawPlotVertical()
    
    update()
    {
        if(this.data = this.getSource('source'))
            this.draw(this.data[0].length, this.data.length);
    }
};


webui_widgets.add('webui-widget-bar-graph', WebUIWidgetBarGraph);
