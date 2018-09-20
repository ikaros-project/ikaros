class WebUIWidgetButton extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "DATA", 'control':'header'},
            {'name':'title', 'default':"Button Title", 'type':'string', 'control': 'textedit'},
            {'name':'label', 'default':"Press", 'type':'string', 'control': 'textedit'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'type', 'default':"command (down)", 'type':'string', 'control': 'menu', 'values': "command (down), command (up), value"},
            {'name':'command', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'single_trig', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'value', 'default':1, 'type':'int', 'control': 'textedit'},
            {'name':'xindex', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'yindex', 'default':0, 'type':'int', 'control': 'textedit'},

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

    button_down()
    {
        if(this.parameters.type == "command (down)" && this.parameters.module)
            this.get("/command/"+this.parameters.module+"/"+this.parameters.command);

        else if(this.parameters.module && this.parameters.parameter)
            this.get("/control/"+this.parameters.module+"/"+this.parameters.parameter+"/"+this.parameters.xindex+"/"+this.parameters.yindex+"/"+this.parameters.value);
    }

    button_up()
    {
        if(this.parameters.type == "command (up)" && this.parameters.module)
            this.get("/command/"+this.parameters.module+"/"+this.parameters.command);

        elseif(this.parameters.module && this.parameters.parameter)
            this.get("/control/"+this.parameters.module+"/"+this.parameters.parameter+"/"+this.parameters.xindex+"/"+this.parameters.yindex+"/0");
    }

    init()
    {
        super.init();
        this.firstChild.onmousedown = function (){
            this.parentElement.button_down();
        }
        this.firstChild.onmouseup = function (){
            this.parentElement.button_up();
        }
    }

    update()
    {
        this.firstChild.innerText = this.parameters.label;
    }
};



webui_widgets.add('webui-widget-button', WebUIWidgetButton);

