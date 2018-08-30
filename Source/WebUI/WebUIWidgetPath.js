class WebUIWidgetPath extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "PARAMETERS", 'control':'header'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'length_module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'length_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'order', 'default':"col", 'type':'string', 'control': 'menu', 'values': "col,row"},
            {'name':'select', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'count', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'lineWidth', 'default':1, 'type':'float', 'control': 'textedit'},
 //           {'name':'lineDash', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'lineCap', 'default':"butt", 'type':'string', 'control': 'menu', 'values': "butt,round,quare"},
            {'name':'lineJoin', 'default':"miter", 'type':'string', 'control': 'menu', 'values': "miter,round,bevel"},
            {'name':'close', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'arrow', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name': "STYLE", 'control':'header'},
            {'name':'color', 'default':'black', 'type':'string', 'control': 'textedit'},   // no default = get from CSS
            {'name':'fill', 'default':'gray', 'type':'string', 'control': 'textedit'},
            {'name':'scales', 'default':"no", 'type':'string', 'control': 'menu', 'values': "yes,no", 'class':'true'},
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

        this.onclick = function () { alert(this.data) }; // last matrix
   }

    requestData(data_set)
    {
        data_set.add(this.parameters['module']+"."+this.parameters['source']);
        if(this.parameters['length_module'])
            data_set.add(this.parameters['length_module']+"."+this.parameters['length_source']);
    }


    drawRows(width, height)
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
            this.canvas.moveTo(x, y);
            
            for(var j=this.parameters.select+2; j<xx;)
            {
                lx = x;
                ly = y;
                x = (d[i][j++]-this.parameters.min_x)*this.parameters.scale_x * width;
                y = (d[i][j++]-this.parameters.min_y)*this.parameters.scale_y * height;
                
                this.canvas.lineTo(x, y);
            }
            
            this.canvas.fill();
            if(this.format.close)
                this.canvas.closePath();
            this.canvas.stroke();
            
            if(this.format.arrow)
                this.drawArrowHead(lx, ly, x, y);
        }
    }


    drawCols(width, height)
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
            this.canvas.moveTo(x, y);
            
            for(var j=1; j<rows;j++)
            {
                lx = x;
                ly = y;
                x = (d[j][i+0]-this.parameters.min_x)*this.parameters.scale_x * width;
                y = (d[j][i+1]-this.parameters.min_y)*this.parameters.scale_y * height;
                
                this.canvas.lineTo(x, y);
            }

            this.canvas.fill();
            if(this.format.close)
                this.canvas.closePath();
            this.canvas.stroke();
            
            if(this.format.arrow)
                this.drawArrowHead(lx, ly, x, y);

            c++;
        }
    }

    drawPlotHorizontal(width, height)
    {
        if(this.parameters.order=="row")
            this.drawRows(width, height);
        else
            this.drawCols(width, height);
    }
    
    update(d)   // default for Graph - should not be needed here
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

/*
            if(this.parameters.order=="row")
                this.count = this.data.length;
            else
                this.count = this.data[0].length;

            if(this.parameters.length_source)
            {
                let r = d[this.parameters.length_module];
                if(r)
                {
                    r = r[this.parameters.length_source];
                    if(r)
                        this.count = r[0][0];
                }
            }
*/
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


webui_widgets.add('webui-widget-path', WebUIWidgetPath);
