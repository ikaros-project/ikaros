class WebUIWidgetRectangle extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "RECTANGLE", 'control':'header'},
            {'name':'title', 'default':"Default Title", 'type':'string', 'control': 'textedit'},
            {'name':'label', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name': "FRAME", 'control':'header'},
            {'name':'background', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame_color', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame_width', 'default':"1", 'type':'int', 'control': 'textedit'},
            {'name':'show_title', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    static html()

    {
        return "<div style='text-align:center;align-items : center;display:flex'> </div>";
    }

    updateFrame()
    {
        let fcolors = String(this.parameters.frame_color ?? "").split(',').map((c) => c.trim()).filter((c) => c !== "");
        if(fcolors.length > 0)
        {
            this.parentElement.style.borderTopColor = fcolors[0];
            this.parentElement.style.borderRightColor = fcolors[1 % fcolors.length];
            this.parentElement.style.borderBottomColor = fcolors[2 % fcolors.length];
            this.parentElement.style.borderLeftColor = fcolors[3 % fcolors.length];
        }
        else
        {
            this.parentElement.style.borderTopColor = "";
            this.parentElement.style.borderRightColor = "";
            this.parentElement.style.borderBottomColor = "";
            this.parentElement.style.borderLeftColor = "";
        }

        let fw = this.parameters.frame_width;
        this.parentElement.style.borderWidth = fw ? fw+"px" : "";
        this.parentElement.style.background = this.parameters.background;
        this.firstChild.innerHTML = "<span>"+this.parameters.label+"</span>"

        super.updateFrame();
    }

    init()
    {

 //       this.text = this.parameters.text;
 //       this.innerText = this.text;
    }
    
    update()
    {
        try {

        }
        catch(err)
        {
        
        }
    }
};

webui_widgets.add('webui-widget-rectangle', WebUIWidgetRectangle);
