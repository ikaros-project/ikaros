class WebUIWidgetBarGraph extends WebUIWidgetCanvas
{
/*
    constructor()
    {
        super();
        this.test = 42;
        console.log("Hej Hej", this.drawLayout);
    }
*/



    static template()
    {
        return [
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};



    init()
    {
        this.data = [10, 83, 32, 56, 2, 7, 55];   // Should connect to main data structure
        
        this.onclick = function () { alert(this.data) };
        
        this.min = 0;
        this.max = 1;
    }



    requestData(data_set)
    {
        data_set.add(this.parameters['module']+"."+this.parameters['source']);
    }



    update(data)    // TODO: Move to canvas only?
    {
        let width = parseInt(getComputedStyle(this.canvasElement).width);
        let height = parseInt(getComputedStyle(this.canvasElement).height);
        if(width != this.canvasElement.width || height != this.canvasElement.height)
        {
            this.canvasElement.width = parseInt(width);
            this.canvasElement.height = parseInt(height);
        }
        
        this.width = parseInt(width);
        this.height = parseInt(height);
        
        this.format.width = this.width - this.format.marginLeft - this.format.marginRight;
        this.format.height = this.height - this.format.titleHeight - this.format.marginBottom;

        this.draw(data);
    }

    // TODO: General functions to draw vertical and horizontal gridlines in rectangle
    
    draw_HorizontalGridlines(top, bottom)
    {
        let n = this.format.horizontalGridlines;
        if(n==0)
            return;
        
        let i=top+this.format.marginTop;
        for(let j=0; j<n; j++)
        {
            this.canvas.beginPath();
            this.canvas.strokeStyle = this.format.gridColor;
            this.canvas.moveTo(-1, i);
            this.canvas.lineTo(this.format.width, i);
            this.canvas.stroke();
            
            i += (bottom-(top+this.format.marginTop))/(n-1);
        }
    }

    draw_leftTickMarks(top, bottom)
    {
        let n = this.format.leftTickMarks;
        if(n==0)
            return;

        let i=top+this.format.marginTop;
        for(let j=0; j<n; j++)
        {
            this.canvas.beginPath();
            this.canvas.lineWidth = 1;
            this.canvas.strokeStyle = this.format.axisColor;    // maybe also have axis properties
            this.canvas.moveTo(-1, i);
            this.canvas.lineTo(-7, i);
            this.canvas.stroke();
            
            i += (bottom-(top+this.format.marginTop))/(n-1);
        }
    }

    draw_rightTickMarks(top, bottom)
    {
        let n = this.format.rightTickMarks;
        if(n==0)
            return;

        let i=top+this.format.marginTop;
        for(let j=0; j<n; j++)
        {
            this.canvas.beginPath();
            this.canvas.lineWidth = 1;
            this.canvas.strokeStyle = this.format.axisColor;
            this.canvas.moveTo(this.format.width, i);
            this.canvas.lineTo(this.format.width+7, i);
            this.canvas.stroke();
            
            i += (bottom-(top+this.format.marginTop))/(n-1);
        }
    }

   // TODO: General functions to draw vertical and horizontal scales outsida a rectangle

    draw_leftScale(top, bottom) // TODO: add heuristics for number of decimals based on max value
    {
        let n = this.format.leftScale
        if(n==0)
            return;

        this.canvas.font = this.format.scaleFont;
        this.canvas.fillStyle = this.format.axisColor;
        this.canvas.textAlign = "right";
        this.canvas.textBaseline="middle";

        let i=top+this.format.marginTop;
        for(let j=0; j<n; j++)
        {
            let v = this.min + (n-j-1)*(this.max-this.min)/(n-1);
            this.canvas.fillText(v, -this.format.scaleOffset, i);
            i += (bottom-(top+this.format.marginTop))/(n-1);
        }
        this.canvas.textBaseline="bottom";
    }

    draw_rightScale(top, bottom) // TODO: add heuristics for number of decimals based on max value
    {
        let n = this.format.rightScale
        if(n==0)
            return;

        this.canvas.font = this.format.scaleFont;
        this.canvas.fillStyle = this.format.axisColor;
        this.canvas.textAlign = "left";
        this.canvas.textBaseline="middle";

        let i=top+this.format.marginTop;
        for(let j=0; j<n; j++)
        {
            let v = this.min + (n-j-1)*(this.max-this.min)/(n-1);
            this.canvas.fillText(v, this.format.width+this.format.scaleOffset, i);
            i += (bottom-(top+this.format.marginTop))/(n-1);
        }
        this.canvas.textBaseline="bottom";
    }

    draw_xAxis(top, bottom)
    {
        if(!this.format.xAxis)
            return;

        this.canvas.beginPath();
        this.canvas.lineWidth = 1;
        this.canvas.strokeStyle = this.format.axisColor;
        this.canvas.moveTo(-1, bottom);
        this.canvas.lineTo(this.format.width, bottom);
        this.canvas.stroke();
    }

    draw_yAxis(top, bottom)
    {
        if(!this.format.yAxis)
            return;

        this.canvas.beginPath();
        this.canvas.lineWidth = 1;
        this.canvas.strokeStyle = this.format.axisColor;
        this.canvas.moveTo(-1, bottom);
        this.canvas.lineTo(-1, top+this.format.marginTop);
        this.canvas.stroke();
    }

    draw_gridFill(top, bottom)
    {
        if(this.format.gridFill=="none")
            return;

        this.canvas.beginPath();
        this.canvas.fillStyle = this.format.gridFill;
        this.canvas.rect(-1, this.format.marginTop-1, this.format.width+1, bottom-top);
        this.canvas.fill();
    }

    draw_frame(top, bottom)
    {
        if(!this.format.frame=="none")
            return;

        this.canvas.beginPath();
        this.canvas.strokeStyle = this.format.frame;
        this.canvas.rect(-1, this.format.marginTop-1, this.format.width+1, this.format.height-this.format.marginTop+1);
        this.canvas.stroke();
    }

    // TODO: Function to draw bars in rectangle

    draw(d)
    {
        this.canvas.clearRect(0, 0, this.width, this.height);
//        this.drawTitle();
        
        this.drawFullLayout(1, 7);
        
        return;

//        this.drawLayout();

        // Vertical Bars

        try {
            let m = this.parameters['module']
            let s = this.parameters['source']
            this.data = d[m][s]

            if(!this.data)
                return;

            let size_y = this.data.length;
            let size_x = this.data[0].length;
            let pane_y = (this.format.height+1)/size_y;
            let n = size_x;
            let bar_spacing = Math.round(this.format.width/n);

            this.canvas.setTransform(1, 0, 0, 1, this.format.marginLeft+0.5, this.format.titleHeight+0.5);

            for(let y=0; y<size_y; y++)
            {
                this.draw_gridFill(pane_y*y-1, pane_y*(y+1)-1);
                this.draw_HorizontalGridlines(pane_y*y-1, pane_y*(y+1)-1)
                this.draw_leftScale(pane_y*y-1, pane_y*(y+1)-1)
                this.draw_rightScale(pane_y*y-1, pane_y*(y+1)-1)
            }

            for(let y=0; y<size_y; y++)
            {
                let bar_offset = Math.round(bar_spacing*this.format.spacing/2);
                let bar_width = Math.round((1-this.format.spacing)*this.format.width/n);
                let bar_height = pane_y-this.format.marginTop;
                let max = 1;

                for(let i=0; i<n; i++)
                {
                    let h = (this.data[y][i]/max)
                    this.canvas.beginPath();
                    this.setColor(i)
                    this.canvas.rect(i*bar_spacing+bar_offset-1, pane_y*(1+y)-1 - bar_height*h, bar_width, bar_height*h);
                    this.canvas.fill();
                    this.canvas.stroke();
                }
            }
            
           for(let y=0; y<size_y; y++)
            {
                this.draw_xAxis(pane_y*y-1, pane_y*(y+1)-1);
                this.draw_yAxis(pane_y*y-1, pane_y*(y+1)-1);
                this.draw_leftTickMarks(pane_y*y-1, pane_y*(y+1)-1);
                this.draw_rightTickMarks(pane_y*y-1, pane_y*(y+1)-1);
                this.draw_frame(pane_y*y-1, pane_y*(y+1)-1);
           }
           
            for(let i=0; i<n; i++)
            {
 //                this.canvas.rect(i*bar_spacing+bar_offset-1, pane_y*(1+y)-1 - bar_height*h, bar_width, bar_height*h);

                this.canvas.fillStyle = "white"; // this.format.gridColor;
                this.canvas.textAlign = "center";
                this.canvas.textBaseline= "bottom";

                let l = this.parameters.labels.split(',');
                for(let j=0; j<n; j++)
                {
                    this.canvas.fillText(l[j].trim(), (j+0.5)*bar_spacing, this.format.height+24);
                 }
            }
        }
        catch(err)
        {
        }

        // Horizontal Bars


    }
};


//test = new WebUIWidgetBarGraph();

webui_widgets.add('webui-widget-bar-graph', WebUIWidgetBarGraph);
