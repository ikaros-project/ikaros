class WebUIWidgetText extends WebUIWidget
{
    static template()
    {
        return [
            {'name': "PARAMETERS", 'control':'header'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'title', 'default':"Default Title", 'type':'string', 'control': 'textedit'},
            {'name':'text', 'default':"Default Text", 'type':'string', 'control': 'textedit'},
            {'name': "STYLE", 'control':'header'},
            {'name':'show_title', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    static html()
    {
        return "<div> </div>";
    }

    init()
    {
        this.text = this.parameters.text;
        this.innerText = this.text;
    }
    
    update()
    {
        this.text = this.parameters.text;
        this.data = this.text;
        this.innerText = this.text;
    }
};



webui_widgets.add('webui-widget-text', WebUIWidgetText);

