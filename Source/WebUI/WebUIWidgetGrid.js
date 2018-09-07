class WebUIWidgetGrid extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "DATA", 'control':'header'},
            
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'order', 'default':"col", 'type':'string', 'control': 'menu', 'values': "col,row"},
            {'name':'select', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'count', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'min', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'max', 'default':1, 'type':'float', 'control': 'textedit'},

            {'name': "STYLE", 'control':'header'},

            {'name':'color', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'fill', 'default':"gray", 'type':'string', 'control': 'menu', 'values': "gray,fire,spectrum,custom color table"},
            {'name':'coloTable', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'lineWidth', 'default':1, 'type':'float', 'control': 'textedit'},
 //           {'name':'lineDash', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'lineCap', 'default':"butt", 'type':'string', 'control': 'menu', 'values': "butt,round,quare"},
            {'name':'lineJoin', 'default':"miter", 'type':'string', 'control': 'menu', 'values': "miter,round,bevel"},
            
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

            {'name':'show_title', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':true, 'type':'bool', 'control': 'checkbox'},
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

    requestData(data_set)
    {
        data_set.add(this.parameters['module']+"."+this.parameters['source']);
        if(this.parameters['length_module'])
            data_set.add(this.parameters['length_module']+"."+this.parameters['length_source']);
    }

    drawPlotHorizontal(width, height, index, transform)
    {
        let d = this.data;
        let rows = d.length;
        let cols = d[0].length;
        
        this.canvas.lineWidth = this.format.lineWidth;
        this.canvas.lineCap = this.format.lineCap;
        this.canvas.lineJoin = this.format.lineJoin;

        let dx = width/cols;
        let dy = height/rows;
        let sx = dx;
        let sy = dy;

        for(var i=0; i<rows; i++)
            for(var j=0; j<cols; j++)
            {
                this.setColor(i+j);
                this.canvas.beginPath();
                let v = Math.round(16*(d[j][i]-this.parameters.min)/(this.parameters.max-this.parameters.min));
                let c = v.toString(16);
                this.canvas.fillStyle = "#" + c + c + c;
                this.canvas.rect(dx*i, dy*j, sx, sy);
                this.canvas.fill();
                this.canvas.stroke();
            }
    }

    update(d) // default for Graph - should not be needed here
    {
        // update parameters
        // FIXME: should be moved to graph later for all graphs
        
        this.parameters.select = (this.parameters.select ? this.parameters.select : 0);
        this.parameters.scale = 1/(this.parameters.max == this.parameters.min ? 1 : this.parameters.max-this.parameters.min);
        
        this.parameters.min_x = (this.parameters.min_x ? this.parameters.min_x : this.parameters.min);
        this.parameters.max_x = (this.parameters.max_x ? this.parameters.max_x : this.parameters.max);
        this.parameters.scale_x = 1/(this.parameters.max_x == this.parameters.min_x ? 1 : this.parameters.max_x-this.parameters.min_x);
        
        this.parameters.min_y = (this.parameters.min_y ? this.parameters.min_y : this.parameters.min);
        this.parameters.max_y = (this.parameters.max_y ? this.parameters.max_y : this.parameters.max);
        this.parameters.scale_y = 1/(this.parameters.max_y == this.parameters.min_y ? 1 : this.parameters.max_y-this.parameters.min_y);

        // draw if data available
        if(!d)
            return;
        
        try {
            let m = this.parameters['module'];
            let s = this.parameters['source'];
            this.data = d[m][s];

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


webui_widgets.add('webui-widget-grid', WebUIWidgetGrid);
