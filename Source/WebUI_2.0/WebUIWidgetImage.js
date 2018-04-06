class WebUIWidgetImage extends WebUIWidgetCanvas
{
    static template()
    {
        return [
            {'name': "PARAMETERS", 'control':'header'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'title', 'default':"Image", 'type':'string', 'control': 'textedit'},
            {'name':'file', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'format', 'default':"gray", 'type':'string', 'control': 'menu', 'values': "gray,fire,spectrum,red,green,blue,rgb"},
            {'name':'scale', 'default':"both", 'type':'string', 'control': 'menu', 'values': "none,width,height,both"},
            {'name': "STYLE", 'control':'header'},
            {'name':'show_title', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    requestData(data_set)
    {
        if(!this.parameters.file)
            data_set.add(this.parameters.module+"."+this.parameters.source+":"+this.parameters.format);
    }

    updateFrame()
    {
        this.oversampling = (this.parameters.file ? 4 : 1);
        this.imageObj = new Image();
        if(this.parameters.file)
            this.imageObj.src = "/"+this.parameters.file;
        else
        {
            this.canvas.fillStyle="black";
            this.canvas.fillRect(0, 0, this.width, this.height);
        }
        super.updateFrame();
    }

    loadData(data)
    {
        if(this.parameters.module)
        {
            var d = data[this.parameters.module];
            if(!d) return;
            d = d[this.parameters.source+':'+this.parameters.format]
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

    update(d)
    {
        try
        {
            let w = this.oversampling*this.width;   // this could be done in updateFrame and stored
            let h = this.oversampling*this.height;
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
        catch(err)
        {
//            this.canvas.fillText(err, 0, 20);
            this.canvas.fillStyle="black";
            this.canvas.fillRect(0, 0, this.oversampling*this.width, this.oversampling*this.height);
        }
    }
};



webui_widgets.add('webui-widget-image', WebUIWidgetImage);



/*
ImageStream.prototype.LoadData = function(data)
{
    if(this.module)
    {
        var d = data[this.module];
        if(!d) return;
        d = d[this.source+':'+this.type]
        if(!d) return;
        this.imageObj.onload = function ()
        {
            load_count--;
        };
        this.imageObj.src = d;
        return 1;
    }

    return 0;
}
ImageStream.prototype.Update = function(data)
{
    this.context.drawImage(this.imageObj, 0, 0, this.oversampling*this.width, this.oversampling*this.height);
}
*/
