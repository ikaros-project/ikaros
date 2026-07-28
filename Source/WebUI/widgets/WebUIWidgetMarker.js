class WebUIWidgetMarker extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "MARKER", 'control':'header'},
            
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'},
//            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
//            {'name':'length_module', 'default':"", 'type':'source', 'control': 'textedit'},
//            {'name':'length_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'order', 'default':"col", 'type':'string', 'control': 'menu', 'options': "col,row"},
            {'name':'select_x', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'select_value_column', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'point_count', 'default':0, 'type':'int', 'control': 'textedit'},

            {'name': "MARKER STYLE", 'control':'header'},

            {'name':'marker_type', 'default':"circle", 'type':'string', 'control': 'menu', 'options': "none,circle,cross"}, // dot, square, rectangle?
            {'name':'size', 'default':0.02, 'type':'float', 'control': 'textedit'},
            {'name':'stroke_color', 'default':'', 'type':'string', 'control': 'textedit'},   // no default = get from CSS would be a good functionality
            {'name':'fill_color', 'default':'gray', 'type':'string', 'control': 'textedit'},
            {'name':'line_width', 'default':1, 'type':'float', 'control': 'textedit'},
 //           {'name':'line_dash', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'line_cap', 'default':"butt", 'type':'string', 'control': 'menu', 'options': "butt,round,square"},
            {'name':'line_join', 'default':"miter", 'type':'string', 'control': 'menu', 'options': "miter,round,bevel"},

            {'name': "LABEL STYLE", 'control':'header'},

            {'name':'label_type', 'default':"none", 'type':'string', 'control': 'menu', 'options': "none,labels, alphabetical, numbered, x_value, y_value, z_value, xy_value, value"},
            {'name':'labels', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'label_font', 'default':"18px sans-serif", 'type':'string', 'control': 'textedit'},
            {'name':'label_decimals', 'default':2, 'type':'int', 'control': 'textedit'},
            {'name':'label_prefix', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'label_suffix', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'label_align', 'default':"center", 'type':'string', 'control': 'menu', 'options': "left, center, right"},
            {'name':'label_baseline', 'default':"middle", 'type':'string', 'control': 'menu', 'options': "top, bottom, middle, alphabetic, hanging"},
            {'name':'label_offset_x', 'default':"0", 'type':'float', 'control': 'textedit'},
            {'name':'label_offset_y', 'default':"0", 'type':'float', 'control': 'textedit'},

            
            {'name': "COORDINATE SYSTEM", 'control':'header'},

            {'name':'scale_visibility', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no,invisible", 'class':'true'},
            {'name':'x_min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'x_max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'y_min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'y_max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'flip_x_axis', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no"},
            {'name':'flip_y_axis', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no"},
            {'name':'flip_x_canvas', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no"},
            {'name':'flip_y_canvas', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no"},
        ]
    }

    init()
    {
        super.init();
        this.data = [];

        this.onclick = function () {
            if(main.edit_mode)
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

    getSelectX()
    {
        if(this.parameters.select_x !== undefined && this.parameters.select_x !== "")
            return Math.max(0, Math.trunc(Number(this.parameters.select_x) || 0));
        return Math.max(0, Math.trunc(Number(this.parameters.select) || 0));
    }

    formatLabelValue(value)
    {
        const numeric = Number(value);
        if(!Number.isFinite(numeric))
            return "";
        const configuredDecimals = Number(this.parameters.label_decimals);
        const decimals = Number.isFinite(configuredDecimals) ? Math.max(0, Math.min(20, Math.trunc(configuredDecimals))) : 0;
        return numeric.toFixed(decimals);
    }

    drawRows(width, height, index, transform)
    {
        let s = Math.max(0, Number(this.parameters.size) || 0) * (width+height) / 2;
        let d = this.data;
        const selectX = this.getSelectX();

        if (Array.isArray(d) && (d.length === 0 || !Array.isArray(d[0])))
            d = [d];
        if(!Array.isArray(d) || d.length === 0 || !Array.isArray(d[0]) || d[0].length < selectX + 2)
            return;

        let rows = d.length;
        this.canvas.lineWidth = Math.max(1, Number(this.format.line_width) || 1);
        this.canvas.lineCap = this.format.line_cap;
        this.canvas.lineJoin = this.format.line_join;

        //let xx = (this.parameters.point_count ? this.parameters.select+2*this.parameters.point_count : d[0].length);
        
        for(var i=0; i<rows; i++)
        {
            if(!Array.isArray(d[i]) || d[i].length < selectX + 2)
                continue;
            this.setColor(i);
            this.canvas.beginPath();
            
            let lx = 0;
            let ly = 0;
            let x = (d[i][selectX+0]-this.parameters.x_min)*this.parameters.scale_x * width;
            let y = (d[i][selectX+1]-this.parameters.y_min)*this.parameters.scale_y * height;
            if(!Number.isFinite(x) || !Number.isFinite(y))
                continue;
            
            for(var j=selectX; j<selectX+2;)
            {
                lx = x;
                ly = y;
                x = (d[i][j++]-this.parameters.x_min)*this.parameters.scale_x * width;
                y = (d[i][j++]-this.parameters.y_min)*this.parameters.scale_y * height;
                
                if(this.parameters.marker_type == "circle")
                {
                    this.canvas.arc(...transform(x, y), s, 0, 2*Math.PI);
                }
                else if(this.parameters.marker_type == "cross")
                {
                    this.canvas.moveTo(...transform(x-s, y));
                    this.canvas.lineTo(...transform(x+s, y));
                    this.canvas.moveTo(...transform(x, y-s));
                    this.canvas.lineTo(...transform(x, y+s));
               }
            }
            
            this.canvas.fill();
            this.canvas.stroke();
        }
    }

    drawCols(width, height, index, transform)
    {
        let l = String(this.parameters.labels ?? "").trim() === "" ? [] : String(this.parameters.labels).split(',');
        let n = l.length;
    
        let s = Math.max(0, Number(this.parameters.size) || 0) * (width+height) / 2;
        let d = this.data;
        const selectX = this.getSelectX();
        if (Array.isArray(d) && (d.length === 0 || !Array.isArray(d[0])))
            d = [d];
        if(!Array.isArray(d) || d.length === 0 || !Array.isArray(d[0]) || d[0].length < selectX + 2)
            return;
        let rows = d.length;
        
        const configuredLabelOffsetX = Number(this.parameters.label_offset_x);
        const configuredLabelOffsetY = Number(this.parameters.label_offset_y);
        const labelOffsetX = Number.isFinite(configuredLabelOffsetX) ? configuredLabelOffsetX : 0;
        const labelOffsetY = Number.isFinite(configuredLabelOffsetY) ? configuredLabelOffsetY : 0;

        this.canvas.lineWidth = Math.max(1, Number(this.format.line_width) || 1);
        this.canvas.lineCap = this.format.line_cap;
        this.canvas.lineJoin = this.format.line_join;

        this.canvas.font = this.parameters.label_font;
        this.canvas.textAlign = this.parameters.label_align;
        this.canvas.textBaseline = this.parameters.label_baseline;

        const pointCount = Math.max(0, Math.trunc(Number(this.parameters.point_count) || 0));
        let xx = pointCount ? selectX + 2 * pointCount : d[0].length;
        xx = Math.min(xx, d[0].length);
        let c = 0;
        for(var i=selectX; i<xx; i+=2)
        {
            if(i+1 >= d[0].length)
                break;
            let lx = 0;
            let ly = 0;
            let x = (d[0][i+0]-this.parameters.x_min)*this.parameters.scale_x * width;
            let y = (d[0][i+1]-this.parameters.y_min)*this.parameters.scale_y * height;
            
            for(var j=0; j<rows;j++)
            {
                if(!Array.isArray(d[j]) || i+1 >= d[j].length)
                    continue;
                x = (d[j][i+0]-this.parameters.x_min)*this.parameters.scale_x * width;
                y = (d[j][i+1]-this.parameters.y_min)*this.parameters.scale_y * height;
                if(!Number.isFinite(x) || !Number.isFinite(y))
                    continue;

                this.setColor(c);
                this.canvas.beginPath();
                
                if(this.parameters.marker_type == "circle")
                {
                    this.canvas.arc(...transform(x, y), s, 0, 2*Math.PI);
                }
                else if(this.parameters.marker_type == "cross")
                {
                    this.canvas.moveTo(...transform(x-s/2, y));
                    this.canvas.lineTo(...transform(x+s/2, y));
                    this.canvas.moveTo(...transform(x, y-s/2));
                    this.canvas.lineTo(...transform(x, y+s/2));
               }

                this.canvas.fill();
                this.canvas.stroke();
                
                if(this.parameters.label_type != "none")
                {
                    let lbl = n > 0 ? l[j % n] : "";
                    if(this.parameters.label_type == "alphabetical")
                        lbl = String.fromCharCode(65+j);
                    if(this.parameters.label_type == "numbered")
                        lbl = j;
                    if(this.parameters.label_type == "x_value")
                        lbl = this.formatLabelValue(d[j][i+0]);
                    else if(this.parameters.label_type == "y_value")
                        lbl = this.formatLabelValue(d[j][i+1]);
                    else if(this.parameters.label_type == "xy_value")
                        lbl = this.formatLabelValue(d[j][i+0])+", "+this.formatLabelValue(d[j][i+1]);
                    else if(this.parameters.label_type == "z_value")
                         lbl = this.formatLabelValue(d[j][i+2]);
                    else if(this.parameters.label_type == "value")
                    {
                        const valueColumn = Math.max(0, Math.trunc(Number(this.parameters.select_value_column) || 0));
                        lbl = this.formatLabelValue(d[j][valueColumn]);
                    }

                    this.canvas.fillText(this.parameters.label_prefix+lbl+this.parameters.label_suffix, ...transform(x+labelOffsetX, y+labelOffsetY));
                }
            }
            c++;
        }
    }

    drawPlotHorizontal(width, height, index, transform)
    {
        if(this.parameters.order=="row")
            this.drawRows(width, height, index, transform);
        else
            this.drawCols(width, height, index, transform);
    }

    update(d)
    {
        this.parameters.select_x = this.getSelectX();

        this.parameters.scale_x = 1/(this.parameters.x_max == this.parameters.x_min ? 1 : this.parameters.x_max-this.parameters.x_min);

        this.parameters.scale_y = 1/(this.parameters.y_max == this.parameters.y_min ? 1 : this.parameters.y_max-this.parameters.y_min);

        this.data = this.getSource('source');
        if(this.getMatrixRank(this.data) == 1)
            this.data = [this.data];
        if(!Array.isArray(this.data) || this.data.length === 0 || !Array.isArray(this.data[0]))
        {
            this.clearCanvas();
            return;
        }

        this.beginCanvasDraw();

        this.drawHorizontal(1, 1);  // Draw grid over image - should be Graph:draw() with no arguments
    }
};


webui_widgets.add('webui-widget-marker', WebUIWidgetMarker);
