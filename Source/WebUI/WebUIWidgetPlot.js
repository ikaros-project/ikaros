class WebUIWidgetPlot extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "PLOT", 'control':'header'},
            {'name':'title', 'default':"Plot", 'type':'string', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'select', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'buffer_size', 'default':50, 'type':'int', 'control': 'textedit'},
            {'name':'direction', 'default':"vertical", 'type':'string', 'min':0, 'max':2, 'control': 'menu', 'options': "vertical", 'class':'true'},
            {'name': "STYLE", 'control':'header'},
            {'name':'color', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'show_title', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'},

        ]};

    init()
    {
      

        super.init();
        this.data = [];
        this.buffer = [];
        this.ix = 0;

        this.onclick = function () { alert(this.data) }; // last matrix
   }

    getBufferSize()
    {
        if(this.parameters.buffer_size == 0)
            return this.width;
        else
            return this.parameters.buffer_size;
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
        let min = this.parameters.min;
        let max = this.parameters.max;
        let w = this.getBufferSize();
        let dx = width/w;

        for(let xx=0; xx<this.data[0].length; xx++)
        {
            let x = 0;

            this.canvas.beginPath();
            this.setColor(xx);

            if(this.ix >= 1)
            {
                this.canvas.moveTo(x, height-height*(this.buffer[0][y][xx]-min)/(max-min));
                x += dx;
                for(let i=1; i<this.ix; i++)
                {
                    this.canvas.lineTo(x, height-height*(this.buffer[i][y][xx]-min)/(max-min));
                    x += dx;
                }
            }

            if(this.ix <this.buffer.length)
            {
                this.canvas.moveTo(x, height-height*(this.buffer[this.ix][y][xx]-min)/(max-min));
                x += dx;
                for(let i=this.ix+1; i<this.buffer.length; i++)
                {
                    this.canvas.lineTo(x, height-height*(this.buffer[i][y][xx]-min)/(max-min));
                    x += dx;
                }
            }
            this.canvas.stroke();
        }
    }

    update()
    {

        if(this.data = this.getSource('source'))
        {
            if(this.buffer.length < this.getBufferSize())
                this.buffer.push(this.data);
            else
                this.buffer[this.ix] = this.data;

            if(this.ix++ >= this.getBufferSize())
                this.ix = 0;

            this.draw(this.data[0].length, this.data.length);
        }
    }
};


webui_widgets.add('webui-widget-plot', WebUIWidgetPlot);
