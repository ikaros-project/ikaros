class WebUIWidgetPlot extends WebUIWidgetGraph
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
            {'name':'close', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'arrow', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'buffer_size', 'default':50, 'type':'int', 'control': 'textedit'},
            {'name':'direction', 'default':"vertical", 'type':'string', 'min':0, 'max':2, 'control': 'menu', 'values': "vertical"},
            {'name': "STYLE", 'control':'header'},
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
        this.buffer = [];
        this.ix = 0;

        this.onclick = function () { alert(this.data) }; // last matrix
   }

    requestData(data_set)
    {
        data_set.add(this.parameters['module']+"."+this.parameters['source']);
        if(this.parameters['length_module'])
            data_set.add(this.parameters['length_module']+"."+this.parameters['length_source']);
    }


    drawRows(d, rows)
    {
        this.canvas.clearRect(0, 0, this.width, this.height);
        
        var xx = (this.count ? this.select+2*this.count : d[0].length);
        
        for(var i=0; i<rows; i++)
        {
            this.canvas.lineWidth = this.line_width_LUT[i % this.line_width_LUT.length];
            this.canvas.setLineDash(this.line_dash_LUT[i % this.line_dash_LUT.length]);
            this.canvas.strokeStyle = this.stroke_LUT[i % this.stroke_LUT.length];
            this.canvas.fillStyle = this.fill_LUT[i % this.fill_LUT.length];
            
            this.canvas.beginPath();
            
            var lx = 0;
            var ly = 0;
            var x = (d[i][this.select+0]-this.min_x)*this.scale_x * this.width;
            var y = (d[i][this.select+1]-this.min_y)*this.scale_y * this.height;
            this.canvas.moveTo(x, y);
            
            for(var j=this.select+2; j<xx;)
            {
                lx = x;
                ly = y;
                x = (d[i][j++]-this.min_x)*this.scale_x * this.width;
                y = (d[i][j++]-this.min_y)*this.scale_y * this.height;
                
                this.canvas.lineTo(x, y);
            }
            
            if(this.fill_LUT[i % this.fill_LUT.length]!= 'none')
                this.canvas.fill();
            if(this.close)
                this.canvas.closePath();
            this.canvas.stroke();
            
            if(this.arrow_head_LUT[i % this.arrow_head_LUT.length]=="yes")
                this.canvas.drawArrowHead(lx, ly, x, y);
        }
    }



    drawCols = function(d, rows)
    {
        this.canvas.clearRect(0, 0, this.width, this.height);
        this.canvas.lineWidth = this.stroke_width;
        
        var xx = (this.count ? this.select+2*this.count : d[0].length);
        
        for(var i=this.select; i<xx; i+=2)
        {
            this.canvas.lineWidth = this.line_width_LUT[i % this.line_width_LUT.length];
            this.canvas.setLineDash(this.line_dash_LUT[i % this.line_dash_LUT.length]);
            this.canvas.strokeStyle = this.stroke_LUT[i % this.stroke_LUT.length];
            this.canvas.fillStyle = this.fill_LUT[i % this.fill_LUT.length];
            
            this.canvas.beginPath();
            
            var lx = 0;
            var ly = 0;
            var x = (d[0][i+0]-this.min_x)*this.scale_x * this.width;
            var y = (d[0][i+1]-this.min_y)*this.scale_y * this.height;
            this.canvas.moveTo(x, y);
            
            for(var j=1; j<rows;j++)
            {
                lx = x;
                ly = y;
                x = (d[j][i+0]-this.min_x)*this.scale_x * this.width;
                y = (d[j][i+1]-this.min_y)*this.scale_y * this.height;
                
                this.canvas.lineTo(x, y);
            }

            if(this.fill_LUT[i % this.fill_LUT.length]!= 'none')
                this.canvas.fill();
            if(this.close)
                this.canvas.closePath();
            this.canvas.stroke();
            
            if(this.arrow)
                this.canvas.drawArrowHead(lx, ly, x, y);
        }
    }



    update(d)
    {
        if(!d)
            return;
        
        try {
            let m = this.parameters['module'];
            let s = this.parameters['source'];
            this.data = d[m][s];

            if(!this.data)
                return;

            let rows = this.data.length;

            if(this.parameters.length_source)
            {
                let r = d[this.parameters.length_module];
                if(r)
                {
                    r = r[this.parameters.length_source];
                    if(r)
                        rows = r[0][0];
                }
            }

            this.canvas.setTransform(1, 0, 0, 1, -0.5, -0.5);
            if(this.order)
                this.drawRows(d, rows);
            else
                this.drawCols(d, rows);
        }
        catch(err)
        {
//            console.log(err);
        }
    }
};


webui_widgets.add('webui-widget-path', WebUIWidgetPath);
