// This is a temporary place holder
// Text should probably use HTML later rather than canvas - but who knows?


class WebUIWidgetText extends WebUIWidget
{
    static template()
    {
        return [
            {'name':'text', 'default':"Default Text", 'type':'string', 'control': 'textedit'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    static html()
    {
        return `
            <style>
                webui-widget-text { background-color: rgba(0,0,0,0); color: red; padding: 5px; }
            </style>
            <div>TEXT</div>
        `;
    }

    init()
    {
        this.text = this.parameters.text;
        this.data = this.text;
        this.innerText = this.text;
        this.onclick = function () { alert(this.data) };
    }
    
    
    update()
    {
        this.text = this.parameters.text;
        this.data = this.text;
        this.innerText = this.text;
    }
};



webui_widgets.add('webui-widget-text', WebUIWidgetText);

