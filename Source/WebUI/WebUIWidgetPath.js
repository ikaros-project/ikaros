class WebUIWidgetPath extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "PATH", 'control':'header'},
            
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'},
//            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
//            {'name':'length_module', 'default':"", 'type':'source', 'control': 'textedit'},
//            {'name':'length_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'order', 'default':"col", 'type':'string', 'control': 'menu', 'values': "col,row"},
            {'name':'select', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'count', 'default':0, 'type':'int', 'control': 'textedit'},

             {'name': "STYLE", 'control':'header'},

            {'name':'color', 'default':'', 'type':'string', 'control': 'textedit'},   // TODO: no default = get from CSS would be a good functionality
            {'name':'fill', 'default':'gray', 'type':'string', 'control': 'textedit'},
            {'name':'lineWidth', 'default':1, 'type':'float', 'control': 'textedit'},
 //           {'name':'lineDash', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'lineCap', 'default':"butt", 'type':'string', 'control': 'menu', 'values': "butt,round,quare"},
            {'name':'lineJoin', 'default':"miter", 'type':'string', 'control': 'menu', 'values': "miter,round,bevel"},
            
            {'name':'close', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'arrow', 'default':false, 'type':'bool', 'control': 'checkbox'},
            
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

    drawRows(width, height, index, transform)
    {
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
            this.canvas.moveTo(...transform(x, y));
            
            for(var j=this.parameters.select+2; j<xx;)
            {
                lx = x;
                ly = y;
                x = (d[i][j++]-this.parameters.min_x)*this.parameters.scale_x * width;
                y = (d[i][j++]-this.parameters.min_y)*this.parameters.scale_y * height;
                
                this.canvas.lineTo(...transform(x, y));
            }
            
            this.canvas.fill();
            if(this.parameters.close)
                this.canvas.closePath();
            this.canvas.stroke();
            
            if(this.parameters.arrow)
                this.drawArrowHead(...transform(lx, ly), ...transform(x, y));
        }
    }

    drawCols(width, height, index, transform)
    {
        let d = this.data;
        let rows = this.data.length;
        this.canvas.lineWidth = this.format.lineWidth;
        this.canvas.lineCap = this.format.lineCap;
        this.canvas.lineJoin = this.format.lineJoin;

        let xx = (this.parameters.count ? this.parameters.select+2*this.parameters.count : d[0].length);
        let c = 0;
        for(var i=this.parameters.select; i<xx; i+=2)
        {
            this.setColor(c);
            this.canvas.beginPath();
            
            let lx = 0;
            let ly = 0;
            let x = (d[0][i+0]-this.parameters.min_x)*this.parameters.scale_x * width;
            let y = (d[0][i+1]-this.parameters.min_y)*this.parameters.scale_y * height;
            this.canvas.moveTo(...transform(x, y));
            
            for(var j=1; j<rows;j++)
            {
                lx = x;
                ly = y;
                x = (d[j][i+0]-this.parameters.min_x)*this.parameters.scale_x * width;
                y = (d[j][i+1]-this.parameters.min_y)*this.parameters.scale_y * height;
                
                this.canvas.lineTo(...transform(x, y));
            }

            this.canvas.fill();
            if(this.parameters.close)
                this.canvas.closePath();
            this.canvas.stroke();
            
            if(this.parameters.arrow)
                this.drawArrowHead(...transform(lx, ly), ...transform(x, y));

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

    update(d) // default for Graph - should not be needed here
    {
        // update parameters
        // FIXME: should be moved to graph later for all graphs
        
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
        
        if(this.data = this.getSource('source'))
        {
            this.canvas.setTransform(1, 0, 0, 1, -0.5, -0.5);
            this.canvas.clearRect(0, 0, this.width, this.height);
            this.canvas.translate(this.format.marginLeft, this.format.marginTop); //

            this.drawHorizontal(1, 1);  // Draw grid over image - should be Graph:draw() with no arguments
        }
     }
};


webui_widgets.add('webui-widget-path', WebUIWidgetPath);
