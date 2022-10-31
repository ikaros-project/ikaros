class WebUIWidgetImage extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "IMAGE", 'control':'header'},
            {'name':'title', 'default':"Image", 'type':'string', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'opacity_source', 'default':"", 'type':'source', 'control': 'textedit'},

            {'name':'file', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'index', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'index', 'default':"", 'type':'opacity_source', 'control': 'textedit'},

            {'name': "CONTROL", 'control':'header'},
                
            {'name':'module', 'default':"", 'type':'module', 'control': 'textedit'},
            {'name':'command', 'default':"", 'type':'string', 'control': 'textedit'},

            {'name': "STYLE", 'control':'header'},

            {'name':'format', 'default':"gray", 'type':'string', 'control': 'menu', 'values': "gray,fire,spectrum,red,green,blue,rgb"},
            {'name':'scale', 'default':"both", 'type':'string', 'control': 'menu', 'values': "none,width,height,both"},
            {'name':'opacity', 'default':1, 'min': 0, 'max':1, 'type':'float', 'control': 'slider'},
            
            {'name': "COORDINATE SYSTEM", 'control':'header'},

            {'name':'scales', 'default':"no", 'type':'string', 'control': 'menu', 'values': "yes,no", 'class':'true'},
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
        ]};

    init()
    {
        super.init();
        
        this.onclick = function (evt)
        {
            let lw = this.parameters.labels ? parseInt(this.parameters.labelWidth) : 0;
            let r = this.canvasElement.getBoundingClientRect();
            let x = (evt.clientX - r.left - this.format.spaceLeft - lw)/(r.width - this.format.spaceLeft - this.format.spaceRight- lw);
            let y = (evt.clientY - r.top - this.format.spaceTop)/(r.height - this.format.spaceTop - this.format.spaceBottom);
            
            if(this.parameters.command && this.parameters.module)
                this.get("/command/"+this.parameters.module+"/"+this.parameters.command+"/"+x+"/"+y+"/1");
        }
    }

    requestData(data_set)
    {
        if(!this.parameters.file)
            data_set.add(this.parameters.source+":"+this.parameters.format);
        if(this.parameters.index)
            data_set.add(this.parameters.index);
        if(this.parameters.opacity_source)
            data_set.add(this.parameters.opacity_source);
    }

    updateFrame()
    {
        if(this.parameters.opacity != 1)
            this.canvas.canvas.style.opacity = this.parameters.opacity;
        
        this.oversampling = 1; //(this.parameters.file ? 4 : 1);
        this.imageObj = new Image();
        this.imageCount = 0;
        if(this.parameters.file) //  && this.parameters.file.indexOf(",")!=-1
        {
            this.imageObjects = [];
            let img_names = this.parameters.file.split(',');
            this.imageCount = img_names.length;
            let i = 0;
            for(let img_name of img_names)
            {
                this.imageObjects[i] = new Image();
                this.imageObjects[i].src = "/"+img_name.trim();
                i++;
            }
        }
        else
        {
            this.canvas.fillStyle="black";
            this.canvas.fillRect(0, 0, this.width, this.height);
        }
        super.updateFrame();
    }

    loadData(data)
    {
        if(this.parameters.source)
        {
            let d = data[this.parameters.source+":"+this.parameters.format];
            if(!d) return;
            this.imageObj.onload = function ()
            {
                controller.load_count--;
            };
            this.imageObj.src = d;
            return 1;
        }

        return 0;
    }

    drawPlotHorizontal(width, height)   // Draw actual image in a coordinate system
    {
        let w = this.oversampling*width;   // this could be done in updateFrame and stored
        let h = this.oversampling*height;
        
        if(this.imageCount)
        {
            if(this.parameters.scale == "width")
                h = this.imageObjects[0].height;
            else if(this.parameters.scale == "height")
                w = this.imageObjects[0].width;
            else if(this.parameters.scale == "none")
            {
                w = this.imageObjects[0].width;
                h = this.imageObjects[0].height;
            }
            let ix = 0;
            let index = this.getSource("index");
            if(index)
            {
                ix = Math.floor(index[0][0]);
                if(ix < 0)
                    ix = 0;
                else if(ix >= this.imageCount)
                    ix = this.imageCount-1;
            }
            this.canvas.drawImage(this.imageObjects[ix], 0, 0, w, h);
        }
        else
        {
            if(this.parameters.scale == "width")
                h = this.imageObj.height;
            else if(this.parameters.scale == "height")
                w = this.imageObj.width;
            else if(this.parameters.scale == "none")
            {
                w = this.imageObj.width;
                h = this.iimageObj.height;
            }

            this.canvas.drawImage(this.imageObj, 0, 0, w, h);
        }
    }
    
    update(d)
    {
        try
        {
            let o = this.getSource('opacity_source');
            if(o)
            this.canvas.canvas.style.opacity = o;
            this.canvas.setTransform(1, 0, 0, 1, -0.5, -0.5);
            this.canvas.clearRect(0, 0, this.width, this.height);
            this.canvas.translate(this.format.marginLeft, this.format.marginTop); //

            this.drawHorizontal(1, 1);  // Draw grid over image
        }
        catch(err)
        {
            this.canvas.fillStyle="black";
            this.canvas.fillRect(0, 0, this.oversampling*this.width, this.oversampling*this.height);
        }
    }
};



webui_widgets.add('webui-widget-image', WebUIWidgetImage);
