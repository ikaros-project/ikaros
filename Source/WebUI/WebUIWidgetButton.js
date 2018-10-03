class WebUIWidgetButton extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "DATA", 'control':'header'},
            {'name':'title', 'default':"Button Title", 'type':'string', 'control': 'textedit'},
            {'name':'label', 'default':"Press", 'type':'string', 'control': 'textedit'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'command', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'commandUp', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'value', 'default':1, 'type':'string', 'control': 'textedit'},
            {'name':'valueUp', 'default':0, 'type':'string', 'control': 'textedit'},
 //           {'name':'singleTrig', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'xindex', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'yindex', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'enableModule', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'enableSource', 'default':"", 'type':'source', 'control': 'textedit'},
                
            {'name': "FRAME", 'control':'header'},
            {'name':'show_title', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    static html()
    {
        return "<button></button>";
    }

    requestData(data_set)
    {
        if(this.parameters.enableModule && this.parameters.enableSource)
            data_set.add(this.parameters.enableModule+"."+this.parameters.enableSource);
    }

    button_down(evt)
    {
        let p = this.parentElement.parameters;
        if(p.command && p.module)
            this.parentElement.get("/command/"+p.module+"/"+p.command+"/"+p.xindex+"/"+p.yindex+"/"+p.value);

        else if(p.module && p.parameter)
            this.parentElement.get("/control/"+p.module+"/"+p.parameter+"/"+p.xindex+"/"+p.yindex+"/"+p.value);
    }

    button_up(evt)
    {
        let p = this.parentElement.parameters;
        if(p.commandUp && p.module)
            this.parentElement.get("/command/"+p.module+"/"+p.commandUp+"/"+p.xindex+"/"+p.yindex+"/"+p.valueUp);

        else if(p.module && p.parameter)
            this.parentElement.get("/control/"+p.module+"/"+p.parameter+"/"+p.xindex+"/"+p.yindex+"/"+p.valueUp);
    }

    init()
    {
        super.init();
        this.firstChild.addEventListener("mousedown", this.button_down, true);
        this.firstChild.addEventListener("mouseup", this.button_up, true);
        this.firstChild.addEventListener("click", this.button_click, true);
    }

    update(d)
    {
        this.firstChild.innerText = this.parameters.label;

        try {
            if(this.parameters.enableModule && this.parameters.enableSource)
                this.firstChild.disabled = (d[this.parameters.enableModule][this.parameters.enableSource][0][0] == 0 ? true : false);
        }
        catch(err)
        {}
    }
};



webui_widgets.add('webui-widget-button', WebUIWidgetButton);

