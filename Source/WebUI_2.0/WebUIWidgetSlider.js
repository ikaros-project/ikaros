class WebUIWidgetSlider extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "PARAMETERS", 'control':'header'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'title', 'default':"Button Title", 'type':'string', 'control': 'textedit'},
            {'name':'label', 'default':"Press", 'type':'string', 'control': 'textedit'},
            {'name':'single_trig', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name': "STYLE", 'control':'header'},
            {'name':'show_title', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    static html()
    {
        return '<div class="vranger"><input type="range"><br><input type="range"><br><input type="range"><br></div>';
    }

    update()
    {
//        this.firstChild.innerText = this.parameters.label;
    }
};



webui_widgets.add('webui-widget-slider', WebUIWidgetSlider);

