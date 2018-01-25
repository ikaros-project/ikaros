class WebUIWidgetCanvas extends WebUIWidget
{
/*
    constructor()
    {
        super();
    }
*/
    static template()
    {
        return [
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};
    
    static html()
    {
        return `
            <style>
                canvas { background-color: rgba(0,0,0,0); }
                main[data-mode="run"] canvas { cursor: default; }
            </style>
            <canvas style="height: 100%; width:100%" height="100" width="100"></canvas>
        `;
    }

    getProp(attribute, index)
    {
        try
        {
            if(index)
                return getComputedStyle(this.canvasElement).getPropertyValue(attribute).split(",")[index];
            else
                return getComputedStyle(this.canvasElement).getPropertyValue(attribute).trim();
        }
        catch(err)
        {
            return undefined;
        }
    }

    getInt(attribute, index)
    {
        try
        {
            if(index)
                return parseInt(this.getProp(attribute).split(",")[index]);
            else
                return parseInt(this.getProp(attribute));
        }
        catch(err)
        {
            return 0;
        }
    }

    getFloat(attribute, index)
    {
        try
        {
            if(index)
                return parseFloat(this.getProp(attribute).split(",")[index]);
            else
                return parseFloat(this.getProp(attribute));
        }
        catch(err)
        {
            return 0;
        }
    }

    getBool(attribute, index)
    {
        try
        {
            if(index)
                return ['yes','true'].includes(this.getProp(attribute).split(",")[index].toLowerCase());
            else
                return ['yes','true'].includes(this.getProp(attribute).toLowerCase());
        }
        catch(err)
        {
            return false;
        }
    }

   connectedCallback()
    {
        this.innerHTML = this.constructor.html();
        this.canvasElement = this.querySelector('canvas');
        this.canvas = this.canvasElement.getContext("2d");

        // read CSS variables - TODO: allow to work also when some variable is missing

        this.format = {}

        this.format.titleHeight =           this.getInt('--title-height');
        this.format.titleFont =             this.getProp('--title-font');
        this.format.titleColor =            this.getProp('--title-color');
        this.format.titleBackground =       this.getProp('--title-background');
        this.format.titleMargins =          this.getBool('--title-margins');
        this.format.titleAlign =            this.getProp('--title-align');
        this.format.titleOffsetX =          this.getFloat('--title-offset', 0);
        this.format.titleOffsetY =          this.getFloat('--title-offset', 1);

        this.format.marginLeft =            this.getInt('--margin-left');
        this.format.marginRight =           this.getInt('--margin-right');
        this.format.marginTop =             this.getInt('--margin-top');
        this.format.marginBottom =          this.getInt('--margin-bottom');

        this.format.spaceLeft =             this.getInt('--space-left');
        this.format.spaceRight =            this.getInt('--space-right');
        this.format.spaceTop =              this.getInt('--space-top');
        this.format.spaceBottom =           this.getInt('--space-bottom');

       this.format.spacing =                this.getFloat('--spacing');

        this.format.color =                 this.getProp('--color');
        this.format.lineWidth =             this.getProp('--line-width');
        this.format.fill =                  this.getProp('--fill');

        this.format.gridColor =             this.getProp('--grid-color');
        this.format.gridLineWidth =         this.getProp('--grid-line-width');
        this.format.gridFill =              this.getProp('--grid-fill');

        this.format.frame =                 this.getProp('--frame');
        this.format.xAxis =                 this.getBool('--x-axis');
        this.format.yAxis =                 this.getBool('--y-axis');
        this.format.axisColor =             this.getProp('--axis-color');
        this.format.verticalGridlines =     this.getInt('--vertical-gridlines');
        this.format.horizontalGridlines =   this.getInt('--horizontal-gridlines');
        this.format.leftTickMarks =         this.getInt('--left-tick-marks');
        this.format.rightTickMarks =        this.getInt('--right-tick-marks');
        this.format.leftScale =             this.getInt('--left-scale');
        this.format.rightScale =            this.getInt('--right-scale');
        this.format.scaleOffset =           this.getInt('--scale-offset');
        this.format.scaleFont =             this.getProp('--scale-font');
        this.format.labels =                this.getBool('--labels');

        this.format.direction =             this.getProp('--direction');

        // read parameters

        // set colors etc from parameters if set
    }


    update()
    {
        let width = parseInt(getComputedStyle(this.canvasElement).width);
        let height = parseInt(getComputedStyle(this.canvasElement).height);
        if(width != this.canvasElement.width || height != this.canvasElement.height)
        {
            this.canvasElement.width = parseInt(width);
            this.canvasElement.height = parseInt(height);
        }

        draw();
    }


    drawTitle()
    {
        this.canvas.beginPath();
        this.canvas.fillStyle = this.format.titleBackground;
        if(this.format.titleBackgroundMargins)
            this.canvas.rect(+this.format.marginLeft-0.5, 0, this.width - this.format.marginRight-0.5, +this.format.titleHeight-0.5);
        else
            this.canvas.rect(-0.5, -0.5, this.width, +this.format.titleHeight);
        this.canvas.fill();

        this.canvas.font = this.format.titleFont;
        this.canvas.fillStyle = this.format.titleColor;
        this.canvas.textAlign = this.format.titleAlign;
        this.canvas.textBaseline="bottom";

        if(this.format.titleMargins)
        {
            if(this.canvas.textAlign == 'left')
                this.canvas.fillText(this.parameters.title, this.format.marginLeft+this.format.titleOffsetX, this.format.titleHeight+this.format.titleOffsetY-1);
            else if(this.canvas.textAlign == 'right')
                this.canvas.fillText(this.parameters.title, this.width - this.format.marginRight-this.format.titleOf. this.format.titleHeight+this.format.titleOffsetY-1);
            else if(this.canvas.textAlign == 'center')
                this.canvas.fillText(this.parameters.title, +this.format.marginLeft+this.format.width/2, +this.format.titleHeight+this.format.titleOffsetY-1);
        }
        else
        {
             if(this.canvas.textAlign == 'left')
                this.canvas.fillText(this.parameters.title, this.format.titleOffsetX-1, this.format.titleHeight+this.format.titleOffsetY-1);
            else if(this.canvas.textAlign == 'right')
                this.canvas.fillText(this.parameters.title, this.width-this.format.titleOffsetX, this.format.titleHeight+this.format.titleOffsetY-1);
            else if(this.canvas.textAlign == 'center')
                this.canvas.fillText(this.parameters.title, this.width/2, this.format.titleHeight+this.format.titleOffsetY-1);
        }
    }

    // DRAW GRID UN RECT H & V

    setColor(i)
    {
        var l = this.format.color.split(",");
        var n = l.length;
        this.canvas.strokeStyle = l[i % n].trim();

        l = this.format.fill.split(",");
        n = l.length;
        this.canvas.fillStyle = l[i % n].trim();
    }


    drawLayout()
    {
        this.canvas.beginPath();
        this.canvas.lineWidth = 1;
        this.canvas.strokeStyle = "gray";

        this.canvas.moveTo(-0.5, this.format.titleHeight-1);
        this.canvas.lineTo(this.width, this.format.titleHeight-1);

        this.canvas.moveTo(-0.5, this.height-this.format.marginBottom);
        this.canvas.lineTo(this.width, this.height-this.format.marginBottom);

        this.canvas.moveTo(this.format.marginLeft-1, -0.5);
        this.canvas.lineTo(this.format.marginLeft-1, this.height);

        this.canvas.moveTo(this.width-this.format.marginRight, -0.5);
        this.canvas.lineTo(this.width-this.format.marginRight, this.height);

        this.canvas.stroke();
    }


    drawRow(width, height, row)
    {
    
    }

    drawColumn()
    {
    
    }

    drawHorizontalPlot()
    {
        // draw gridlines
        // draw row
    }

    drawVerticalPlot()
    {
        // draw gridlines
        // draw column
    }

    drawPane()
    {
        // draw axes and frames
    }

    drawHorizontalGridlines(width, height)
    {
        let n = this.format.horizontalGridlines;
        if(n==0)
            return;
        
        let p=0;
        for(let j=0; j<n; j++)
        {
            let q = Math.round(p)
            this.canvas.beginPath();
            this.canvas.strokeStyle = this.format.gridColor;
            this.canvas.moveTo(0, q);
            this.canvas.lineTo(width, q);
            this.canvas.stroke();
            p += height/(n-1);
        }
    }

    drawFullLayout(size_x, size_y)
    {
        this.canvas.setTransform(1, 0, 0, 1, -0.5, -0.5);

        this.canvas.beginPath();
        this.canvas.lineWidth = 1;
        this.canvas.strokeStyle = "gray";

        // margins

        this.canvas.moveTo(0.5, this.format.marginTop-1);
        this.canvas.lineTo(this.width, this.format.marginTop-1);

        this.canvas.moveTo(0.5, this.height-this.format.marginBottom);
        this.canvas.lineTo(this.width, this.height-this.format.marginBottom);

        this.canvas.moveTo(this.format.marginLeft-1, 0.5);
        this.canvas.lineTo(this.format.marginLeft-1, this.height);

        this.canvas.moveTo(this.width-this.format.marginRight, 0.5);
        this.canvas.lineTo(this.width-this.format.marginRight, this.height);

        this.canvas.stroke();

        // boxes
        
        let pane_y = (this.height - this.format.marginTop - this.format.marginBottom+1)/size_y;
        let pane_x = (this.width - this.format.marginLeft - this.format.marginRight+1)/size_x;


        for(let y=0; y<size_y; y++)
        {
            let top = Math.round(y*(pane_y));
            let bottom = Math.round((y+1)*pane_y);
            
            this.canvas.setTransform(1, 0, 0, 1, this.format.marginLeft-1.5, top+this.format.marginTop-1.5);
            this.canvas.beginPath();
            this.canvas.lineWidth = 1;
            this.canvas.strokeStyle = "red";
            this.canvas.rect(0, 0, this.format.width+1, bottom-top);
            this.canvas.stroke();

            // axes
            this.canvas.setTransform(1, 0, 0, 1, this.format.marginLeft-1.5, top+this.format.marginTop+this.format.spaceTop-1.5);
            this.canvas.beginPath();
            this.canvas.lineWidth = 1;
            this.canvas.strokeStyle = "yellow";
            this.canvas.rect(0, 0, this.format.width+1, bottom-top-this.format.spaceTop);
            this.canvas.stroke();

            // gridlines, ticks and scales
            this.canvas.setTransform(1, 0, 0, 1, this.format.marginLeft-1.5, this.format.marginTop+top+this.format.spaceTop-1.5);
            this.canvas.beginPath();
            this.canvas.lineWidth = 1;
            this.canvas.strokeStyle = "blue";
            this.canvas.rect(0, 0, this.format.width+1, bottom-top-this.format.spaceTop);
            this.canvas.stroke();
            
            this.drawHorizontalGridlines(this.format.width+1, bottom-top-this.format.spaceTop)

            // plot
            this.canvas.setTransform(1, 0, 0, 1, this.format.marginLeft+this.format.spaceLeft-1.5, this.format.marginTop+top+this.format.spaceTop-1.5);
            this.canvas.beginPath();
            this.canvas.lineWidth = 1;
            this.canvas.strokeStyle = "green";
            this.canvas.rect(0, 0, this.format.width-this.format.spaceLeft-this.format.spaceRight+1, bottom-top-this.format.spaceTop-this.format.spaceBottom);
            this.canvas.stroke();

        }
    }


    draw()
    {
        this.canvas.beginPath();
        this.canvas.lineWidth = lineWidth;
        this.canvas.strokeStyle = c;

        this.canvas.moveTo(width*Math.random(), height*Math.random());
        this.canvas.lineTo(width*Math.random(), height*Math.random());
        
        this.canvas.stroke();
        
        var w = this;
        setTimeout(function () { w.draw()}, 100)
    }
};



webui_widgets.add('webui-widget-canvas', WebUIWidgetCanvas);
