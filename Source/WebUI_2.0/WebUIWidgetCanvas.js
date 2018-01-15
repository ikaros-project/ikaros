class WebUIWidgetCanvas extends WebUIWidget
{

    static template()
    {
        return [
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};
    
    static html()
    {
        return `
            <style>
                canvas { background-color: rgba(0,0,0,0); }
                main[data-mode="run"] canvas { cursor: default; }
            </style>
            <canvas style="height: 100%; width:100%" height="100" width="100"></canvas>
        `;
    }

    connectedCallback()
    {
        this.innerHTML = this.constructor.html();
        this.canvasElement = this.querySelector('canvas');
        this.canvas = this.canvasElement.getContext("2d");
//        this.update();
    }

    update()
    {
        let width = parseInt(getComputedStyle(this.canvasElement).width);
        let height = parseInt(getComputedStyle(this.canvasElement).height);
        if(width != this.canvasElement.width || height != this.canvasElement.height)
        {
            this.canvasElement.width = parseInt(width);
            this.canvasElement.height = parseInt(height);
        }
        draw();
    }


    draw()
    {
        this.canvas.beginPath();
        this.canvas.lineWidth = lineWidth;
        this.canvas.strokeStyle = c;

        this.canvas.moveTo(width*Math.random(), height*Math.random());
        this.canvas.lineTo(width*Math.random(), height*Math.random());
        
        this.canvas.stroke();
        
        var w = this;
        setTimeout(function () { w.draw()}, 100)
    }
};



webui_widgets.add('webui-widget-canvas', WebUIWidgetCanvas);
