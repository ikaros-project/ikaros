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
            {'name':'index_source', 'default':"", 'type':'source', 'control': 'textedit'},

            {'name': "CONTROL", 'control':'header'},
                
            {'name':'command', 'default':"", 'type':'source', 'control': 'textedit'},

            {'name': "STYLE", 'control':'header'},

            {'name':'format', 'default':"gray", 'type':'string', 'control': 'menu', 'options': "gray,fire,spectrum,red,green,blue,rgb"},
            {'name':'scale', 'default':"both", 'type':'string', 'control': 'menu', 'options': "none,width,height,both"},
            {'name':'opacity', 'default':1, 'min': 0, 'max':1, 'type':'float', 'control': 'slider'},
            
            {'name': "COORDINATE SYSTEM", 'control':'header'},

            {'name':'scale_visibility', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no", 'class':'true'},
            {'name':'x_min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'x_max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'y_min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'y_max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'flip_x_axis', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no"},
            {'name':'flip_y_axis', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no"},
            {'name':'flip_x_canvas', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no"},
            {'name':'flip_y_canvas', 'default':"no", 'type':'string', 'control': 'menu', 'options': "yes,no"},
        ]};

    init()
    {
        super.init();
        
        this.onclick = function (evt)
        {
            if(main.edit_mode)
                return;
            let lw = this.parameters.labels ? parseInt(this.parameters.label_width) : 0;
            let r = this.canvasElement.getBoundingClientRect();
            let x = (evt.clientX - r.left - this.format.space_left - lw)/(r.width - this.format.space_left - this.format.space_right- lw);
            let y = (evt.clientY - r.top - this.format.space_top)/(r.height - this.format.space_top - this.format.space_bottom);
            
            if(this.parameters.command)
                this.send_command(this.parameters.command, 1, x, y);
        }
    }

    requestData(data_set)
    {
        if(!this.parameters.file && this.parameters.source)
            data_set.add(this.resolveControlPath(this.parameters.source)+":"+this.parameters.format);
        if(this.parameters.index_source)
            this.addSource(data_set, this.parameters.index_source);
        if(this.parameters.opacity_source)
            this.addSource(data_set, this.parameters.opacity_source);
    }

    updateFrame()
    {
        const configuredOpacity = Number(this.parameters.opacity);
        this.canvas.canvas.style.opacity = Number.isFinite(configuredOpacity) ? Math.max(0, Math.min(1, configuredOpacity)) : 1;
        
        this.oversampling = 1; //(this.parameters.file ? 4 : 1);
        this.imageObj = new Image();
        this.imageCount = 0;
        if(this.parameters.file) //  && this.parameters.file.indexOf(",")!=-1
        {
            this.imageObjects = [];
            let img_names = String(this.parameters.file ?? "").split(',').map((name) => name.trim()).filter((name) => name !== "");
            this.imageCount = img_names.length;
            let i = 0;
            for(let img_name of img_names)
            {
                this.imageObjects[i] = new Image();
                this.imageObjects[i].onload = () => this.update();
                this.imageObjects[i].src = "/"+img_name;
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
            let d = data[this.resolveControlPath(this.parameters.source)+":"+this.parameters.format];
            if(!d)
                return 0;
            const image = this.imageObj;
            const loadToken = (this.image_load_token || 0) + 1;
            this.image_load_token = loadToken;
            let completed = false;
            const finishLoad = () =>
            {
                if(completed || this.image_load_token !== loadToken)
                    return;
                completed = true;
                controller.load_count--;
            };
            image.onload = finishLoad;
            image.onerror = finishLoad;
            if(image.src === d && image.complete)
                return 0;
            image.src = d;
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
            if (!this.imageObjects || !this.imageObjects[0])
                return;
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
            let index = this.getSource("index_source");
            if(index)
            {
                if (Array.isArray(index))
                    ix = Math.floor(this.sourceScalar(index, 0));
                else
                    ix = Math.floor(index);
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
                h = this.imageObj.height;
            }

            this.canvas.drawImage(this.imageObj, 0, 0, w, h);
        }
    }
    
    update(d)
    {
        try
        {
            const opacitySource = this.getSource('opacity_source');
            if(opacitySource !== undefined && opacitySource !== null)
            {
                const opacityValue = this.sourceScalar(opacitySource);
                const numericOpacity = Number(opacityValue);
                if(Number.isFinite(numericOpacity))
                    this.canvas.canvas.style.opacity = Math.max(0, Math.min(1, numericOpacity));
            }
            this.beginCanvasDraw();

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
