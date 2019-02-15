class WebUIWidgetSlider extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "SLIDE VERTICAL", 'control':'header'},
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
         return `<div class="vranger">
                    <div><span class="slider_label">XXXXXXXX</span><input type="range"><span class="slider_value">0</span></div>
                    <div><span class="slider_label">X</span><input type="range"><span class="slider_value">0</span></div>
                    <div><span class="slider_label">X</span><input type="range"><span class="slider_value">0.34</span></div>
                </div>`;
    }
    update()
    {
        console.log(this.firstChild.children[0].children[1]);
    }
};



webui_widgets.add('webui-widget-slider', WebUIWidgetSlider);

