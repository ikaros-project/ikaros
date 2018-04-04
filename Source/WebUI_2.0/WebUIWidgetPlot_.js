class WebUIWidgetPlot extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "PARAMETERS", 'control':'header'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'},
 //           {'name':'direction', 'default':'all', 'type':'string', 'min':0, 'max':2, 'control': 'menu', 'values': "horizontal,vertical"},
            {'name': "STYLE", 'control':'header'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    init()
    {
        super.init();
        
        this.buffer = [];
        this.data = [];   // Should connect to main data structure
        
        this.onclick = function () { alert(this.data) };
        
        this.min = 0;
        this.max = 1;
    }

    requestData(data_set)
    {
        data_set.add(this.parameters['module']+"."+this.parameters['source']);
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
/*
        this.canvas.beginPath();
        this.canvas.arc(width/2, height/2, width/2, 0, 2*Math.PI, false);
        this.canvas.stroke();
*/
    }

    drawPlotHorizontal(width, height, y)
    {
//        super.drawPlotVertical(width, height, y); // draws the frame
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
    }

    drawPlotVertical(width, height, y)
    {
//        super.drawPlotVertical(width, height, y); // draws the frame
        let n = this.data[0].length;
        let bar_width = (width)/(n + (n-1)*this.format.spacing);
        let bar_spacing = Math.round((1 + this.format.spacing) * bar_width);
        bar_width = Math.round(bar_width);
        let min = 0;
        let max = 1;

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

    update(d)
    {
 //       let widgetStyles = window.getComputedStyle(this);
//        let test = widgetStyles.getPropertyValue('--test');
//        console.log(test);
    
    
//        let width = parseInt(getComputedStyle(this.canvasElement).width);
//        let height = parseInt(getComputedStyle(this.canvasElement).height);

        let width = this.parameters.width;
        let height = this.parameters.height;

        if(width != this.canvasElement.width || height != this.canvasElement.height)
        {
            this.canvasElement.width = parseInt(width);
            this.canvasElement.height = parseInt(height);
        }

        this.width = parseInt(width)+1;     // Add once since coordinate system is shifted 0.5 pixels
        this.height = parseInt(height)+1;

        this.format.width = this.width - this.format.marginLeft - this.format.marginRight;
        this.format.height = this.height - this.format.marginTop - this.format.marginBottom - this.format.titleHeight;

//        this.drawLayout();

        try {
            let m = this.parameters['module']
            let s = this.parameters['source']
            this.data = d[m][s]

            if(!this.data)
                return;

            let size_y = this.data.length;
            let size_x = this.data[0].length;

            this.draw(size_y, size_y);
        }
        catch(err)
        {
            console.log(err);
        }
    }
};


webui_widgets.add('webui-widget-plot', WebUIWidgetPlot);