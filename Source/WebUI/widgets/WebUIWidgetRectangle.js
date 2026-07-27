class WebUIWidgetRectangle extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "RECTANGLE", 'control':'header'},
            {'name':'title', 'default':"Default Title", 'type':'string', 'control': 'textedit'},
            {'name':'label', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'text_color', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'font', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'text_align', 'default':"center", 'type':'string', 'control': 'menu', 'options': "left,center,right"},
            {'name':'vertical_align', 'default':"center", 'type':'string', 'control': 'menu', 'options': "top,center,bottom"},
            {'name':'padding', 'default':0, 'type':'int', 'control': 'textedit'},
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
        const label = document.createElement("span");
        label.textContent = this.parameters.label;
        this.firstChild.replaceChildren(label);
        this.firstChild.style.color = this.parameters.text_color;
        this.firstChild.style.font = this.parameters.font;
        this.firstChild.style.textAlign = this.parameters.text_align;
        this.firstChild.style.justifyContent = this.parameters.text_align;
        this.firstChild.style.alignItems = ({top:"flex-start", center:"center", bottom:"flex-end"})[this.parameters.vertical_align] || "center";
        this.firstChild.style.padding = `${Number(this.parameters.padding) || 0}px`;

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
