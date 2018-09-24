class WebUIWidgetGrid extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "DATA", 'control':'header'},
            
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'labels', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'labelWidth', 'default':100, 'type':'int', 'control': 'textedit'},

            {'name': "STYLE", 'control':'header'},

            {'name':'color', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'fill', 'default':"gray", 'type':'string', 'control': 'menu', 'values': "gray,fire,spectrum,custom"},
            {'name':'colorTable', 'default':'', 'type':'string', 'control': 'textedit'},
            {'name':'lineWidth', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'shape', 'default':"rectangle", 'type':'string', 'control': 'menu', 'values': "rectangle,square,circle"},
            {'name':'size', 'default':1, 'type':'float', 'control': 'textedit'},

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
        this.canvas.textAlign = 'left';
        this.canvas.textBaseline = 'middle';

        let ct = LUT_gray;
        if(this.parameters.fill == 'fire')
            ct = LUT_fire;
        else if(this.parameters.fill == 'spectrum')
            ct = LUT_spectrum;

        if(this.parameters.colorTable != "")
        {
            let q = this.parameters.colorTable;
            ct = this.parameters.colorTable.split(',');
        }

        let labels = this.parameters.labels === "" ? [] : this.parameters.labels.split(',');
        let ln = labels.length;
        let ls = (ln ? parseInt(this.parameters.labelWidth) : 0);
        let n = ct.length;
        let dx = (width-ls)/cols;
        let dy = height/rows;
        let sx = dx*this.parameters.size;
        let sy = dy*this.parameters.size;

        if(this.parameters.shape == 'square' || this.parameters.shape == 'circle')
        {
            let minimum = Math.min(sx, sy);
            sx = minimum;
            sy = minimum;
        }

        for(var i=0; i<rows; i++)
        {
            if(ln)
            {
                this.canvas.fillStyle = "black";    // FIXME: Should really use the default color form the stylesheet
                this.canvas.fillText(labels[i % (ln+1)].trim(), 0, dy*i+dy/2);
            }

            for(var j=0; j<cols; j++)
            {
                this.setColor(i+j);
                this.canvas.beginPath();
                let f = (d[i][j]-this.parameters.min)/(this.parameters.max-this.parameters.min);
                let ix = Math.floor((n-1)*f);
                this.canvas.fillStyle = ct[ix].trim();
                
                if(this.parameters.shape == 'circle')
                    this.canvas.arc(ls+dx*j+dx/2, dy*i+dy/2, sx/2, 0, 2*Math.PI);
                else
                    this.canvas.rect(ls+dx*j+dx/2-sx/2, dy*i+dy/2-sy/2, sx, sy);

                this.canvas.fill();
                this.canvas.stroke();
            }
        }
    }

    update(d) // default for Graph - should not be needed here
    {
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
