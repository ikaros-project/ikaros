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
            {'name':'order', 'default':"col", 'type':'string', 'control': 'menu', 'values': "col,row"},
            {'name':'select', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'selectValue', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'count', 'default':0, 'type':'int', 'control': 'textedit'},

            {'name': "MARKER STYLE", 'control':'header'},

            {'name':'markerType', 'default':"circle", 'type':'string', 'control': 'menu', 'values': "none,circle,cross"}, // dot, square, rectangle?
            {'name':'size', 'default':0.02, 'type':'float', 'control': 'textedit'},
            {'name':'color', 'default':'', 'type':'string', 'control': 'textedit'},   // no default = get from CSS would be a good functionality
            {'name':'fill', 'default':'gray', 'type':'string', 'control': 'textedit'},
            {'name':'lineWidth', 'default':1, 'type':'float', 'control': 'textedit'},
 //           {'name':'lineDash', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'lineCap', 'default':"butt", 'type':'string', 'control': 'menu', 'values': "butt,round,square"},
            {'name':'lineJoin', 'default':"miter", 'type':'string', 'control': 'menu', 'values': "miter,round,bevel"},

            {'name': "LABEL STYLE", 'control':'header'},

            {'name':'labelType', 'default':"none", 'type':'string', 'control': 'menu', 'values': "none,labels, alphabetical, numbered, x_value, y_value, z_value, xy_value, value"},
            {'name':'labels', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'labelFont', 'default':"18px sans-serif", 'type':'string', 'control': 'textedit'},
            {'name':'labelDecimals', 'default':2, 'type':'int', 'control': 'textedit'},
            {'name':'labelPrefix', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'labelPostfix', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'labelAlign', 'default':"center", 'type':'string', 'control': 'menu', 'values': "left, center, right"},
            {'name':'labelBaseline', 'default':"middle", 'type':'string', 'control': 'menu', 'values': "top, bottom, middle, alphabetic, hanging"},
            {'name':'labelOffsetX', 'default':"0", 'type':'float', 'control': 'textedit'},
            {'name':'labelOffsetY', 'default':"0", 'type':'float', 'control': 'textedit'},

            
            {'name': "COORDINATE SYSTEM", 'control':'header'},

            {'name':'scales', 'default':"no", 'type':'string', 'control': 'menu', 'values': "yes,no,invisible", 'class':'true'},
            {'name':'min_x', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'max_x', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'min_y', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'max_y', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'flipXAxis', 'default':"no", 'type':'string', 'control': 'menu', 'values': "yes,no"},
            {'name':'flipYAxis', 'default':"no", 'type':'string', 'control': 'menu', 'values': "yes,no"},
            {'name':'flipXCanvas', 'default':"no", 'type':'string', 'control': 'menu', 'values': "yes,no"},
            {'name':'flipYCanvas', 'default':"no", 'type':'string', 'control': 'menu', 'values': "yes,no"},

            {'name': "FRAME", 'control':'header'},

            {'name':'show_title', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]
    }

    init()
    {
        super.init();
        this.data = [];

        this.onclick = function () {
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
/*
    requestData(data_set)
    {
        data_set.add(this.parameters['module']+"."+this.parameters['source']);
        if(this.parameters['length_module'])
            data_set.add(this.parameters['length_module']+"."+this.parameters['length_source']);
    }
*/
    drawRows(width, height, index, transform)
    {
        let s = this.parameters.size*(width+height)/2
        let d = this.data;
        let rows = this.data.length;
        this.canvas.lineWidth = this.format.lineWidth;
        this.canvas.lineCap = this.format.lineCap;
        this.canvas.lineJoin = this.format.lineJoin;

        let xx = (this.parameters.count ? this.parameters.select+2*this.parameters.count : d[0].length);
        
        for(var i=0; i<rows; i++)
        {
            this.setColor(i);
            this.canvas.beginPath();
            
            let lx = 0;
            let ly = 0;
            let x = (d[i][this.parameters.select+0]-this.parameters.min_x)*this.parameters.scale_x * width;
            let y = (d[i][this.parameters.select+1]-this.parameters.min_y)*this.parameters.scale_y * height;
            
            for(var j=this.parameters.select+2; j<xx;)
            {
                lx = x;
                ly = y;
                x = (d[i][j++]-this.parameters.min_x)*this.parameters.scale_x * width;
                y = (d[i][j++]-this.parameters.min_y)*this.parameters.scale_y * height;
                
                if(this.parameters.markerType == "circle")
                {
                    this.canvas.arc(...transform(x, y), s, 0, 2*Math.PI);
                }
                else if(this.parameters.markerType == "cross")
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
        let l = this.parameters.labels.split(',');
        let n = l.length;
    
        let s = this.parameters.size*(width+height)/2
        let d = this.data;
        let rows = this.data.length;
        
        this.parameters.labelOffsetX = parseFloat(this.parameters.labelOffsetX);    // FIXME: should be converted somewhere else
        this.parameters.labelOffsetY = parseFloat(this.parameters.labelOffsetY);

        this.canvas.lineWidth = this.format.lineWidth;
        this.canvas.lineCap = this.format.lineCap;
        this.canvas.lineJoin = this.format.lineJoin;

        this.canvas.font = this.parameters.labelFont;
        this.canvas.textAlign = this.parameters.labelAlign;
        this.canvas.textBaseline = this.parameters.labelBaseline;

        let xx = (this.parameters.count ? this.parameters.select+2*this.parameters.count : d[0].length);
        let c = 0;
        for(var i=this.parameters.select; i<xx; i+=2)
        {
            let lx = 0;
            let ly = 0;
            let x = (d[0][i+0]-this.parameters.min_x)*this.parameters.scale_x * width;
            let y = (d[0][i+1]-this.parameters.min_y)*this.parameters.scale_y * height;
            
            for(var j=0; j<rows;j++)
            {
                x = (d[j][i+0]-this.parameters.min_x)*this.parameters.scale_x * width;
                y = (d[j][i+1]-this.parameters.min_y)*this.parameters.scale_y * height;

                this.setColor(c);
                this.canvas.beginPath();
                
                if(this.parameters.markerType == "circle")
                {
                    this.canvas.arc(...transform(x, y), s, 0, 2*Math.PI);
                }
                else if(this.parameters.markerType == "cross")
                {
                    this.canvas.moveTo(...transform(x-s/2, y));
                    this.canvas.lineTo(...transform(x+s/2, y));
                    this.canvas.moveTo(...transform(x, y-s/2));
                    this.canvas.lineTo(...transform(x, y+s/2));
               }

                this.canvas.fill();
                this.canvas.stroke();
                
                if(this.parameters.labelType != "none")
                {
                    let lbl = l[j % n];
                    if(this.parameters.labelType == "alphabetical")
                        lbl = String.fromCharCode(65+j);
                    if(this.parameters.labelType == "numbered")
                        lbl = j;
                    if(this.parameters.labelType == "x_value")
                        lbl = d[j][i+0].toFixed(this.parameters.labelDecimals);
                    else if(this.parameters.labelType == "y_value")
                        lbl = d[j][i+1].toFixed(this.parameters.labelDecimals);
                    else if(this.parameters.labelType == "xy_value")
                        lbl = d[j][i+0].toFixed(this.parameters.labelDecimals)+", "+d[j][i+1].toFixed(this.parameters.labelDecimals);
                    else if(this.parameters.labelType == "z_value")
                         lbl = d[j][i+2].toFixed(this.parameters.labelDecimals);
                    else if(this.parameters.labelType == "value")
                        lbl = d[j][this.parameters.selectValue].toFixed(this.parameters.labelDecimals);

                    this.canvas.fillText(this.parameters.labelPrefix+lbl+this.parameters.labelPostfix, ...transform(x+this.parameters.labelOffsetX, y+this.parameters.labelOffsetY));
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
        this.parameters.select = (this.parameters.select ? this.parameters.select : 0);

        this.parameters.min_x = (typeof this.parameters.min_x !== 'undefined' ? this.parameters.min_x : this.parameters.min);
        this.parameters.max_x = (typeof this.parameters.max_x !== 'undefined' ? this.parameters.max_x : this.parameters.max);
        this.parameters.scale_x = 1/(this.parameters.max_x == this.parameters.min_x ? 1 : this.parameters.max_x-this.parameters.min_x);
        
        this.parameters.min_y = (typeof this.parameters.min_y !== 'undefined' ? this.parameters.min_y : this.parameters.min);
        this.parameters.max_y = (typeof this.parameters.max_y !== 'undefined' ? this.parameters.max_y : this.parameters.max);
        this.parameters.scale_y = 1/(this.parameters.max_y == this.parameters.min_y ? 1 : this.parameters.max_y-this.parameters.min_y);

        // draw if data available
        if(!d)
            return;
        
        try {
            this.data = d[this.parameters['source']];

            if(!this.data)
                return;

            this.canvas.setTransform(1, 0, 0, 1, -0.5, -0.5);
            this.canvas.clearRect(0, 0, this.width, this.height);
            this.canvas.translate(this.format.marginLeft, this.format.marginTop); //

            this.drawHorizontal(1, 1);  // Draw grid over image - should be Graph:draw() with no arguments
        }
        catch(err)
        {
        }
    }
};


webui_widgets.add('webui-widget-marker', WebUIWidgetMarker);
