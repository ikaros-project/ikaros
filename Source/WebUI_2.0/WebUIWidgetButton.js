class WebUIWidgetBUtton extends WebUIWidget
{
    static template()
    {
        return [
            {'name': "PARAMETERS", 'control':'header'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'title', 'default':"Button", 'type':'string', 'control': 'textedit'},
            {'name': "STYLE", 'control':'header'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    static html()
    {
        return "<button></button>";
    }

    init()
    {
        this.firstChild.innerText = this.parameters.title;
    }
    
    update()
    {
        this.firstChild.innerText = this.parameters.title;
    }
};



webui_widgets.add('webui-widget-button', WebUIWidgetBUtton);

