class WebUIWidgetGraph extends WebUIWidgetCanvas
{
    init()
    {
        super.init();
        this.computedMin = null;
        this.computedMax = null;
        this.computedMinY = null;
        this.computedMaxY = null;
    }

    getFiniteValues(data)
    {
        const values = [];
        const visit = (value) =>
        {
            if(Array.isArray(value))
            {
                for(const item of value)
                    visit(item);
                return;
            }
            const numeric = parseFloat(value);
            if(Number.isFinite(numeric))
                values.push(numeric);
        };
        visit(data);
        return values;
    }

    getYRange()
    {
        let min = parseFloat(this.parameters.y_min);
        let max = parseFloat(this.parameters.y_max);
        if(Number.isFinite(this.computedMinY) && Number.isFinite(this.computedMaxY))
        {
            min = this.computedMinY;
            max = this.computedMaxY;
        }
        else if('y_min' in this.parameters && 'y_max' in this.parameters)
        {
            min = parseFloat(this.parameters.y_min);
            max = parseFloat(this.parameters.y_max);
        }
        else if(Number.isFinite(this.computedMin) && Number.isFinite(this.computedMax))
        {
            min = this.computedMin;
            max = this.computedMax;
        }
        if(!Number.isFinite(min))
            min = 0;
        if(!Number.isFinite(max))
            max = 1;
        if(min === max)
            max = min + 1;
        return {min, max};
    }

    getPlotYForValue(value, height)
    {
        const {min, max} = this.getYRange();
        let y = (max - value) * height / (max - min);
        if(this.format.flip_y_axis)
            y = height - y;
        return y;
    }

    formatScaleValue(value)
    {
        const decimals = this.format.decimals;
        if(!Number.isFinite(value))
            return "";
        return value.toFixed(decimals).replace(/\.?0+$/, "");
    }

    getEffectiveSpaceLeft(height)
    {
        const base = this.format.space_left || 0;
        const n = this.format.left_scale_ticks;
        if(!this.canvas || !n || n <= 0)
            return base;

        const {min, max} = this.getYRange();
        this.canvas.save();
        this.canvas.font = this.format.scale_font;
        let maxWidth = 0;
        for(let j=0; j<n; j++)
        {
            let v = min + (n-j-1)*(max-min)/(n-1);
            if(this.format.flip_y_axis)
                v = max - v;
            const text = this.formatScaleValue(v);
            maxWidth = Math.max(maxWidth, this.canvas.measureText(text).width);
        }
        this.canvas.restore();
        return Math.max(base, Math.ceil(maxWidth + this.format.scale_offset + 10));
    }
    
    drawLeftTickMarks(top, bottom)
    {
        let n = this.format.left_tick_marks;
        if(n==0)
            return;

        let i=0;
        for(let j=0; j<n; j++)
        {
            this.canvas.beginPath();
            this.canvas.lineWidth = 1;
            this.canvas.strokeStyle = this.format.axis_color;    // maybe also have axis properties
            this.canvas.moveTo(-1, i);
            this.canvas.lineTo(-7, i);
            this.canvas.stroke();
            
            i += (bottom)/(n-1);
        }
    }

    drawRightTickMarks(top, bottom)
    {
        let n = this.format.right_tick_marks;
        if(n==0)
            return;

        let i=0;
        for(let j=0; j<n; j++)
        {
            this.canvas.beginPath();
            this.canvas.lineWidth = 1;
            this.canvas.strokeStyle = this.format.axis_color;
            this.canvas.moveTo(this.format.width, i);
            this.canvas.lineTo(this.format.width+7, i);
            this.canvas.stroke();
            
            i += (bottom)/(n-1);
        }
    }

    drawBottomTickMarks(width, height)
    {
        let n = this.format.bottom_tick_marks;
        if(n==0)
            return;

        let i=0;
        for(let j=0; j<n; j++)
        {
            this.canvas.beginPath();
            this.canvas.lineWidth = 1;
            this.canvas.strokeStyle = this.format.axis_color;
            this.canvas.moveTo(i, height);
            this.canvas.lineTo(i, height+7);
            this.canvas.stroke();
            i += width/(n-1);
        }
    }

    drawLeftScale(width, height)
    {
        let n = this.format.left_scale_ticks;
        if(n==0)
            return;

        const {min, max} = this.getYRange();

        this.canvas.font = this.format.scale_font;
        this.canvas.fillStyle = this.format.axis_color;
        this.canvas.textAlign = "right";
        this.canvas.textBaseline = "middle";

        let i=0;
        for(let j=0; j<n; j++)
        {
            let v = min + (n-j-1)*(max-min)/(n-1);
            if(this.format.flip_y_axis)
                v = max - v;
            this.canvas.fillText(this.formatScaleValue(v), -this.format.scale_offset, i);
            i += height/(n-1);
        }
        this.canvas.textBaseline="bottom";
    }

    drawRightScale(width, height)
    {
        let n = this.format.right_scale_ticks;
        if(n==0)
            return;

        const {min, max} = this.getYRange();

        this.canvas.font = this.format.scale_font;
        this.canvas.fillStyle = this.format.axis_color;
        this.canvas.textAlign = "left";
        this.canvas.textBaseline="middle";

        let i=0;
        for(let j=0; j<n; j++)
        {
            let v = min + (n-j-1)*(max-min)/(n-1);
            if(this.format.flip_y_axis)
                v = max - v;
            this.canvas.fillText(this.formatScaleValue(v), width+this.format.scale_offset, i);
            i += height/(n-1);
        }
        this.canvas.textBaseline="bottom";
    }

    drawBottomScale(width, height)
    {
//        this.canvas.fillStyle = '#ccffff';
//        this.canvas.fillRect(0, 0, width, height);

        let n = this.format.bottom_scale_ticks;
        if(n==0)
            return;

        let min = parseFloat(this.parameters.y_min);
        let max = parseFloat(this.parameters.y_max);
        if('x_min' in this.parameters && 'x_max' in this.parameters)
        {
            min = parseFloat(this.parameters.x_min);
            max = parseFloat(this.parameters.x_max);
        }

        this.canvas.font = this.format.scale_font;
        this.canvas.fillStyle = this.format.axis_color;
        this.canvas.textAlign = "center";
        this.canvas.textBaseline="top";

        let i=0;
        for(let j=0; j<n; j++)
        {
            let v = min + j*(max-min)/(n-1);
            if(this.format.flip_x_axis)
                v = max - v;
            this.canvas.fillText(v.toFixed(this.format.decimals), i, height+this.format.scale_offset);
            i += width/(n-1);
        }
        this.canvas.textBaseline="bottom";
    }

    drawXAxis(width, height)
    {
        if(!this.format.show_x_axis)
            return;

        const {min, max} = this.getYRange();
        let y = height;
        if(min <= 0 && max >= 0)
            y = this.getPlotYForValue(0, height);

        this.canvas.beginPath();
        this.canvas.lineWidth = 1;
        this.canvas.strokeStyle = this.format.axis_color;
        this.canvas.moveTo(0, y);
        this.canvas.lineTo(width, y);
        this.canvas.stroke();
    }

    drawYAxis(width, height)
    {
        if(!this.format.show_y_axis)
            return;

        this.canvas.beginPath();
        this.canvas.lineWidth = 1;
        this.canvas.strokeStyle = this.format.axis_color;
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
        let n = this.format.horizontal_grid_lines;
        if(n==0)
            return;

        const {min, max} = this.getYRange();
        for(let j=0; j<n; j++)
        {
            const value = min + (n-j-1) * (max-min) / (n-1);
            let q = Math.round(this.getPlotYForValue(value, height));
            this.canvas.beginPath();
            this.canvas.strokeStyle = this.format.grid_color;
            this.canvas.moveTo(0, q);
            this.canvas.lineTo(width, q);
            this.canvas.stroke();
        }
    }

    drawVerticalGridlines(width, height)
    {
        let n = this.format.vertical_grid_lines;
        if(n==0)
            return;
        
        let p=0;
        for(let j=0; j<n; j++)
        {
            let q = Math.round(p)
            this.canvas.beginPath();
            this.canvas.strokeStyle = this.format.grid_color;
            this.canvas.moveTo(q, 0);
            this.canvas.lineTo(q, height);
            this.canvas.stroke();
            p += width/(n-1);
        }
    }

    drawHorizontalGridlinesOver(width, height)
    {
        let n = this.format.horizontal_grid_lines_over;
        if(n==0)
            return;

        const {min, max} = this.getYRange();
        for(let j=0; j<n; j++)
        {
            const value = min + (n-j-1) * (max-min) / (n-1);
            let q = Math.round(this.getPlotYForValue(value, height));
            this.canvas.beginPath();
            this.canvas.strokeStyle = this.format.grid_color;
            this.canvas.moveTo(0, q);
            this.canvas.lineTo(width, q);
            this.canvas.stroke();
        }
    }

    drawVerticalGridlinesOver(width, height)
    {
        let n = this.format.vertical_grid_lines_over;
        if(n==0)
            return;
        
        let p=0;
        for(let j=0; j<n; j++)
        {
            let q = Math.round(p)
            this.canvas.beginPath();
            this.canvas.strokeStyle = this.format.grid_color;
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
        
        if(!this.format.show_x_labels)
            return;
        
        let labels = (this.parameters.labels_x ? this.parameters.labels_x : this.parameters.labels);
        if(!labels)
            return;
        
        this.canvas.font = this.format.label_font;
        this.canvas.fillStyle = this.format.labelColor;
        this.canvas.textAlign = "center";
        this.canvas.textBaseline= "top";

        let l = String(labels).split(',');
//        let n = this.data.length;
        let bar_width = (width)/(n + (n-1)*this.format.spacing);
        let bar_spacing = Math.round((1 + this.format.spacing) * bar_width);

        this.canvas.save();
        this.canvas.translate(this.format.space_left+Math.round(bar_width)/2, 5);
        for(let i=0; i<n; i++)
        {
            const label = (l[i] ?? "").trim();
            this.canvas.fillText(label, 0, 0);
            this.canvas.translate(bar_spacing, 0);
        }
        this.canvas.restore();
    }

    drawLabelsHorizontal(width, height, n)
    {
//        this.canvas.fillStyle = '#ccffcc';
//        this.canvas.fillRect(0, 0, width, height);

        if(!this.format.show_y_labels)
            return;
        
        let labels = (this.parameters.labels_y ? this.parameters.labels_y : this.parameters.labels);
        if(!labels)
            return;

        this.canvas.font = this.format.label_font;
        this.canvas.fillStyle = this.format.labelColor;
        this.canvas.textAlign = "right";
        this.canvas.textBaseline= "middle";

        let l = String(labels).split(',');
//        let n = this.data.length;
        let bar_height = (height)/(n + (n-1)*this.format.spacing);
        let bar_spacing = Math.round((1 + this.format.spacing) * bar_height);

        this.canvas.save();
        this.canvas.translate(0, this.format.space_top+Math.round(bar_height)/2);
        for(let i=0; i<n; i++)
        {
            const label = (l[i] ?? "").trim();
            this.canvas.fillText(label, 0, 0);
            this.canvas.translate(0, bar_spacing);
        }
        this.canvas.restore();
    }

    drawVertical(size_x, size_y)
    {
        let pane_y = Math.round((this.format.height)/size_y);
        let pane_x = Math.round(this.format.width);
        const effectiveSpaceLeft = this.getEffectiveSpaceLeft(pane_y);
        let plot_height = pane_y-this.format.space_top-this.format.space_bottom;
        let plot_width = pane_x-effectiveSpaceLeft-this.format.space_right;

        this.drawLabelsHorizontal(plot_width, this.format.height, size_y);

        this.canvas.save();
            this.canvas.translate(effectiveSpaceLeft, 0);
            this.drawVerticalGridlines(plot_width, this.format.height);
            this.drawBottomScale(plot_width, this.format.height);
            this.drawBottomTickMarks(plot_width, this.format.height);
        this.canvas.restore();

        this.canvas.save();
        for(let y=0; y<size_y; y++)
        {
            this.canvas.save();
                this.canvas.translate(0, this.format.space_top);
                this.canvas.save();
                    this.canvas.translate(effectiveSpaceLeft, 0);
                    this.drawHorizontalGridlines(plot_width, plot_height);
                this.canvas.restore();
                this.canvas.save();
                    this.canvas.translate(effectiveSpaceLeft, 0);
                        if(this.format.flip_x_canvas)
                        {
                            this.canvas.translate(plot_width, 0);
                            this.canvas.scale(-1, 1);
                        }
                        if(this.format.flip_y_canvas)
                        {
                            this.canvas.translate(0, plot_height);
                            this.canvas.scale(1, -1);
                        }

                        if(this.format.flip_x_axis && this.format.flip_y_axis)
                            this.drawPlotVertical(plot_width, plot_height, y, function (x, y) { return [plot_width-x, plot_height-y] });
                        else if(this.format.flip_y_axis)
                            this.drawPlotVertical(plot_width, plot_height, y, function (x, y) { return [x, plot_height-y] });
                        else if(this.format.flip_x_axis)
                            this.drawPlotVertical(plot_width, plot_height, y, function (x, y) { return [plot_width-x, y] });
                        else
                            this.drawPlotVertical(plot_width, plot_height, y, function (x, y) { return [x, y] });

                this.canvas.restore();
                this.canvas.save();
                    this.canvas.translate(effectiveSpaceLeft, 0);
                    this.drawLeftScale(plot_width, plot_height);
                    this.drawRightScale(plot_width, plot_height);
                    this.drawLeftTickMarks(plot_width, plot_height);
                    this.drawRightTickMarks(plot_width, plot_height);
                this.canvas.restore();
                this.canvas.save();
                    this.canvas.translate(effectiveSpaceLeft, 0);
                    this.drawHorizontalGridlinesOver(plot_width, plot_height);
                this.canvas.restore();
                this.canvas.translate(0, -this.format.space_top);
                this.canvas.save();
                    this.canvas.translate(effectiveSpaceLeft, 0);
                    this.canvas.translate(0, this.format.space_top);
                    this.drawXAxis(plot_width, plot_height);
                    this.drawYAxis(plot_width, plot_height);
                    this.canvas.translate(0, -this.format.space_top);
                    this.drawFrame(plot_width, pane_y);
                this.canvas.restore();
            this.canvas.restore();
            this.canvas.translate(0, pane_y);
        }
        this.drawLabelsVertical(plot_width, this.format.height, size_x);
        this.canvas.restore();
        this.canvas.translate(effectiveSpaceLeft, 0);
        this.drawVerticalGridlinesOver(plot_width, this.format.height);
    }

    drawHorizontal(size_x, size_y)
    {
        let pane_y = Math.round(this.format.height);
        let pane_x = Math.round((this.format.width)/size_y);
        let plot_height = pane_y-this.format.space_top-this.format.space_bottom;
        const effectiveSpaceLeft = this.getEffectiveSpaceLeft(pane_y);
        let plot_width = pane_x-effectiveSpaceLeft-this.format.space_right;

        this.drawLabelsHorizontal(this.format.width, plot_height, size_x);
        this.canvas.save();
            this.canvas.translate(0, this.format.height);
            this.drawLabelsVertical(this.format.width, this.format.height, size_y);
        this.canvas.restore();

        this.canvas.save();
            this.canvas.translate(0, this.format.space_top);
            this.canvas.save();
                this.canvas.translate(effectiveSpaceLeft, 0);
                this.drawLeftScale(plot_width, plot_height);
                this.drawLeftTickMarks(plot_width, plot_height);
                this.drawHorizontalGridlines(plot_width, plot_height);
            this.canvas.restore();
            this.canvas.save();
                this.canvas.translate(effectiveSpaceLeft, 0);
                this.drawRightScale(plot_width, plot_height);
                this.drawRightTickMarks(plot_width, plot_height);
            this.canvas.restore();
        this.canvas.restore();

        this.canvas.save();
            for(let y=0; y<size_y; y++)
            {
                this.canvas.save();
                    this.canvas.translate(effectiveSpaceLeft, 0);
                    this.drawVerticalGridlines(plot_width, pane_y);
                    this.drawBottomScale(plot_width, pane_y); // ****
                    this.canvas.save();
                        this.canvas.translate(0, this.format.space_top);
                        if(this.format.flip_x_canvas)
                        {
                            this.canvas.translate(plot_width, 0);
                            this.canvas.scale(-1, 1);
                        }
                        if(this.format.flip_y_canvas)
                        {
                            this.canvas.translate(0, plot_height);
                            this.canvas.scale(1, -1);
                        }
                
                        if(this.format.flip_x_axis && this.format.flip_y_axis)
                            this.drawPlotHorizontal(plot_width, plot_height, y, function (x, y) { return [plot_width-x, plot_height-y] });
                        else if(this.format.flip_y_axis)
                            this.drawPlotHorizontal(plot_width, plot_height, y, function (x, y) { return [x, plot_height-y] });
                        else if(this.format.flip_x_axis)
                            this.drawPlotHorizontal(plot_width, plot_height, y, function (x, y) { return [plot_width-x, y] });
                        else
                            this.drawPlotHorizontal(plot_width, plot_height, y, function (x, y) { return [x, y] });
                
                    this.canvas.restore();
                    this.drawBottomTickMarks(plot_width, pane_y);
                    this.drawVerticalGridlinesOver(plot_width, pane_y);
                    this.canvas.save();
                        this.canvas.translate(0, this.format.space_top);
                        this.drawXAxis(plot_width, plot_height);
                        this.drawYAxis(plot_width, plot_height);
                    this.canvas.restore();
                    this.drawFrame(plot_width, pane_y);
                this.canvas.restore();
                this.canvas.translate(pane_x, 0);
            }
        this.canvas.restore();
        this.canvas.translate(0, this.format.space_top);
        this.canvas.save();
            this.canvas.translate(effectiveSpaceLeft, 0);
            this.drawHorizontalGridlinesOver(plot_width, plot_height);
        this.canvas.restore();
     }

    draw(size_x, size_y)    // draw handles the layout of the graphs in horizontal or vertical sections
    {
        this.resetCanvasTransform(-0.5, -0.5);
        this.canvas.clearRect(0, 0, this.width, this.height);
//        this.drawTitle();
        this.canvas.translate(this.format.margin_left, this.format.margin_top); // +0*this.format.titleHeight
        
        if(this.parameters.orientation == 'vertical')
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
        this.resetCanvasTransform(-0.5, -0.5);
        this.canvas.clearRect(0, 0, this.width, this.height);
        this.canvas.translate(this.format.margin_left, this.format.margin_top); // +0*this.format.titleHeight
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
