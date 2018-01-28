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

        this.format.spacing =               this.getFloat('--spacing');

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
        this.format.bottomScale =           this.getInt('--bottom-scale');
        this.format.scaleOffset =           this.getInt('--scale-offset');
        this.format.scaleFont =             this.getProp('--scale-font');
        this.format.labels =                this.getBool('--labels');

        this.format.direction =             this.getProp('--direction');

        this.format.decimals =              this.getInt('--decimals');
        this.format.min =                   this.getInt('--min');
        this.format.max =                   this.getInt('--max');

        // read parameters

        // set colors etc from parameters if set
    }

    drawTitle()
    {
        this.canvas.beginPath();
        this.canvas.lineWidth = 1;
        this.canvas.strokeStyle = "purple";
        this.canvas.fillStyle = this.format.titleBackground;

        if(this.format.titleMargins)
            this.canvas.rect(this.format.marginLeft, 0, this.width - this.format.marginLeft - this.format.marginRight, this.format.titleHeight);
        else
            this.canvas.rect(0, 0, this.width, this.format.titleHeight);
        this.canvas.fill();
        this.canvas.stroke();
        
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
                this.canvas.fillText(this.parameters.title, +this.format.marginLeft+this.format.width/2, this.format.titleHeight+this.format.titleOffsetY-1);
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

        this.canvas.moveTo(0, this.format.marginTop);
        this.canvas.lineTo(this.width, this.format.marginTop);

        this.canvas.moveTo(0, this.format.height);
        this.canvas.lineTo(this.width, this.format.height);

        this.canvas.moveTo(this.format.marginLeft, 0);
        this.canvas.lineTo(this.format.marginLeft, this.height);

        this.canvas.moveTo(this.width-this.format.marginRight, 0);
        this.canvas.lineTo(this.width-this.format.marginRight, this.height);

        this.canvas.stroke();
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

    drawLeftScale(width, height)
    {
        let n = this.format.leftScale;
        if(n==0)
            return;

        this.canvas.font = this.format.scaleFont;
        this.canvas.fillStyle = this.format.axisColor;
        this.canvas.textAlign = "right";
        this.canvas.textBaseline="middle";

        let i=0;
        for(let j=0; j<n; j++)
        {
            let v = this.format.min + (n-j-1)*(this.format.max-this.format.min)/(n-1);   // ????
            this.canvas.fillText(v.toFixed(this.format.decimals), -this.format.scaleOffset, i);
            i += height/(n-1);
        }
        this.canvas.textBaseline="bottom";
    }

    drawRightScale(width, height)
    {
        let n = this.format.rightScale;
        if(n==0)
            return;

        this.canvas.font = this.format.scaleFont;
        this.canvas.fillStyle = this.format.axisColor;
        this.canvas.textAlign = "left";
        this.canvas.textBaseline="middle";

        let i=0;
        for(let j=0; j<n; j++)
        {
            let v = this.format.min + (n-j-1)*(this.format.max-this.format.min)/(n-1);
            this.canvas.fillText(v.toFixed(this.format.decimals), width+this.format.scaleOffset, i);
            i += height/(n-1);
        }
        this.canvas.textBaseline="bottom";
    }

    drawBottomScale(width, height)
    {
        let n = this.format.bottomScale;
        if(n==0)
            return;

        this.canvas.font = this.format.scaleFont;
        this.canvas.fillStyle = this.format.axisColor;
        this.canvas.textAlign = "center";
        this.canvas.textBaseline="top";

        let i=0;
        for(let j=0; j<n; j++)
        {
            let v = this.format.min + (n-j-1)*(this.format.max-this.format.min)/(n-1);
            this.canvas.fillText(v.toFixed(this.format.decimals), i, height+this.format.scaleOffset);
            i += width/(n-1);
        }
        this.canvas.textBaseline="bottom";
    }

    drawXAxis(width, height)
    {
        if(!this.format.xAxis)
            return;

        this.canvas.beginPath();
        this.canvas.lineWidth = 1;
        this.canvas.strokeStyle = this.format.axisColor;
        this.canvas.moveTo(0, height);
        this.canvas.lineTo(width, height);
        this.canvas.stroke();
    }

    drawYAxis(width, height)
    {
        if(!this.format.yAxis)
            return;

        this.canvas.beginPath();
        this.canvas.lineWidth = 1;
        this.canvas.strokeStyle = this.format.axisColor;
        this.canvas.moveTo(0, 0);
        this.canvas.lineTo(0, height);
        this.canvas.stroke();
    }
/*
    draw_gridFill(top, bottom)
    {
        if(this.format.gridFill=="none")
            return;

        this.canvas.beginPath();
        this.canvas.lineWidth = 1;
        this.canvas.fillStyle = this.format.gridFill;
        this.canvas.rect(-1, this.format.marginTop-1, this.format.width+1, bottom-top);
        this.canvas.fill();
    }
*/
    drawFrame(width, height)
    {
        if(this.format.frame=="none")
            return;

        this.canvas.beginPath();
        this.canvas.strokeStyle = this.format.frame;
        this.canvas.rect(0, 0, width, height);
        this.canvas.stroke();
    }

    drawPane(width, height)
    {
        this.canvas.beginPath();
        this.canvas.lineWidth = 1;
        this.canvas.strokeStyle = "red";
        this.canvas.rect(0, 0, width, height);
        this.canvas.stroke();
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

    drawVerticalGridlines(width, height)
    {
        let n = this.format.verticalGridlines;
        if(n==0)
            return;
        
        let p=0;
        for(let j=0; j<n; j++)
        {
            let q = Math.round(p)
            this.canvas.beginPath();
            this.canvas.strokeStyle = this.format.gridColor;
            this.canvas.moveTo(q, 0);
            this.canvas.lineTo(q, height);
            this.canvas.stroke();
            p += width/(n-1);
        }
    }

    drawPlotVertical(width, height)
    {
        this.canvas.beginPath();
        this.canvas.lineWidth = 1;
        this.canvas.strokeStyle = "green";
        this.canvas.rect(0, 0, width, height);
        this.canvas.stroke();
    }

    drawPlotHorizontal(width, height)
    {
        this.canvas.beginPath();
        this.canvas.lineWidth = 1;
        this.canvas.strokeStyle = "green";
        this.canvas.rect(0, 0, width, height);
        this.canvas.stroke();
    }

    drawLabelsVertical(width, height)
    {
        this.canvas.fillStyle = "white"; // this.format.gridColor;
        this.canvas.textAlign = "center";
        this.canvas.textBaseline= "top";

        let l = this.parameters.labels.split(',');
        let n = this.data[0].length;
        let bar_width = (width)/(n + (n-1)*this.format.spacing);
        let bar_spacing = Math.round((1 + this.format.spacing) * bar_width);
        bar_width = Math.round(bar_width);

        this.canvas.translate(this.format.spaceLeft+bar_width/2, 0);
        for(let i=0; i<n; i++)
        {
            this.canvas.fillText(l[i].trim(), 0, 0);
            this.canvas.translate(bar_spacing, 0);
        }
    }

    drawLabelsHorizontal(width, height)
    {
        this.canvas.fillStyle = "white"; // this.format.gridColor;
        this.canvas.textAlign = "right";
        this.canvas.textBaseline= "middle";

        let l = this.parameters.labels.split(',');
        let n = this.data[0].length;
        let bar_height = (height)/(n + (n-1)*this.format.spacing);
        let bar_spacing = Math.round((1 + this.format.spacing) * bar_height);
        bar_height = Math.round(bar_height);

        this.canvas.translate(0, this.format.spaceTop+bar_height/2);
        for(let i=0; i<n; i++)
        {
            this.canvas.fillText(l[i].trim(), 0, 0);
            this.canvas.translate(0, bar_spacing);
        }
    }

   // draw handles the layout of the graphs in horizontal or vertical secrions
    
    draw(size_x, size_y)
    {
        this.canvas.setTransform(1, 0, 0, 1, -0.5, -0.5);
        this.canvas.clearRect(0, 0, this.width, this.height);
        this.drawTitle();
        this.canvas.translate(0, this.format.titleHeight);
//        this.drawLayout();

        this.canvas.translate(this.format.marginLeft, this.format.marginTop);

        if(this.format.direction == 'vertical')
        {
            let pane_y = Math.round((this.format.height)/size_y);   // -this.format.marginTop
            let pane_x = Math.round(this.format.width);

            let plot_height = pane_y-this.format.spaceTop-this.format.spaceBottom;
            let plot_width = pane_x-this.format.spaceLeft-this.format.spaceRight;

            let grid_height = pane_y-this.format.spaceTop-this.format.spaceBottom;
            let grid_width = pane_x;

            for(let y=0; y<size_y; y++)
            {
                this.canvas.save();
                this.canvas.translate(0, this.format.spaceTop);
                this.drawHorizontalGridlines(grid_width, grid_height);
                
                this.canvas.save();
                this.canvas.translate(this.format.spaceLeft, 0);
                this.drawPlotVertical(plot_width, plot_height, y);
                this.canvas.restore();

                this.drawLeftScale(grid_width, grid_height);
                this.drawRightScale(grid_width, grid_height);
                this.drawXAxis(grid_width, grid_height);
                this.drawYAxis(grid_width, grid_height);

                this.canvas.translate(0, -this.format.spaceTop);
                this.drawFrame(grid_width, pane_y);

                this.canvas.restore();
                this.canvas.translate(0, pane_y);
            }
            
            this.drawLabelsVertical(plot_width, this.format.height);
        }
        else
        {
            let pane_y = Math.round(this.format.height);
            let pane_x = Math.round((this.format.width)/size_x);

            let plot_height = pane_y-this.format.spaceTop-this.format.spaceBottom;
            let plot_width = pane_x-this.format.spaceLeft-this.format.spaceRight;

            let grid_height = pane_y;
            let grid_width = pane_x-this.format.spaceLeft-this.format.spaceRight;

            this.canvas.save();
            this.drawLabelsHorizontal(this.format.width, plot_height);
            this.canvas.restore();
            
            for(let x=0; x<size_x; x++)
            {
                this.canvas.save();
                this.canvas.translate(this.format.spaceLeft, 0);
                this.drawVerticalGridlines(grid_width, grid_height);
                this.drawBottomScale(grid_width, grid_height);
                
                this.canvas.save();
                this.canvas.translate(0, this.format.spaceTop);
                this.drawPlotHorizontal(plot_width, plot_height, x);
                this.canvas.restore();

                this.drawBottomScale(grid_width, grid_height);
                this.drawXAxis(grid_width, grid_height);
                this.drawYAxis(grid_width, grid_height);

                this.canvas.translate(-this.format.spaceLeft, 0);
                this.drawFrame(pane_x, this.format.height);

                this.canvas.restore();
                this.canvas.translate(pane_x, 0);
            }
        }
    }
};



webui_widgets.add('webui-widget-canvas', WebUIWidgetCanvas);
