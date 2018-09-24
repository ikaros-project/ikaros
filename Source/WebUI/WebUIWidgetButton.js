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
            {'name':'commandDown', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'commandUp', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'valueDown', 'default':1, 'type':'string', 'control': 'textedit'},
            {'name':'valueUp', 'default':0, 'type':'string', 'control': 'textedit'},
            {'name':'single_trig', 'default':true, 'type':'bool', 'control': 'checkbox'},
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

    button_click()
    {
        if(this.parameters.command && this.parameters.module)
            this.get("/command/"+this.parameters.module+"/"+this.parameters.command+"/"+this.parameters.xindex+"/"+this.parameters.yindex+"/"+this.parameters.valueDown);
    }
    
    button_down()
    {
        if(this.parameters.commandDown && this.parameters.module)
            this.get("/command/"+this.parameters.module+"/"+this.parameters.commandDown+"/"+this.parameters.xindex+"/"+this.parameters.yindex+"/"+this.parameters.valueDown);

        else if(this.parameters.module && this.parameters.parameter)
            this.get("/control/"+this.parameters.module+"/"+this.parameters.parameter+"/"+this.parameters.xindex+"/"+this.parameters.yindex+"/"+this.parameters.valueDown);
    }

    button_up()
    {
        if(this.parameters.commandUp && this.parameters.module)
            this.get("/command/"+this.parameters.module+"/"+this.parameters.commandUp+"/"+this.parameters.xindex+"/"+this.parameters.yindex+"/"+this.parameters.valueUp);

        else if(this.parameters.module && this.parameters.parameter)
            this.get("/control/"+this.parameters.module+"/"+this.parameters.parameter+"/"+this.parameters.xindex+"/"+this.parameters.yindex+"/"+this.parameters.valueUp);
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
        this.firstChild.onclick = function (){
            this.parentElement.button_click();
        }
    }

    update()
    {
        this.firstChild.innerText = this.parameters.label;
    }
};



webui_widgets.add('webui-widget-button', WebUIWidgetButton);

