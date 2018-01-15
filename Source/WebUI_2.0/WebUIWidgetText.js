// This is a temporary place holder
// Text should probably use HTML later rather than cnvas - but who knows?


class WebUIWidgetText extends WebUIWidget
{

    static template()
    {
        return [
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'text', 'default':"Default Text", 'type':'string', 'control': 'textedit'}
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

/*
        console.log("INIT TEXT");
        console.log(this);
        console.log(this.element);
        console.log(this.element.querySelector('div'));
        this.element.querySelector('div').innerText  = this.text;
*/
        this.onclick = function () { alert(this.data) };
    }
    
    
    update()
    {
        console.log("UPDATE");

        this.text = this.parameters.text;
        this.data = this.text;
        this.innerText = this.text;
        
        return;

       let width = parseInt(getComputedStyle(this.canvasElement).width);
        let height = parseInt(getComputedStyle(this.canvasElement).height);
        if(width != this.canvasElement.width || height != this.canvasElement.height)
        {
            this.canvasElement.width = parseInt(width);
            this.canvasElement.height = parseInt(height);
        }
        
        this.width = parseInt(width);
        this.height = parseInt(height);

        
    }
};



webui_widgets.add('webui-widget-text', WebUIWidgetText);

