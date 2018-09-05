class WebUIWidgetGraph extends WebUIWidgetCanvas
{
    init()
    {
        super.init();
        this.parameters.min = 0;
        this.parameters.max = 1;
    }
    
    drawLeftTickMarks(top, bottom)
    {
        let n = this.format.leftTickMarks;
        if(n==0)
            return;

        let i=0;
        for(let j=0; j<n; j++)
        {
            this.canvas.beginPath();
            this.canvas.lineWidth = 1;
            this.canvas.strokeStyle = this.format.axisColor;    // maybe also have axis properties
            this.canvas.moveTo(-1, i);
            this.canvas.lineTo(-7, i);
            this.canvas.stroke();
            
            i += (bottom)/(n-1);
        }
    }

    drawRightTickMarks(top, bottom)
    {
        let n = this.format.rightTickMarks;
        if(n==0)
            return;

        let i=0;
        for(let j=0; j<n; j++)
        {
            this.canvas.beginPath();
            this.canvas.lineWidth = 1;
            this.canvas.strokeStyle = this.format.axisColor;
            this.canvas.moveTo(this.format.width, i);
            this.canvas.lineTo(this.format.width+7, i);
            this.canvas.stroke();
            
            i += (bottom)/(n-1);
        }
    }

    drawBottomTickMarks(width, height)
    {
        let n = this.format.bottomTickMarks;
        if(n==0)
            return;

        let i=0;
        for(let j=0; j<n; j++)
        {
            this.canvas.beginPath();
            this.canvas.lineWidth = 1;
            this.canvas.strokeStyle = this.format.axisColor;
            this.canvas.moveTo(i, height);
            this.canvas.lineTo(i, height+7);
            this.canvas.stroke();
            i += width/(n-1);
        }
    }

    drawLeftScale(width, height)
    {
        let n = this.format.leftScale;
        if(n==0)
            return;

        let min = parseFloat(this.parameters.min);
        let max = parseFloat(this.parameters.max);
        if('min_y' in this.parameters && 'max_y' in this.parameters)
        {
            min = parseFloat(this.parameters.min_y);
            max = parseFloat(this.parameters.max_y);
        }

        this.canvas.font = this.format.scaleFont;
        this.canvas.fillStyle = this.format.axisColor;
        this.canvas.textAlign = "right";
        this.canvas.textBaseline = "middle";

        let i=0;
        for(let j=0; j<n; j++)
        {
            let v = min + (n-j-1)*(max-min)/(n-1);
            if(this.format.flipYAxis)
                v = max - v;
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

        let min = parseFloat(this.parameters.min);
        let max = parseFloat(this.parameters.max);
        if('min_y' in this.parameters && 'max_y' in this.parameters)
        {
            min = parseFloat(this.parameters.min_y);
            max = parseFloat(this.parameters.max_y);
        }

        this.canvas.font = this.format.scaleFont;
        this.canvas.fillStyle = this.format.axisColor;
        this.canvas.textAlign = "left";
        this.canvas.textBaseline="middle";

        let i=0;
        for(let j=0; j<n; j++)
        {
            let v = min + (n-j-1)*(max-min)/(n-1);
            if(this.format.flipYAxis)
                v = max - v;
            this.canvas.fillText(v.toFixed(this.format.decimals), width+this.format.scaleOffset, i);
            i += height/(n-1);
        }
        this.canvas.textBaseline="bottom";
    }

    drawBottomScale(width, height)
    {
//        this.canvas.fillStyle = '#ccffff';
//        this.canvas.fillRect(0, 0, width, height);

        let n = this.format.bottomScale;
        if(n==0)
            return;

        let min = parseFloat(this.parameters.min);
        let max = parseFloat(this.parameters.max);
        if('min_x' in this.parameters && 'max_x' in this.parameters)
        {
            min = parseFloat(this.parameters.min_x);
            max = parseFloat(this.parameters.max_x);
        }

        this.canvas.font = this.format.scaleFont;
        this.canvas.fillStyle = this.format.axisColor;
        this.canvas.textAlign = "center";
        this.canvas.textBaseline="top";

        let i=0;
        for(let j=0; j<n; j++)
        {
            let v = min + j*(max-min)/(n-1);
            if(this.format.flipXAxis)
                v = max - v;
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

    drawFrame(width, height)
    {
        if(this.format.frame=="none")
            return;

        this.canvas.beginPath();
        this.canvas.strokeStyle = this.format.frame;
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

    drawHorizontalGridlinesOver(width, height)
    {
        let n = this.format.horizontalGridlinesOver;
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

    drawVerticalGridlinesOver(width, height)
    {
        let n = this.format.verticalGridlinesOver;
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

    drawLabelsVertical(width, height, n)
    {
//        this.canvas.fillStyle = '#ffffcc';
 //       this.canvas.fillRect(0, 0, width, height);
        
        if(!this.format.drawLabelsX)
            return;
        
        let labels = (this.parameters.labels_x ? this.parameters.labels_x : this.parameters.labels);
        if(!labels)
            return;
        
        this.canvas.font = this.format.labelFont;
        this.canvas.fillStyle = this.format.labelColor;
        this.canvas.textAlign = "center";
        this.canvas.textBaseline= "top";

        let l = labels.split(',');
//        let n = this.data.length;
        let bar_width = (width)/(n + (n-1)*this.format.spacing);
        let bar_spacing = Math.round((1 + this.format.spacing) * bar_width);

        this.canvas.save();
        this.canvas.translate(this.format.spaceLeft+Math.round(bar_width)/2, 5);
        for(let i=0; i<n; i++)
        {
            this.canvas.fillText(l[i].trim(), 0, 0);
            this.canvas.translate(bar_spacing, 0);
        }
        this.canvas.restore();
    }

    drawLabelsHorizontal(width, height, n)
    {
//        this.canvas.fillStyle = '#ccffcc';
//        this.canvas.fillRect(0, 0, width, height);

        if(!this.format.drawLabelsY)
            return;
        
        let labels = (this.parameters.labels_y ? this.parameters.labels_y : this.parameters.labels);
        if(!labels)
            return;

        this.canvas.font = this.format.labelFont;
        this.canvas.fillStyle = this.format.labelColor;
        this.canvas.textAlign = "right";
        this.canvas.textBaseline= "middle";

        let l = labels.split(',');
//        let n = this.data.length;
        let bar_height = (height)/(n + (n-1)*this.format.spacing);
        let bar_spacing = Math.round((1 + this.format.spacing) * bar_height);

        this.canvas.save();
        this.canvas.translate(0, this.format.spaceTop+Math.round(bar_height)/2);
        for(let i=0; i<n; i++)
        {
            this.canvas.fillText(l[i].trim(), 0, 0);
            this.canvas.translate(0, bar_spacing);
        }
        this.canvas.restore();
    }

    drawVertical(size_x, size_y)
    {
        let pane_y = Math.round((this.format.height)/size_y);
        let pane_x = Math.round(this.format.width);
        let plot_height = pane_y-this.format.spaceTop-this.format.spaceBottom;
        let plot_width = pane_x-this.format.spaceLeft-this.format.spaceRight;

        this.drawLabelsHorizontal(plot_width, this.format.height, size_y);

        this.canvas.save();
            this.canvas.translate(this.format.spaceLeft, 0);
            this.drawVerticalGridlines(plot_width, this.format.height);
            this.drawBottomScale(plot_width, this.format.height);
            this.drawBottomTickMarks(plot_width, this.format.height);
        this.canvas.restore();

        this.canvas.save();
        for(let y=0; y<size_y; y++)
        {
            this.canvas.save();
                this.canvas.translate(0, this.format.spaceTop);
                this.drawHorizontalGridlines(pane_x, plot_height);
                this.canvas.save();
                    this.canvas.translate(this.format.spaceLeft, 0);
                        if(this.format.flipXCanvas)
                        {
                            this.canvas.translate(plot_width, 0);
                            this.canvas.scale(-1, 1);
                        }
                        if(this.format.flipYCanvas)
                        {
                            this.canvas.translate(0, plot_height);
                            this.canvas.scale(1, -1);
                        }

                        if(this.format.flipXAxis && this.format.flipYAxis)
                            this.drawPlotVertical(plot_width, plot_height, y, function (x, y) { return [plot_width-x, plot_height-y] });
                        else if(this.format.flipYAxis)
                            this.drawPlotVertical(plot_width, plot_height, y, function (x, y) { return [x, plot_height-y] });
                        else if(this.format.flipXAxis)
                            this.drawPlotVertical(plot_width, plot_height, y, function (x, y) { return [plot_width-x, y] });
                        else
                            this.drawPlotVertical(plot_width, plot_height, y, function (x, y) { return [x, y] });

                this.canvas.restore();
                this.drawLeftScale(pane_x, plot_height);
                this.drawRightScale(pane_x, plot_height);
                this.drawLeftTickMarks(plot_width, plot_height);
                this.drawRightTickMarks(plot_width, plot_height);
                this.drawHorizontalGridlinesOver(pane_x, plot_height);
                this.canvas.translate(0, -this.format.spaceTop);
                this.drawXAxis(pane_x, pane_y);
                this.drawYAxis(pane_x, pane_y);
                this.drawFrame(pane_x, pane_y);
            this.canvas.restore();
            this.canvas.translate(0, pane_y);
        }
        this.drawLabelsVertical(plot_width, this.format.height, size_x);
        this.canvas.restore();
        this.canvas.translate(this.format.spaceLeft, 0);
        this.drawVerticalGridlinesOver(plot_width, this.format.height);
    }

    drawHorizontal(size_x, size_y)
    {
        let pane_y = Math.round(this.format.height);
        let pane_x = Math.round((this.format.width)/size_y);
        let plot_height = pane_y-this.format.spaceTop-this.format.spaceBottom;
        let plot_width = pane_x-this.format.spaceLeft-this.format.spaceRight;

        this.drawLabelsHorizontal(this.format.width, plot_height, size_x);
        this.canvas.save();
            this.canvas.translate(0, this.format.height);
            this.drawLabelsVertical(this.format.width, this.format.height, size_y);
        this.canvas.restore();

        this.canvas.save();
            this.canvas.translate(0, this.format.spaceTop);
            this.drawLeftScale(pane_x, plot_height);
            this.drawLeftTickMarks(plot_width, plot_height);
            this.drawHorizontalGridlines(this.format.width, plot_height);
            this.drawRightScale(this.format.width, plot_height);
            this.drawRightTickMarks(this.format.width, plot_height);
        this.canvas.restore();

        this.canvas.save();
            for(let y=0; y<size_y; y++)
            {
                this.canvas.save();
                    this.canvas.translate(this.format.spaceLeft, 0);
                    this.drawVerticalGridlines(plot_width, pane_y);
                    this.drawBottomScale(plot_width, pane_y); // ****
                    this.canvas.save();
                        this.canvas.translate(0, this.format.spaceTop);
                        if(this.format.flipXCanvas)
                        {
                            this.canvas.translate(plot_width, 0);
                            this.canvas.scale(-1, 1);
                        }
                        if(this.format.flipYCanvas)
                        {
                            this.canvas.translate(0, plot_height);
                            this.canvas.scale(1, -1);
                        }
                
                        if(this.format.flipXAxis && this.format.flipYAxis)
                            this.drawPlotHorizontal(plot_width, plot_height, y, function (x, y) { return [plot_width-x, plot_height-y] });
                        else if(this.format.flipYAxis)
                            this.drawPlotHorizontal(plot_width, plot_height, y, function (x, y) { return [x, plot_height-y] });
                        else if(this.format.flipXAxis)
                            this.drawPlotHorizontal(plot_width, plot_height, y, function (x, y) { return [plot_width-x, y] });
                        else
                            this.drawPlotHorizontal(plot_width, plot_height, y, function (x, y) { return [x, y] });
                
                    this.canvas.restore();
                    this.drawBottomTickMarks(plot_width, pane_y);
                    this.drawVerticalGridlinesOver(plot_width, pane_y);
                    this.canvas.translate(-this.format.spaceLeft, 0);
                    this.drawXAxis(pane_x, pane_y);
                    this.drawYAxis(pane_x, pane_y);
                    this.drawFrame(pane_x, pane_y);
                this.canvas.restore();
                this.canvas.translate(pane_x, 0);
            }
        this.canvas.restore();
        this.canvas.translate(0, this.format.spaceTop);
        this.drawHorizontalGridlinesOver(this.format.width, plot_height);
     }

    draw(size_x, size_y)    // draw handles the layout of the graphs in horizontal or vertical sections
    {
        this.canvas.setTransform(1, 0, 0, 1, -0.5, -0.5);
        this.canvas.clearRect(0, 0, this.width, this.height);
//        this.drawTitle();
        this.canvas.translate(this.format.marginLeft, this.format.marginTop); // +0*this.format.titleHeight
        
        if(this.parameters.direction == 'vertical')
        {
            this.drawVertical(size_x, size_y);
        }
        else
        {
            this.drawHorizontal(size_x, size_y);
        }
    }

    update(d) // USED ONLY FOR TESTING
    {
        this.canvas.setTransform(1, 0, 0, 1, -0.5, -0.5);
        this.canvas.clearRect(0, 0, this.width, this.height);
        this.canvas.translate(this.format.marginLeft, this.format.marginTop); // +0*this.format.titleHeight
        try {
//            this.drawVertical(1, 1);
            this.drawHorizontal(1, 1, 0, this.transform);
        }
        catch(err)
        {
//            console.log(err);
        }
    }
};


webui_widgets.add('webui-widget-graph', WebUIWidgetGraph);
